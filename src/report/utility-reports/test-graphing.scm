;; -*-scheme-*-

;; This is a sample guile report generator for GnuCash.
;; It illustrates the basic techniques used to create
;; new reports for GnuCash.

(define-module (gnucash report test-graphing))
(use-modules (gnucash main)) ;; FIXME: delete after we finish modularizing.
(use-modules (gnucash gnc-module))

(debug-enable 'debug)
(debug-enable 'backtrace)

(gnc:module-load "gnucash/report/report-system" 0)
(gnc:module-load "gnucash/gnome-utils" 0) ;for gnc:html-build-url

(define (simple-pie-chart)
  (let ((chart (gnc:make-html-piechart)))
    (gnc:html-piechart-set-title! chart "Pie Chart Title")
    (gnc:html-piechart-set-subtitle! chart "Pie Chart SubTitle")
    (gnc:html-piechart-set-width! chart 320)
    (gnc:html-piechart-set-height! chart 240)
    ;; (gnc:html-piechart-set-data! chart (unzip1 combined))
    (gnc:html-piechart-set-data! chart '(25 45 30))
    ;; (gnc:html-piechart-set-labels! chart legend-labels))
    (gnc:html-piechart-set-labels! chart '("foo" "bar" "baz"))
    (gnc:html-piechart-set-colors! chart (gnc:assign-colors 3))
    chart
    )
)

(define (simple-bar-chart)
  (let ((chart (gnc:make-html-barchart))
        (text (gnc:make-html-text (gnc:html-markup-p "[bar goes here]"))))
    (gnc:html-barchart-set-title! chart "Bar Chart Title")
    (gnc:html-barchart-set-subtitle! chart "Bar Chart SubTitle")
    ;;(gnc:html-barchart-set-data! chart '((25 45 30) (75 55 70)))
    (gnc:html-barchart-append-row! chart '(25 45 30))
    (gnc:html-barchart-append-row! chart '(75 55 70))
    (gnc:html-barchart-set-width! chart 320)
    (gnc:html-barchart-set-height! chart 240)
    (gnc:html-barchart-set-row-labels! chart '("row1" "row2"))
    (gnc:html-barchart-set-col-labels! chart '("foo" "bar" "baz"))
    (gnc:html-barchart-set-col-colors! chart (gnc:assign-colors 3))
    chart))

(define (simple-scatter-chart)
  (gnc:make-html-text (gnc:html-markup-p "[scatter goes here]"))
)

(define (options-generator)    
  (let* ((options (gnc:new-options)))
    (gnc:options-set-default-section options "Test Graphing")
    options)
  )

;; This is the rendering function. It accepts a database of options
;; and generates an object of type <html-document>.  See the file
;; report-html.txt for documentation; the file report-html.scm
;; includes all the relevant Scheme code. The option database passed
;; to the function is one created by the options-generator function
;; defined above.
(define (test-graphing-renderer report-obj)
  ;; These are some helper functions for looking up option values.
  (define (get-op section name)
    (gnc:lookup-option (gnc:report-options report-obj) section name))
  
  (define (op-value section name)
    (gnc:option-value (get-op section name)))

  (let ((document (gnc:make-html-document)))

    (gnc:html-document-set-title! document (_ "Graphs"))

    (gnc:html-document-add-object!
     document
     (gnc:make-html-text
      (gnc:html-markup-p
       (gnc:html-markup/format
        (_ "Sample graphs:")))))

    (gnc:html-document-add-object!
     document
     (gnc:make-html-text (gnc:html-markup-p "Pie:")))
    (gnc:html-document-add-object! document (simple-pie-chart))

    (gnc:html-document-add-object!
     document
     (gnc:make-html-text (gnc:html-markup-p "Bar:")))
    (gnc:html-document-add-object! document (simple-bar-chart))

    (gnc:html-document-add-object!
     document
     (gnc:make-html-text (gnc:html-markup-p "Scatter:")))
    (gnc:html-document-add-object! document (simple-scatter-chart))
    
    (gnc:html-document-add-object! 
     document 
     (gnc:make-html-text 
      (gnc:html-markup-p (_ "Done."))))
      
    document
    )
  )

;; Here we define the actual report with gnc:define-report
(gnc:define-report
 
 ;; The version of this report.
 'version 1
 
 ;; The name of this report. This will be used, among other things,
 ;; for making its menu item in the main menu. You need to use the
 ;; untranslated value here!
 'name (N_ "Test Graphing")

 ;; The name in the menu
 ;; (only necessary if it differs from the name)
 'menu-name (N_ "Sample graphs.")

 ;; A tip that is used to provide additional information about the
 ;; report to the user.
 'menu-tip (N_ "Sample graphs.")

 ;; A path describing where to put the report in the menu system.
 ;; In this case, it's going under the utility menu.
 'menu-path (list gnc:menuname-utility)

 ;; The options generator function defined above.
 'options-generator options-generator
 ;; 'options-generator gnc:new-options
 
 ;; The rendering function defined above.
 'renderer test-graphing-renderer)
