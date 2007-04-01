/********************************************************************\
 * Copyright (C) 2000,2001 Gnumatic Inc.                            *
 *                                                                  *
 * This program is free software; you can redistribute it and/or    *
 * modify it under the terms of the GNU General Public License as   *
 * published by the Free Software Foundation; either version 2 of   *
 * the License, or (at your option) any later version.              *
 *                                                                  *
 * This program is distributed in the hope that it will be useful,  *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    *
 * GNU General Public License for more details.                     *
 *                                                                  *
 * You should have received a copy of the GNU General Public License*
 * along with this program; if not, contact:                        *
 *                                                                  *
 * Free Software Foundation           Voice:  +1-617-542-5942       *
 * 51 Franklin Street, Fifth Floor    Fax:    +1-617-542-2652       *
 * Boston, MA  02110-1301,  USA       gnu@gnu.org                   *
\********************************************************************/

#include "config.h"

#include <glib.h>
#include <glib/gstdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>
#include <errno.h>
#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#include "gnc-engine.h"
#include "gnc-pricedb-p.h"
#include "Scrub.h"
#include "SX-book.h"
#include "SX-book-p.h"
#include "Transaction.h"
#include "TransactionP.h"
#include "TransLog.h"
#include "sixtp-dom-parsers.h"
#include "io-gncxml-v2.h"
#include "io-gncxml-gen.h"

#include "sixtp.h"
#include "sixtp-parsers.h"
#include "sixtp-utils.h"
#include "gnc-xml.h"
#include "io-utils.h"

static QofLogModule log_module = GNC_MOD_IO;

/* map pointers, e.g. of type FILE*, to GThreads */
static GHashTable *threads = NULL;
G_LOCK_DEFINE_STATIC(threads);

typedef struct {
  gint fd;
  gchar *filename;
  gchar *perms;
  gboolean compress;
} gz_thread_params_t;

static pid_t gzip_child_pid = 0;

/* Callback structure */
struct file_backend {
  gboolean        ok;
  gpointer        data;
  sixtp_gdv2    * gd;
  const char    * tag;
  sixtp         * parser;
  FILE          * out;
  QofBook       * book;
};

#define GNC_V2_STRING "gnc-v2"
extern const gchar *gnc_v2_book_version_string;        /* see gnc-book-xml-v2 */

void
run_callback(sixtp_gdv2 *data, const char *type)
{
    if(data->countCallback)
    {
        data->countCallback(data, type);
    }
}

static void
clear_up_account_commodity(
    gnc_commodity_table *tbl, Account *act,
    gnc_commodity * (*getter) (const Account *account),
    void (*setter) (Account *account, gnc_commodity *comm),
    int (*scu_getter) (const Account *account),
    void (*scu_setter) (Account *account, int scu))
{
    gnc_commodity *gcom;
    gnc_commodity *com = getter(act);
    int old_scu;

    if (scu_getter)
      old_scu = scu_getter(act);
    else
      old_scu = 0;

    if(!com)
    {
        return;
    }
    
    gcom = gnc_commodity_table_lookup(tbl, gnc_commodity_get_namespace(com),
                                      gnc_commodity_get_mnemonic(com));

    if (gcom == com)
    {
        return;
    }
    else if(!gcom)
    {
        PWARN("unable to find global commodity for %s adding new",
                  gnc_commodity_get_unique_name(com));
        gnc_commodity_table_insert(tbl, com);
    }
    else
    {
        setter(act, gcom);
        if (old_scu != 0 && scu_setter)
          scu_setter(act, old_scu);
        gnc_commodity_destroy(com);
    }
}

static void
clear_up_transaction_commodity(
    gnc_commodity_table *tbl, Transaction *trans,
    gnc_commodity * (*getter) (const Transaction *trans),
    void (*setter) (Transaction *trans, gnc_commodity *comm))
{
    gnc_commodity *gcom;
    gnc_commodity *com = getter(trans);

    if(!com)
    {
        return;
    }
    
    gcom = gnc_commodity_table_lookup(tbl, gnc_commodity_get_namespace(com),
                                      gnc_commodity_get_mnemonic(com));

    if(gcom == com)
    {
        return;
    }
    else if(!gcom)
    {
        PWARN("unable to find global commodity for %s adding new",
                  gnc_commodity_get_unique_name(com));
        gnc_commodity_table_insert(tbl, com);
    }
    else
    {
        xaccTransBeginEdit(trans);
        setter(trans, gcom);
        xaccTransCommitEdit(trans);
        gnc_commodity_destroy(com);
    }
}

static gboolean
add_account_local(sixtp_gdv2 *data, Account *act)
{
    gnc_commodity_table *table;
    Account *parent, *root;
    int type;

    table = gnc_book_get_commodity_table (data->book);

    clear_up_account_commodity(table, act,
                                       DxaccAccountGetCurrency,
                                       DxaccAccountSetCurrency,
                                       NULL, NULL);

    clear_up_account_commodity(table, act,
                               xaccAccountGetCommodity,
                               xaccAccountSetCommodity,
                               xaccAccountGetCommoditySCUi,
                               xaccAccountSetCommoditySCU);

    xaccAccountScrubCommodity (act);
    xaccAccountScrubKvp (act);

    /* Backwards compatability.  If there's no parent, see if this
     * account is of type ROOT.  If not, find or create a ROOT
     * account and make that the parent. */
    type = xaccAccountGetType(act);
    if (type == ACCT_TYPE_ROOT) {
      gnc_book_set_root_account(data->book, act);
    } else {
      parent = gnc_account_get_parent(act);
      if (parent == NULL) {
	root = gnc_book_get_root_account(data->book);
	gnc_account_append_child(root, act);
      }
    }

    data->counter.accounts_loaded++;
    run_callback(data, "account");

    return FALSE;
}

static gboolean
add_book_local(sixtp_gdv2 *data, QofBook *book)
{
    data->counter.books_loaded++;
    run_callback(data, "book");

    return FALSE;
}

static gboolean
add_commodity_local(sixtp_gdv2 *data, gnc_commodity *com)
{
    gnc_commodity_table *table;

    table = gnc_book_get_commodity_table (data->book);

    gnc_commodity_table_insert(table, com);

    data->counter.commodities_loaded++;
    run_callback(data, "commodities");

    return TRUE;
}

static gboolean
add_transaction_local(sixtp_gdv2 *data, Transaction *trn)
{
    gnc_commodity_table *table;
    
    table = gnc_book_get_commodity_table (data->book);

    xaccTransBeginEdit (trn);
    clear_up_transaction_commodity(table, trn,
                                   xaccTransGetCurrency,
                                   xaccTransSetCurrency);

    xaccTransScrubCurrency (trn);
    xaccTransCommitEdit (trn);

    data->counter.transactions_loaded++;
    run_callback(data, "transaction");
    return TRUE;
}

static gboolean
add_schedXaction_local(sixtp_gdv2 *data, SchedXaction *sx)
{
     SchedXactions *sxes;
     sxes = gnc_book_get_schedxactions(data->book);
     gnc_sxes_add_sx(sxes, sx);
     data->counter.schedXactions_loaded++;
     run_callback(data, "schedXactions");
     return TRUE;
}

