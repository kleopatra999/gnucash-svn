/* 
 * gnc-plugin-page-register.c -- 
 *
 * Copyright (C) 2003 Jan Arne Petersen <jpetersen@uni-bonn.de>
 * Copyright (C) 2003 David Hampton <hampton@employees.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, contact:
 *
 * Free Software Foundation           Voice:  +1-617-542-5942
 * 59 Temple Place - Suite 330        Fax:    +1-617-542-2652
 * Boston, MA  02111-1307,  USA       gnu@gnu.org
 */

#include "config.h"

#include <gtk/gtk.h>
#include <g-wrap-wct.h>

#include "gnc-plugin-page-register.h"
#include "gnc-plugin-register.h"
#include "gnc-plugin-menu-additions.h"
#include "gnc-plugin-page-report.h"

#include "dialog-account.h"
#include "dialog-print-check.h"
#include "dialog-transfer.h"
#include "dialog-utils.h"
#include "druid-stock-split.h"
#include "global-options.h"
#include "gnc-book.h"
#include "gnc-gconf-utils.h"
#include "gnc-component-manager.h"
#include "gnc-date.h"
#include "gnc-date-edit.h"
#include "gnc-gnome-utils.h"
#include "gnc-gobject-utils.h"
#include "gnc-gui-query.h"
#include "gnc-icons.h"
#include "gnc-split-reg.h"
#include "gnc-ui-util.h"
#include "gnc-window.h"
#include "gnucash-sheet.h"
#include "lot-viewer.h"
#include "QueryNew.h"
#include "window-reconcile.h"
#include "window-report.h"

#include "messages.h"
#include "gnc-engine-util.h"

/* This static indicates the debugging module that this .o belongs to.  */
static short module = MOD_GUI;

#define DEFAULT_LINES_OPTION_SECTION GCONF_GENERAL_REGISTER
#define DEFAULT_LINES_OPTION_NAME    KEY_NUMBER_OF_ROWS
#define DEFAULT_LINES_AMOUNT         20

