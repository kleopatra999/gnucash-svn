;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; net-barchart.scm : Display a time series for either net worth or
;; net profit.
;;
;; By Robert Merkel <rgmerk@mira.net>
;; and Christian Stimming <stimming@tu-harburg.de>
;;
;; This program is free software; you can redistribute it and/or    
;; modify it under the terms of the GNU General Public License as   
;; published by the Free Software Foundation; either version 2 of   
;; the License, or (at your option) any later version.              
;;                                                                  
;; This program is distributed in the hope that it will be useful,  
;; but WITHOUT ANY WARRANTY; without even the implied warranty of   
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    
;; GNU General Public License for more details.                     
;;                                                                  
;; You should have received a copy of the GNU General Public License
;; along with this program; if not, contact:
;;
;; Free Software Foundation           Voice:  +1-617-542-5942
;; 59 Temple Place - Suite 330        Fax:    +1-617-542-2652
;; Boston, MA  02111-1307,  USA       gnu@gnu.org
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(gnc:support "report/net-barchart.scm")
(gnc:depend  "report-html.scm")
(gnc:depend  "date-utilities.scm")

(let ((optname-from-date (N_ "From"))
      (optname-to-date (N_ "To"))
      (optname-stepsize (N_ "Step Size"))
      (optname-report-currency (N_ "Report's currency"))

      (optname-accounts (N_ "Accounts"))

      (optname-inc-exp (N_ "Show Income/Expense"))
      (optname-show-profit (N_ "Show Net Profit"))

      (optname-sep-bars (N_ "Show Asset & Liability bars"))
      (optname-net-bars (N_ "Show Net Worth bars"))

      (optname-plot-width (N_ "Plot Width"))
      (optname-plot-height (N_ "Plot Height")))

  (define (options-generator inc-exp?)
    (let* ((options (gnc:new-options)) 
           ;; This is just a helper function for making options.
           ;; See gnucash/src/scm/options.scm for details.
           (add-option 
            (lambda (new-option)
              (gnc:register-option options new-option))))

      (gnc:options-add-date-interval!
       options gnc:pagename-general
       optname-from-date optname-to-date "a")

      (gnc:options-add-interval-choice! 
       options gnc:pagename-general optname-stepsize "b" 'MonthDelta)

      (add-option
       (gnc:make-account-list-option
	gnc:pagename-accounts optname-accounts
	"c"
	(N_ "Report on these accounts, if chosen account level allows.")
	(lambda ()
	  (filter
	   (if inc-exp?
	       gnc:account-is-inc-exp?
	       (lambda (account) (not (gnc:account-is-inc-exp? account))))
	   (gnc:group-get-subaccounts (gnc:get-current-group))))
	(lambda (accounts)
	  (list #t
		(filter 
		 (if inc-exp?
		     gnc:account-is-inc-exp?
		     (lambda (account)
		       (not (gnc:account-is-inc-exp? account))))
		 accounts)))
	#t))

      (gnc:options-add-currency! 
       options gnc:pagename-general optname-report-currency "d")
      
      (add-option
       (gnc:make-simple-boolean-option
        gnc:pagename-display 
	(if inc-exp? optname-inc-exp optname-sep-bars)
        "a" 
	(if inc-exp? 
	    (N_ "Show Income and Expenses?")
	    (N_ "Show the Asset and the Liability bars?"))
	#t))

      (add-option
       (gnc:make-simple-boolean-option
        gnc:pagename-display 
	(if inc-exp? optname-show-profit optname-net-bars)
        "b" 
	(if inc-exp?
	    (N_ "Show the net profit?")
	    (N_ "Show a Net Worth bar?")) 
	#t))


      (gnc:options-add-plot-size! 
       options gnc:pagename-display 
       optname-plot-width optname-plot-height "c" 500 400)

      (gnc:options-set-default-section options gnc:pagename-general)

      options))
  
  ;; This is the rendering function. It accepts a database of options
  ;; and generates an object of type <html-document>.  See the file
  ;; report-html.txt for documentation; the file report-html.scm
  ;; includes all the relevant Scheme code. The option database passed
  ;; to the function is one created by the options-generator function
  ;; defined above.
  (define (net-renderer report-obj inc-exp?)

    ;; This is a helper function for looking up option values.
    (define (op-value section name)
      (gnc:option-value 
       (gnc:lookup-option (gnc:report-options report-obj) section name)))

    ;; This calculates the balances for all the 'accounts' for each
    ;; element of the list 'dates'. If income?==#t, the signs get
    ;; reversed according to income-sign-reverse general option
    ;; settings. Returns a commodity-collector.
    (define (collector-fn accounts dates income?)
      (map 
       (lambda (date)
	 ((if inc-exp?
	      (if income?
		  gnc:accounts-get-comm-total-income
		  gnc:accounts-get-comm-total-expense)
	      gnc:accounts-get-comm-total-assets)
	  accounts 
	  (lambda (account)
	    (if inc-exp?
		;; for inc-exp, 'date' is a pair of time values, else
		;; it is a time value.
		(gnc:account-get-comm-balance-interval 
		 account (car date) (cadr date) #f)
		(gnc:account-get-comm-balance-at-date 
		 account date #f)))))
       dates))

    ;; This exchanges a commodity-collector to one single
    ;; 'report-currency' according to the exchange-fn. Returns a
    ;; double.
    (define (collector-to-double-fn report-currency exchange-fn)
      (lambda (commodity-collector)
        (gnc:numeric-to-double
         (gnc:gnc-monetary-amount
          (gnc:sum-collector-commodity commodity-collector
                                       report-currency 
                                       exchange-fn)))))

    ;; Helper - Adds two commodity-collectors.
    (define (collector-combine collector1 collector2)
      (let ((new-collector (gnc:make-commodity-collector)))
	(new-collector 'merge collector1 #f)
	(new-collector 'merge collector2 #f)
	new-collector))

    (let* ((to-date-tp (gnc:timepair-end-day-time 
			(gnc:date-option-absolute-time
                         (op-value gnc:pagename-general 
				   optname-to-date))))
	   (from-date-tp (gnc:timepair-start-day-time 
			  (gnc:date-option-absolute-time
                           (op-value gnc:pagename-general 
				     optname-from-date))))
	   (interval (op-value gnc:pagename-general optname-stepsize))
	   (accounts (op-value gnc:pagename-accounts optname-accounts))

	   (show-sep? (op-value gnc:pagename-display 
				(if inc-exp? optname-inc-exp 
				    optname-sep-bars)))
	   (show-net? (op-value gnc:pagename-display 
				(if inc-exp? optname-show-profit
				    optname-net-bars)))
	   (height (op-value gnc:pagename-display optname-plot-height))
	   (width (op-value gnc:pagename-display optname-plot-width))

           (report-currency (op-value gnc:pagename-general
                                      optname-report-currency))

	   (exchange-alist (gnc:make-exchange-alist
			    report-currency to-date-tp))
	   (exchange-fn-internal (gnc:make-exchange-function exchange-alist))
	   ;;(exchange-fn (lambda (foreign)
	   ;;              (exchange-fn-internal foreign report-currency)))

	   (dates-list ((if inc-exp? gnc:make-date-interval-list
			    gnc:make-date-list)
                        ((if inc-exp? gnc:timepair-start-day-time 
			     gnc:timepair-end-day-time) from-date-tp) 
                        (gnc:timepair-end-day-time to-date-tp)
                        (eval interval)))

	   (classified-accounts (gnc:decompose-accountlist accounts))
	   (document (gnc:make-html-document))
	   (chart (gnc:make-html-barchart))

           (assets-collector-list 
	    (collector-fn 
	     (if inc-exp? 
		 accounts
		 (assoc-ref classified-accounts 'asset))
	     dates-list #t))
	   (liab-collector-list 
	    (collector-fn 
	     (if inc-exp?
		 accounts
                 (assoc-ref classified-accounts 'liability))
	     dates-list #f))
	   (net-collector-list
            (map collector-combine assets-collector-list
                 liab-collector-list))
	   (assets-list
            (map (collector-to-double-fn report-currency exchange-fn-internal)
                 assets-collector-list))
           (liability-list
            (map (collector-to-double-fn report-currency exchange-fn-internal)
                 liab-collector-list))
	   (net-list
	    (map (collector-to-double-fn report-currency exchange-fn-internal)
		 net-collector-list))
	   (date-string-list (map 
			      (if inc-exp?
				  (lambda (date-list-item)
				    (gnc:timepair-to-datestring
				     (car date-list-item)))
				  gnc:timepair-to-datestring)
			      dates-list))
	   (non-zeros #f))
      (define (add-column! data-list)
	(begin
	  (gnc:html-barchart-append-column! chart data-list)
	  (if (gnc:not-all-zeros data-list) (set! non-zeros #t))
	  #f))
      
      (if 
       (not (null? accounts))
       (begin

	 (gnc:html-barchart-set-title! 
	  chart (if inc-exp? (_ "Income/Expense Chart") 
		    (_ "Net Worth Chart")))
	 (gnc:html-barchart-set-subtitle!
	  chart (sprintf #f
			 (_ "%s to %s")
			 (gnc:timepair-to-datestring from-date-tp) 
			 (gnc:timepair-to-datestring to-date-tp)))
	 (gnc:html-barchart-set-width! chart width)
	 (gnc:html-barchart-set-height! chart height)
	 (gnc:html-barchart-set-row-labels! chart date-string-list)
	 (gnc:html-barchart-set-y-axis-label!
	  chart (gnc:commodity-get-mnemonic report-currency))
	 ;; Determine whether we have enough space for horizontal labels
	 ;; -- kind of a hack. Assumptions: y-axis labels and legend
	 ;; require 200 pixels, and each x-axes label needs 60 pixels.
	 (gnc:html-barchart-set-row-labels-rotated?! 
	  chart (< (/ (- width 200) 
		      (length date-string-list)) 60))
	 
	 (if show-sep?
	     (begin
	       (add-column! assets-list)
	       (add-column!		      ;;(if inc-exp?
		(map - liability-list)
		;;liability-list)
		)))
	 (if show-net?
	     (add-column! net-list))
	 (gnc:html-barchart-set-col-labels! 
	  chart (append
		 (if show-sep?
		     (if inc-exp?
			 (list (_ "Income") (_ "Expense"))
			 (list (_ "Assets") (_ "Liabilities")))
		     '())
		 (if show-net?
		     (if inc-exp?
			 (list (_ "Net Profit"))
			 (list (_ "Net Worth")))
		     '())))
	 (gnc:html-barchart-set-col-colors! 
	  chart (append
		 (if show-sep?
		     '("blue" "red") '())
		 (if show-net?
		     '("green") '())))
	 
	 (if show-sep?
	     (let ((urls
		    (list
		     (gnc:make-report-anchor
		      (if inc-exp?
			  "Income Over Time"
			  "Assets Over Time")
		      report-obj
		      (list 
		       (list gnc:pagename-display
			     "Use Stacked Bars" #t)
		       (list gnc:pagename-general
			     gnc:optname-reportname
			     (if inc-exp?
				 (_ "Income Chart")
				 (_ "Asset Chart")))))
		     (gnc:make-report-anchor
		      (if inc-exp?
			  "Expense Over Time"
			  "Liabilities Over Time")
		      report-obj
		      (list 
		       (list gnc:pagename-display
			     "Use Stacked Bars" #t)
		       (list gnc:pagename-general
			     gnc:optname-reportname
			     (if inc-exp?
				 (_ "Expense Chart")
				 (_ "Liability Chart"))))))))
	       (gnc:html-barchart-set-button-1-bar-urls! 
		chart urls)
	       (gnc:html-barchart-set-button-1-legend-urls! 
		chart urls)))
	 
	 (if non-zeros
	     (gnc:html-document-add-object! document chart) 
	     (gnc:html-document-add-object!
	      document
	      (gnc:html-make-empty-data-warning))))
       
       ;; else no accounts selected

       (gnc:html-document-add-object!
	document
	(gnc:html-make-no-account-warning)))
       
	 
	 document))
      
      ;; Here we define the actual report
      (gnc:define-report
       'version 1
       'name (N_ "Net Worth Barchart")
       'menu-path (list gnc:menuname-asset-liability)
       'options-generator (lambda () (options-generator #f))
       'renderer (lambda (report-obj) (net-renderer report-obj #f)))

  (gnc:define-report
   'version 1
   'name (N_ "Income/Expense Chart")
   'menu-path (list gnc:menuname-income-expense)
   'options-generator (lambda () (options-generator #t))
   'renderer (lambda (report-obj) (net-renderer report-obj #t))))