static gboolean
add_template_transaction_local( sixtp_gdv2 *data,
                                gnc_template_xaction_data *txd )
{
    GList *n;
    Account *tmpAcct;
    Account *acctRoot = NULL;
    QofBook *book;

    book = data->book;

    /* expect a struct of: */
    /* . template accounts. */
    /* . transactions in those accounts. */
    for ( n = txd->accts; n; n = n->next ) {
        if ( gnc_account_get_parent( (Account*)n->data ) == NULL ) {
            /* remove the gnc_book_init-created account of the same name */
            acctRoot = gnc_book_get_template_root(book);
            tmpAcct = gnc_account_lookup_by_name( acctRoot,
                                    xaccAccountGetName( (Account*)n->data ) );
            if ( tmpAcct != NULL ) {
/* XXX hack alert FIXME .... Should this be 'Remove', or 'Destroy'?
 * If we just remove, then this seems to be a memory leak to me, since
 * it is never reparented.  Shouldn't it be a Destroy ???
 */
                gnc_account_remove_child( acctRoot, tmpAcct );
            }

            gnc_account_append_child( acctRoot, (Account*)n->data );
        }

    }

    for ( n = txd->transactions; n; n = n->next ) {
        /* insert transactions into accounts */
        add_transaction_local( data, (Transaction*)n->data );
    }

    return TRUE;
}

static gboolean
add_pricedb_local(sixtp_gdv2 *data, GNCPriceDB *db)
{
    /* gnc_pricedb_print_contents(db, stdout); */
    return TRUE;
}

static void
do_counter_cb (const char *type, gpointer data_p, gpointer be_data_p)
{
  GncXmlDataType_t *data = data_p;
  struct file_backend *be_data = be_data_p;

  g_return_if_fail (type && data && be_data);
  g_return_if_fail (data->version == GNC_FILE_BACKEND_VERS);

  if (be_data->ok == TRUE)
    return;

  if (!safe_strcmp (be_data->tag, data->type_name))
    be_data->ok = TRUE;

  /* XXX: should we do anything with this counter? */
}

static gboolean
gnc_counter_end_handler(gpointer data_for_children,
                        GSList* data_from_children, GSList* sibling_data,
                        gpointer parent_data, gpointer global_data,
                        gpointer *result, const gchar *tag)
{
    char *strval;
    gint64 val;
    char *type;
    xmlNodePtr tree = (xmlNodePtr)data_for_children;
    gxpf_data *gdata = (gxpf_data*)global_data;
    sixtp_gdv2 *sixdata = (sixtp_gdv2*)gdata->parsedata;
    gboolean ret = TRUE;
    
    if (parent_data)
        return TRUE;

    /* OK.  For some messed up reason this is getting called again with a
       NULL tag.  So we ignore those cases */
    if (!tag)
        return TRUE;
    
    g_return_val_if_fail(tree, FALSE);

    /* Note: BADXML.
     *
     * This is invalid xml because the namespace isn't declared in the
     * tag itself. This should be changed to 'type' at some point. */
    type = (char*)xmlGetProp(tree, BAD_CAST "cd:type");
    strval = dom_tree_to_text(tree);
    if (!string_to_gint64(strval, &val))
    {
        PERR ("string_to_gint64 failed with input: %s",
                  strval ? strval : "(null)");
        ret = FALSE;
    }
    else if (safe_strcmp(type, "transaction") == 0)
    {
        sixdata->counter.transactions_total = val;
    }
    else if (safe_strcmp(type, "account") == 0)
    {
        sixdata->counter.accounts_total = val;
    }
    else if (safe_strcmp(type, "book") == 0)
    {
        sixdata->counter.books_total = val;
    }
    else if (safe_strcmp(type, "commodity") == 0)
    {
        sixdata->counter.commodities_total = val;
    }
    else if (safe_strcmp(type, "schedxaction") == 0)
    {
        sixdata->counter.schedXactions_total = val;
    }
    else if (safe_strcmp(type, "budget") == 0)
    {
        sixdata->counter.budgets_total = val;
    }
    else
    {
      struct file_backend be_data;

      be_data.ok = FALSE;
      be_data.tag = type;

      qof_object_foreach_backend (GNC_FILE_BACKEND, do_counter_cb, &be_data);

      if (be_data.ok == FALSE)
      {
        PERR("Unknown type: %s", type ? type : "(null)");
	/* Do *NOT* flag this as an error. Gnucash 1.8 writes invalid
	 * xml by writing the 'cd:type' attribute without providing
	 * the namespace in the gnc:count-data tag.  The parser is
	 * entirely within its rights to refuse to read this bad
	 * attribute. Gnucash will function correctly without the data
	 * in this tag, so just let the error pass. */
        ret = TRUE;
      }
    }

    g_free (strval);
    xmlFree (type);
    xmlFreeNode(tree);
    return ret;
}

static sixtp*
gnc_counter_sixtp_parser_create(void)
{
     return sixtp_dom_parser_new(gnc_counter_end_handler, NULL, NULL);
}

static void
debug_print_counter_data(load_counter *data)
{
    DEBUG("Transactions: Total: %d, Loaded: %d",
          data->transactions_total, data->transactions_loaded);
    DEBUG("Accounts: Total: %d, Loaded: %d",
          data->accounts_total, data->accounts_loaded);
    DEBUG("Books: Total: %d, Loaded: %d",
          data->books_total, data->books_loaded);
    DEBUG("Commodities: Total: %d, Loaded: %d",
          data->commodities_total, data->commodities_loaded);
    DEBUG("Scheduled Tansactions: Total: %d, Loaded: %d",
          data->schedXactions_total, data->schedXactions_loaded);
    DEBUG("Budgets: Total: %d, Loaded: %d",
          data->budgets_total, data->budgets_loaded);
}

static void
file_rw_feedback (sixtp_gdv2 *gd, const char *type)
{
    load_counter *counter;
    int loaded, total, percentage;

    g_assert(gd != NULL);
    if (!gd->gui_display_fn)
      return;

    counter = &gd->counter;
    loaded = counter->transactions_loaded + counter->accounts_loaded +
      counter->books_loaded + counter->commodities_loaded +
      counter->schedXactions_loaded + counter->budgets_loaded;
    total = counter->transactions_total + counter->accounts_total +
      counter->books_total + counter->commodities_total +
      counter->schedXactions_total + counter->budgets_total;
    if (total == 0)
      total = 1;

    percentage = (loaded * 100)/total;
    if (percentage > 100) {
      /* FIXME: Perhaps the below should be replaced by:
	 print_counter_data(counter); */
//      printf("Transactions: Total: %d, Loaded: %d\n",
//             counter->transactions_total, counter->transactions_loaded);
//      printf("Accounts: Total: %d, Loaded: %d\n",
//             counter->accounts_total, counter->accounts_loaded);
//      printf("Books: Total: %d, Loaded: %d\n",
//             counter->books_total, counter->books_loaded);
//      printf("Commodities: Total: %d, Loaded: %d\n",
//             counter->commodities_total, counter->commodities_loaded);
//      printf("Scheduled Tansactions: Total: %d, Loaded: %d\n",
//             counter->schedXactions_total, counter->schedXactions_loaded);
//      printf("Budgets: Total: %d, Loaded: %d\n",
//	     counter->budgets_total, counter->budgets_loaded);
    }
    gd->gui_display_fn(NULL, percentage);
}

