;; -*-scheme-*-
;; $Id$
;; 
;;  Richard -Gilligan- Uschold
;; These are help strings for each TXF code. See taxtxf.scm and
;; txf-export-help.scm
;;
(gnc:support "report/txf-export.scm")

(define txf-version 19990)		; equivalent to YEAR.X

;;      #(Code "  Form X \\ Description" "Help" Format Multiple "CatId")
(define txf-help-catagories
  (list #(H001 "< help \\ Name of Current account is exported." "" 0 #f "")
	#(H002 "^ help \\ Name of Parent account is exported." "" 0 #f "")
	#(H003 "# help \\ Not implimented yet, Do NOT Use!" "" 0 #f "")))

(define txf-income-catagories
  (list #(N000 "  none \\ Tax Report Only - No TXF Export" "" 0 #f "")
	#(H256 "Help F1040 \\ Form 1040 - the main tax form" "" 1 #f "")
	#(N261 "  F1040 \\ Alimony received" "" 1 #f "Alimony receive")
	#(N257 "  F1040 \\ Other income, misc." "" 1 #f "Other Inc")
        #(N259 "  F1040 \\ Prizes, awards" "" 1 #f "Prizes, awards")
	#(N520 "  F1040 \\ RR retirement inc., spouse" "" 1 #f "RR retirement i")
	#(N519 "  F1040 \\ RR retirement income, self" "" 1 #f "RR retirement i")
	#(N258 "  F1040 \\ Sick pay or disability pay" "" 1 #f "Sick Pay")
	#(N483 "  F1040 \\ Social Security inc., spouse" "" 1 #f "Social Security")
	#(N266 "  F1040 \\ Social Security income, self" "" 1 #f "Social Security")
	#(N269 "  F1040 \\ Taxable fringe benefits" "" 1 #f "Taxable fringe")

	#(H634 "Help F1099-G \\ Form 1099-G - certian Government payments" "" 1 #t "")
	#(N672 "  F1099-G \\ Qualified state tuition earnings" "" 1 #t "")
	#(N260 "  F1099-G \\ State and local tax refunds" "" 1 #t "")
	#(N479 "  F1099-G \\ Unemployment compensation" "" 1 #t "Unemployment In")

	#(H553 "Help F1099-MISC \\ Form 1099-MISC - MISCellaneous income" "" 1 #t "")
	#(N562 "^ F1099-MISC \\ Crop insurance proceeds" "" 1 #t "")
	#(N559 "^ F1099-MISC \\ Fishing boat proceeds" "" 1 #t "")
	#(N560 "^ F1099-MISC \\ Medical/health payments" "" 1 #t "")
	#(N561 "^ F1099-MISC \\ Nonemployee compensation" "" 1 #t "")
	#(N557 "^ F1099-MISC \\ Other income" "" 1 #t "")
	#(N259 "^ F1099-MISC \\ Prizes and awards" "" 1 #t "")
	#(N555 "^ F1099-MISC \\ Rents" "" 1 #t "")
	#(N556 "^ F1099-MISC \\ Royalties" "" 1 #t "")

	#(H629 "Help F1099=MSA \\ Form 1099-MSA Medical Savings Account" "" 1 #t "")
	#(N632 "  F1099-MSA \\ MSA earnings on excess contrib" "" 1 #t "")
	#(N631 "  F1099-MSA \\ MSA gross distribution" "" 1 #t "")

	#(H473 "Help F1099-R \\ Form 1099-R - Rretirement distributions" "" 1 #t "")
	#(N623 "^ F1099-R \\ SIMPLE total gross distribution" "" 1 #t "")
	#(N624 "^ F1099-R \\ SIMPLE total taxable distribution" "" 1 #t "")
	#(N477 "^ F1099-R \\ Total IRA gross distribution" "" 1 #t "")
	#(N478 "^ F1099-R \\ Total IRA taxable distribution" "" 1 #t "")
	#(N475 "^ F1099-R \\ Total pension gross distribution" "" 1 #t "")
	#(N476 "^ F1099-R \\ Total pension taxable distribution" "" 1 #t "")

	#(H380 "Help F2106 \\ employee business expenses" "" 1 #t "")
	#(N387 "  F2106 \\ Reimb. business expenses (non-meal/ent.)" "" 1 #t "")
	#(N388 "  F2106 \\ Reimb. meal/entertainment expenses" "" 1 #t "")

	#(H503 "Help F4137 \\ Form 4137 - tips not reported" "" 1 #t "")
	#(N505 "  F4137 \\ Total cash/tips not reported to employer" "" 1 #t "")

	#(H412 "Help F4684 \\ Form 4684 - casualties and thefts" "" 1 #t "")
	#(N416 "  F4684 \\ FMV after casualty" "" 1 #t "")
	#(N415 "  F4684 \\ FMV before casualty" "" 1 #t "")
	#(N414 "  F4684 \\ Insurance/reimbursement" "" 1 #t "")

	#(H569 "Help F4835 \\ Form 4835 - farm rental income" "" 1 #t "")
	#(N573 "  F4835 \\ Agricultural program payments" "" 1 #t "")
	#(N575 "  F4835 \\ CCC loans forfeited/repaid" "" 1 #t "")
	#(N574 "  F4835 \\ CCC loans reported/election" "" 1 #t "")
	#(N577 "  F4835 \\ Crop insurance proceeds deferred" "" 1 #t "")
	#(N576 "  F4835 \\ Crop insurance proceeds received" "" 1 #t "")
	#(N578 "  F4835 \\ Other income" "" 1 #t "")
	#(N571 "  F4835 \\ Sale of livestock/produce" "" 1 #t "")
	#(N572 "  F4835 \\ Total cooperative distributions" "" 1 #t "")

	#(H427 "Help F6252 \\ Form 6252 - income from casual sales" "" 1 #t "")
	#(N429 "  F6252 \\ Debt assumed by buyer" "" 1 #t "")
	#(N431 "  F6252 \\ Depreciation allowed" "" 1 #t "")
	#(N435 "  F6252 \\ Payments received prior years" "" 1 #t "")
	#(N434 "  F6252 \\ Payments received this year" "" 1 #t "")
	#(N428 "  F6252 \\ Selling price" "" 1 #t "")

	#(H441 "Help F8815 \\ Form 8815 - EE U.S. savings bonds sold for education" "" 1 #f "")
	#(N444 "  F8815 \\ EE US svgs. bonds proceeds" "" 1 #f "")
	#(N443 "  F8815 \\ Nontaxable education benefits" "" 1 #f "")
	#(N445 "  F8815 \\ Post-89 EE bond face value" "" 1 #f "")

	#(H639 "Help F8863 \\ Form 8863 - Hope and Lifetime Learning education credits" "" 1 #t "")
	#(N637 "  F8863 \\ Hope credit" "" 1 #t "")
	#(N638 "  F8863 \\ Lifetime learning credit" "" 1 #t "")

	#(N392 "  Home Sale \\ Home Sale worksheets (was F2119)" "" 1 #t "")
	#(N393 "  Home Sale \\ Selling price of old home" "" 1 #t "")

	#(H285 "Help Sched B \\ Schedule B - interest and dividend income" "" 3 #t "")
	#(N487 "< Sched B \\ Dividend income, non-taxable" "" 3 #t "_DivInc TaxFree")
	#(N286 "^ Sched B \\ Dividend income, Ordinary" "" 3 #t "_DivInc")
	#(N287 "< Sched B \\ Interest income" "" 3 #t "_IntInc")
	#(N489 "< Sched B \\ Interest income, non-taxable" "_IntInc TaxFree" 3 #t "")
	#(N492 "< Sched B \\ Interest income, OID bonds" "" 3 #t "")
	#(N524 "< Sched B \\ Interest income, Seller-financed mortgage" "" 3 #t "Seller-financed")
	#(N289 "< Sched B \\ Interest income, State and muni bond" "" 3 #t "")
	#(N490 "< Sched B \\ Interest income, taxed only by fed" "" 3 #t "")
	#(N491 "< Sched B \\ Interest income, taxed only by state" "" 3 #t "")
	#(N290 "< Sched B \\ Interest income, tax-exempt private activity bond" "" 3 #t "")
	#(N288 "< Sched B \\ Interest income, US government" "" 3 #t "")

	#(H291 "Help Sched C \\ Schedule C - self-employment income" "" 3 #t "")
	#(N293 "  Sched C \\ Gross receipts or sales" "" 1 #t "")
	#(N303 "  Sched C \\ Other business income" "" 1 #t "")

	;; Note N488 was erroniously on Sched B
	#(H320 "Help Sched D \\ Schedule D - capital gains and losses " "" 3 #t "")
	#(N488 "^ Sched D \\ Dividend income, capital gain distributions" "" 3 #t "_LT CapGnDst")
	#(N323 "# Sched D \\ Long Term gain/loss - security" "" 1 #f "")
	#(N321 "# Sched D \\ Short Term gain/loss - security" "" 1 #f "")
	#(N810 "# Sched D \\ Short/Long Term gain or loss" "" 1 #f "")

	#(H325 "Help Sched E \\ Schedule E - rental and royalty income" "" 3 #t "")
	#(N326 "  Sched E \\ Rents received" "" 1 #t "")
	#(N327 "  Sched E \\ Royalties received" "" 1 #t "")

	#(H343 "Help Sched F \\ Schedule F - Farm income and expense" "" 3 #t "")
	#(N372 "  Sched F \\ Agricultural program payments" "" 1 #t "")
	#(N374 "  Sched F \\ CCC loans forfeited or repaid" "" 1 #t "")
	#(N373 "  Sched F \\ CCC loans reported/election" "" 1 #t "")
	#(N376 "  Sched F \\ Crop insurance proceeds deferred" "" 1 #t "")
	#(N375 "  Sched F \\ Crop insurance proceeds received" "" 1 #t "")
	#(N370 "  Sched F \\ Custom hire income" "" 1 #t "")
	#(N377 "  Sched F \\ Other farm income" "" 1 #t "")
	#(N369 "  Sched F \\ Resales of livestock/items" "" 1 #t "")
	#(N368 "  Sched F \\ Sales livestock/product raised" "" 1 #t "")
	#(N371 "  Sched F \\ Total cooperative distributions" "" 1 #t "")

	#(H446 "Help Sched K-1 \\ Schedule K-1 - partnership income, credits, deductions" "" 3 #t "")
	#(N452 "  Sched K-1 \\ Dividends, ordinary" "" 1 #t "")
	#(N455 "  Sched K-1 \\ Guaranteed partner payments" "" 1 #t "")
	#(N451 "  Sched K-1 \\ Interest income" "" 1 #t "")

	#(N454 "# Sched K-1 \\ Net LT capital gain or loss" "" 1 #t "")
	#(N453 "# Sched K-1 \\ Net ST capital gain or loss" "" 1 #t "")
	#(N456 "# Sched K-1 \\ Net Section 1231 gain or loss" "" 1 #t "")
	#(N448 "# Sched K-1 \\ Ordinary income or loss" "" 1 #t "")
	#(N450 "# Sched K-1 \\ Other rental income or loss" "" 1 #t "")
	#(N449 "# Sched K-1 \\ Rental real estate income or loss" "" 1 #t "")

	#(N527 "  Sched K-1 \\ Royalties" "" 1 #t "")
	#(N528 "  Sched K-1 \\ Tax-exempt interest income" "" 1 #t "")

	#(H458 "Help W-2 \\ Form W-2 - Wages earned and taxes withheld" "" 3 #t "")
	#(N465 "^ W-2 \\ Dependent care benefits, self" "" 1 #t "")
	#(N512 "^ W-2 \\ Dependent care benefits, spouse" "" 1 #t "")
	#(N267 "^ W-2 \\ Reimbursed moving expenses, self" "" 1 #t "")
	#(N546 "^ W-2 \\ Reimbursed moving expenses, spouse" "" 1 #t "")
	#(N460 "^ W-2 \\ Salary or wages, self" "" 1 #t "Salary")
	#(N506 "^ W-2 \\ Salary or wages, spouse" "" 1 #t "Salary")

	#(H547 "Help W-2G \\ Form W-2G - gambling winnings" "" 3 #t "")
	#(N549 "^ W-2G \\ Gross winnings" "" 1 #t "")))

(define txf-expense-catagories
  (list #(N000 "  none \\ Tax Report Only - No TXF Export" "" 0 #f "")
	#(H256 "Help F1040 \\ Form 1040 - the main tax form" "" 1 #f "")
	#(N264 "  F1040 \\ Alimony paid" "" 1 #f "Alimony paid")
	#(N265 "< F1040 \\ Early withdrawal penalty" "" 3 #f "Early withdrawa")
	#(N521 "  F1040 \\ Federal estimated tax, qrtrly" "" 1 #f "Tax:Fed Estimat")
	#(N613 "  F1040 \\ Fed tax withheld, RR retire, self" "" 1 #f "Tax:Fed wh, RR")
	#(N614 "  F1040 \\ Fed tax withheld, RR retire, spouse" "" 1 #f "Tax Spouse:Fed")
	#(N611 "  F1040 \\ Fed tax withheld, Social Security, self" "" 1 #f "Tax:Fed wh, Soc")
	#(N612 "  F1040 \\ Fed tax withheld, Social Security, spouse" "" 1 #f "Tax Spouse:Fed")
	#(N482 "  F1040 \\ IRA contrib., non-work spouse" "" 1 #f "")
	#(N262 "  F1040 \\ IRA contribution, self" "" 1 #f "IRA Contrib")
	#(N481 "  F1040 \\ IRA contribution, spouse" "" 1 #f "")
	#(N263 "  F1040 \\ Keogh deduction, self" "" 1 #f "Keogh deduction")
	#(N516 "  F1040 \\ Keogh deduction, spouse" "" 1 #f "Keogh deduction")
	#(N608 "  F1040 \\ Medical savings contribution, spouse" "" 1 #f "Med savings con")
	#(N607 "  F1040 \\ Medical savings contribution, self" "" 1 #f "Med savings con")
	#(N517 "  F1040 \\ SEP-IRA deduction, self" "" 1 #f "SEP deduction,")
	#(N518 "  F1040 \\ SEP-IRA deduction, spouse" "" 1 #f "SEP deduction,")
	#(N609 "  F1040 \\ SIMPLE contribution, self" "" 1 #f "SIMPLE contribu")
	#(N610 "  F1040 \\ SIMPLE contribution, spouse" "" 1 #f "SIMPLE contribu")
	#(N636 "  F1040 \\ Student loan interest" "" 1 #f "Student loan in")

	#(H634 "Help F1099-G \\ Form 1099-G - certian Government payments" "" 1 #t "")
	#(N606 "  F1099-G \\ Fed tax withheld, unemplyment comp" "" 1 #t "")
	#(N605 "  F1099-G \\ Unemployment comp repaid" "" 1 #t "")

	#(H553 "Help F1099-MISC \\ Form 1099-MISC - MISCellaneous income" "" 1 #t "")
	#(N558 "^ F1099-MISC \\ Federal tax withheld" "" 1 #t "")
	#(N563 "^ F1099-MISC \\ State tax withheld" "" 1 #t "")

	#(H473 "Help F1099-R \\ Form 1099-R - Rretirement distributions" "" 1 #t "")
	#(N532 "^ F1099-R \\ IRA federal tax withheld" "" 1 #t "")
	#(N534 "^ F1099-R \\ IRA local tax withheld" "" 1 #t "")
	#(N533 "^ F1099-R \\ IRA state tax withheld" "" 1 #t "")
	#(N529 "^ F1099-R \\ Pension federal tax withheld" "" 1 #t "")
	#(N531 "^ F1099-R \\ Pension local tax withheld" "" 1 #t "")
	#(N530 "^ F1099-R \\ Pension state tax withheld" "" 1 #t "")
	#(N625 "^ F1099-R \\ SIMPLE federal tax withheld" "" 1 #t "")
	#(N627 "^ F1099-R \\ SIMPLE local tax withheld" "" 1 #t "")
	#(N626 "^ F1099-R \\ SIMPLE state tax withheld" "" 1 #t "")

	#(H380 "Help F2106 \\ employee business expenses" "" 1 #t "")
	#(N382 "  F2106 \\ Automobile expenses" "" 1 #t "")
	#(N381 "  F2106 \\ Education expenses" "" 1 #t "")
	#(N391 "  F2106 \\ Employee home office expenses" "" 1 #t "")
	#(N389 "  F2106 \\ Job seeking expenses" "" 1 #t "")
	#(N384 "  F2106 \\ Local transportation expenses" "" 1 #t "")
	#(N386 "  F2106 \\ Meal/entertainment expenses" "" 1 #t "")
	#(N385 "  F2106 \\ Other business expenses" "" 1 #t "")
	#(N390 "  F2106 \\ Special clothing expenses" "" 1 #t "")
	#(N383 "  F2106 \\ Travel (away from home)" "" 1 #t "")

	#(H400 "Help F2441 \\ Form 2441 - child and dependent credit" "" 1 #t "")
	#(N401 "< F2441 \\ Qualifying child/dependent care expenses" "" 3 #f "Childcare")
	#(N402 "< F2441 \\ Qualifying household expenses" "" 3 #f "Household, Decu")

	#(H403 "Help F3903 \\ Form 3903 - moving expenses" "" 1 #t "")
	#(N406 "  F3903 \\ Transport/storage of goods" "" 1 #t "")
	#(N407 "  F3903 \\ Travel/lodging, except meals" "" 1 #t "")

	#(H412 "Help F4684 \\ Form 4684 - casualties and thefts" "" 1 #t "")
	#(N413 "  F4684 \\ Basis of casualty property" "" 1 #t "")

	#(H569 "Help F4835 \\ Form 4835 - farm rental income" "" 1 #t "")
	#(N579 "  F4835 \\ Car and truck expenses" "" 1 #t "")
	#(N580 "  F4835 \\ Chemicals" "" 1 #t "")
	#(N581 "  F4835 \\ Conservation expenses" "" 1 #t "")
	#(N582 "  F4835 \\ Custom hire expenses" "" 1 #t "")
	#(N583 "  F4835 \\ Employee benefit programs" "" 1 #t "")
	#(N584 "  F4835 \\ Feed purchased" "" 1 #t "")
	#(N585 "  F4835 \\ Fertilizers and lime" "" 1 #t "")
	#(N586 "  F4835 \\ Freight and trucking" "" 1 #t "")
	#(N587 "  F4835 \\ Gasoline, fuel, and oil" "" 1 #t "")
	#(N588 "  F4835 \\ Insurance (other than health)" "" 1 #t "")
	#(N589 "  F4835 \\ Interest expense, mortgage" "" 1 #t "")
	#(N590 "  F4835 \\ Interest expense, other" "" 1 #t "")
	#(N591 "  F4835 \\ Labor hired" "" 1 #t "")
	#(N602 "  F4835 \\ Other farm expenses" "" 1 #t "")
	#(N592 "  F4835 \\ Pension/profit-sharing plans" "" 1 #t "")
	#(N594 "  F4835 \\ Rent/lease land, animals" "" 1 #t "")
	#(N593 "  F4835 \\ Rent/lease vehicles, equip." "" 1 #t "")
	#(N595 "  F4835 \\ Repairs and maintenance" "" 1 #t "")
	#(N596 "  F4835 \\ Seeds and plants purchased" "" 1 #t "")
	#(N597 "  F4835 \\ Storage and warehousing" "" 1 #t "")
	#(N598 "  F4835 \\ Supplies purchased" "" 1 #t "")
	#(N599 "  F4835 \\ Taxes" "" 1 #t "")
	#(N600 "  F4835 \\ Utilities" "" 1 #t "")
	#(N601 "  F4835 \\ Vet, breeding, medicine" "" 1 #t "")

	#(H425 "Help F4952 \\ Form 4952 - investment interest" "" 1 #t "")
	#(N426 "  F4952 \\ Investment interest expense" "" 1 #f "_IntExp")

	#(H427 "Help F6252 \\ Form 6252 - income from casual sales" "" 1 #t "")
	#(N432 "  F6252 \\ Expenses of sale" "" 1 #t "")

	#(H441 "Help F8815 \\ Form 8815 - EE U.S. savings bonds sold for education" "" 1 #f "")
	#(N442 "  F8815 \\ Qualified higher education expenses" "" 1 #f "")

	#(H536 "Help F8829 \\ Form 8829 - business use of your home" "" 1 #f "")
	#(N537 "  F8829 \\ Deductible mortgage interest" "" 1 #t "")
	#(N539 "  F8829 \\ Insurance" "" 1 #t "")
	#(N542 "  F8829 \\ Other expenses" "" 1 #t "")
	#(N538 "  F8829 \\ Real estate taxes" "" 1 #t "")
	#(N540 "  F8829 \\ Repairs and maintenance" "" 1 #t "")
	#(N541 "  F8829 \\ Utilities" "" 1 #t "")

	#(H617 "Help F8839 \\ Form 8839 - adoption expenses" "" 1 #f "")
	#(N618 "  F8839 \\ Adoption fees" "" 1 #t "")
	#(N620 "  F8839 \\ Attorney fees" "" 1 #t "")
	#(N619 "  F8839 \\ Court costs" "" 1 #t "")
	#(N622 "  F8839 \\ Other expenses" "" 1 #t "")
	#(N621 "  F8839 \\ Traveling expenses" "" 1 #t "")

	#(N392 "  Home Sale \\ Home Sale worksheets (was F2119)" "" 1 #t "")
	#(N397 "  Home Sale \\ Cost of new home" "" 1 #t "")
	#(N394 "  Home Sale \\ Expense of sale" "" 1 #t "")
	#(N396 "  Home Sale \\ Fixing-up expenses" "" 1 #t "")

        #(H270 "Help Sched A \\ Schedule A - itemized deductions" "" 3 #t "")
	#(N280 "  Sched A \\ Cash charity contributions" "" 1 #f "")
	#(N484 "  Sched A \\ Doctors, dentists, hospitals" "" 1 #f "Medical:Doctor")
	#(N272 "  Sched A \\ Gambling losses" "" 1 #f "")
	#(N545 "  Sched A \\ Home mortgage interest (no 1098)" "" 1 #f "")
	#(N283 "  Sched A \\ Home mortgage interest (1098)" "" 1 #f "Mortgage Int:Ba")
	#(N282 "  Sched A \\ Investment management fees" "" 1 #f "Investment Inte")
	#(N544 "  Sched A \\ Local income taxes" "" 1 #f "")
	#(N274 "  Sched A \\ Medical travel and lodging" "" 1 #f "")
	#(N273 "  Sched A \\ Medicine and drugs" "" 1 #f "Medical:Medicin")
	#(N523 "  Sched A \\ Misc., no 2% AGI limit" "" 1 #f "")
	#(N486 "  Sched A \\ Misc., subject to 2% AGI limit" "" 1 #f "")
	#(N485 "  Sched A \\ Non-cash charity contributions" "" 1 #f "")
	#(N277 "  Sched A \\ Other taxes" "" 1 #f "")
	#(N535 "  Sched A \\ Personal property taxes" "" 1 #f "")
	#(N284 "  Sched A \\ Points paid (no 1098)" "" 1 #f "Mortgage Points")
	#(N276 "  Sched A \\ Real estate taxes" "" 1 #f "Tax:Property")
	#(N522 "  Sched A \\ State estimated tax, qrtrly" "" 1 #f "")
	#(N275 "  Sched A \\ State income taxes" "" 1 #f "")
	#(N271 "  Sched A \\ Subscriptions" "" 1 #f "")
	#(N281 "  Sched A \\ Tax preparation fees" "" 1 #f "")

	#(H285 "Help Sched B \\ Schedule B - interest and dividend income" "" 3 #t "")
	#(N615 "< Sched B \\ Fed tax withheld, dividend income" "" 3 #t "")
	#(N616 "< Sched B \\ Fed tax withheld, interest income" "" 3 #t "")

	#(H291 "Help Sched C \\ Schedule C - self-employment income" "" 3 #t "")
	#(N304 "  Sched C \\ Advertising" "" 1 #t "")
	#(N305 "  Sched C \\ Bad debts from sales/services" "" 1 #t "")
	#(N306 "  Sched C \\ Car and truck expenses" "" 1 #t "")
	#(N307 "  Sched C \\ Commissions and fees" "" 1 #t "")
	#(N494 "  Sched C \\ Cost of Goods Sold - Labor" "" 1 #t "")
	#(N495 "  Sched C \\ Cost of Goods Sold - Materials/supplies" "" 1 #t "")
	#(N496 "  Sched C \\ Cost of Goods Sold - Other costs" "" 1 #t "")
	#(N493 "  Sched C \\ Cost of Goods Sold - Purchases" "" 1 #t "")
	#(N309 "  Sched C \\ Depletion" "" 1 #t "")
	#(N308 "  Sched C \\ Employee benefit programs" "" 1 #t "")
	#(N310 "  Sched C \\ Insurance, other than health" "" 1 #t "")
	#(N311 "  Sched C \\ Interest expense, mortgage" "" 1 #t "")
	#(N312 "  Sched C \\ Interest expense, other" "" 1 #t "")
	#(N298 "  Sched C \\ Legal and professional fees" "" 1 #t "")
	#(N294 "  Sched C \\ Meals and entertainment" "" 1 #t "")
	#(N313 "  Sched C \\ Office expenses" "" 1 #t "")
	#(N302 "  Sched C \\ Other business expenses" "" 1 #t "")
	#(N314 "  Sched C \\ Pension/profit sharing plans" "" 1 #t "")
	#(N300 "  Sched C \\ Rent/lease other business property" "" 1 #t "")
	#(N299 "  Sched C \\ Rent/lease vehicles, equip." "" 1 #t "")
	#(N315 "  Sched C \\ Repairs and maintenance" "" 1 #t "")
	#(N296 "  Sched C \\ Returns and allowances" "" 1 #t "")
	#(N301 "  Sched C \\ Supplies (not from Cost of Goods Sold)" "" 1 #t "")
	#(N316 "  Sched C \\ Taxes and licenses" "" 1 #t "")
	#(N317 "  Sched C \\ Travel" "" 1 #t "")
	#(N318 "  Sched C \\ Utilities" "" 1 #t "")
	#(N297 "  Sched C \\ Wages paid" "" 1 #t "")

	#(H325 "Help Sched E \\ Schedule E - rental and royalty income" "" 3 #t "")
	#(N328 "  Sched E \\ Advertising" "" 1 #t "")
	#(N329 "  Sched E \\ Auto and travel" "" 1 #t "")
	#(N330 "  Sched E \\ Cleaning and maintenance" "" 1 #t "")
	#(N331 "  Sched E \\ Commissions" "" 1 #t "")
	#(N332 "  Sched E \\ Insurance" "" 1 #t "")
	#(N333 "  Sched E \\ Legal and professional fees" "" 1 #t "")
	#(N502 "  Sched E \\ Management fees" "" 1 #t "")
	#(N334 "  Sched E \\ Mortgage interest expense" "" 1 #t "")
	#(N341 "  Sched E \\ Other expenses" "" 1 #t "")
	#(N335 "  Sched E \\ Other interest expense" "" 1 #t "")
	#(N336 "  Sched E \\ Repairs" "" 1 #t "")
	#(N337 "  Sched E \\ Supplies" "" 1 #t "")
	#(N338 "  Sched E \\ Taxes" "" 1 #t "")
	#(N339 "  Sched E \\ Utilities" "" 1 #t "")

	#(H343 "Help Sched F \\ Schedule F - Farm income and expense" "" 3 #t "")
	#(N543 "  Sched F \\ Car and truck expenses" "" 1 #t "")
	#(N366 "  Sched F \\ Chemicals" "" 1 #t "")
	#(N362 "  Sched F \\ Conservation expenses" "" 1 #t "")
	#(N378 "  Sched F \\ Cost of resale livestock/items" "" 1 #t "")
	#(N367 "  Sched F \\ Custom hire expenses" "" 1 #t "")
	#(N364 "  Sched F \\ Employee benefit programs" "" 1 #t "")
	#(N350 "  Sched F \\ Feed purchased" "" 1 #t "")
	#(N352 "  Sched F \\ Fertilizers and lime" "" 1 #t "")
	#(N361 "  Sched F \\ Freight and trucking" "" 1 #t "")
	#(N356 "  Sched F \\ Gasoline, fuel, and oil" "" 1 #t "")
	#(N359 "  Sched F \\ Insurance, other than health" "" 1 #t "")
	#(N346 "  Sched F \\ Interest expense, mortgage" "" 1 #t "")
	#(N347 "  Sched F \\ Interest expense, other" "" 1 #t "")
	#(N344 "  Sched F \\ Labor hired" "" 1 #t "")
	#(N365 "  Sched F \\ Other farm expenses" "" 1 #t "")
	#(N363 "  Sched F \\ Pension/profit sharing plans" "" 1 #t "")
	#(N348 "  Sched F \\ Rent/lease land, animals" "" 1 #t "")
	#(N349 "  Sched F \\ Rent/lease vehicles, equip." "" 1 #t "")
	#(N345 "  Sched F \\ Repairs and maintenance" "" 1 #t "")
	#(N351 "  Sched F \\ Seeds and plants purchased" "" 1 #t "")
	#(N357 "  Sched F \\ Storage and warehousing" "" 1 #t "")
	#(N353 "  Sched F \\ Supplies purchased" "" 1 #t "")
	#(N358 "  Sched F \\ Taxes" "" 1 #t "")
	#(N360 "  Sched F \\ Utilities" "" 1 #t "")
	#(N355 "  Sched F \\ Vet, breeding, and medicine" "" 1 #t "")

	#(H565 "Help Sched H \\ Schedule H - Household employees" "" 3 #t "")
	#(N567 "^ Sched H \\ Cash wages paid" "" 1 #f "")
	#(N568 "^ Sched H \\ Federal tax withheld" "" 1 #f "")

	#(H458 "Help W-2 \\ Form W-2 - Wages earned and taxes withheld" "" 3 #t "")
	#(N461 "^ W-2 \\ Federal tax withheld, self" "" 1 #t "Tax:Fed")
	#(N507 "^ W-2 \\ Federal tax withheld, spouse" "" 1 #t "")
	#(N463 "^ W-2 \\ Local tax withheld, self" "" 1 #t "")
	#(N509 "^ W-2 \\ Local tax withheld, spouse" "" 1 #t "")
	#(N480 "^ W-2 \\ Medicare tax withheld, self" "" 1 #t "Tax:Medicare")
	#(N510 "^ W-2 \\ Medicare tax withheld, spouse" "" 1 #t "")
	#(N462 "^ W-2 \\ Social Security tax withheld, self" "" 1 #t "Tax:Soc Sec")
	#(N508 "^ W-2 \\ Social Security tax withheld, spouse" "" 1 #t "")
	#(N464 "^ W-2 \\ State tax withheld, self" "" 1 #t "Tax:State")
	#(N511 "^ W-2 \\ State tax withheld, spouse" "" 1 #t "")

	#(H547 "Help W-2G \\ Form W-2G - gambling winnings" "" 3 #t "")
	#(N550 "^ W-2G \\ Federal tax withheld" "" 1 #t "")
	#(N551 "^ W-2G \\ State tax withheld" "" 1 #t "")))