static void gnc_plugin_page_register_class_init (GncPluginPageRegisterClass *klass);
static void gnc_plugin_page_register_init (GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_finalize (GObject *object);

/* static Account *gnc_plugin_page_register_get_current_account (GncPluginPageRegister *page); */

static GtkWidget *gnc_plugin_page_register_create_widget (GncPluginPage *plugin_page);
static void gnc_plugin_page_register_destroy_widget (GncPluginPage *plugin_page);
static void gnc_plugin_page_register_window_changed (GncPluginPage *plugin_page, GtkWidget *window);
static void gnc_plugin_page_register_merge_actions (GncPluginPage *plugin_page, GtkUIManager *ui_merge);
static void gnc_plugin_page_register_unmerge_actions (GncPluginPage *plugin_page, GtkUIManager *ui_merge);

static gchar *gnc_plugin_page_register_get_tab_name (GncPluginPage *plugin_page);

/* Callbacks */
static gboolean gnc_plugin_page_register_button_press_cb (GtkWidget *widget,
							      GdkEventButton *event,
			       				      GncPluginPageRegister *page);

/* Callbacks for the "Sort By" dialog */
void gnc_plugin_page_register_sort_button_cb(GtkToggleButton *button, GncPluginPageRegister *page);
void gnc_plugin_page_register_sort_response_cb(GtkDialog *dialog, gint response, GncPluginPageRegister *plugin_page);

/* Callbacks for the "Filter By" dialog */
void gnc_plugin_page_register_filter_select_range_cb(GtkRadioButton *button, GncPluginPageRegister *page);
void gnc_plugin_page_register_filter_start_cb(GtkWidget *radio, GncPluginPageRegister *page);
void gnc_plugin_page_register_filter_end_cb(GtkWidget *radio, GncPluginPageRegister *page);
void gnc_plugin_page_register_filter_response_cb(GtkDialog *dialog, gint response, GncPluginPageRegister *plugin_page);
void gnc_plugin_page_register_filter_status_all_cb(GtkButton *button, GncPluginPageRegister *plugin_page);
void gnc_plugin_page_register_filter_status_one_cb(GtkToggleButton *button, GncPluginPageRegister *page);

/* Command callbacks */
static void gnc_plugin_page_register_cmd_print_check (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_cut (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_copy (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_paste (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_edit_account (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_cut_transaction (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_copy_transaction (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_paste_transaction (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_void_transaction (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_unvoid_transaction (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_reverse_transaction (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_view_sort_by (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_view_filter_by (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_style_changed (GtkAction *action, GtkRadioAction *current, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_style_double_line (GtkToggleAction *action, GncPluginPageRegister *plugin_page);

static void gnc_plugin_page_register_cmd_reconcile (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_transfer (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_stock_split (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_lots (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_enter_transaction (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_cancel_transaction (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_delete_transaction (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_blank_transaction (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_duplicate_transaction (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_reinitialize_transaction (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_expand_transaction (GtkToggleAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_exchange_rate (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_jump (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_schedule (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_account_report (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_transaction_report (GtkAction *action, GncPluginPageRegister *plugin_page);

static void gnc_plugin_page_help_changed_cb( GNCSplitReg *gsr, GncPluginPageRegister *register_page );
static void gnc_plugin_page_register_refresh_cb (GHashTable *changes, gpointer user_data);

/************************************************************/
/*                          Actions                         */
/************************************************************/

static GtkActionEntry gnc_plugin_page_register_actions [] =
{
	/* File menu */

	{ "FilePrintAction", GTK_STOCK_PRINT, N_("_Print Check..."), NULL, NULL,
	  G_CALLBACK (gnc_plugin_page_register_cmd_print_check) },

	/* Edit menu */

	{ "EditCutAction", GTK_STOCK_CUT, N_("Cu_t"), NULL,
	  NULL,
	  G_CALLBACK (gnc_plugin_page_register_cmd_cut) },
	{ "EditCopyAction", GTK_STOCK_COPY, N_("_Copy"), NULL,
	  NULL,
	  G_CALLBACK (gnc_plugin_page_register_cmd_copy) },
	{ "EditPasteAction", GTK_STOCK_PASTE, N_("_Paste"), NULL,
	  NULL,
	  G_CALLBACK (gnc_plugin_page_register_cmd_paste) },
	{ "EditEditAccountAction", GNC_STOCK_EDIT_ACCOUNT, N_("Edit Account"), "<control>e",
	  N_("Edit the selected account"),
	  G_CALLBACK (gnc_plugin_page_register_cmd_edit_account) },

	/* Transaction menu */

	{ "CutTransactionAction", GTK_STOCK_CUT, N_("Cu_t Transaction"), "", NULL,
	  G_CALLBACK (gnc_plugin_page_register_cmd_cut_transaction) },
	{ "CopyTransactionAction", GTK_STOCK_COPY, N_("_Copy Transaction"), "", NULL,
	  G_CALLBACK (gnc_plugin_page_register_cmd_copy_transaction) },
	{ "PasteTransactionAction", GTK_STOCK_PASTE, N_("_Paste Transaction"), "", NULL,
	  G_CALLBACK (gnc_plugin_page_register_cmd_paste_transaction) },
	{ "DuplicateTransactionAction", GTK_STOCK_COPY, N_("Du_plicate Transaction"), "",
	  N_("Make a copy of the current transaction"),
	  G_CALLBACK (gnc_plugin_page_register_cmd_duplicate_transaction) },
	{ "DeleteTransactionAction", GTK_STOCK_DELETE, N_("Delete Transaction"), NULL,
	  N_("Delete the current transaction"),
	  G_CALLBACK (gnc_plugin_page_register_cmd_delete_transaction) },
	{ "RemoveTransactionSplitsAction", GTK_STOCK_CLEAR, N_("Remo_ve Transaction Splits"), NULL,
	  N_("Remove all splits in the current transaction"),
	  G_CALLBACK (gnc_plugin_page_register_cmd_reinitialize_transaction) },
	{ "RecordTransactionAction", GTK_STOCK_ADD, N_("_Enter Transaction"), NULL,
	  N_("Record the current transaction"),
	  G_CALLBACK (gnc_plugin_page_register_cmd_enter_transaction) },
	{ "CancelTransactionAction", GTK_STOCK_CANCEL, N_("Ca_ncel Transaction"), NULL,
	  N_("_Cancel the current transaction"),
	  G_CALLBACK (gnc_plugin_page_register_cmd_cancel_transaction) },
	{ "VoidTransactionAction", NULL, N_("_Void Transaction"), NULL, NULL,
	  G_CALLBACK (gnc_plugin_page_register_cmd_void_transaction) },
	{ "UnvoidTransactionAction", NULL, N_("_Unvoid Transaction"), NULL, NULL,
	  G_CALLBACK (gnc_plugin_page_register_cmd_unvoid_transaction) },
	{ "ReverseTransactionAction", NULL, N_("_Reverse Transaction"), NULL, NULL,
	  G_CALLBACK (gnc_plugin_page_register_cmd_reverse_transaction) },

	/* View menu */

	{ "ViewSortByAction", NULL, N_("_Sort By..."), NULL, NULL,
	  G_CALLBACK (gnc_plugin_page_register_cmd_view_sort_by) },
	{ "ViewFilterByAction", NULL, N_("_Filter By..."), NULL, NULL,
	  G_CALLBACK (gnc_plugin_page_register_cmd_view_filter_by) },

	/* Actions menu */

	{ "ActionsTransferAction", GTK_STOCK_MISSING_IMAGE, N_("_Transfer..."), "<control>t",
	  N_("Transfer funds from one account to another"),
	  G_CALLBACK (gnc_plugin_page_register_cmd_transfer) },
	{ "ActionsReconcileAction", NULL, N_("_Reconcile..."), "<control>r",
	  N_("Reconcile the selected account"),
	  G_CALLBACK (gnc_plugin_page_register_cmd_reconcile) },
	{ "ActionsStockSplitAction", NULL, N_("Stoc_k Split..."), NULL,
	  N_("Record a stock split or a stock merger"),
	  G_CALLBACK (gnc_plugin_page_register_cmd_stock_split) },
	{ "ActionsLotsAction", NULL, N_("_Lot Viewer..."), NULL,
	  N_("Bring up the lot viewer/editor window"),
	  G_CALLBACK (gnc_plugin_page_register_cmd_lots) },
	{ "BlankTransactionAction", GTK_STOCK_MISSING_IMAGE, N_("_Blank Transaction"), NULL,
	  N_("Move to the blank transaction at the bottom of the register"),
	  G_CALLBACK (gnc_plugin_page_register_cmd_blank_transaction) },
	{ "EditExchangeRateAction", GTK_STOCK_MISSING_IMAGE, N_("Edit E_xchange Rate"), NULL,
	  N_("Exit the exchange rate for the current transaction"),
	  G_CALLBACK (gnc_plugin_page_register_cmd_exchange_rate) },
	{ "JumpTransactionAction", GTK_STOCK_JUMP_TO, N_("_Jump"), NULL,
	  N_("Jump to the corresponding transaction in the other account"),
	  G_CALLBACK (gnc_plugin_page_register_cmd_jump) },
	{ "ScheduleTransactionAction", GTK_STOCK_COPY, N_("Schedule..."), NULL,
	  N_("Edit scheduled transactions"),
	  G_CALLBACK (gnc_plugin_page_register_cmd_schedule) },

	/* Reports menu */

	{ "ReportsAccountReportAction", NULL, N_("_Account Report"), NULL, NULL,
	  G_CALLBACK (gnc_plugin_page_register_cmd_account_report) },
	{ "ReportsTransactionReportAction", NULL, N_("_Transaction Report"), NULL, NULL,
	  G_CALLBACK (gnc_plugin_page_register_cmd_transaction_report) },
};
static guint gnc_plugin_page_register_n_actions = G_N_ELEMENTS (gnc_plugin_page_register_actions);

static GtkToggleActionEntry toggle_entries[] = {
	{ "ViewStyleDoubleLineAction", NULL, N_("_Double Line"), NULL, NULL,
	  G_CALLBACK (gnc_plugin_page_register_cmd_style_double_line), FALSE },

	{ "SplitTransactionAction", GTK_STOCK_MISSING_IMAGE, N_("S_plit Transaction"), NULL,
	  N_("Show all splits in the current transaction"),
	  G_CALLBACK (gnc_plugin_page_register_cmd_expand_transaction), FALSE },
};
static guint n_toggle_entries = G_N_ELEMENTS (toggle_entries);

static GtkRadioActionEntry radio_entries_2 [] =
{
	{ "ViewStyleBasicAction", NULL, N_("_Basic Ledger"), NULL, NULL, REG_STYLE_LEDGER },
	{ "ViewStyleAutoSplitAction", NULL, N_("_Auto-Split Ledger"), NULL, NULL, REG_STYLE_AUTO_LEDGER },
	{ "ViewStyleJournalAction", NULL, N_("Transaction _Journal"), NULL, NULL, REG_STYLE_JOURNAL }
};
static guint n_radio_entries_2 = G_N_ELEMENTS (radio_entries_2);

static const gchar *actions_requiring_account[] = {
	"EditEditAccountAction",
	"ActionsReconcileAction",
	"ActionsLotsAction",
	NULL
};

/* DRH - Suggest short_labels be added to libegg */
static action_short_labels short_labels[] = {
  { "ActionsTransferAction", 	  N_("Transfer") },
  { "RecordTransactionAction", 	  N_("Enter") },
  { "CancelTransactionAction", 	  N_("Cancel") },
  { "DeleteTransactionAction", 	  N_("Delete") },
  { "DuplicateTransactionAction", N_("Duplicate") },
  { "SplitTransactionAction",     N_("Split") },
  { "ScheduleTransactionAction",  N_("Schedule") },
  { "BlankTransactionAction",     N_("Blank") },
  { NULL, NULL },
};


struct {
  const char *action_name;
  int value;
} status_actions[] = {
  { "filter_status_reconciled",   CLEARED_RECONCILED },
  { "filter_status_cleared",      CLEARED_CLEARED },
  { "filter_status_voided",       CLEARED_VOIDED },
  { "filter_status_frozen",       CLEARED_FROZEN },
  { "filter_status_unreconciled", CLEARED_NO },
  { NULL, 0 },
};
#define CLEARED_VALUE "cleared_value"


/************************************************************/
/*                      Data Structures                     */
/************************************************************/

struct GncPluginPageRegisterPrivate
{
	GtkActionGroup *action_group;
	guint merge_id;
	GtkUIManager *ui_merge;

	GNCLedgerDisplay *ledger;
	GNCSplitReg *gsr;

	GtkWidget *widget;

	char *ui_description;

	gint component_manager_id;

	const char *lines_opt_section;
	const char *lines_opt_name;
	gint lines_default;
	gint disallowCaps;

	struct {
	  GtkWidget *dialog;
	  SortType original_sort_type;
	} sd;

	struct {
	  GtkWidget *dialog;
	  cleared_match_t original_cleared_match;
	  cleared_match_t cleared_match;
	  time_t original_start_time;
	  time_t original_end_time;
	  time_t start_time;
	  time_t end_time;
	} fd;
};

static GObjectClass *parent_class = NULL;

/************************************************************/
/*                      Implementation                      */
/************************************************************/

GType
gnc_plugin_page_register_get_type (void)
{
	static GType gnc_plugin_page_register_type = 0;

	if (gnc_plugin_page_register_type == 0) {
		static const GTypeInfo our_info = {
			sizeof (GncPluginPageRegisterClass),
			NULL,
			NULL,
			(GClassInitFunc) gnc_plugin_page_register_class_init,
			NULL,
			NULL,
			sizeof (GncPluginPageRegister),
			0,
			(GInstanceInitFunc) gnc_plugin_page_register_init
		};
		
		gnc_plugin_page_register_type = g_type_register_static (GNC_TYPE_PLUGIN_PAGE,
									GNC_PLUGIN_PAGE_REGISTER_NAME,
									&our_info, 0);
	}

	return gnc_plugin_page_register_type;
}

static GncPluginPage *
gnc_plugin_page_register_new_common (GNCLedgerDisplay *ledger)
{
	GncPluginPageRegister *register_page;
	GncPluginPage *plugin_page;
	GNCSplitReg *gsr;
	const GList *item;
	GList *book_list;
	gchar *label;
	QofQuery *q;
	gboolean use_new;

	/* Is there an existing page? */
	gsr = gnc_ledger_display_get_user_data (ledger);
	if (gsr) {
	  item = gnc_gobject_tracking_get_list(GNC_PLUGIN_PAGE_REGISTER_NAME);
	  for ( ; item; item = g_list_next(item)) {
	    register_page = (GncPluginPageRegister *)item->data;
	    if (register_page->priv->gsr == gsr)
	      return GNC_PLUGIN_PAGE(register_page);
	  }
	}

	register_page = g_object_new (GNC_TYPE_PLUGIN_PAGE_REGISTER, NULL);
	register_page->priv->ledger = ledger;

	plugin_page = GNC_PLUGIN_PAGE(register_page);
	label = gnc_plugin_page_register_get_tab_name(plugin_page);
	gnc_plugin_page_set_title(plugin_page, label);
	gnc_plugin_page_set_tab_name(plugin_page, label);
	g_free(label);

	use_new = gnc_gconf_get_bool(GCONF_GENERAL_REGISTER, KEY_USE_NEW, NULL);
	gnc_plugin_page_set_use_new_window(plugin_page, use_new);

	q = gnc_ledger_display_get_query (ledger);
	book_list = qof_query_get_books (q);
	for (item = book_list; item; item = g_list_next(item))
	  gnc_plugin_page_add_book (plugin_page, (QofBook *)item->data);
	// Do not free the list. It is owned by the query.
	
	g_object_set_data(G_OBJECT(plugin_page),
			  GNC_PLUGIN_HIDE_MENU_ADDITIONS_NAME,
			  GINT_TO_POINTER(1));

	register_page->priv->component_manager_id = 0;
	return plugin_page;
}

GncPluginPage *
gnc_plugin_page_register_new (Account *account, gboolean subaccounts)
{
	GNCLedgerDisplay *ledger;

	if (subaccounts)
	  ledger = gnc_ledger_display_subaccounts (account);
	else
	  ledger = gnc_ledger_display_simple (account);

	return gnc_plugin_page_register_new_common(ledger);
}

GncPluginPage *
gnc_plugin_page_register_new_gl (void)
{
	GNCLedgerDisplay *ledger;

	ledger = gnc_ledger_display_gl ();
	return gnc_plugin_page_register_new_common(ledger);
}

GncPluginPage *
gnc_plugin_page_register_new_ledger (GNCLedgerDisplay *ledger)
{
	return gnc_plugin_page_register_new_common(ledger);
}

static void
gnc_plugin_page_register_class_init (GncPluginPageRegisterClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GncPluginPageClass *gnc_plugin_class = GNC_PLUGIN_PAGE_CLASS(klass);

	parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = gnc_plugin_page_register_finalize;

	gnc_plugin_class->tab_icon        = GNC_STOCK_ACCOUNT;
	gnc_plugin_class->plugin_name     = GNC_PLUGIN_PAGE_REGISTER_NAME;
	gnc_plugin_class->create_widget   = gnc_plugin_page_register_create_widget;
	gnc_plugin_class->destroy_widget  = gnc_plugin_page_register_destroy_widget;
	gnc_plugin_class->window_changed  = gnc_plugin_page_register_window_changed;
	gnc_plugin_class->merge_actions   = gnc_plugin_page_register_merge_actions;
	gnc_plugin_class->unmerge_actions = gnc_plugin_page_register_unmerge_actions;
}

static void
gnc_plugin_page_register_init (GncPluginPageRegister *plugin_page)
{
	GncPluginPageRegisterPrivate *priv;
	GncPluginPage *parent;
	GtkActionGroup *action_group;

	priv = g_new0 (GncPluginPageRegisterPrivate, 1);
	plugin_page->priv = priv;

	/* Init parent declared variables */
	parent = GNC_PLUGIN_PAGE(plugin_page);
	gnc_plugin_page_set_title(parent, _("General Ledger"));
	gnc_plugin_page_set_tab_name(parent, _("General Ledger"));
	gnc_plugin_page_set_uri(parent, "default:");

	/* Create menu and toolbar information */
	action_group = gtk_action_group_new ("GncPluginPageRegisterActions");
	priv->action_group = action_group;
	gtk_action_group_add_actions (action_group, gnc_plugin_page_register_actions,
				      gnc_plugin_page_register_n_actions, plugin_page);
	gtk_action_group_add_toggle_actions (action_group,
					     toggle_entries, n_toggle_entries,
					     plugin_page);
	gtk_action_group_add_radio_actions (action_group,
					    radio_entries_2, n_radio_entries_2,
					    REG_STYLE_LEDGER,
					    G_CALLBACK(gnc_plugin_page_register_cmd_style_changed),
					    plugin_page);

	gnc_plugin_init_short_names (action_group, short_labels);

	priv->ui_description = g_strdup("gnc-plugin-page-register-ui.xml");

	priv->lines_opt_section = DEFAULT_LINES_OPTION_SECTION;
	priv->lines_opt_name    = DEFAULT_LINES_OPTION_NAME;
	priv->lines_default     = DEFAULT_LINES_AMOUNT;
	priv->disallowCaps      = 0;
	priv->fd.cleared_match  = CLEARED_ALL;
}

static void
gnc_plugin_page_register_finalize (GObject *object)
{
	GncPluginPageRegister *page;

	ENTER("object %p", object);
	page = GNC_PLUGIN_PAGE_REGISTER (object);

	g_return_if_fail (GNC_IS_PLUGIN_PAGE_REGISTER (page));
	g_return_if_fail (page->priv != NULL);

	g_free (page->priv->ui_description);
	g_free (page->priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);
	LEAVE(" ");
}


Account *
gnc_plugin_page_register_get_account (GncPluginPageRegister *page)
{ 
	GNCLedgerDisplayType ledger_type;
	Account *leader;

	ledger_type = gnc_ledger_display_type (page->priv->ledger);
	leader = gnc_ledger_display_leader (page->priv->ledger);

	if ((ledger_type == LD_SINGLE) || (ledger_type == LD_SUBACCOUNT))
	  return leader;
	return NULL;
}


static void
gnc_plugin_page_register_update_menus (GncPluginPageRegister *page)
{ 
	GncPluginPageRegisterPrivate *priv ;
	Account *account;
	SplitRegister *sr;
	GtkAction *action;
	int i;

	priv = page->priv;
	account = gnc_plugin_page_register_get_account (page);
	gnc_plugin_update_actions(priv->action_group, actions_requiring_account,
				  "sensitive", account != NULL);

	/* Set "style" radio button */
	sr = gnc_ledger_display_get_split_register(priv->ledger);
	for (i = n_radio_entries_2 - 1; i > 0; i--) {
	  DEBUG(" index %d: comparing %x to %x", i, radio_entries_2[i].value, sr->style);
	  if (radio_entries_2[i].value == sr->style) {
	    DEBUG("match");
	    break;
	  }
	}

	/* Either a match was found, or fell out with i = 0 */
	action = gtk_action_group_get_action(priv->action_group, radio_entries_2[i].name);
	g_signal_handlers_block_by_func(action, gnc_plugin_page_register_cmd_style_changed, page);
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION(action), TRUE);
	g_signal_handlers_unblock_by_func(action, gnc_plugin_page_register_cmd_style_changed, page);

	/* Set "double line" toggle button */
	action = gtk_action_group_get_action (priv->action_group,
					      "ViewStyleDoubleLineAction");
	g_signal_handlers_block_by_func(action, gnc_plugin_page_register_cmd_style_double_line, page);
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION(action), sr->use_double_line);
	g_signal_handlers_unblock_by_func(action, gnc_plugin_page_register_cmd_style_double_line, page);
}


/* Virtual Functions */

static GtkWidget *
gnc_plugin_page_register_create_widget (GncPluginPage *plugin_page)
{
	GncPluginPageRegister *page;
	GncPluginPageRegisterPrivate *priv;
	GncWindow *gnc_window;
	guint numRows;
	GtkWidget *gsr;
	SplitRegister *sr;

	ENTER("page %p", plugin_page);
	page = GNC_PLUGIN_PAGE_REGISTER (plugin_page);
	priv = page->priv;
	if (priv->widget != NULL) {
		LEAVE("existing widget %p", priv->widget);
		return priv->widget;
	}

	priv->widget = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (priv->widget);
	
	if (priv->lines_opt_section) {
	  numRows = gnc_gconf_get_float (priv->lines_opt_section,
					 priv->lines_opt_name, NULL);
	} else {
	  numRows = priv->lines_default;
	}

	numRows = MIN(numRows, 50);
	gnc_window = GNC_WINDOW(GNC_PLUGIN_PAGE(page)->window);
	gsr = gnc_split_reg_new(priv->ledger,
				gnc_window_get_gtk_window(gnc_window),
				numRows, 0, priv->disallowCaps);
	priv->gsr = (GNCSplitReg *)gsr;
	gtk_widget_show (gsr);
	gtk_box_pack_start (GTK_BOX (priv->widget), gsr, TRUE, TRUE, 0);

	g_signal_connect (G_OBJECT (gsr), "help-changed",
			  G_CALLBACK ( gnc_plugin_page_help_changed_cb ),
			  page );
	g_signal_connect (G_OBJECT (gsr), "button-press-event",
			  G_CALLBACK (gnc_plugin_page_register_button_press_cb), page);

	sr = gnc_ledger_display_get_split_register(priv->ledger);
	gnc_split_register_config(sr, sr->type, sr->style, sr->use_double_line);
	gnc_ledger_display_refresh(priv->ledger);

	gnc_plugin_page_register_update_menus (page);

	plugin_page->summarybar = gsr_create_summary_bar(priv->gsr);
	if (plugin_page->summarybar)
	  gtk_widget_show_all(plugin_page->summarybar);

	priv->component_manager_id =
	  gnc_register_gui_component(GNC_PLUGIN_PAGE_REGISTER_NAME,
				     gnc_plugin_page_register_refresh_cb,
				     NULL, page);

	/* DRH - Probably lots of other stuff from regWindowLedger should end up here. */
	LEAVE(" ");
	return priv->widget;
}

static void
gnc_plugin_page_register_destroy_widget (GncPluginPage *plugin_page)
{
	GncPluginPageRegister *page;
	GncPluginPageRegisterPrivate *priv;

	ENTER("page %p", plugin_page);
	page = GNC_PLUGIN_PAGE_REGISTER (plugin_page);
	priv = page->priv;

	if (priv->widget == NULL)
		return;

	if (priv->component_manager_id) {
	  gnc_unregister_gui_component(priv->component_manager_id);
	  priv->component_manager_id = 0;
	}

	if (page->priv->sd.dialog) {
	  gtk_widget_destroy(page->priv->sd.dialog);
	  memset(&page->priv->sd, 0, sizeof(page->priv->sd));
	}

	if (page->priv->fd.dialog) {
	  gtk_widget_destroy(page->priv->fd.dialog);
	  memset(&page->priv->fd, 0, sizeof(page->priv->fd));
	}

	gtk_widget_hide(priv->widget);
	gnc_ledger_display_close (priv->ledger);
	priv->ledger = NULL;
	LEAVE(" ");
}

static void
gnc_plugin_page_register_window_changed (GncPluginPage *plugin_page,
					 GtkWidget *window)
{
	GncPluginPageRegister *page;
	
	g_return_if_fail (GNC_IS_PLUGIN_PAGE_REGISTER (plugin_page));

	page = GNC_PLUGIN_PAGE_REGISTER(plugin_page);
	page->priv->gsr->window = 
	  GTK_WIDGET(gnc_window_get_gtk_window(GNC_WINDOW(window)));
}
	
static void
gnc_plugin_page_register_merge_actions (GncPluginPage *plugin_page,
					GtkUIManager *ui_merge)
{
	GncPluginPageRegister *register_page;
	GncPluginPageRegisterPrivate *priv;
	
	g_return_if_fail (GNC_IS_PLUGIN_PAGE_REGISTER (plugin_page));

	register_page = GNC_PLUGIN_PAGE_REGISTER(plugin_page);
	priv = register_page->priv;

	priv->ui_merge = ui_merge;
	priv->merge_id = gnc_plugin_add_actions (priv->ui_merge,
						 priv->action_group,
						 priv->ui_description);
}
	
static void
gnc_plugin_page_register_unmerge_actions (GncPluginPage *plugin_page,
					      GtkUIManager *ui_merge)
{
	GncPluginPageRegister *plugin_page_register = GNC_PLUGIN_PAGE_REGISTER(plugin_page);
	
	g_return_if_fail (GNC_IS_PLUGIN_PAGE_REGISTER (plugin_page_register));
	g_return_if_fail (plugin_page_register->priv->merge_id != 0);
	g_return_if_fail (plugin_page_register->priv->action_group != NULL);

	gtk_ui_manager_remove_ui (ui_merge, plugin_page_register->priv->merge_id);
	gtk_ui_manager_remove_action_group (ui_merge, plugin_page_register->priv->action_group);

	plugin_page_register->priv->ui_merge = NULL;
}

static gchar *
gnc_plugin_page_register_get_tab_name (GncPluginPage *plugin_page)
{
	GNCLedgerDisplayType ledger_type;
  	GNCLedgerDisplay *ld;
	SplitRegister *reg;
	Account *leader;

	g_return_val_if_fail (GNC_IS_PLUGIN_PAGE_REGISTER (plugin_page), _("unknown"));

	ld = GNC_PLUGIN_PAGE_REGISTER (plugin_page)->priv->ledger;
	reg = gnc_ledger_display_get_split_register (ld);
	ledger_type = gnc_ledger_display_type (ld);
	leader = gnc_ledger_display_leader (ld);

	switch (ledger_type) {
	 case LD_SINGLE:
	  return g_strdup(xaccAccountGetName (leader));

	 case LD_SUBACCOUNT:
	  return g_strdup_printf("%s+", xaccAccountGetName (leader));

	 case LD_GL:
	  switch (reg->type) {
	   case GENERAL_LEDGER:
	   case INCOME_LEDGER:
	    return g_strdup(_("General Ledger"));
	   case PORTFOLIO_LEDGER:
	    return g_strdup(_("Portfolio"));
	   case SEARCH_LEDGER:
	    return g_strdup(_("Search Results"));
	   default:
	    break;
	  }

	 default:
	  break;
	}

	return g_strdup(_("unknown"));
}

/* Callbacks */
static gboolean
gnc_plugin_page_register_button_press_cb (GtkWidget *widget,
					  GdkEventButton *event,
					  GncPluginPageRegister *page)
{
	GtkWidget *menu;

	if (event->button == 3 && page->priv->ui_merge != NULL) {
		/* Maybe show a different popup menu if no account is selected. */
		menu = gtk_ui_manager_get_widget (page->priv->ui_merge, "/RegisterPopup");
		if (menu)
		  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
				  event->button, event->time);
		return TRUE;
	}

	return FALSE;
}

/************************************************************/
/*                     "Sort By" Dialog                     */
/************************************************************/

/** This function is called when the "Sort By..." dialog is closed.
 *  If the dialog was closed by any method other than clicking the OK
 *  button, the original sorting order will be restored.
 *
 *  @param dialog A pointer to the dialog box.
 *
 *  @param response A numerical value indicating why the dialog box was closed.
 *
 *  @param page A pointer to the GncPluginPageRegister associated with
 *  this dialog box.
 */
void
gnc_plugin_page_register_sort_response_cb (GtkDialog *dialog,
					   gint response,
					   GncPluginPageRegister *page)
{
  g_return_if_fail(GTK_IS_DIALOG(dialog));
  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(page));

  ENTER(" ");
  if (response != GTK_RESPONSE_OK) {
    /* Restore the original sort order */
    gnc_split_reg_set_sort_type(page->priv->gsr,
				page->priv->sd.original_sort_type);
  }
  page->priv->sd.dialog = NULL;
  gtk_widget_destroy(GTK_WIDGET(dialog));
  LEAVE(" ");
}


/** This function is called when a radion button in the "Sort By..." 
 *  dialog is clicked.
 *
 *  @param button The button that was toggled.
 *
 *  @param page A pointer to the GncPluginPageRegister associated with
 *  this dialog box.
 */
void
gnc_plugin_page_register_sort_button_cb (GtkToggleButton *button,
					 GncPluginPageRegister *page)
{
  const gchar *name;
  SortType type;

  g_return_if_fail(GTK_IS_TOGGLE_BUTTON(button));
  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(page));

  name = gtk_widget_get_name(GTK_WIDGET(button));
  ENTER("button %s(%p), page %p", name, button, page);
  type = SortTypefromString(name);
  gnc_split_reg_set_sort_type(page->priv->gsr, type);
  LEAVE(" ");
}

/************************************************************/
/*                    "Filter By" Dialog                    */
/************************************************************/

/** This function updates the "cleared match" term of the register
 *  query.  It unconditionally removes any old "cleared match" query
 *  term, then adds back a new query term if needed.  There seems to
 *  be a bug in the current g2 register code such that when the number
 *  of entries in the register doesn't fill up the window, the blank
 *  space at the end of the window isn't correctly redrawn.  This
 *  function works around that problem, but a root cause analysis
 *  should probably be done.
 *
 *  @param page A pointer to the GncPluginPageRegister that is
 *  associated with this filter dialog.
 */
static void
gnc_ppr_update_status_query (GncPluginPageRegister *page)
{
  GncPluginPageRegisterPrivate *priv;
  GSList *param_list;
  Query *query;

  ENTER(" ");
  priv = page->priv;
  query = gnc_ledger_display_get_query( priv->ledger );
  if (!query) {
    LEAVE("no query found");
    return;
  }

  /* Remove the old status match */
  param_list = gncQueryBuildParamList (SPLIT_RECONCILE, NULL);
  if (param_list) {
    gncQueryPurgeTerms (query, param_list);
    g_slist_free(param_list);
  }

  /* Install the new status match */
  if (priv->fd.cleared_match != CLEARED_ALL)
    xaccQueryAddClearedMatch(query, priv->fd.cleared_match, QUERY_AND);

  gnc_ledger_display_refresh (priv->ledger);
  /* The register widget doesn't seem to clean up when it shrinks.
   * Redraw the entire widget so the screen looks clean. */
  gtk_widget_queue_draw(priv->widget);
  LEAVE(" ");
}


/** This function updates the "date posted" term of the register
 *  query.  It unconditionally removes any old "date posted" query
 *  term, then adds back a new query term if needed.  There seems to
 *  be a bug in the current g2 register code such that when the number
 *  of entries in the register doesn't fill up the window, the blank
 *  space at the end of the window isn't correctly redrawn.  This
 *  function works around that problem, but a root cause analysis
 *  should probably be done.
 *
 *  @param page A pointer to the GncPluginPageRegister that is
 *  associated with this filter dialog.
 */
static void
gnc_ppr_update_date_query (GncPluginPageRegister *page)
{
  GncPluginPageRegisterPrivate *priv;
  GSList *param_list;
  Query *query;

  ENTER(" ");
  priv = page->priv;
  if (!priv->ledger) {
    LEAVE("no ledger");
    return;
  }

  query = gnc_ledger_display_get_query( priv->ledger );
  if (!query) {
    LEAVE("no query");
    return;
  }

  /* Delete any existing old date spec. */
  param_list = gncQueryBuildParamList(SPLIT_TRANS, TRANS_DATE_POSTED, NULL);
  if (param_list) {
    gncQueryPurgeTerms (query, param_list);
    g_slist_free(param_list);
  }

  if (priv->fd.start_time || priv->fd.end_time) {
    /* Build a new spec */
    xaccQueryAddDateMatchTT(query,
			    priv->fd.start_time != 0, priv->fd.start_time,
			    priv->fd.end_time != 0,   priv->fd.end_time,
			    QUERY_AND);
  }

  gnc_ledger_display_refresh (priv->ledger);
  /* The register widget doesn't seem to clean up when it shrinks.
   * Redraw the entire widget so the screen looks clean. */
  gtk_widget_queue_draw(priv->widget);
  LEAVE(" ");
}


/** This function is called whenever one of the status entries is
 *  checked or unchecked.  It updates the status value maintained for
 *  the filter dialog, and calls another function to do the work of
 *  applying the change to the register itself.
 *
 *  @param button The toggle button that was changed.
 *
 *  @param page A pointer to the GncPluginPageRegister that is
 *  associated with this filter dialog.
 */
void
gnc_plugin_page_register_filter_status_one_cb (GtkToggleButton *button,
					       GncPluginPageRegister *page)
{
  const gchar *name;
  gint i, value;

  g_return_if_fail(GTK_IS_CHECK_BUTTON(button));
  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(page));

  name = gtk_widget_get_name(GTK_WIDGET(button));
  ENTER("toggle button %s (%p), plugin_page %p", name, button, page);

  /* Determine what status bit to change */
  value = CLEARED_NONE;
  for (i = 0; status_actions[i].action_name; i++) {
    if (strcmp(name, status_actions[i].action_name) == 0) {
      value = status_actions[i].value;
      break;
    }
  }

  /* Compute the new match status */
  if (gtk_toggle_button_get_active(button))
    page->priv->fd.cleared_match |= value;
  else
    page->priv->fd.cleared_match &= ~value;
  gnc_ppr_update_status_query(page);
  LEAVE(" ");
}


/** This function is called whenever the "select all" status button is
 *  clicked.  It updates all of the checkbox widgets, then updates the
 *  query on the register.
 *
 *  @param button The button that was clicked.
 *
 *  @param page A pointer to the GncPluginPageRegister that is
 *  associated with this filter dialog.
 */
void
gnc_plugin_page_register_filter_status_all_cb (GtkButton *button,
					       GncPluginPageRegister *page)
{
  GtkWidget *widget;
  gint i;

  g_return_if_fail(GTK_IS_BUTTON(button));
  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(page));

  ENTER("(button %p, page %p)", button, page);

  /* Turn on all the check menu items */
  for (i = 0; status_actions[i].action_name; i++) {
    widget = gnc_glade_lookup_widget(GTK_WIDGET(button),
				     status_actions[i].action_name);
    g_signal_handlers_block_by_func(widget, gnc_plugin_page_register_filter_status_one_cb, page);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(widget), TRUE);
    g_signal_handlers_unblock_by_func(widget, gnc_plugin_page_register_filter_status_one_cb, page);
  }

  /* Set the requested status */
  page->priv->fd.cleared_match = CLEARED_ALL;
  gnc_ppr_update_status_query(page);
  LEAVE(" ");
}


/** This function computes the starting and ending times for the
 *  filter by examining the dialog widgets to see which ones are
 *  selected, and will pull times out of the data entry boxes if
 *  necessary.  This function must exist to handle the case where the
 *  "show all" button was Seleucid, and the user clicks on the "select
 *  range" button.  Since it exists, it make sense for the rest of the
 *  callbacks to take advantage of it.
 *
 *  @param page A pointer to the GncPluginPageRegister that is
 *  associated with this filter dialog.
 */
static void
get_filter_times(GncPluginPageRegister *page)
{
  GtkWidget *button, *today, *gde;
  time_t time_val;

  button = gnc_glade_lookup_widget(page->priv->fd.dialog, "start_date_choose");
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))) {
    gde = gnc_glade_lookup_widget(button, "start_date");
    time_val = gnc_date_edit_get_date(GNC_DATE_EDIT(gde));
    time_val = gnc_timet_get_day_start(time_val);
    page->priv->fd.start_time = time_val;
  } else {
    today = gnc_glade_lookup_widget(page->priv->fd.dialog, "start_date_today");
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(today))) {
      page->priv->fd.start_time = gnc_timet_get_today_start();
    } else {
      page->priv->fd.start_time = 0;
    }
  }

  button = gnc_glade_lookup_widget(page->priv->fd.dialog, "end_date_choose");
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))) {
    gde = gnc_glade_lookup_widget(button, "end_date");
    time_val = gnc_date_edit_get_date(GNC_DATE_EDIT(gde));
    time_val = gnc_timet_get_day_end(time_val);
    page->priv->fd.end_time = time_val;
  } else {
    today = gnc_glade_lookup_widget(page->priv->fd.dialog, "end_date_today");
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(today))) {
      page->priv->fd.end_time = gnc_timet_get_today_end();
    } else {
      page->priv->fd.end_time = 0;
    }
  }
}


/** This function is called when the "select range" radio button
 *  changes state.  Since there are only two choices in this radio
 *  group, this one signal can be used to handle all cases.  This
 *  function is responsible for setting the sensitivity of the table
 *  of widgets underneath the "select range" choice, and updating the
 *  time limitation on the register query.  This is handled by a
 *  helper function when the radio button is selected (as potentially
 *  all the widgets in the table need to be inspected), and is trivial
 *  for th eother case.
 *
 *  @param button A pointer to the "select range" radio button.
 *
 *  @param page A pointer to the GncPluginPageRegister that is
 *  associated with this filter dialog.
 */
void
gnc_plugin_page_register_filter_select_range_cb (GtkRadioButton *button,
						 GncPluginPageRegister *page)
{
  GtkWidget *table;
  gboolean active;

  g_return_if_fail(GTK_IS_RADIO_BUTTON(button));
  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(page));

  ENTER("(button %p, page %p)", button, page);
  table = gnc_glade_lookup_widget(GTK_WIDGET(button), "select_range_table");
  active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
  gtk_widget_set_sensitive(table, active);
  if (active) {
    get_filter_times(page);
  } else {
    page->priv->fd.start_time = 0;
    page->priv->fd.end_time = 0;
  }
  gnc_ppr_update_date_query(page);
  LEAVE(" ");
}


/** This function is called when one of the start date entry widgets
 *  is updated.  It simply calls common routines to determine the
 *  start/end times and update the register query.
 *
 *  @param unused A pointer to a GncDateEntry widgets, but it could be
 *  any widget.
 *
 *  @param page A pointer to the GncPluginPageRegister that is
 *  associated with this filter dialog.
 */
static void
gnc_plugin_page_register_filter_gde_changed_cb (GtkWidget *unused,
						GncPluginPageRegister *page)
{
  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(page));

  ENTER("(widget %s(%p), page %p)", gtk_widget_get_name(unused), unused, page);
  get_filter_times(page);
  gnc_ppr_update_date_query(page);
  LEAVE(" ");
}


/** This function is called when one of the start date radio buttons
 *  is selected.  It updates the sensitivity of the date entry widget,
 *  then calls a common routine to determine the start/end times and
 *  update the register query.
 *
 *  *Note: This function is actually called twice for each new radio
 *  button selection.  The first time call is to uncheck the old
 *  button, and the second time to check the new button.  This does
 *  make a kind of sense, as radio buttons are nothing more than
 *  linked toggle buttons where only one can be active.
 *
 *  @param button The button whose state is changing.  This will be
 *  the previously selected button the first of the pair of calls to
 *  this function, and will be the newly selected button the second
 *  time.
 *
 *  @param page A pointer to the GncPluginPageRegister that is
 *  associated with this filter dialog.
 */
void
gnc_plugin_page_register_filter_start_cb (GtkWidget *radio,
					  GncPluginPageRegister *page)
{
  GtkWidget *widget, *gde;
  gboolean active;

  g_return_if_fail(GTK_IS_RADIO_BUTTON(radio));
  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(page));

  ENTER("(radio %s(%p), page %p)", gtk_widget_get_name(radio), radio, page);
  if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio))) {
    LEAVE("1st callback of pair. Defer to 2nd callback.");
    return;
  }

  widget = gnc_glade_lookup_widget(radio, "start_date_choose");
  active = (widget == radio);
  gde = gnc_glade_lookup_widget(radio, "start_date");
  gtk_widget_set_sensitive(gde, active);

  get_filter_times(page);
  gnc_ppr_update_date_query(page);
  LEAVE(" ");
}


/** This function is called when one of the end date radio buttons is
 *  selected.  It updates the sensitivity of the date entry widget,
 *  then calls a common routine to determine the start/end times and
 *  update the register query.
 *
 *  *Note: This function is actually called twice for each new radio
 *  button selection.  The first time call is to uncheck the old
 *  button, and the second time to check the new button.  This does
 *  make a kind of sense, as radio buttons are nothing more than
 *  linked toggle buttons where only one can be active.
 *
 *  @param button The button whose state is changing.  This will be
 *  the previously selected button the first of the pair of calls to
 *  this function, and will be the newly selected button the second
 *  time.
 *
 *  @param page A pointer to the GncPluginPageRegister that is
 *  associated with this filter dialog.
 */
void
gnc_plugin_page_register_filter_end_cb (GtkWidget *radio,
					GncPluginPageRegister *page)
{
  GtkWidget *widget, *gde;
  gboolean active;

  g_return_if_fail(GTK_IS_RADIO_BUTTON(radio));
  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(page));

  ENTER("(radio %s(%p), page %p)", gtk_widget_get_name(radio), radio, page);
  if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio))) {
    LEAVE("1st callback of pair. Defer to 2nd callback.");
    return;
  }

  widget = gnc_glade_lookup_widget(radio, "end_date_choose");
  active = (widget == radio);
  gde = gnc_glade_lookup_widget(radio, "end_date");
  gtk_widget_set_sensitive(gde, active);

  get_filter_times(page);
  gnc_ppr_update_date_query(page);
  LEAVE(" ");
}


/** This function is called when the "Filter By..." dialog is closed.
 *  If the dialog was closed by any method other than clicking the OK
 *  button, the original sorting order will be restored.
 *
 *  @param dialog A pointer to the dialog box.
 *
 *  @param response A numerical value indicating why the dialog box was closed.
 *
 *  @param page A pointer to the GncPluginPageRegister associated with
 *  this dialog box.
 */
void
gnc_plugin_page_register_filter_response_cb (GtkDialog *dialog,
					     gint response,
					     GncPluginPageRegister *page)
{
  g_return_if_fail(GTK_IS_DIALOG(dialog));
  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(page));

  ENTER(" ");
  if (response != GTK_RESPONSE_OK) {
    /* Remove the old status match */
    page->priv->fd.cleared_match = page->priv->fd.original_cleared_match;
    gnc_ppr_update_status_query(page);
    page->priv->fd.start_time = page->priv->fd.original_start_time;
    page->priv->fd.end_time = page->priv->fd.original_end_time;
    gnc_ppr_update_date_query(page);
  }
  page->priv->fd.dialog = NULL;
  gtk_widget_destroy(GTK_WIDGET(dialog));
  LEAVE(" ");
}


/************************************************************/
/*                  Report Helper Functions                 */
/************************************************************/

static char *
gnc_reg_get_name (GNCLedgerDisplay *ledger, gboolean for_window)
{
  Account *leader;
  SplitRegister *reg;
  gchar *account_name;
  gchar *reg_name;
  gchar *name;
  GNCLedgerDisplayType ledger_type;

  if (ledger == NULL)
    return NULL;

  reg = gnc_ledger_display_get_split_register (ledger);
  ledger_type = gnc_ledger_display_type (ledger);

  switch (reg->type)
  {
    case GENERAL_LEDGER:
    case INCOME_LEDGER:
      if (for_window)
        reg_name = _("General Ledger");
      else
        reg_name = _("General Ledger Report");
      break;
    case PORTFOLIO_LEDGER:
      if (for_window)
        reg_name = _("Portfolio");
      else
        reg_name = _("Portfolio Report");
      break;
    case SEARCH_LEDGER:
      if (for_window)
        reg_name = _("Search Results");
      else
        reg_name = _("Search Results Report");
      break;
    default:
      if (for_window)
        reg_name = _("Register");
      else
        reg_name = _("Register Report");
      break;
  }

  leader = gnc_ledger_display_leader (ledger);

  if ((leader != NULL) && (ledger_type != LD_GL))
  {
    account_name = xaccAccountGetFullName (leader,
                                           gnc_get_account_separator ());

    if (ledger_type == LD_SINGLE)
    {
      name = g_strconcat (account_name, " - ", reg_name, NULL);
    }
    else 
    {
      name = g_strconcat (account_name, " ", _("and subaccounts"), " - ", reg_name, NULL);
    }
    g_free(account_name);
  }
  else
    name = g_strdup (reg_name);

  return name;
}

static int
report_helper (GNCLedgerDisplay *ledger, Split *split, Query *query)
{
  SplitRegister *reg = gnc_ledger_display_get_split_register (ledger);
  Account *account;
  char *str;
  SCM qtype;
  SCM args;
  SCM func;
  SCM arg;

  args = SCM_EOL;

  func = scm_c_eval_string ("gnc:register-report-create");
  g_return_val_if_fail (SCM_PROCEDUREP (func), -1);

  arg = scm_makfrom0str (gnc_split_register_get_credit_string (reg));
  args = scm_cons (arg, args);

  arg = scm_makfrom0str (gnc_split_register_get_debit_string (reg));
  args = scm_cons (arg, args);

  str = gnc_reg_get_name (ledger, FALSE);
  arg = scm_makfrom0str (str);
  args = scm_cons (arg, args);
  g_free (str);

  arg = SCM_BOOL (reg->use_double_line);
  args = scm_cons (arg, args);

  arg = SCM_BOOL (reg->style == REG_STYLE_JOURNAL);
  args = scm_cons (arg, args);

  if (!query)
  {
    query = gnc_ledger_display_get_query (ledger);
    g_return_val_if_fail (query != NULL, -1);
  }

  qtype = scm_c_eval_string("<gnc:Query*>");
  g_return_val_if_fail (qtype != SCM_UNDEFINED, -1);

  arg = gw_wcp_assimilate_ptr (query, qtype);
  args = scm_cons (arg, args);
  g_return_val_if_fail (arg != SCM_UNDEFINED, -1);


  if (split)
  {
    qtype = scm_c_eval_string("<gnc:Split*>");
    g_return_val_if_fail (qtype != SCM_UNDEFINED, -1);
    arg = gw_wcp_assimilate_ptr (split, qtype);
  }
  else
  {
    arg = SCM_BOOL_F;
  }
  args = scm_cons (arg, args);
  g_return_val_if_fail (arg != SCM_UNDEFINED, -1);


  qtype = scm_c_eval_string("<gnc:Account*>");
  g_return_val_if_fail (qtype != SCM_UNDEFINED, -1);

  account = gnc_ledger_display_leader (ledger);
  arg = gw_wcp_assimilate_ptr (account, qtype);
  args = scm_cons (arg, args);
  g_return_val_if_fail (arg != SCM_UNDEFINED, -1);


  /* Apply the function to the args */
  arg = scm_apply (func, args, SCM_EOL);
  g_return_val_if_fail (SCM_EXACTP (arg), -1);

  return scm_num2int (arg, SCM_ARG1, __FUNCTION__);
}

/************************************************************/
/*                     Command callbacks                    */
/************************************************************/

static void
gnc_plugin_page_register_cmd_print_check (GtkAction *action,
					  GncPluginPageRegister *plugin_page)
{
  GncPluginPageRegisterPrivate *priv;
  SplitRegister * reg;
  Split         * split;
  Transaction   * trans;
  const char    * payee;
  const char    * memo;
  gnc_numeric   amount;
  time_t        date;


  ENTER("(action %p, plugin_page %p)", action, plugin_page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  priv = plugin_page->priv;
  reg = gnc_ledger_display_get_split_register (priv->ledger);
  split    = gnc_split_register_get_current_split(reg);
  trans    = xaccSplitGetParent(split);

  if(split && trans)
  {
    payee  = xaccTransGetDescription(trans);
    memo   = xaccTransGetNotes(trans);
    if (memo == NULL)
      memo = "";
    amount = xaccSplitGetAmount(split);
    amount = gnc_numeric_abs (amount);
    date   = xaccTransGetDate(trans);

    gnc_ui_print_check_dialog_create(plugin_page, payee, amount, date, memo);
  }
  LEAVE(" ");
}


static void
gnc_plugin_page_register_cmd_cut (GtkAction *action,
				  GncPluginPageRegister *page)
{
  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(page));

  ENTER("(action %p, page %p)", action, page);
  gnucash_register_cut_clipboard(page->priv->gsr->reg);
  LEAVE("");
}


static void
gnc_plugin_page_register_cmd_copy (GtkAction *action,
				   GncPluginPageRegister *page)
{
  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(page));

  ENTER("(action %p, page %p)", action, page);
  gnucash_register_copy_clipboard(page->priv->gsr->reg);
  LEAVE("");
}