static const char *BOOK_TAG = "gnc:book";
static const char *BOOK_ID_TAG = "book:id";
static const char *BOOK_SLOTS_TAG = "book:slots";
static const char *ACCOUNT_TAG = "gnc:account";
static const char *PRICEDB_TAG = "gnc:pricedb";
static const char *COMMODITY_TAG = "gnc:commodity";
static const char *COUNT_DATA_TAG = "gnc:count-data";
static const char *TRANSACTION_TAG = "gnc:transaction";
static const char *SCHEDXACTION_TAG = "gnc:schedxaction";
static const char *TEMPLATE_TRANSACTION_TAG = "gnc:template-transactions";
static const char *BUDGET_TAG = "gnc:budget";

static void
add_item_cb (const char *type, gpointer data_p, gpointer be_data_p)
{
  GncXmlDataType_t *data = data_p;
  struct file_backend *be_data = be_data_p;

  g_return_if_fail (type && data && be_data);
  g_return_if_fail (data->version == GNC_FILE_BACKEND_VERS);

  if (be_data->ok)
    return;

  if (!safe_strcmp (be_data->tag, data->type_name)) {
    if (data->add_item)
      (data->add_item)(be_data->gd, be_data->data);

    be_data->ok = TRUE;
  }
}

static gboolean
book_callback(const char *tag, gpointer globaldata, gpointer data)
{
    sixtp_gdv2 *gd = (sixtp_gdv2*)globaldata;

    if(safe_strcmp(tag, ACCOUNT_TAG) == 0)
    {
        add_account_local(gd, (Account*)data);
    }
    else if(safe_strcmp(tag, PRICEDB_TAG) == 0)
    {
        add_pricedb_local(gd, (GNCPriceDB*)data);
    }
    else if(safe_strcmp(tag, COMMODITY_TAG) == 0)
    {
        add_commodity_local(gd, (gnc_commodity*)data);
    }
    else if(safe_strcmp(tag, TRANSACTION_TAG) == 0)
    {
        add_transaction_local(gd, (Transaction*)data);
    }
    else if(safe_strcmp(tag, SCHEDXACTION_TAG) == 0)
    {
        add_schedXaction_local(gd, (SchedXaction*)data);
    }
    else if(safe_strcmp(tag, TEMPLATE_TRANSACTION_TAG) == 0)
    {
        add_template_transaction_local( gd, (gnc_template_xaction_data*)data );
    }
    else if(safe_strcmp(tag, BUDGET_TAG) == 0)
    {
        // Nothing needed here.
    }
    else
    {
      struct file_backend be_data;

      be_data.ok = FALSE;
      be_data.tag = tag;
      be_data.gd = gd;
      be_data.data = data;

      qof_object_foreach_backend (GNC_FILE_BACKEND, add_item_cb, &be_data);

      if (be_data.ok == FALSE)
      {
        PWARN ("unexpected tag %s", tag);
      }
    }
    return TRUE;
}

static gboolean
generic_callback(const char *tag, gpointer globaldata, gpointer data)
{
    sixtp_gdv2 *gd = (sixtp_gdv2*)globaldata;

    if(safe_strcmp(tag, BOOK_TAG) == 0)
    {
        add_book_local(gd, (QofBook*)data);
        book_callback(tag, globaldata, data);
    }
    else
    {
        // PWARN ("importing pre-book-style XML data file");
        book_callback(tag, globaldata, data);
    }
    return TRUE;
}

static void
add_parser_cb (const char *type, gpointer data_p, gpointer be_data_p)
{
  GncXmlDataType_t *data = data_p;
  struct file_backend *be_data = be_data_p;

  g_return_if_fail (type && data && be_data);
  g_return_if_fail (data->version == GNC_FILE_BACKEND_VERS);

  if (be_data->ok == FALSE)
    return;

  if (data->create_parser)
    if(!sixtp_add_some_sub_parsers(
           be_data->parser, TRUE,
           data->type_name, (data->create_parser)(),
           NULL, NULL))
      be_data->ok = FALSE;
}

static void
scrub_cb (const char *type, gpointer data_p, gpointer be_data_p)
{
  GncXmlDataType_t *data = data_p;
  struct file_backend *be_data = be_data_p;

  g_return_if_fail (type && data && be_data);
  g_return_if_fail (data->version == GNC_FILE_BACKEND_VERS);

  if (data->scrub)
    (data->scrub)(be_data->book);
}

static sixtp_gdv2 *
gnc_sixtp_gdv2_new (
    QofBook *book,
    gboolean exporting,
    countCallbackFn countcallback,
    QofBePercentageFunc gui_display_fn)
{
    sixtp_gdv2 *gd = g_new0(sixtp_gdv2, 1);

    if (gd == NULL) return NULL;

    gd->book = book;
    gd->counter.accounts_loaded = 0;
    gd->counter.accounts_total = 0;
    gd->counter.books_loaded = 0;
    gd->counter.books_total = 0;
    gd->counter.commodities_loaded = 0;
    gd->counter.commodities_total = 0;
    gd->counter.transactions_loaded = 0;
    gd->counter.transactions_total = 0;
    gd->counter.prices_loaded = 0;
    gd->counter.prices_total = 0;
    gd->counter.schedXactions_loaded = 0;
    gd->counter.schedXactions_total = 0;
    gd->counter.budgets_loaded = 0;
    gd->counter.budgets_total = 0;
    gd->exporting = exporting;
    gd->countCallback = countcallback;
    gd->gui_display_fn = gui_display_fn;
    return gd;
}

