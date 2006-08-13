;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;  business-reports.scm
;;  load the business report definitions
;;
;;  Copyright (c) 2002 Derek Atkins <derek@ihtfp.com>
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
;; 51 Franklin Street, Fifth Floor    Fax:    +1-617-542-2652
;; Boston, MA  02110-1301,  USA       gnu@gnu.org
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-module (gnucash report business-reports))
(use-modules (gnucash gnc-module))
(gnc:module-load "gnucash/report/standard-reports" 0)
(gnc:module-load "gnucash/business-utils" 0)

;; this defines gnc:url-type-ownerreport and pulls in gnome-utils
;; to define gnc:html-build-url
(gnc:module-load "gnucash/business-gnome" 0)

(define gnc:menuname-business-reports (N_ "_Business"))

(define (guid-ref idstr type guid)
  (gnc:html-build-url type (string-append idstr guid) #f))

(define (gnc:customer-anchor-text customer)
  (guid-ref "customer=" gnc:url-type-customer (gncCustomerReturnGUID customer)))

(define (gnc:job-anchor-text job)
  (guid-ref "job=" gnc:url-type-job (gncJobReturnGUID job)))

(define (gnc:vendor-anchor-text vendor)
  (guid-ref "vendor=" gnc:url-type-vendor (gncVendorReturnGUID vendor)))

(define (gnc:employee-anchor-text employee)
  (guid-ref "employee=" gnc:url-type-employee (gncEmployeeReturnGUID employee)))

(define (gnc:invoice-anchor-text invoice)
  (guid-ref "invoice=" gnc:url-type-invoice (gncInvoiceReturnGUID invoice)))

(define (gnc:owner-anchor-text owner)
  (let ((type (gncOwnerGetType (gncOwnerGetEndOwner owner))))
    (case type
      ((GNC-OWNER-CUSTOMER)
       (gnc:customer-anchor-text (gncOwnerGetCustomer owner)))

      ((GNC-OWNER-VENDOR)
       (gnc:vendor-anchor-text (gncOwnerGetVendor owner)))

      ((GNC-OWNER-EMPLOYEE)
       (gnc:employee-anchor-text (gncOwnerGetEmployee owner)))

      ((GNC-OWNER-JOB)
       (gnc:job-anchor-text (gncOwnerGetJob owner)))

      (else
       ""))))

(define (gnc:owner-report-text owner acc)
  (let* ((end-owner (gncOwnerGetEndOwner owner))
	 (type (gncOwnerGetType end-owner))
	 (ref #f))

    (case type
      ((GNC-OWNER-CUSTOMER)
       (set! ref "owner=c:"))

      ((GNC-OWNER-VENDOR)
       (set! ref "owner=v:"))

      ((GNC-OWNER-EMPLOYEE)
       (set! ref "owner=e:"))

      (else (set! ref "unknown-type=")))

    (if ref
	(begin
	  (set! ref (string-append ref (gncOwnerReturnGUID end-owner)))
	  (if acc
	      (set! ref (string-append ref "&acct="
				       (gnc:account-get-guid acc))))
	  (gnc:html-build-url gnc:url-type-ownerreport ref #f))
	ref)))

(export gnc:menuname-business-reports)

(use-modules (gnucash report fancy-invoice))
(use-modules (gnucash report invoice))
(use-modules (gnucash report easy-invoice))
(use-modules (gnucash report owner-report))
(use-modules (gnucash report payables))
(use-modules (gnucash report receivables))

(define gnc:invoice-report-create gnc:invoice-report-create-internal)

(export gnc:invoice-report-create
	gnc:customer-anchor-text gnc:job-anchor-text gnc:vendor-anchor-text
	gnc:invoice-anchor-text gnc:owner-anchor-text gnc:owner-report-text)
(re-export gnc:owner-report-create)