static void
gnc_plugin_page_register_cmd_paste (GtkAction *action,
				    GncPluginPageRegister *page)
{
  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(page));

  ENTER("(action %p, page %p)", action, page);
  gnucash_register_paste_clipboard(page->priv->gsr->reg);
  LEAVE("");
}


static void
gnc_plugin_page_register_cmd_edit_account (GtkAction *action,
					   GncPluginPageRegister *page)
{
  Account *account;

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(page));

  ENTER("(action %p, page %p)", action, page);
  account = gnc_plugin_page_register_get_account (page);
  if (account)
    gnc_ui_edit_account_window (account);
  LEAVE(" ");
}


static void
gnc_plugin_page_register_cmd_cut_transaction (GtkAction *action,
					      GncPluginPageRegister *page)
{
  SplitRegister *reg;

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(page));

  ENTER("(action %p, page %p)", action, page);
  reg = gnc_ledger_display_get_split_register(page->priv->ledger);
  gnc_split_register_cut_current(reg);
  LEAVE(" ");
}


static void
gnc_plugin_page_register_cmd_copy_transaction (GtkAction *action,
					       GncPluginPageRegister *page)
{
  SplitRegister *reg;

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(page));

  ENTER("(action %p, page %p)", action, page);
  reg = gnc_ledger_display_get_split_register(page->priv->ledger);
  gnc_split_register_copy_current(reg);
  LEAVE(" ");
}