static gboolean
qof_session_load_from_xml_file_v2_full(
    FileBackend *fbe, QofBook *book,
    sixtp_push_handler push_handler, gpointer push_user_data)
{
    Account *root;
    QofBackend *be = &fbe->be;
    sixtp_gdv2 *gd;
    sixtp *top_parser;
    sixtp *main_parser;
    sixtp *book_parser;
    struct file_backend be_data;
    gboolean retval;

    gd = gnc_sixtp_gdv2_new(book, FALSE, file_rw_feedback, be->percentage);

    top_parser = sixtp_new();
    main_parser = sixtp_new();
    book_parser = sixtp_new();

    if(!sixtp_add_some_sub_parsers(
        top_parser, TRUE,
        GNC_V2_STRING, main_parser,
        NULL, NULL))
    {
        goto bail;
    }

    if(!sixtp_add_some_sub_parsers(
           main_parser, TRUE,
           COUNT_DATA_TAG, gnc_counter_sixtp_parser_create(),
           BOOK_TAG, book_parser,

           /* the following are present here only to support 
            * the older, pre-book format.  Basically, the top-level 
            * book is implicit. */
           PRICEDB_TAG, gnc_pricedb_sixtp_parser_create(),
           COMMODITY_TAG, gnc_commodity_sixtp_parser_create(),
           ACCOUNT_TAG, gnc_account_sixtp_parser_create(),
           TRANSACTION_TAG, gnc_transaction_sixtp_parser_create(),
           SCHEDXACTION_TAG, gnc_schedXaction_sixtp_parser_create(),
           TEMPLATE_TRANSACTION_TAG, gnc_template_transaction_sixtp_parser_create(),
           NULL, NULL))
    {
        goto bail;
    }

    if(!sixtp_add_some_sub_parsers(
           book_parser, TRUE,
           BOOK_ID_TAG, gnc_book_id_sixtp_parser_create(),
           BOOK_SLOTS_TAG, gnc_book_slots_sixtp_parser_create(),
           COUNT_DATA_TAG, gnc_counter_sixtp_parser_create(),
           PRICEDB_TAG, gnc_pricedb_sixtp_parser_create(),
           COMMODITY_TAG, gnc_commodity_sixtp_parser_create(),
           ACCOUNT_TAG, gnc_account_sixtp_parser_create(),
           BUDGET_TAG, gnc_budget_sixtp_parser_create(),
           TRANSACTION_TAG, gnc_transaction_sixtp_parser_create(),
           SCHEDXACTION_TAG, gnc_schedXaction_sixtp_parser_create(),
           TEMPLATE_TRANSACTION_TAG, gnc_template_transaction_sixtp_parser_create(),
           NULL, NULL))
    {
        goto bail;
    }

    be_data.ok = TRUE;
    be_data.parser = book_parser;
    qof_object_foreach_backend (GNC_FILE_BACKEND, add_parser_cb, &be_data);
    if (be_data.ok == FALSE)
      goto bail;

    /* stop logging while we load */
    xaccLogDisable ();
    xaccDisableDataScrubbing();

    if (push_handler) {
        gpointer parse_result = NULL;
        gxpf_data gpdata;

        gpdata.cb = generic_callback;
        gpdata.parsedata = gd;
        gpdata.bookdata = book;

        retval = sixtp_parse_push(top_parser, push_handler, push_user_data,
                                  NULL, &gpdata, &parse_result);
    } else {
        retval = gnc_xml_parse_file(top_parser, fbe->fullpath,
                                    generic_callback, gd, book);
    }

    if (!retval) {
        sixtp_destroy(top_parser);
        xaccLogEnable ();
        xaccEnableDataScrubbing();
        goto bail;
    }
    debug_print_counter_data(&gd->counter);

    /* destroy the parser */
    sixtp_destroy (top_parser);
    g_free(gd);

    xaccEnableDataScrubbing();

    /* Mark the book as saved */
    qof_book_mark_saved (book);

    /* Call individual scrub functions */
    memset(&be_data, 0, sizeof(be_data));
    be_data.book = book;
    qof_object_foreach_backend (GNC_FILE_BACKEND, scrub_cb, &be_data);

    /* fix price quote sources */
    root = gnc_book_get_root_account(book);
    xaccAccountTreeScrubQuoteSources (root, gnc_book_get_commodity_table(book));

    /* Fix account and transaction commodities */
    xaccAccountTreeScrubCommodities (root);

    /* Fix split amount/value */
    xaccAccountTreeScrubSplits (root);

    /* commit all groups, this completes the BeginEdit started when the
     * account_end_handler finished reading the account.
     */
    gnc_account_foreach_descendant(root,
				   (AccountCb) xaccAccountCommitEdit,
				   NULL);

    /* start logging again */
    xaccLogEnable ();

    return TRUE;

 bail:
    g_free(gd);
    return FALSE;
}

gboolean
qof_session_load_from_xml_file_v2(FileBackend *fbe, QofBook *book)
{
    return qof_session_load_from_xml_file_v2_full(fbe, book, NULL, NULL);
}

/***********************************************************************/

static void
write_counts(FILE* out, ...)
{
    va_list ap;
    char *type;

    va_start(ap, out);
    type = va_arg(ap, char *);

    while(type)
    {
        int amount = va_arg(ap, int);

        if(amount != 0)
        {
#if GNUCASH_REALLY_BUILD_AN_XML_TREE_ON_OUTPUT
	    char *val;
	    xmlNodePtr node;

            val = g_strdup_printf("%d", amount);

            node = xmlNewNode(NULL, BAD_CAST COUNT_DATA_TAG);
	    /* Note: BADXML.
	     *
	     * This is invalid xml because the namespace isn't
	     * declared in the tag itself. This should be changed to
	     * 'type' at some point. */
            xmlSetProp(node, BAD_CAST "cd:type", BAD_CAST type);
            xmlNodeAddContent(node, BAD_CAST val);

            xmlElemDump(out, NULL, node);
            fprintf(out, "\n");
        
            g_free(val);
            xmlFreeNode(node);
#else
            fprintf(out, "<%s %s=\"%s\">%d</%s>\n",
		    COUNT_DATA_TAG, "cd:type", type, amount, COUNT_DATA_TAG);
#endif
        
        }
        
        type = va_arg(ap, char *);
    }

    va_end(ap);
}

static gint
compare_namespaces(gconstpointer a, gconstpointer b)
{
  const gchar *sa = (const gchar *) a;
  const gchar *sb = (const gchar *) b;
  return(safe_strcmp(sa, sb));
}

static gint
compare_commodity_ids(gconstpointer a, gconstpointer b)
{
  const gnc_commodity *ca = (const gnc_commodity *) a;
  const gnc_commodity *cb = (const gnc_commodity *) b;
  return(safe_strcmp(gnc_commodity_get_mnemonic(ca),
                     gnc_commodity_get_mnemonic(cb)));
}

static void write_pricedb (FILE *out, QofBook *book, sixtp_gdv2 *gd);
static void write_transactions (FILE *out, QofBook *book, sixtp_gdv2 *gd);
static void write_template_transaction_data (FILE *out, QofBook *book, sixtp_gdv2 *gd);
static void write_schedXactions(FILE *out, QofBook *book, sixtp_gdv2 *gd);
static void write_budget (QofInstance *ent, gpointer data);

static void
write_counts_cb (const char *type, gpointer data_p, gpointer be_data_p)
{
  GncXmlDataType_t *data = data_p;
  struct file_backend *be_data = be_data_p;

  g_return_if_fail (type && data && be_data);
  g_return_if_fail (data->version == GNC_FILE_BACKEND_VERS);

  if (data->get_count)
    write_counts (be_data->out, data->type_name,
                  (data->get_count) (be_data->book),
                  NULL);
}

static void
write_data_cb (const char *type, gpointer data_p, gpointer be_data_p)
{
  GncXmlDataType_t *data = data_p;
  struct file_backend *be_data = be_data_p;

  g_return_if_fail (type && data && be_data);
  g_return_if_fail (data->version == GNC_FILE_BACKEND_VERS);

  if (data->write)
    (data->write)(be_data->out, be_data->book);
}

