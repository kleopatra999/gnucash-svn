%module sw_app_utils
%{
/* Includes the header in the wrapper code */
#include <config.h>
#include <option-util.h>
#include <gnc-euro.h>
#include <gnc-exp-parser.h>
#include <gnc-ui-util.h>
#include <gnc-gettext-util.h>
#include <gnc-helpers.h>
#include <gnc-accounting-period.h>
#include <gnc-session.h>
#include <gnc-component-manager.h>

#include <g-wrap-wct.h> //Temporary. Adds no link dep?!?
#include "engine-helpers.h"
%}

%typemap(in) GNCPrintAmountInfo "$1 = gnc_scm2printinfo($input);"
%typemap(out) GNCPrintAmountInfo "$result = gnc_printinfo2scm($1);"

%typemap(out) GncCommodityList {
  SCM list = SCM_EOL;
  GList *node;

  for (node = $1; node; node = node->next)
    list = scm_cons(gnc_quoteinfo2scm(node->data), list);

  $result = scm_reverse(list);
}

// Temporary SWIG<->G-wrap converters for engine types
%typemap(in) QofBook * "$1 = (QofBook *)gw_wcp_get_ptr($input);"
%typemap(out) QofBook * {
  $result = gw_wcp_assimilate_ptr($1, scm_c_eval_string("<gnc:Book*>"));
}
%typemap(in) gboolean "$1 = SCM_NFALSEP($input) ? TRUE : FALSE;"
%typemap(out) gboolean "$result = $1 ? SCM_BOOL_T : SCM_BOOL_F;"

%typemap(in) gnc_numeric "$1 = gnc_scm_to_numeric($input);"
%typemap(out) gnc_numeric "$result = gnc_numeric_to_scm($1);"
%typemap(in) gnc_commodity * "$1 = (gnc_commodity *)gw_wcp_get_ptr($input);"
%typemap(out) gnc_commodity * {
  $result = gw_wcp_assimilate_ptr($1, scm_c_eval_string("<gnc:commodity*>"));
}
%typemap(in) Account * "$1 = (Account *)gw_wcp_get_ptr($input);"
%typemap(out) Account * {
  $result = gw_wcp_assimilate_ptr($1, scm_c_eval_string("<gnc:Account*>"));
}
%typemap(in) AccountGroup * "$1 = (AccountGroup *)gw_wcp_get_ptr($input);"
%typemap(out) AccountGroup * {
  $result = gw_wcp_assimilate_ptr($1, scm_c_eval_string("<gnc:AccountGroup*>"));
}
%typemap(in) gnc_commodity_table * {
  $1 = (gnc_commodity_table *)gw_wcp_get_ptr($input);
}
%typemap(out) gnc_commodity_table * {
  $result = gw_wcp_assimilate_ptr($1,
       scm_c_eval_string("<gnc:commodity-table*>"));
}
%typemap(in) gnc_commodity * {
  $1 = (gnc_commodity *)gw_wcp_get_ptr($input);
}
%typemap(out) gnc_commodity * {
  $result = gw_wcp_assimilate_ptr($1,
       scm_c_eval_string("<gnc:commodity*>"));
}

// End of temporary typemaps.

typedef void (*GNCOptionChangeCallback) (gpointer user_data);
typedef int GNCOptionDBHandle;

QofBook * gnc_get_current_book (void);
AccountGroup * gnc_get_current_group (void);

char * gnc_gettext_helper(const char *string);

GNCOptionDB * gnc_option_db_new(SCM guile_options);
void gnc_option_db_destroy(GNCOptionDB *odb);

void gnc_option_db_set_option_selectable_by_name(SCM guile_option,
      const char *section, const char *name, gboolean selectable);

%inline %{
typedef GList GncCommodityList;

GncCommodityList *
gnc_commodity_table_get_quotable_commodities(const gnc_commodity_table * table);
%}

gnc_commodity * gnc_default_currency (void);
gnc_commodity * gnc_default_report_currency (void);

void gncp_option_invoke_callback(GNCOptionChangeCallback callback, void *data);
void gnc_option_db_register_option(GNCOptionDBHandle handle,
        SCM guile_option);

const char * gnc_locale_default_iso_currency_code (void);

char * gnc_account_get_full_name (const Account *account);

GNCPrintAmountInfo gnc_default_print_info (gboolean use_symbol);
GNCPrintAmountInfo gnc_account_print_info (const Account *account,
        gboolean use_symbol);
GNCPrintAmountInfo gnc_commodity_print_info (const gnc_commodity *commodity,
        gboolean use_symbol);
GNCPrintAmountInfo gnc_share_print_info_places (int decplaces);
const char * xaccPrintAmount (gnc_numeric val, GNCPrintAmountInfo info);

gboolean gnc_reverse_balance (const Account *account);

gboolean gnc_is_euro_currency(const gnc_commodity * currency);
gnc_numeric gnc_convert_to_euro(const gnc_commodity * currency,
        gnc_numeric value);
gnc_numeric gnc_convert_from_euro(const gnc_commodity * currency,
        gnc_numeric value);


typedef int time_t;
time_t gnc_accounting_period_fiscal_start(void);
time_t gnc_accounting_period_fiscal_end(void);

//temporary, until QofIdType is swigified

%typemap(in) QofIdType { $1 = gw_wcp_get_ptr($input); }
SCM gnc_make_kvp_options(QofIdType id_type);
void gnc_register_kvp_option_generator(QofIdType id_type, SCM generator);