static void
gnc_plugin_page_register_cmd_paste_transaction (GtkAction *action,
						GncPluginPageRegister *page)
{
  SplitRegister *reg;

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(page));

  ENTER("(action %p, page %p)", action, page);
  reg = gnc_ledger_display_get_split_register(page->priv->ledger);
  gnc_split_register_paste_current(reg);
  LEAVE(" ");
}


static void
gnc_plugin_page_register_cmd_void_transaction (GtkAction *action,
					       GncPluginPageRegister *page)
{
  GtkWidget *dialog, *entry;
  SplitRegister *reg;
  Transaction *trans;
  GladeXML *xml;
  const char *reason;
  gint result;

  ENTER("(action %p, page %p)", action, page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(page));

  reg = gnc_ledger_display_get_split_register(page->priv->ledger);
  trans = gnc_split_register_get_current_trans(reg);
  if (trans == NULL)
    return;
  if (xaccTransHasSplitsInState(trans, VREC)) {
    gnc_error_dialog(NULL, _("This transaction has already been voided."));
    return;
  }
  if (xaccTransHasReconciledSplits(trans) || xaccTransHasSplitsInState(trans, CREC)) {
    gnc_error_dialog(NULL, _("You cannot void a transaction with reconciled or cleared splits."));
    return;
  }

  xml = gnc_glade_xml_new("register.glade", "Void Transaction");
  dialog = glade_xml_get_widget(xml, "Void Transaction");
  entry = glade_xml_get_widget(xml, "reason");

  result = gtk_dialog_run(GTK_DIALOG(dialog));
  if (result == GTK_RESPONSE_OK) {
    reason = gtk_entry_get_text(GTK_ENTRY(entry));
    if (reason == NULL)
      reason = "";
    gnc_split_register_void_current_trans(reg, reason);
  }

  /* All done. Get rid of it. */
  gtk_widget_destroy(dialog);
  g_free(xml);
}