static void
write_book(FILE *out, QofBook *book, sixtp_gdv2 *gd)
{
    struct file_backend be_data;

#ifdef IMPLEMENT_BOOK_DOM_TREES_LATER
    /* We can't just blast out the dom tree, because the dom tree
     * doesn't have the books, transactions, etc underneath it.
     * But that is just as well, since I think the performance
     * will be much better if we write out as we go along 
     */
    xmlNodePtr node;

    node = gnc_book_dom_tree_create(book);

    if(!node)
    {
        return;
    }
    
    xmlElemDump(out, NULL, node);
    if(fprintf(out, "\n") < 0)
	{
		qof_backend_set_error(qof_book_get_backend(book), ERR_FILEIO_WRITE_ERROR);
		return;
	}

    xmlFreeNode(node);
#endif

    be_data.out = out;
    be_data.book = book;
    be_data.gd = gd;
    if(fprintf( out, "<%s version=\"%s\">\n", BOOK_TAG, gnc_v2_book_version_string) < 0)
	{
		qof_backend_set_error(qof_book_get_backend(book), ERR_FILEIO_WRITE_ERROR);
		return;
	}
    write_book_parts (out, book);

    /* gd->counter.{foo}_total fields should have all these totals
       already collected.  I don't know why we're re-calling all these
       functions.  */
    write_counts(out,
                 "commodity",
                 gnc_commodity_table_get_size(
                     gnc_book_get_commodity_table(book)),
                 "account",
                 1 + gnc_account_n_descendants(gnc_book_get_root_account(book)),
                 "transaction",
                 gnc_book_count_transactions(book),
                 "schedxaction",
                 g_list_length(gnc_book_get_schedxactions(book)->sx_list),
		 "budget", qof_collection_count(
                     qof_book_get_collection(book, GNC_ID_BUDGET)),
		 NULL);

    qof_object_foreach_backend (GNC_FILE_BACKEND, write_counts_cb, &be_data);

    write_commodities(out, book, gd);
    write_pricedb(out, book, gd);
    write_accounts(out, book, gd);
    write_transactions(out, book, gd);
    write_template_transaction_data(out, book, gd);
    write_schedXactions(out, book, gd);

    qof_collection_foreach(qof_book_get_collection(book, GNC_ID_BUDGET), 
        write_budget, &be_data);

    qof_object_foreach_backend (GNC_FILE_BACKEND, write_data_cb, &be_data);

    if(fprintf( out, "</%s>\n", BOOK_TAG ) < 0) {
		qof_backend_set_error(qof_book_get_backend(book), ERR_FILEIO_WRITE_ERROR);
	}
}

void
write_commodities(FILE *out, QofBook *book, sixtp_gdv2 *gd)
{
    gnc_commodity_table *tbl;
    GList *namespaces;
    GList *lp;

    tbl = gnc_book_get_commodity_table(book);

    namespaces = gnc_commodity_table_get_namespaces(tbl);
    if(namespaces) 
    {
        namespaces = g_list_sort(namespaces, compare_namespaces);
    }

    for(lp = namespaces; lp; lp = lp->next) 
    {
        GList *comms, *lp2;
	xmlNodePtr comnode;

	comms = gnc_commodity_table_get_commodities(tbl, lp->data);
	comms = g_list_sort(comms, compare_commodity_ids);

	for(lp2 = comms; lp2; lp2 = lp2->next) {
	    comnode = gnc_commodity_dom_tree_create(lp2->data);
	    if (comnode == NULL)
	      continue;

	    xmlElemDump(out, NULL, comnode);
	    fprintf(out, "\n");

	    xmlFreeNode(comnode);
	    gd->counter.commodities_loaded++;
	    run_callback(gd, "commodities");
	  }

	g_list_free (comms);
    }

    if (namespaces) g_list_free (namespaces);
}

static void
write_pricedb(FILE *out, QofBook *book, sixtp_gdv2 *gd)
{
    xmlNodePtr node;

    node = gnc_pricedb_dom_tree_create(gnc_book_get_pricedb(book));

    if(!node)
    {
        return;
    }
    
    xmlElemDump(out, NULL, node);
    fprintf(out, "\n");

    xmlFreeNode(node);
}

static int
xml_add_trn_data(Transaction *t, gpointer data)
{
    struct file_backend *be_data = data;
    xmlNodePtr node;

    node = gnc_transaction_dom_tree_create(t);

    xmlElemDump(be_data->out, NULL, node);
    fprintf(be_data->out, "\n");

    xmlFreeNode(node);
    be_data->gd->counter.transactions_loaded++;
    run_callback(be_data->gd, "transaction");
    return 0;
}

static void
write_transactions(FILE *out, QofBook *book, sixtp_gdv2 *gd)
{
    struct file_backend be_data;

    be_data.out = out;
    be_data.gd = gd;
    xaccAccountTreeForEachTransaction(gnc_book_get_root_account(book),
				      xml_add_trn_data,
				      (gpointer) &be_data);
}

static void
write_template_transaction_data( FILE *out, QofBook *book, sixtp_gdv2 *gd )
{
    Account *ra;
    struct file_backend be_data;

    be_data.out = out;
    be_data.gd = gd;

    ra = gnc_book_get_template_root(book);
    if ( gnc_account_n_descendants(ra) > 0 )
    {
        fprintf( out, "<%s>\n", TEMPLATE_TRANSACTION_TAG );
        write_account_tree( out, ra, gd );
        xaccAccountTreeForEachTransaction( ra, xml_add_trn_data, (gpointer)&be_data );
        fprintf( out, "</%s>\n", TEMPLATE_TRANSACTION_TAG );
    }
}

static void
write_schedXactions( FILE *out, QofBook *book, sixtp_gdv2 *gd)
{
     GList *schedXactions;
     SchedXaction *tmpSX;
     xmlNodePtr node;
     
     schedXactions = gnc_book_get_schedxactions(book)->sx_list;

     if ( schedXactions == NULL )
          return;

     do {
          tmpSX = schedXactions->data;
          node = gnc_schedXaction_dom_tree_create( tmpSX );
          xmlElemDump( out, NULL, node );
          fprintf( out, "\n" );
          xmlFreeNode( node );
          gd->counter.schedXactions_loaded++;
          run_callback(gd, "schedXactions");
     } while ( (schedXactions = schedXactions->next) );
}

static void
write_budget (QofInstance *ent, gpointer data)
{
    xmlNodePtr node;
    struct file_backend* be = data;

    GncBudget *bgt = GNC_BUDGET(ent);
    node = gnc_budget_dom_tree_create(bgt);
    xmlElemDump( be->out, NULL, node );
    fprintf( be->out, "\n" );
    xmlFreeNode( node );
    
    be->gd->counter.budgets_loaded++;
    run_callback(be->gd, "budgets");    
}

void
gnc_xml2_write_namespace_decl (FILE *out, const char *namespace)
{
  g_return_if_fail (namespace);
  fprintf(out, "\n     xmlns:%s=\"http://www.gnucash.org/XML/%s\"",
          namespace, namespace);
}

static void
do_write_namespace_cb (const char *type, gpointer data_p, gpointer file_p)
{
  GncXmlDataType_t *data = data_p;
  FILE *out = file_p;

  g_return_if_fail (type && data && out);
  g_return_if_fail (data->version == GNC_FILE_BACKEND_VERS);

  if (data->ns)
    (data->ns)(out);
}