static void
gnc_plugin_page_register_cmd_unvoid_transaction (GtkAction *action,
						 GncPluginPageRegister *page)
{
  SplitRegister *reg;
  Transaction *trans;


  ENTER("(action %p, page %p)", action, page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(page));

  reg = gnc_ledger_display_get_split_register(page->priv->ledger);
  trans = gnc_split_register_get_current_trans(reg);
  if (!xaccTransHasSplitsInState(trans, VREC)) {
    gnc_error_dialog(NULL, _("This transaction is not voided."));
    return;
  }
  gnc_split_register_unvoid_current_trans(reg);
  LEAVE(" ");
}


static void
gnc_plugin_page_register_cmd_reverse_transaction (GtkAction *action,
						  GncPluginPageRegister *page)
{
  SplitRegister *reg;
  GNCSplitReg *gsr;
  Transaction *trans, *new_trans;
  kvp_frame *txn_frame;
  kvp_value *kvp_val;


  ENTER("(action %p, page %p)", action, page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(page));

  reg = gnc_ledger_display_get_split_register(page->priv->ledger);
  trans = gnc_split_register_get_current_trans(reg);
  if (trans == NULL)
    return;

  txn_frame = xaccTransGetSlots( trans );
  if ( txn_frame != NULL ) {
    kvp_val = kvp_frame_get_slot( txn_frame, "reversed-by" );
    if ( kvp_val ) {
      // GUID *fromSXId = kvp_value_get_guid( kvp_val );
      gnc_error_dialog(GNC_PLUGIN_PAGE(page)->window,
		       _("This transaction has already been reversed."));
      return;
    }
  }

  new_trans = xaccTransClone(trans);
  xaccTransReverse(new_trans);

  /* Clear transaction level info */
  xaccTransBeginEdit(new_trans);
  xaccTransSetDatePostedSecs(new_trans, time(NULL));
  xaccTransSetDateEnteredSecs(new_trans, time(NULL));
  xaccTransCommitEdit(new_trans);

  /* Now update the original with a pointer to the new one */
  xaccTransBeginEdit(trans);
  kvp_val = kvp_value_new_guid (xaccTransGetGUID(new_trans));
  kvp_frame_set_slot_nc(txn_frame, "reversed-by", kvp_val);
  xaccTransCommitEdit(trans);

  /* Now jump to new trans */
  gsr = gnc_plugin_page_register_get_gsr(GNC_PLUGIN_PAGE(page));
  gnc_split_reg_jump_to_split(gsr, xaccTransGetSplit(new_trans, 0));
  LEAVE(" ");
}