static void
write_v2_header (FILE *out)
{
    fprintf(out, "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n");
    fprintf(out, "<" GNC_V2_STRING);
    
    gnc_xml2_write_namespace_decl (out, "gnc");
    gnc_xml2_write_namespace_decl (out, "act");
    gnc_xml2_write_namespace_decl (out, "book");
    gnc_xml2_write_namespace_decl (out, "cd");
    gnc_xml2_write_namespace_decl (out, "cmdty");
    gnc_xml2_write_namespace_decl (out, "price");
    gnc_xml2_write_namespace_decl (out, "slot");
    gnc_xml2_write_namespace_decl (out, "split");
    gnc_xml2_write_namespace_decl (out, "sx");
    gnc_xml2_write_namespace_decl (out, "trn");
    gnc_xml2_write_namespace_decl (out, "ts");
    gnc_xml2_write_namespace_decl (out, "fs");
    gnc_xml2_write_namespace_decl (out, "bgt");
    gnc_xml2_write_namespace_decl (out, "recurrence");
    gnc_xml2_write_namespace_decl (out, "lot");

    /* now cope with the plugins */
    qof_object_foreach_backend (GNC_FILE_BACKEND, do_write_namespace_cb, out);

    fprintf(out, ">\n");
}

gboolean
gnc_book_write_to_xml_filehandle_v2(QofBook *book, FILE *out)
{
    QofBackend *be;
    sixtp_gdv2 *gd;

    if (!out) return FALSE;

    write_v2_header (out);

    write_counts(out,
                 "book", 1,
                 NULL);

    be = qof_book_get_backend(book);
    gd = gnc_sixtp_gdv2_new(book, FALSE, file_rw_feedback, be->percentage);
    gd->counter.commodities_total =
      gnc_commodity_table_get_size(gnc_book_get_commodity_table(book));
    gd->counter.accounts_total = 1 + 
      gnc_account_n_descendants(gnc_book_get_root_account(book));
    gd->counter.transactions_total = gnc_book_count_transactions(book);
    gd->counter.schedXactions_total =
      g_list_length(gnc_book_get_schedxactions(book)->sx_list);
    gd->counter.budgets_total = qof_collection_count(
        qof_book_get_collection(book, GNC_ID_BUDGET));

    write_book(out, book, gd);

    fprintf(out, "</" GNC_V2_STRING ">\n\n");
    
    g_free(gd);
    return TRUE;
}

/*
 * This function is called by the "export" code.
 */
gboolean
gnc_book_write_accounts_to_xml_filehandle_v2(QofBackend *be, QofBook *book, FILE *out)
{
    gnc_commodity_table *table;
    Account *root;
    int ncom, nacc;
    sixtp_gdv2 *gd;

    if (!out) return FALSE;

    root = gnc_book_get_root_account(book);
    nacc = 1 + gnc_account_n_descendants(root);

    table = gnc_book_get_commodity_table(book);
    ncom = gnc_commodity_table_get_size(table);

    write_v2_header (out);

    write_counts(out,
                 "commodity", ncom,
                 "account", nacc,
                 NULL);

    gd = gnc_sixtp_gdv2_new(book, TRUE, file_rw_feedback, be->percentage);
    gd->counter.commodities_total = ncom;
    gd->counter.accounts_total = nacc;

    write_commodities(out, book, gd);

    write_accounts(out, book, gd);

    fprintf(out, "</" GNC_V2_STRING ">\n\n");

    g_free(gd);
    return TRUE;
}

#define BUFLEN 4096

/* Compress or decompress function that is to be run in a separate thread.
 * Returns 1 on success or 0 otherwise, stuffed into a pointer type. */
static gpointer
gz_thread_func(gz_thread_params_t *params)
{
    gchar buffer[BUFLEN];
    guint bytes;
    gssize written;
    gzFile *file;
    gint success = 0;

#ifdef G_OS_WIN32
    {
        gchar *conv_name = g_win32_locale_filename_from_utf8(params->filename);
        gchar *perms;

        if (!conv_name) {
            g_warning("Could not convert '%s' to system codepage",
                      params->filename);
            goto cleanup_gz_thread_func;
        }

        if (strchr(params->perms, 'b'))
            perms = g_strdup(params->perms);
        else
            perms = g_strdup_printf("%cb%s", *params->perms, params->perms + 1);

        file = gzopen(conv_name, perms);
        g_free(perms);
        g_free(conv_name);
    }
#else /* !G_OS_WIN32 */
    file = gzopen(params->filename, params->perms);
#endif /* G_OS_WIN32 */

    if (file == NULL) {
        g_warning("Child threads gzopen failed");
        goto cleanup_gz_thread_func;
    }

    if (params->compress)
        while ((bytes = read(params->fd, buffer, BUFLEN)) > 0)
            gzwrite(file, buffer, bytes);
    else
        while ((bytes = gzread(file, buffer, BUFLEN)) > 0)
            written = write(params->fd, buffer, bytes);

    gzclose(file);
    success = 1;

cleanup_gz_thread_func:
    g_free(params->filename);
    g_free(params->perms);
    g_free(params);

    return GINT_TO_POINTER(success);
}

static FILE *
try_gz_open (const char *filename, const char *perms, gboolean use_gzip,
             gboolean compress)
{
  if (strstr(filename, ".gz.") != NULL) /* its got a temp extension */
      use_gzip = TRUE;

  if (!use_gzip)
    return g_fopen(filename, perms);

  if (g_thread_supported()) {
    /* use threads */

    int filedes[2];
    GThread *thread;
    GError *error = NULL;
    gz_thread_params_t *params;
    FILE *file;

    if (pipe(filedes) < 0) {
      g_warning("Pipe call failed. Opening uncompressed file.");
      return g_fopen(filename, perms);
    }

    params = g_new(gz_thread_params_t, 1);
    params->fd = filedes[compress ? 0 : 1];
    params->filename = g_strdup(filename);
    params->perms = g_strdup(perms);
    params->compress = compress;

    thread = g_thread_create((GThreadFunc) gz_thread_func, params, TRUE, &error);
    if (!thread) {
      g_warning("Could not create thread for (de)compression: %s",
                error->message);
      g_error_free(error);
      g_free(params->filename);
      g_free(params->perms);
      g_free(params);
      close(filedes[0]);
      close(filedes[1]);

      return g_fopen(filename, perms);
    }

    if (compress)
      file = fdopen(filedes[1], "w");
    else
      file = fdopen(filedes[0], "r");

    G_LOCK(threads);
    if (!threads)
      threads = g_hash_table_new(g_direct_hash, g_direct_equal);

    g_hash_table_insert(threads, file, thread);
    G_UNLOCK(threads);

    return file;

  } else {
#ifdef G_OS_WIN32
    g_assert_not_reached();
#else
    /* use fork */

    int filedes[2];
    pid_t pid;

    /* avoid reading from file that is still being written to
       by a child process */
    g_assert(gzip_child_pid == 0);

    if (pipe(filedes) < 0) {
      PWARN("Pipe call failed. Opening uncompressed file.");
      return g_fopen(filename, perms);
    }

    pid = fork();
    switch (pid) {
    case -1:
      PWARN("Fork call failed. Opening uncompressed file.");
      return g_fopen(filename, perms);

    case 0: /* child */ {
      char buffer[BUFLEN];
      unsigned bytes;
      ssize_t written;
      gzFile *file;

      file = gzopen(filename, perms);
      if (file == NULL) {
        PWARN("child gzopen failed\n");
        exit(0);
      }
      if (compress) {
        close(filedes[1]);
        while ((bytes = read(filedes[0], buffer, BUFLEN)) > 0)
          gzwrite(file, buffer, bytes);
      }
      else
      {
        close(filedes[0]);
        while ((bytes = gzread(file, buffer, BUFLEN)) > 0)
          written = write(filedes[1], buffer, bytes);
      }
      gzclose(file);
      _exit(0);
    }

    default: /* parent */
      if (compress) {
        /* the calling code must wait_for_gzip() */
        gzip_child_pid = pid;
      }
      sleep(2);
      if (compress) {
        close(filedes[0]);
        return fdopen(filedes[1], "w");
      }
      else
      {
        close(filedes[1]);
        return fdopen(filedes[0], "r");
      }
    }
#endif /* G_OS_WIN32 */
  }
}