static void
gnc_plugin_page_register_cmd_view_sort_by (GtkAction *action,
					   GncPluginPageRegister *page)
{
  GtkWidget *dialog, *button;
  GladeXML *xml;
  SortType sort;
  const gchar *name;
  gchar *title;

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(page));
  ENTER("(action %p, page %p)", action, page);

  if (page->priv->sd.dialog) {
    gtk_window_present(GTK_WINDOW(page->priv->sd.dialog));
    LEAVE("existing dialog");
    return;
  }

  /* Create the dialog */
  xml = gnc_glade_xml_new ("register.glade", "Sort By");
  dialog = glade_xml_get_widget (xml, "Sort By");
  page->priv->sd.dialog = dialog;
  gtk_window_set_transient_for(GTK_WINDOW(dialog),
			       GTK_WINDOW(GNC_PLUGIN_PAGE(page)->window));
  title = g_strdup_printf(N_("Sort %s by..."),
			  gnc_plugin_page_get_tab_name(GNC_PLUGIN_PAGE(page)));
  gtk_window_set_title(GTK_WINDOW(dialog), title);
  g_free(title);

  /* Set the button for the current sort order */
  sort = gnc_split_reg_get_sort_type(page->priv->gsr);
  name = SortTypeasString(sort);
  button = glade_xml_get_widget (xml, name);
  DEBUG("current sort %d, button %s(%p)", sort, name, button);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
  page->priv->sd.original_sort_type = sort;

  /* Wire it up */
  glade_xml_signal_autoconnect_full(xml, gnc_glade_autoconnect_full_func,
				    page);

  /* Show it */
  gtk_widget_show(dialog);
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_view_filter_by (GtkAction *action,
					     GncPluginPageRegister *page)
{
  GtkWidget *dialog, *toggle, *button, *start_date, *end_date, *table;
  time_t start_time, end_time, time_val;
  GladeXML *xml;
  gboolean sensitive, value;
  Query *query;
  gchar *title;
  int i;

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(page));
  ENTER("(action %p, page %p)", action, page);

  if (page->priv->fd.dialog) {
    gtk_window_present(GTK_WINDOW(page->priv->fd.dialog));
    LEAVE("existing dialog");
    return;
  }

  /* Create the dialog */
  xml = gnc_glade_xml_new ("register.glade", "Filter By");
  dialog = glade_xml_get_widget (xml, "Filter By");
  page->priv->fd.dialog = dialog;
  gtk_window_set_transient_for(GTK_WINDOW(dialog),
			       GTK_WINDOW(GNC_PLUGIN_PAGE(page)->window));
  title = g_strdup_printf(N_("Filter %s by..."),
			  gnc_plugin_page_get_tab_name(GNC_PLUGIN_PAGE(page)));
  gtk_window_set_title(GTK_WINDOW(dialog), title);
  g_free(title);

  /* Set the check buttons for the current status */
  for (i = 0; status_actions[i].action_name; i++) {
    toggle = glade_xml_get_widget (xml, status_actions[i].action_name);
    value = page->priv->fd.cleared_match & status_actions[i].value;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle), value);
  }
  page->priv->fd.original_cleared_match = page->priv->fd.cleared_match;

  /* Set the date info */
  button = glade_xml_get_widget(xml, "filter_show_range");
  query = gnc_ledger_display_get_query (page->priv->ledger);
  xaccQueryGetDateMatchTT(query, &start_time, &end_time);
  page->priv->fd.original_start_time = start_time;
  page->priv->fd.start_time = start_time;
  page->priv->fd.original_end_time = end_time;
  page->priv->fd.end_time = end_time;

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
			       start_time || end_time);
  table = glade_xml_get_widget(xml, "select_range_table");
  gtk_widget_set_sensitive(GTK_WIDGET(table), start_time || end_time);

  {
    /* Start date info */
    if (start_time == 0) {
      button = glade_xml_get_widget(xml, "start_date_earliest");
      sensitive = FALSE;
    } else {
      if ((start_time >= gnc_timet_get_today_start()) &&
	  (start_time <= gnc_timet_get_today_end())) {
	button = glade_xml_get_widget(xml, "start_date_today");
	sensitive = FALSE;
      } else {
	button = glade_xml_get_widget(xml, "start_date_choose");
	sensitive = TRUE;
      }
    }
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    start_date = glade_xml_get_widget(xml, "start_date");
    gtk_widget_set_sensitive(GTK_WIDGET(start_date), sensitive);
    time_val = xaccQueryGetEarliestDateFound (query); 
    gnc_date_edit_set_time(GNC_DATE_EDIT(start_date), time_val);
    g_signal_connect (G_OBJECT (start_date), "date-changed",
		      G_CALLBACK (gnc_plugin_page_register_filter_gde_changed_cb),
		      page);
  }

  {
    /* End date info */
    if (end_time == 0) {
      button = glade_xml_get_widget(xml, "end_date_latest");
      sensitive = FALSE;
    } else {
      if ((end_time >= gnc_timet_get_today_start()) &&
	  (end_time <= gnc_timet_get_today_end())) {
	button = glade_xml_get_widget(xml, "end_date_today");
	sensitive = FALSE;
      } else {
	button = glade_xml_get_widget(xml, "end_date_choose");
	sensitive = TRUE;
      }
    }
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    end_date = glade_xml_get_widget(xml, "end_date");
    gtk_widget_set_sensitive(GTK_WIDGET(end_date), sensitive);
    time_val = xaccQueryGetLatestDateFound (query); 
    gnc_date_edit_set_time(GNC_DATE_EDIT(end_date), time_val);
    g_signal_connect (G_OBJECT (end_date), "date-changed",
		      G_CALLBACK (gnc_plugin_page_register_filter_gde_changed_cb),
		      page);
  }

  /* Wire it up */
  glade_xml_signal_autoconnect_full(xml, gnc_glade_autoconnect_full_func, page);

  /* Show it */
  gtk_widget_show_all(dialog);
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_style_changed (GtkAction *action,
					    GtkRadioAction *current,
					    GncPluginPageRegister *plugin_page)
{
  GncPluginPageRegisterPrivate *priv;
  SplitRegisterStyle value;

  ENTER("(action %p, radio action %p, plugin_page %p)",
	action, current, plugin_page);

  g_return_if_fail(GTK_IS_ACTION(action));
  g_return_if_fail(GTK_IS_RADIO_ACTION(current));
  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  priv = plugin_page->priv;
  value = gtk_radio_action_get_current_value(current);
  gnc_split_reg_change_style(priv->gsr, value);

  /* The register widget doesn't seem to clean up when it shrinks.
   * Redraw the entire widget so the screen looks clean. */
  gtk_widget_queue_draw(priv->widget);
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_style_double_line (GtkToggleAction *action,
						GncPluginPageRegister *plugin_page)
{
  GncPluginPageRegisterPrivate *priv;
  SplitRegister *reg;
  gboolean use_double_line;

  ENTER("(action %p, plugin_page %p)", action, plugin_page);

  g_return_if_fail(GTK_IS_ACTION(action));
  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  priv = plugin_page->priv;
  reg = gnc_ledger_display_get_split_register (priv->ledger);

  use_double_line =  gtk_toggle_action_get_active (action);
  if (use_double_line != reg->use_double_line) {
    gnc_split_register_config(reg, reg->type, reg->style, use_double_line);
    gnc_ledger_display_refresh(priv->ledger);
  }
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_transfer (GtkAction *action,
				       GncPluginPageRegister *page)
{
  Account *account;
  GtkWidget *window;

  ENTER("(action %p, plugin_page %p)", action, page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(page));

  account = gnc_plugin_page_register_get_account (page);
  window = GNC_PLUGIN_PAGE (page)->window;
  gnc_xfer_dialog (window, account);
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_reconcile (GtkAction *action,
					GncPluginPageRegister *page)
{
  Account *account;
  GtkWidget *window;
  RecnWindow * recnData;

  ENTER("(action %p, plugin_page %p)", action, page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(page));

  account = gnc_plugin_page_register_get_account (page);

  window = GNC_PLUGIN_PAGE (page)->window;
  recnData = recnWindow (window, account);
  gnc_ui_reconcile_window_raise (recnData);
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_stock_split (GtkAction *action,
					  GncPluginPageRegister *page)
{
  Account *account;

  ENTER("(action %p, plugin_page %p)", action, page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(page));

  account = gnc_plugin_page_register_get_account (page);
  gnc_stock_split_dialog (NULL, account);
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_lots (GtkAction *action,
				   GncPluginPageRegister *page)
{
  Account *account;

  ENTER("(action %p, plugin_page %p)", action, page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(page));

  account = gnc_plugin_page_register_get_account (page);
  gnc_lot_viewer_dialog (account);
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_enter_transaction (GtkAction *action,
						GncPluginPageRegister *plugin_page)
{
  GncPluginPageRegisterPrivate *priv;

  ENTER("(action %p, plugin_page %p)", action, plugin_page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  priv = plugin_page->priv;
  gnc_split_reg_enter(priv->gsr, FALSE);
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_cancel_transaction (GtkAction *action,
						 GncPluginPageRegister *plugin_page)
{
  GncPluginPageRegisterPrivate *priv;

  ENTER("(action %p, plugin_page %p)", action, plugin_page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  priv = plugin_page->priv;
  gnc_split_register_cancel_cursor_trans_changes
    (gnc_ledger_display_get_split_register(priv->ledger));
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_delete_transaction (GtkAction *action,
						 GncPluginPageRegister *plugin_page)
{
  GncPluginPageRegisterPrivate *priv;

  ENTER("(action %p, plugin_page %p)", action, plugin_page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  priv = plugin_page->priv;
  gsr_default_delete_handler(priv->gsr, NULL);
  LEAVE(" ");

}

static void
gnc_plugin_page_register_cmd_blank_transaction (GtkAction *action,
						GncPluginPageRegister *plugin_page)
{
  GncPluginPageRegisterPrivate *priv;
  SplitRegister *reg;

  ENTER("(action %p, plugin_page %p)", action, plugin_page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  priv = plugin_page->priv;
  reg = gnc_ledger_display_get_split_register (priv->ledger);

  if (gnc_split_register_save (reg, TRUE))
    gnc_split_register_redraw (reg);

  gnc_split_reg_jump_to_blank (priv->gsr);
}

static void
gnc_plugin_page_register_cmd_duplicate_transaction (GtkAction *action,
						    GncPluginPageRegister *plugin_page)
{
  GncPluginPageRegisterPrivate *priv;

  ENTER("(action %p, plugin_page %p)", action, plugin_page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  priv = plugin_page->priv;
  gnc_split_register_duplicate_current
    (gnc_ledger_display_get_split_register(priv->ledger));
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_reinitialize_transaction (GtkAction *action,
						       GncPluginPageRegister *plugin_page)
{
  GncPluginPageRegisterPrivate *priv;

  ENTER("(action %p, plugin_page %p)", action, plugin_page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  priv = plugin_page->priv;
  gsr_default_reinit_handler(priv->gsr, NULL);
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_expand_transaction (GtkToggleAction *action,
						 GncPluginPageRegister *plugin_page)
{
  GncPluginPageRegisterPrivate *priv;
  SplitRegister *reg;
  gboolean expand;

  ENTER("(action %p, plugin_page %p)", action, plugin_page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  priv = plugin_page->priv;
  reg = gnc_ledger_display_get_split_register (priv->ledger);
  expand = gtk_toggle_action_get_active (action);
  gnc_split_register_expand_current_trans (reg, expand);
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_exchange_rate (GtkAction *action,
					    GncPluginPageRegister *plugin_page)
{
  GncPluginPageRegisterPrivate *priv;
  SplitRegister *reg;

  ENTER("(action %p, plugin_page %p)", action, plugin_page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  priv = plugin_page->priv;
  reg = gnc_ledger_display_get_split_register (priv->ledger);

  /* XXX Ignore the return value -- we don't care if this succeeds */
  (void)gnc_split_register_handle_exchange (reg, TRUE);
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_jump (GtkAction *action,
				   GncPluginPageRegister *plugin_page)
{
  GncPluginPageRegisterPrivate *priv;
  GncPluginPage *new_page;
  GncPluginPageRegister *new_reg_page;
  GtkWidget *window;
  GNCSplitReg *gsr;
  SplitRegister *reg;
  Account *account;
  Account *leader;
  Split *split;

  ENTER("(action %p, plugin_page %p)", action, plugin_page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  priv = plugin_page->priv;
  window = GNC_PLUGIN_PAGE (plugin_page)->window;
  if (window == NULL) {
    LEAVE("no window");
    return;
  }

  reg = gnc_ledger_display_get_split_register (priv->ledger);
  split = gnc_split_register_get_current_split (reg);
  if (split == NULL) {
    LEAVE("no split (1)");
    return;
  }

  account = xaccSplitGetAccount (split);
  if (account == NULL) {
    LEAVE("no account");
    return;
  }

  leader = gnc_ledger_display_leader (priv->ledger);
  if (account == leader)
  {
    split = xaccSplitGetOtherSplit (split);
    if (split == NULL) {
      LEAVE("no split (2)");
      return;
    }

    account = xaccSplitGetAccount (split);
    if (account == NULL) {
      LEAVE("no account (2)");
      return;
    }

    if (account == leader) {
      LEAVE("register open for account");
      return;
    }
  }

  new_page = gnc_plugin_page_register_new (account, FALSE);
  if (new_page == NULL) {
    LEAVE("couldn't create new page");
    return;
  }
  new_reg_page = GNC_PLUGIN_PAGE_REGISTER(new_page);

  /* DRH - This will be a problem when someone attempts to use the
     'Jump' command from the scheduled transaction window. */
  gnc_main_window_open_page (GNC_MAIN_WINDOW(window), new_page);
  gsr = gnc_plugin_page_register_get_gsr (new_page);
  gnc_split_reg_jump_to_split(gsr, split);
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_schedule (GtkAction *action,
				       GncPluginPageRegister *plugin_page)
{
  GncPluginPageRegisterPrivate *priv;

  ENTER("(action %p, plugin_page %p)", action, plugin_page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  priv = plugin_page->priv;
  gsr_default_schedule_handler(priv->gsr, NULL);
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_account_report (GtkAction *action,
					     GncPluginPageRegister *plugin_page)
{
  GncMainWindow *window;
  int id;

  ENTER("(action %p, plugin_page %p)", action, plugin_page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  window = GNC_MAIN_WINDOW(GNC_PLUGIN_PAGE(plugin_page)->window);
  id = report_helper (plugin_page->priv->ledger, NULL, NULL);
  if (id >= 0)
    gnc_main_window_open_report(id, window);
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_transaction_report (GtkAction *action,
						 GncPluginPageRegister *plugin_page)
{
  GncPluginPageRegisterPrivate *priv;
  GncMainWindow *window;
  SplitRegister *reg;
  Split *split;
  Query *query;
  int id;


  ENTER("(action %p, plugin_page %p)", action, plugin_page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  priv = plugin_page->priv;
  reg = gnc_ledger_display_get_split_register (priv->ledger);

  split = gnc_split_register_get_current_split (reg);
  if (!split)
    return;

  query = xaccMallocQuery ();

  xaccQuerySetBook (query, gnc_get_current_book ());

  xaccQueryAddGUIDMatch (query, xaccSplitGetGUID (split),
                         GNC_ID_SPLIT, QUERY_AND);

  window = GNC_MAIN_WINDOW(GNC_PLUGIN_PAGE(plugin_page)->window);
  id = report_helper (priv->ledger, split, query);
  if (id >= 0)
    gnc_main_window_open_report(id, window);
  LEAVE(" ");
}

/************************************************************/
/*                    Auxiliary functions                   */
/************************************************************/

void
gnc_plugin_page_register_set_options (GncPluginPage *plugin_page,
				      const char *lines_opt_section,
				      const char *lines_opt_name,
				      gint lines_default,
				      gint disallowCaps)
{
	GncPluginPageRegister *page;
	GncPluginPageRegisterPrivate *priv;

	g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

	page = GNC_PLUGIN_PAGE_REGISTER (plugin_page);
	priv = page->priv;
	priv->lines_opt_section = lines_opt_section;
	priv->lines_opt_name 	= lines_opt_name;
	priv->lines_default  	= lines_default;
	priv->disallowCaps   	= disallowCaps;
}

void
gnc_plugin_page_register_set_ui_description (GncPluginPage *plugin_page,
					     const char *ui_filename)
{
	GncPluginPageRegister *page;
	GncPluginPageRegisterPrivate *priv;

	g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

	page = GNC_PLUGIN_PAGE_REGISTER (plugin_page);
	priv = page->priv;

	g_free(priv->ui_description);
	priv->ui_description = g_strdup(ui_filename);
}

GNCSplitReg *
gnc_plugin_page_register_get_gsr (GncPluginPage *plugin_page)
{
	GncPluginPageRegister *page;
	GncPluginPageRegisterPrivate *priv;

	g_return_val_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page), NULL);

	page = GNC_PLUGIN_PAGE_REGISTER (plugin_page);
	priv = page->priv;

	return priv->gsr;
}

static void
gnc_plugin_page_help_changed_cb (GNCSplitReg *gsr, GncPluginPageRegister *register_page)
{
	GncPluginPageRegisterPrivate *priv;
	SplitRegister *reg;
	GncWindow *window;
	char *help;

	g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(register_page));

	window = GNC_WINDOW(GNC_PLUGIN_PAGE(register_page)->window);
	if (!window) {
	  // This routine can be called before the page is added to a
	  // window.
	  return;
	}

	/* Get the text from the ledger */
	priv = register_page->priv;
	reg = gnc_ledger_display_get_split_register(priv->ledger);
	help = gnc_table_get_help(reg->table);
	gnc_window_set_status(window, GNC_PLUGIN_PAGE(register_page), help);
	g_free(help);
}

static void
gnc_plugin_page_register_refresh_cb (GHashTable *changes, gpointer user_data)
{
  GncPluginPageRegister *page = user_data;

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(page));

  /* We're only looking for forced updates here. */
  if (changes)
    return;

  gnucash_register_refresh_from_gconf(page->priv->gsr->reg);
  gtk_widget_queue_draw(page->priv->widget);
}