static gboolean
wait_for_gzip(FILE *file)
{
    if (g_thread_supported()) {
        gboolean retval = TRUE;

        G_LOCK(threads);
        if (threads) {
            GThread *thread = g_hash_table_lookup(threads, file);
            if (thread) {
                g_hash_table_remove(threads, file);
                retval = GPOINTER_TO_INT(g_thread_join(thread));
            }
        }
        G_UNLOCK(threads);
        return retval;

    } else {
        pid_t retval;

        if (gzip_child_pid == 0)
            return TRUE;

#ifdef HAVE_SYS_WAIT_H
        retval = waitpid(gzip_child_pid, NULL, WUNTRACED);
#else
        retval = 1;
#endif
        gzip_child_pid = 0;

        return retval != -1;
    }
}

gboolean
gnc_book_write_to_xml_file_v2(
    QofBook *book,
    const char *filename,
    gboolean compress)
{
    FILE *out;

    out = try_gz_open(filename, "w", compress, TRUE);
    if (out == NULL)
    {
        return FALSE;
    }

    gnc_book_write_to_xml_filehandle_v2 (book, out);

    write_emacs_trailer(out);

    if (fclose(out) != 0)
    {
        return FALSE;
    }

    if (compress)
        return wait_for_gzip(out);

    return TRUE;
}

/*
 * Have to pass in the backend as this routine needs the temporary
 * backend for file export, not the real backend which could be
 * postgress or anything else.
 */
gboolean
gnc_book_write_accounts_to_xml_file_v2(
    QofBackend *be,
    QofBook *book,
    const char *filename)
{
    FILE *out;

    out = g_fopen(filename, "w");
    if (out == NULL)
    {
        return FALSE;
    }

    gnc_book_write_accounts_to_xml_filehandle_v2 (be, book, out);

    write_emacs_trailer(out);

    if (fclose(out) != 0)
    {
        return FALSE;
    }

    return TRUE;
}

/***********************************************************************/
static gboolean
is_gzipped_file(const gchar *name)
{
    unsigned char buf[2];
    int fd = g_open(name, O_RDONLY, 0);

    if (fd == -1) {
        return FALSE;
    }

    if (read(fd, buf, 2) != 2) {
        close(fd);
        return FALSE;
    }
    close(fd);

    if (buf[0] == 037 && buf[1] == 0213) {
        return TRUE;
    }

    return FALSE;
}

gboolean
gnc_is_xml_data_file_v2(const gchar *name, gboolean *with_encoding)
{
    if (gnc_is_our_xml_file(name, GNC_V2_STRING, with_encoding))
        return TRUE;

    if (is_gzipped_file(name)) {
        gzFile *file;
        char first_chunk[256];
        int num_read;

#ifdef G_OS_WIN32
        {
            gchar *conv_name = g_win32_locale_filename_from_utf8(name);
            if (!conv_name)
                g_warning("Could not convert '%s' to system codepage", name);
            else {
                file = gzopen(conv_name, "rb");
                g_free(conv_name);
            }
        }
#else
        file = gzopen(name, "r");
#endif
        if (file == NULL)
            return FALSE;

        num_read = gzread(file, first_chunk, sizeof(first_chunk) - 1);
        gzclose(file);

        if (num_read < 1)
            return FALSE;

        return gnc_is_our_first_xml_chunk(first_chunk, GNC_V2_STRING,
                                          with_encoding);
    }

    return FALSE;
}


static void
replace_character_references(gchar *string)
{
    gchar *cursor, *semicolon, *tail;
    glong number;

    for (cursor = strstr(string, "&#");
         cursor && *cursor;
         cursor = strstr(cursor, "&#")) {
        semicolon = strchr(cursor, ';');
        if (semicolon && *semicolon) {

            /* parse number */
            errno = 0;
            if (*(cursor+2) == 'x') {
                number = strtol(cursor+3, &tail, 16);
            } else {
                number = strtol(cursor+2, &tail, 10);
            }
            if (errno || tail != semicolon || number < 0 || number > 255) {
                PWARN("Illegal character reference");
                return;
            }

            /* overwrite '&' with the specified character */
            *cursor = (gchar) number;
            cursor++;
            if (*(semicolon+1)) {
                /* move text after semicolon the the left */
                tail = g_strdup(semicolon+1);
                strcpy(cursor, tail);
                g_free(tail);
            } else {
                /* cut here */
                *cursor = '\0';
            }

        } else {
            PWARN("Unclosed character reference");
            return;
        }
    }
}

static void
conv_free(conv_type *conv) {
    if (conv) {
        g_free(conv->utf8_string);
        g_free(conv);
    }
}

static void
conv_list_free(GList *conv_list) {
    g_list_foreach(conv_list, (GFunc) conv_free, NULL);
    g_list_free(conv_list);
}

typedef struct  {
    GQuark encoding;
    GIConv iconv;
} iconv_item_type;

gint
gnc_xml2_find_ambiguous(const gchar *filename, GList *encodings,
                        GHashTable **unique, GHashTable **ambiguous,
                        GList **impossible)
{
    FILE *file=NULL;
    GList *iconv_list=NULL, *conv_list=NULL, *iter;
    iconv_item_type *iconv_item=NULL, *ascii=NULL;
    const gchar *enc;
    GHashTable *processed=NULL;
    gint n_impossible = 0;
    GError *error=NULL;
    gboolean is_compressed;
    gboolean clean_return=FALSE;

    is_compressed = is_gzipped_file(filename);
    file = try_gz_open(filename, "r", is_compressed, FALSE);
    if (file == NULL) {
        PWARN("Unable to open file %s", filename);
        goto cleanup_find_ambs;
    }

    /* we need ascii */
    ascii = g_new(iconv_item_type, 1);
    ascii->encoding = g_quark_from_string("ASCII");
    ascii->iconv = g_iconv_open("UTF-8", "ASCII");
    if (ascii->iconv == (GIConv) -1) {
        PWARN("Unable to open ASCII ICONV conversion descriptor");
        goto cleanup_find_ambs;
    }

    /* call iconv_open on encodings */
    for (iter = encodings; iter; iter = iter->next) {
        iconv_item = g_new(iconv_item_type, 1);
        iconv_item->encoding = GPOINTER_TO_UINT (iter->data);
        if (iconv_item->encoding == ascii->encoding) {
            continue;
        }

        enc = g_quark_to_string(iconv_item->encoding);
        iconv_item->iconv = g_iconv_open("UTF-8", enc);
        if (iconv_item->iconv == (GIConv) -1) {
            PWARN("Unable to open IConv conversion descriptor for '%s'", enc);
            goto cleanup_find_ambs;
        } else {
            iconv_list = g_list_prepend(iconv_list, iconv_item);
        }
    }

    /* prepare data containers */
    if (unique)
      *unique = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
                                      (GDestroyNotify) conv_free);
    if (ambiguous)
      *ambiguous = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
                                         (GDestroyNotify) conv_list_free);
    if (impossible)
      *impossible = NULL;
    processed = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

    /* loop through lines */
    while (1) {
        gchar line[256], *word, *utf8;
        gchar **word_array, **word_cursor;
        conv_type *conv = NULL;

        if (!fgets(line, sizeof(line)-1, file)) {
            if (feof(file)) {
                break;
            } else {
                goto cleanup_find_ambs;
            }
        }

        g_strchomp(line);
        replace_character_references(line);
        word_array = g_strsplit_set(line, "> <", 0);

        /* loop through words */
        for (word_cursor = word_array; *word_cursor; word_cursor++) {
            word = *word_cursor;
            if (!word)
              continue;

            utf8 = g_convert_with_iconv(word, -1, ascii->iconv,
                                        NULL, NULL, &error);
            if (utf8) {
                /* pure ascii */
                g_free(utf8);
                continue;
            }
            g_error_free(error);
            error = NULL;

            if (g_hash_table_lookup_extended(processed, word, NULL, NULL)) {
                /* already processed */
                continue;
            }

            /* loop through encodings */
            conv_list = NULL;
            for (iter = iconv_list; iter; iter = iter->next) {
                iconv_item = iter->data;
                utf8 = g_convert_with_iconv(word, -1, iconv_item->iconv,
                                            NULL, NULL, &error);
                if (utf8) {
                    conv = g_new(conv_type, 1);
                    conv->encoding = iconv_item->encoding;
                    conv->utf8_string = utf8;
                    conv_list = g_list_prepend(conv_list, conv);
                } else {
                    g_error_free(error);
                    error = NULL;
                }
            }

            /* no successful conversion */
            if (!conv_list) {
                if (impossible)
                    *impossible = g_list_append(*impossible, g_strdup(word));
                n_impossible++;
            }

            /* more than one successful conversion */
            else if (conv_list->next) {
                if (ambiguous) {
                    g_hash_table_insert(*ambiguous, g_strdup(word), conv_list);
                } else {
                    conv_list_free(conv_list);
                }
            }

            /* only one successful conversion */
            else {
                if (unique) {
                    g_hash_table_insert(*unique, g_strdup(word), conv);
                } else {
                    conv_free(conv);
                }
                g_list_free(conv_list);
            }

            g_hash_table_insert(processed, g_strdup(word), NULL);
        }
        g_strfreev(word_array);
    }

    clean_return = TRUE;

 cleanup_find_ambs:

    if (iconv_list) {
        for (iter = iconv_list; iter; iter = iter->next) {
            if (iter->data) {
                g_iconv_close(((iconv_item_type*) iter->data)->iconv);
                g_free(iter->data);
            }
        }
        g_list_free(iconv_list);
    }
    if (processed)
        g_hash_table_destroy(processed);
    if (ascii)
        g_free(ascii);
    if (file) {
        fclose(file);
        if (is_compressed)
            wait_for_gzip(file);
    }

    return (clean_return) ? n_impossible : -1;
}

typedef struct {
    gchar *filename;
    GHashTable *subst;
} push_data_type;

static void
parse_with_subst_push_handler (xmlParserCtxtPtr xml_context,
                               push_data_type *push_data)
{
    const gchar *filename;
    FILE *file = NULL;
    GIConv ascii=(GIConv)-1;
    GString *output=NULL;
    GError *error=NULL;
    gboolean is_compressed;

    filename = push_data->filename;
    is_compressed = is_gzipped_file(filename);
    file = try_gz_open(filename, "r", is_compressed, FALSE);
    if (file == NULL) {
        PWARN("Unable to open file %s", filename);
        goto cleanup_push_handler;
    }

    ascii = g_iconv_open("UTF-8", "ASCII");
    if (ascii == (GIConv) -1) {
        PWARN("Unable to open ASCII ICONV conversion descriptor");
        goto cleanup_push_handler;
    }

    /* loop through lines */
    while (1) {
        gchar line[256], *word, *repl, *utf8;
        gint pos, len;
        gchar *start, *cursor;

        if (!fgets(line, sizeof(line)-1, file)) {
            if (feof(file)) {
                break;
            } else {
                goto cleanup_push_handler;
            }
        }

        replace_character_references(line);
        output = g_string_new(line);

        /* loop through words */
        cursor = output->str;
        pos = 0;
        while (1) {
            /* ignore delimiters */
            while (*cursor=='>' || *cursor==' ' || *cursor=='<' ||
                   *cursor=='\n') {
                cursor++;
                pos += 1;
            }

            if (!*cursor)
                /* welcome to EOL */
                break;

            /* search for a delimiter */
            start = cursor;
            len = 0;
            while (*cursor && *cursor!='>' && *cursor!=' ' && *cursor!='<' &&
                   *cursor!='\n') {
                cursor++;
                len++;
            }

            utf8 = g_convert_with_iconv(start, len, ascii, NULL, NULL, &error);

            if (utf8) {
                /* pure ascii */
                g_free(utf8);
                pos += len;
            } else {
                g_error_free(error);
                error = NULL;

                word = g_strndup(start, len);
                repl = g_hash_table_lookup(push_data->subst, word);
                g_free(word);
                if (repl) {
                    /* there is a replacement */
                    output = g_string_insert(g_string_erase(output, pos, len),
                                             pos, repl);
                    pos += strlen(repl);
                    cursor = output->str + pos;
                } else {
                    /* there is no replacement, return immediately */
                    goto cleanup_push_handler;
                }
            }
        }

        if (xmlParseChunk(xml_context, output->str, output->len, 0) != 0) {
            goto cleanup_push_handler;
        }
    }

    /* last chunk */
    xmlParseChunk(xml_context, "", 0, 1);

 cleanup_push_handler:

    if (output)
        g_string_free(output, TRUE);
    if (ascii != (GIConv) -1)
        g_iconv_close(ascii);
    if (file) {
        fclose(file);
        if (is_compressed)
            wait_for_gzip(file);
    }
}

gboolean
gnc_xml2_parse_with_subst (FileBackend *fbe, QofBook *book, GHashTable *subst)
{
    push_data_type *push_data;
    gboolean success;

    push_data = g_new(push_data_type, 1);
    push_data->filename = fbe->fullpath;
    push_data->subst = subst;

    success = qof_session_load_from_xml_file_v2_full(
        fbe, book, (sixtp_push_handler) parse_with_subst_push_handler,
        push_data);

    if (success)
        qof_book_kvp_changed(book);

    return success;
}
