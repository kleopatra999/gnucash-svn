/* 
 * gnc-main-window.c -- GtkWindow which represents the
 *	GnuCash main window.
 *
 * Copyright (C) 2003 Jan Arne Petersen <jpetersen@uni-bonn.de>
 * Copyright (C) 2003,2005 David Hampton <hampton@employees.org>
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

#include <gdk/gdkpixbuf.h>
#include <gtk/gtk.h>

#include "gnc-main-window.h"

#include "dialog-budget-list.h"
#include "gnc-menu-extensions.h"

#include "dialog-fincalc.h"
#include "dialog-find-transactions.h"
#include "dialog-options.h"
#include "dialog-scheduledxaction.h"
#include "dialog-sxsincelast.h"
#include "dialog-transfer.h"
#include "druid-acct-period.h"
#include "druid-loan.h"
#include "gnc-component-manager.h"
#include "gnc-engine-util.h"
#include "gnc-gnome-utils.h"
#include "gnc-file.h"
#include "gnc-gui-query.h"
#include "gnc-plugin.h"
#include "gnc-plugin-manager.h"
#include "gnc-session.h"
#include "dialog-totd.h"
#include "gnc-ui.h"
#include "gnc-version.h"
#include "gnc-window.h"
#include "messages.h"
#include "druid-merge.h"
#include "dialog-chart-export.h"
#include "gnc-gconf-utils.h"
// +JSLED
#include "gnc-html.h"

enum {
  PAGE_ADDED,
  PAGE_CHANGED,
  LAST_SIGNAL
};


#define DESKTOP_GNOME_INTERFACE "/desktop/gnome/interface"
#define TOOLBAR_STYLE "/desktop/gnome/interface/toolbar_style"

/** Static Globals *******************************************************/
static short module = MOD_GUI;
static GList *active_windows = NULL;

/** Declarations *********************************************************/
static void gnc_main_window_class_init (GncMainWindowClass *klass);
static void gnc_main_window_init (GncMainWindow *window);
static void gnc_main_window_finalize (GObject *object);
static void gnc_main_window_dispose (GObject *object);

static void gnc_main_window_setup_window (GncMainWindow *window);
static void gnc_window_main_window_init (GncWindowIface *iface);

/* Callbacks */
static void gnc_main_window_add_widget (GtkUIManager *merge, GtkWidget *widget, GncMainWindow *window);
static void gnc_main_window_change_current_page (GtkNotebook *notebook, gint pos, GncMainWindow *window);
static void gnc_main_window_switch_page (GtkNotebook *notebook, GtkNotebookPage *notebook_page, gint pos, GncMainWindow *window);
static void gnc_main_window_plugin_added (GncPlugin *manager, GncPlugin *plugin, GncMainWindow *window);
static void gnc_main_window_plugin_removed (GncPlugin *manager, GncPlugin *plugin, GncMainWindow *window);

/* Command callbacks */
static void gnc_main_window_cmd_file_new (GtkAction *action, GncMainWindow *window);
static void gnc_main_window_cmd_file_open (GtkAction *action, GncMainWindow *window);
static void gnc_main_window_cmd_file_open_new_window (GtkAction *action, GncMainWindow *window);
static void gnc_main_window_cmd_file_save (GtkAction *action, GncMainWindow *window);
static void gnc_main_window_cmd_file_save_as (GtkAction *action, GncMainWindow *window);
static void gnc_main_window_cmd_file_qsf_import (GtkAction *action, GncMainWindow *window);
static void gnc_main_window_cmd_file_export_accounts (GtkAction *action, GncMainWindow *window);
static void gnc_main_window_cmd_chart_export (GtkAction *action, GncMainWindow *window);
static void gnc_main_window_cmd_file_print (GtkAction *action, GncMainWindow *window);
static void gnc_main_window_cmd_file_properties (GtkAction *action, GncMainWindow *window);
static void gnc_main_window_cmd_file_close (GtkAction *action, GncMainWindow *window);
static void gnc_main_window_cmd_file_quit (GtkAction *action, GncMainWindow *window);
static void gnc_main_window_cmd_edit_cut (GtkAction *action, GncMainWindow *window);
static void gnc_main_window_cmd_edit_copy (GtkAction *action, GncMainWindow *window);
static void gnc_main_window_cmd_edit_paste (GtkAction *action, GncMainWindow *window);
static void gnc_main_window_cmd_edit_preferences (GtkAction *action, GncMainWindow *window);
static void gnc_main_window_cmd_edit_tax_options (GtkAction *action, GncMainWindow *window);
static void gnc_main_window_cmd_view_refresh (GtkAction *action, GncMainWindow *window);
static void gnc_main_window_cmd_view_toolbar (GtkAction *action, GncMainWindow *window);
static void gnc_main_window_cmd_view_summary (GtkAction *action, GncMainWindow *window);
static void gnc_main_window_cmd_view_statusbar (GtkAction *action, GncMainWindow *window);
static void gnc_main_window_cmd_actions_scheduled_transaction_editor (GtkAction *action, GncMainWindow *window);
static void gnc_main_window_cmd_actions_budget_workbench(GtkAction *action, GncMainWindow *window);
static void gnc_main_window_cmd_actions_since_last_run (GtkAction *action, GncMainWindow *window);
static void gnc_main_window_cmd_actions_mortgage_loan (GtkAction *action, GncMainWindow *window);
static void gnc_main_window_cmd_actions_close_books (GtkAction *action, GncMainWindow *window);
static void gnc_main_window_cmd_tools_price_editor (GtkAction *action, GncMainWindow *window);
static void gnc_main_window_cmd_tools_commodity_editor (GtkAction *action, GncMainWindow *window);
static void gnc_main_window_cmd_tools_financial_calculator (GtkAction *action, GncMainWindow *window);
static void gnc_main_window_cmd_tools_find_transactions (GtkAction *action, GncMainWindow *window);
static void gnc_main_window_cmd_help_tutorial (GtkAction *action, GncMainWindow *window);
static void gnc_main_window_cmd_help_totd (GtkAction *action, GncMainWindow *window);
static void gnc_main_window_cmd_help_contents (GtkAction *action, GncMainWindow *window);
static void gnc_main_window_cmd_help_about (GtkAction *action, GncMainWindow *window);

static void gnc_main_window_cmd_test( GtkAction *action, GncMainWindow *window );

struct GncMainWindowPrivate
{
	GtkWidget *menu_dock;
	GtkWidget *toolbar_dock;
	GtkWidget *notebook;
	GtkWidget *statusbar;
	GtkWidget *progressbar;

	GtkWidget *summarybar_dock;
	gboolean   show_summarybar;

	GtkActionGroup *action_group;

	GncPluginPage *current_page;
	GList *installed_pages;
	gint event_handler_id;


	GHashTable *merged_actions_table;
};

typedef struct {
	guint merge_id;
	GtkActionGroup *action_group;
} MergedActionEntry;

static guint main_window_signals[LAST_SIGNAL] = { 0 };

static GtkActionEntry gnc_menu_entries [] =
{
	/* Toplevel */
	{ "FileAction", NULL, N_("_File"), NULL, NULL, NULL, },
	{ "EditAction", NULL, N_("_Edit"), NULL, NULL, NULL },
	{ "ViewAction", NULL, N_("_View"), NULL, NULL, NULL },
	{ "ActionsAction", NULL, N_("_Actions"), NULL, NULL, NULL },
	{ "ToolsAction", NULL, N_("_Tools"), NULL, NULL, NULL },
	{ "ExtensionsAction", NULL, N_("E_xtensions"), NULL, NULL, NULL },
	{ "HelpAction", NULL, N_("_Help"), NULL, NULL, NULL },
	{ "MiscAction", NULL, N_("_Misc"), NULL, NULL, NULL },

	/* File menu */
	{ "FileNewAction", GTK_STOCK_NEW, N_("_New File"), "<control>n",
	  N_("Create a new file"),
	  G_CALLBACK (gnc_main_window_cmd_file_new) },
	{ "FileOpenAction", GTK_STOCK_OPEN, N_("_Open"), NULL,
	  NULL,
	  G_CALLBACK (gnc_main_window_cmd_file_open) },
	{ "FileOpenNewWindowAction", NULL, N_("Open in a New Window"), NULL,
	  N_("Open a new top-level GnuCash window for the current view"),
	  G_CALLBACK (gnc_main_window_cmd_file_open_new_window) },
	{ "FileSaveAction", GTK_STOCK_SAVE, N_("_Save"), "<control>s",
	  NULL,
	  G_CALLBACK (gnc_main_window_cmd_file_save) },
	{ "FileSaveAsAction", GTK_STOCK_SAVE_AS, N_("Save _As..."), "<shift><control>s",
	  NULL,
	  G_CALLBACK (gnc_main_window_cmd_file_save_as) },
	{ "FileImportAction", NULL, N_("_Import"), NULL, NULL, NULL },
	{ "FileImportQSFAction", GTK_STOCK_CONVERT,
		N_("_QSF Import"), NULL,
		N_("Import a QSF object file"),
		G_CALLBACK (gnc_main_window_cmd_file_qsf_import) },
	{ "FileExportAction", NULL, N_("_Export"), NULL, NULL, NULL },
	{ "FileExportAccountsAction", GTK_STOCK_CONVERT,
		N_("Export _Accounts"), NULL,
		N_("Export the account hierarchy to a new file"),
		G_CALLBACK (gnc_main_window_cmd_file_export_accounts) },
	{ "FileExportChartAction", GTK_STOCK_CONVERT,
		N_("Export _Chart of Accounts"), NULL,
		N_("Export the chart of accounts for a date with balances"),
		G_CALLBACK (gnc_main_window_cmd_chart_export) },
	{ "FilePrintAction", GTK_STOCK_PRINT, N_("_Print..."), "<control>p",
	  NULL,
	  G_CALLBACK (gnc_main_window_cmd_file_print) },
	{ "FilePropertiesAction", GTK_STOCK_PROPERTIES, N_("Properties..."), NULL,
	  NULL,
	  G_CALLBACK (gnc_main_window_cmd_file_properties) },
	{ "FileCloseAction", GTK_STOCK_CLOSE, N_("_Close"), "<control>w",
	  NULL,
	  G_CALLBACK (gnc_main_window_cmd_file_close) },
	{ "FileQuitAction", GTK_STOCK_QUIT, N_("_Quit"), NULL,
	  NULL,
	  G_CALLBACK (gnc_main_window_cmd_file_quit) },

	/* Edit menu */
	{ "EditCutAction", GTK_STOCK_CUT, N_("Cu_t"), "<control>x",
	  NULL,
	  G_CALLBACK (gnc_main_window_cmd_edit_cut) },
	{ "EditCopyAction", GTK_STOCK_COPY, N_("_Copy"), "<control>c",
	  NULL,
	  G_CALLBACK (gnc_main_window_cmd_edit_copy) },
	{ "EditPasteAction", GTK_STOCK_PASTE, N_("_Paste"), "<control>v",
	  NULL,
	  G_CALLBACK (gnc_main_window_cmd_edit_paste) },
	{ "EditPreferencesAction", GTK_STOCK_PREFERENCES, N_("_Preferences"), NULL,
	  NULL,
	  G_CALLBACK (gnc_main_window_cmd_edit_preferences) },
	{ "EditTaxOptionsAction", NULL, N_("Ta_x Options"), NULL,
	  N_("Setup tax information for all income and expense accounts"),
	  G_CALLBACK (gnc_main_window_cmd_edit_tax_options) },

	/* View menu */
	{ "ViewRefreshAction", GTK_STOCK_REFRESH, N_("_Refresh"), "<control>r",
	  N_("Refresh this window"),
	  G_CALLBACK (gnc_main_window_cmd_view_refresh) },

	/* Actions menu */
	{ "ActionsScheduledTransactionsAction", NULL, N_("_Scheduled Transactions"), NULL, NULL, NULL },
	{ "ActionsScheduledTransactionEditorAction", NULL, N_("_Scheduled Transaction Editor"), NULL,
	  N_("The list of Scheduled Transactions"),
	  G_CALLBACK (gnc_main_window_cmd_actions_scheduled_transaction_editor) },
	{ "ActionsSinceLastRunAction", NULL, N_("_Since Last Run..."), NULL,
	  N_("Create Scheduled Transactions since the last time run"),
	  G_CALLBACK (gnc_main_window_cmd_actions_since_last_run) },
	{ "ActionsMortgageLoanAction", NULL, N_("_Mortgage & Loan Repayment..."), NULL,
	  N_("Setup scheduled transactions for repayment of a loan"),
	  G_CALLBACK (gnc_main_window_cmd_actions_mortgage_loan) },
	{ "ActionsBudgetWorkbenchAction", NULL, N_("_Budget Workbench (Experimental)"), NULL, 
      N_("Create, Manage, and Monitor Budgets." ),
      G_CALLBACK(gnc_main_window_cmd_actions_budget_workbench) },
	{ "ActionsCloseBooksAction", NULL, N_("Close Books"), NULL,
	  N_("Archive old data using accounting periods"),
	  G_CALLBACK (gnc_main_window_cmd_actions_close_books) },

	/* Tools menu */
	{ "ToolsPriceEditorAction", NULL, N_("_Price Editor"), NULL,
	  N_("View and edit the prices for stocks and mutual funds"),
	  G_CALLBACK (gnc_main_window_cmd_tools_price_editor) },
	{ "ToolsCommodityEditorAction", NULL, N_("Commodity _Editor"), NULL,
	  N_("View and edit the commodities for stocks and mutual funds"),
	  G_CALLBACK (gnc_main_window_cmd_tools_commodity_editor) },
	{ "ToolsFinancialCalculatorAction", NULL, N_("Financial _Calculator"), NULL,
	  N_("Use the financial calculator"),
	  G_CALLBACK (gnc_main_window_cmd_tools_financial_calculator) },
	{ "ToolsFindTransactionsAction", GTK_STOCK_FIND, N_("_Find Transactions"), "<control>f",
	  N_("Find transactions with a search"),
	  G_CALLBACK (gnc_main_window_cmd_tools_find_transactions) },

	/* Extensions menu */

	/* Help menu */
	{ "HelpTutorialAction", GNOME_STOCK_BOOK_BLUE, N_("Tutorial and Concepts _Guide"), NULL,
	  N_("Open the GnuCash Tutorial"),
	  G_CALLBACK (gnc_main_window_cmd_help_tutorial) },
	{ "HelpTipsOfTheDayAction", NULL, N_("_Tips Of The Day"), NULL,
	  N_("View the Tips of the Day"),
	  G_CALLBACK (gnc_main_window_cmd_help_totd) },
	{ "HelpContentsAction", GTK_STOCK_HELP, N_("_Contents"), NULL,
	  N_("Open the GnuCash Help"),
	  G_CALLBACK (gnc_main_window_cmd_help_contents) },
	{ "HelpAboutAction", GNOME_STOCK_ABOUT, N_("_About"), NULL,
	  NULL,
	  G_CALLBACK (gnc_main_window_cmd_help_about) },

        /* Misc menu */
        { "MiscTestAction", NULL, N_("TEST"), NULL,
          N_("Testing stuff"), G_CALLBACK (gnc_main_window_cmd_test) },

};
static guint gnc_menu_n_entries = G_N_ELEMENTS (gnc_menu_entries);

static GtkToggleActionEntry toggle_entries [] =
{
	{ "ViewToolbarAction", NULL, N_("_Toolbar"), "<shift><control>t",
	  N_("Show/hide the toolbar on this window"),
	  G_CALLBACK (gnc_main_window_cmd_view_toolbar), TRUE },
	{ "ViewSummaryAction", NULL, N_("S_ummary Bar"), NULL,
	  N_("Show/hide the summary bar on this window"),
	  G_CALLBACK (gnc_main_window_cmd_view_summary), TRUE },
	{ "ViewStatusbarAction", NULL, N_("_Status Bar"), NULL,
	  N_("Show/hide the status bar on this window"),
	  G_CALLBACK (gnc_main_window_cmd_view_statusbar), TRUE },
};
static guint n_toggle_entries = G_N_ELEMENTS (toggle_entries);

static GObjectClass *parent_class = NULL;

static GQuark window_type = 0;

/************************************************************
 *                                                          *
 ************************************************************/

static void
gnc_main_window_save_window (GncMainWindow *window, gpointer session)
{
  DEBUG("window %p", window);
}

static void
gnc_main_window_shutdown (gpointer session, gpointer user_data)
{
  DEBUG("session %p (%s)", session, qof_session_get_url (session));
  g_list_foreach (active_windows, (GFunc)gnc_main_window_save_window, session);
}


/** Look through the list of pages installed in this window and see if
 *  the specified page is there.
 *
 *  @param page The page to search for.
 *
 *  @return TRUE if the page is present in the window, FALSE otherwise.
 */
static gboolean
gnc_main_window_page_exists (GncPluginPage *page)
{
	GncMainWindow *window;
	GList *walker;

	for (walker = active_windows; walker; walker = g_list_next(walker)) {
	  window = walker->data;
	  if (g_list_find(window->priv->installed_pages, page)) {
	    return TRUE;
	  }
	}
	return FALSE;
}

/** This function handles any event notifications from the engine.
 *  The only event it currently cares about is the deletion of a book.
 *  When a book is deleted, it runs through all installed pages
 *  looking for pages that reference the just (about to be?) deleted
 *  book.  It closes any page it finds so there are no dangling
 *  references to the book.
 *
 *  @param entity     The guid the item being added, deleted, etc.
 *
 * @param type        The type of the item being added, deleted, etc. This
 *                    function only cares about a type of GNC_ID_BOOK.
 *
 * @param event_type  The type of the event.  This function only cares
 *                    about an event type of GNC_EVENT_DESTROY.
 *
 * @param user_data   A pointer to the window data structure.
 */
static void
gnc_main_window_event_handler (GUID *entity, QofIdType type,
			       GNCEngineEventType event_type,
			       gpointer user_data)
{
	GncMainWindow *window;
	GncPluginPage *page;
	GList *item, *next;

	/* hard failures */
	g_return_if_fail(GNC_IS_MAIN_WINDOW(user_data));

	/* soft failures */
	if (safe_strcmp(type, GNC_ID_BOOK) != 0)
	  return;
	if (event_type !=  GNC_EVENT_DESTROY)
	  return;

	ENTER("entity %p of type %s, event %d, window %p",
	      entity, type, event_type, user_data);
	window = GNC_MAIN_WINDOW(user_data);

	for (item = window->priv->installed_pages; item; item = next) {
	  next = g_list_next(item);
	  page = GNC_PLUGIN_PAGE(item->data);
	  if (!gnc_plugin_page_has_book (page, entity))
	    continue;
	  gnc_main_window_close_page (window, page);
	}
	LEAVE(" ");
}

/************************************************************
 *                                                          *
 ************************************************************/

GType
gnc_main_window_get_type (void)
{
	static GType gnc_main_window_type = 0;

	if (gnc_main_window_type == 0) {
		static const GTypeInfo our_info = {
			sizeof (GncMainWindowClass),
			NULL,
			NULL,
			(GClassInitFunc) gnc_main_window_class_init,
			NULL,
			NULL,
			sizeof (GncMainWindow),
			0,
			(GInstanceInitFunc) gnc_main_window_init
		};

		static const GInterfaceInfo plugin_info = {
		  (GInterfaceInitFunc) gnc_window_main_window_init,
		  NULL,
		  NULL
		};

		gnc_main_window_type = g_type_register_static (GTK_TYPE_WINDOW,
							       "GncMainWindow",
							       &our_info, 0);
		g_type_add_interface_static (gnc_main_window_type,
					     GNC_TYPE_WINDOW,
					     &plugin_info);
	}

	return gnc_main_window_type;
}

GncMainWindow *
gnc_main_window_new (void)
{
	GncMainWindow *window;

	window = g_object_new (GNC_TYPE_MAIN_WINDOW, NULL);
	active_windows = g_list_append (active_windows, window);
	return window;
}

void
gnc_main_window_open_page (GncMainWindow *window,
			   GncPluginPage *page)
{
	GtkWidget *label_box;
	GtkWidget *label;
	const gchar *icon;
	GtkWidget *image;
	GtkNotebook *notebook;
	gint page_num;

	if (window)
	  g_return_if_fail (GNC_IS_MAIN_WINDOW (window));
	g_return_if_fail (GNC_IS_PLUGIN_PAGE (page));
	g_return_if_fail (gnc_plugin_page_has_books(page));

	if (gnc_main_window_page_exists(page)) {
	  window = GNC_MAIN_WINDOW (page->window);
	  notebook = GTK_NOTEBOOK (window->priv->notebook);
	  page_num = gtk_notebook_page_num(notebook, page->notebook_page);
	  gtk_notebook_set_current_page (notebook, page_num);
	  gtk_window_present(GTK_WINDOW(window));
	  return;
	}
	
	if ((window == NULL) && active_windows)
	  window = active_windows->data;
	page->window = GTK_WIDGET(window);
	page->notebook_page = gnc_plugin_page_create_widget (page);
	g_object_set_data (G_OBJECT (page->notebook_page),
			   PLUGIN_PAGE_LABEL, page);

	icon = GNC_PLUGIN_PAGE_GET_CLASS(page)->tab_icon;
	label = gtk_label_new (page->tab_name);
	gtk_widget_show (label);

	if (icon != NULL) {
		/* FIXME */
		label_box = gtk_hbox_new (FALSE, 6);
		gtk_widget_show (label_box);
		image = gtk_image_new_from_stock (icon, GTK_ICON_SIZE_MENU);
		gtk_widget_show (image);
		gtk_box_pack_start (GTK_BOX (label_box), image, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (label_box), label, TRUE, TRUE, 0);
	} else {
		label_box = label;
	}
	
	notebook = GTK_NOTEBOOK (window->priv->notebook);
	gtk_notebook_append_page (notebook, page->notebook_page, label_box);
	gnc_plugin_page_inserted (page);
	gtk_notebook_set_current_page (notebook, -1);

	window->priv->installed_pages =
	  g_list_append (window->priv->installed_pages, page);

	g_signal_emit (window, main_window_signals[PAGE_ADDED], 0, page);
}

void
gnc_main_window_close_page (GncMainWindow *window,
			    GncPluginPage *page)
{
	GtkNotebook *notebook;
	gint page_num;

	if (!page->notebook_page)
		return;

	if (window->priv->current_page == page) {
		gnc_plugin_page_unmerge_actions (page, window->ui_merge);
		gnc_plugin_page_unselected (page);

		window->priv->current_page = NULL;
	}

	notebook = GTK_NOTEBOOK (window->priv->notebook);
	page_num =  gtk_notebook_page_num(notebook, page->notebook_page);
	gtk_notebook_remove_page (notebook, page_num);
	if ( gtk_notebook_get_current_page(notebook) == -1) {
	  /* Synthesize a page changed signal when the last page is removed. */
	  g_signal_emit (window, main_window_signals[PAGE_CHANGED], 0, NULL);
	}
	window->priv->installed_pages =
	  g_list_remove (window->priv->installed_pages, page);

	gnc_plugin_page_removed (page);

	gtk_ui_manager_ensure_update (window->ui_merge);
	gnc_window_set_status (GNC_WINDOW(window), page, NULL);

	gnc_plugin_page_destroy_widget (page);
	g_object_unref(page);

	/* If this isn't the last window, go ahead and destroy it. */
	if (window->priv->installed_pages == NULL) {
		if (g_list_length(active_windows) > 1) {
			gtk_widget_destroy(GTK_WIDGET(window));
		}
	}
}

GncPluginPage *
gnc_main_window_get_current_page (GncMainWindow *window)
{
	return window->priv->current_page;
}

void
gnc_main_window_merge_actions (GncMainWindow *window,
			       const gchar *group_name,
			       GtkActionEntry *actions,
			       guint n_actions,
			       const gchar *filename,
			       gpointer user_data)
{
	GncMainWindowActionData *data;
	MergedActionEntry *entry;
	GError *error = NULL;
	gchar *pathname;

	g_return_if_fail (GNC_IS_MAIN_WINDOW (window));
	g_return_if_fail (group_name != NULL);
	g_return_if_fail (actions != NULL);
	g_return_if_fail (n_actions > 0);
	g_return_if_fail (filename != NULL);

	data = g_new0 (GncMainWindowActionData, 1);
	data->window = window;
	data->data = user_data;

	pathname = gnc_gnome_locate_ui_file (filename);
	if (pathname == NULL)
	  return;

	entry = g_new0 (MergedActionEntry, 1);
	entry->action_group = gtk_action_group_new (group_name);
	gtk_action_group_add_actions (entry->action_group, actions, n_actions, data);
	gtk_ui_manager_insert_action_group (window->ui_merge, entry->action_group, 0);
	entry->merge_id = gtk_ui_manager_add_ui_from_file (window->ui_merge, pathname, &error);
	g_assert(entry->merge_id || error);
	if (entry->merge_id) {
	  gtk_ui_manager_ensure_update (window->ui_merge);
	  g_hash_table_insert (window->priv->merged_actions_table, g_strdup (group_name), entry);
	} else {
	  g_critical("Failed to load ui file.\n  Filename %s\n  Error %s",
		     filename, error->message);
	  g_error_free(error);
	  g_free(entry);
	}
	g_free(pathname);
}

void
gnc_main_window_unmerge_actions (GncMainWindow *window,
				 const gchar *group_name)
{
	MergedActionEntry *entry;

	g_return_if_fail (GNC_IS_MAIN_WINDOW (window));
	g_return_if_fail (group_name != NULL);

	entry = g_hash_table_lookup (window->priv->merged_actions_table, group_name);

	if (entry == NULL)
		return;

	gtk_ui_manager_remove_action_group (window->ui_merge, entry->action_group);
	gtk_ui_manager_remove_ui (window->ui_merge, entry->merge_id);
	gtk_ui_manager_ensure_update (window->ui_merge);

	g_hash_table_remove (window->priv->merged_actions_table, group_name);
}

void
gnc_main_window_actions_updated (GncMainWindow *window)
{
	GtkActionGroup *force;

	g_return_if_fail (GNC_IS_MAIN_WINDOW (window));

	/* Unfortunately gtk_ui_manager_ensure_update doesn't work
	 * here.  Force a full update by adding and removing an empty
	 * action group.
	 */
	force = gtk_action_group_new("force_update");
	gtk_ui_manager_insert_action_group (window->ui_merge, force, 0);
	gtk_ui_manager_ensure_update (window->ui_merge);
	gtk_ui_manager_remove_action_group (window->ui_merge, force);
	g_object_unref(force);
}

GtkActionGroup *
gnc_main_window_get_action_group  (GncMainWindow *window,
				   const gchar *group_name)
{
	MergedActionEntry *entry;

	g_return_val_if_fail (GNC_IS_MAIN_WINDOW (window), NULL);
	g_return_val_if_fail (group_name != NULL, NULL);

	entry = g_hash_table_lookup (window->priv->merged_actions_table, group_name);

	if (entry == NULL)
		return NULL;

	return entry->action_group;
}


static void
gnc_main_window_class_init (GncMainWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	window_type = g_quark_from_static_string ("gnc-main-window");

	object_class->finalize = gnc_main_window_finalize;
	object_class->dispose = gnc_main_window_dispose;

	main_window_signals[PAGE_ADDED] =
	  g_signal_new ("page_added",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GncMainWindowClass, page_added),
			NULL, NULL,
			g_cclosure_marshal_VOID__OBJECT,
			G_TYPE_NONE, 1,
			G_TYPE_OBJECT);

	main_window_signals[PAGE_CHANGED] =
	  g_signal_new ("page_changed",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GncMainWindowClass, page_changed),
			NULL, NULL,
			g_cclosure_marshal_VOID__OBJECT,
			G_TYPE_NONE, 1,
			G_TYPE_OBJECT);

	qof_session_add_close_hook(gnc_main_window_shutdown, NULL);
}

static void
gnc_main_window_init (GncMainWindow *window)
{
	window->priv = g_new0 (GncMainWindowPrivate, 1);

	window->priv->merged_actions_table =
	  g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

	window->priv->event_handler_id =
	  gnc_engine_register_event_handler(gnc_main_window_event_handler,
					    window);

	gnc_main_window_setup_window (window);
}

static void
gnc_main_window_finalize (GObject *object)
{
	GncMainWindow *window;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNC_IS_MAIN_WINDOW (object));

	window = GNC_MAIN_WINDOW (object);
	active_windows = g_list_remove (active_windows, window);

	g_return_if_fail (window->priv != NULL);

	gnc_gconf_remove_notification(G_OBJECT(window), DESKTOP_GNOME_INTERFACE);

	gnc_engine_unregister_event_handler(window->priv->event_handler_id);
	g_hash_table_destroy (window->priv->merged_actions_table);
	g_free (window->priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gnc_main_window_dispose (GObject *object)
{
	GncMainWindow *window;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNC_IS_MAIN_WINDOW (object));

	window = GNC_MAIN_WINDOW (object);

	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gnc_main_window_add_plugin (gpointer plugin,
			    gpointer window)
{
	g_return_if_fail (GNC_IS_MAIN_WINDOW (window));
	g_return_if_fail (GNC_IS_PLUGIN (plugin));

	gnc_plugin_add_to_window (GNC_PLUGIN (plugin),
				  GNC_MAIN_WINDOW (window),
				  window_type);
}

static void
gnc_main_window_update_toolbar (GncMainWindow *window,
				const gchar *style)
{
	GEnumClass   *enum_class;
	GEnumValue   *enum_value;
	GSList       *list, *tmp;

	ENTER("window %p, style %s", window, style);
	enum_class = g_type_class_ref (GTK_TYPE_TOOLBAR_STYLE);
	enum_value = g_enum_get_value_by_nick (enum_class, style);
	if (!enum_value) {
	  char *pre;

	  pre = index(style, '_');
	  if (pre == NULL)
	    return;
	  *pre = '-';
	  enum_value = g_enum_get_value_by_nick (enum_class, style);
	  if (!enum_value) {
	    return;
	  }
	}

	list = gtk_ui_manager_get_toplevels(window->ui_merge, GTK_UI_MANAGER_TOOLBAR);
	for (tmp = list; tmp; tmp = tmp->next) {
	  gtk_toolbar_set_style(GTK_TOOLBAR(tmp->data), enum_value->value);
	}
	g_slist_free(list);
	LEAVE("");
}

static void
gnc_main_window_gconf_changed (GConfClient *client,
			       guint cnxn_id,
			       GConfEntry *entry,
			       gpointer user_data)
{
	GncMainWindow *window;
	GConfValue *value;
	const gchar *key;

	window = GNC_MAIN_WINDOW(user_data);

	key = gconf_entry_get_key(entry);
	value = gconf_entry_get_value(entry);
	if (!key || !value)
	  return;

	if (strcmp(key, TOOLBAR_STYLE) == 0) {
	  gnc_main_window_update_toolbar(window, gconf_value_get_string(value));
	  return;
	}
}

static void
gnc_main_window_setup_window (GncMainWindow *window)
{
	GncMainWindowPrivate *priv;
	GtkWidget *main_vbox;
	guint merge_id;
	GncPluginManager *manager;
	GList *plugins;
	GError *error = NULL;
	gchar *filename, *style;
	SCM debugging;

	/* Create widgets and add them to the window */
	main_vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (main_vbox);
	gtk_container_add (GTK_CONTAINER (window), main_vbox);

	priv = window->priv;
	priv->menu_dock = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (priv->menu_dock);
	gtk_box_pack_start (GTK_BOX (main_vbox), priv->menu_dock,
			    FALSE, TRUE, 0);

	priv->notebook = gtk_notebook_new ();
	gtk_widget_show (priv->notebook);
	g_signal_connect (G_OBJECT (priv->notebook), "change-current-page",
			  G_CALLBACK (gnc_main_window_change_current_page), window);
	g_signal_connect (G_OBJECT (priv->notebook), "switch-page",
			  G_CALLBACK (gnc_main_window_switch_page), window);
	gtk_box_pack_start (GTK_BOX (main_vbox), priv->notebook,
			    TRUE, TRUE, 0);

	priv->show_summarybar = TRUE;
	priv->summarybar_dock = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (priv->summarybar_dock);
	gtk_box_pack_start (GTK_BOX (main_vbox), priv->summarybar_dock,
			    FALSE, TRUE, 0);

	priv->statusbar = gtk_statusbar_new ();
	gtk_widget_show (priv->statusbar);
	gtk_box_pack_start (GTK_BOX (main_vbox), priv->statusbar,
	                           FALSE, TRUE, 0);
        gtk_statusbar_set_has_resize_grip( GTK_STATUSBAR(priv->statusbar), TRUE );

	priv->progressbar = gtk_progress_bar_new ();
	gtk_widget_show (priv->progressbar);
	gtk_box_pack_start (GTK_BOX (priv->statusbar), priv->progressbar,
			    FALSE, TRUE, 0);

	window->ui_merge = gtk_ui_manager_new ();

	/* Create menu and toolbar information */
	priv->action_group = gtk_action_group_new ("MainWindowActions");
	gtk_action_group_add_actions (priv->action_group, gnc_menu_entries,
				      gnc_menu_n_entries, window);
	gtk_action_group_add_toggle_actions (priv->action_group, 
					     toggle_entries, n_toggle_entries, 
					     window);
	gtk_ui_manager_insert_action_group (window->ui_merge, priv->action_group, 0);

	g_signal_connect (G_OBJECT (window->ui_merge), "add_widget",
			  G_CALLBACK (gnc_main_window_add_widget), window);
	filename = gnc_gnome_locate_ui_file("gnc-main-window-ui.xml");

	/* Can't do much without a ui. */
	g_assert (filename);

	merge_id = gtk_ui_manager_add_ui_from_file (window->ui_merge,
						    filename, &error);
	g_free(filename);
	g_assert(merge_id || error);
	if (merge_id) {
	  gtk_window_add_accel_group (GTK_WINDOW (window),
				      gtk_ui_manager_get_accel_group(window->ui_merge));
	  gtk_ui_manager_ensure_update (window->ui_merge);
	} else {
	  g_critical("Failed to load ui file.\n  Filename %s\n  Error %s",
		     filename, error->message);
	  g_error_free(error);
	  g_assert(merge_id != 0);
	}

	gnc_gconf_add_notification(G_OBJECT(window), DESKTOP_GNOME_INTERFACE,
				   gnc_main_window_gconf_changed);
	style = gnc_gconf_get_string(TOOLBAR_STYLE, NULL, NULL);
	gnc_main_window_update_toolbar(window, style);
	g_free(style);

        /* Testing */
        {
                guint new_merge_id;
                GtkActionGroup *eag;
                GtkActionEntry newEntry[] =
                        {
                          { "BarAction", NULL, N_("_GtkHtml3 test"), "<control>3", NULL, G_CALLBACK (gnc_main_window_cmd_test) }
                        };

                eag = gtk_action_group_new ("MainWindowActions2");

                gtk_action_group_add_actions (eag, newEntry,
                                              G_N_ELEMENTS (newEntry), window);
                gtk_ui_manager_insert_action_group( window->ui_merge, eag, 0 );

                new_merge_id = gtk_ui_manager_new_merge_id( window->ui_merge );

                gtk_ui_manager_add_ui( window->ui_merge, new_merge_id,
                                       // "/menubar/Actions", no
                                       // "/menubar/Actions/ActionsPlaceholder", no
                                       // "/menubar/AdditionalMenusPlaceholder/AReportAction", no 
                                       "/menubar/AdditionalMenusPlaceholder", // winnah!
                                       "BarAction",
                                       "BarAction", GTK_UI_MANAGER_MENUITEM, FALSE );
                gtk_ui_manager_ensure_update( window->ui_merge );
        }

        /* Now update any old-style add-in menus */
        gnc_extensions_menu_setup( GTK_WINDOW(window), WINDOW_NAME_MAIN, window->ui_merge );

	/* Now update the "eXtensions" menu */
	debugging = scm_c_eval_string("(gnc:debugging?)");
	if (debugging == SCM_BOOL_F) {
	  GtkAction*  action;

	  action = gtk_action_group_get_action(priv->action_group,"ExtensionsAction");
	  g_object_set(G_OBJECT(action), "visible", FALSE, NULL);
	}

	/* GncPluginManager stuff */
	manager = gnc_plugin_manager_get ();
	plugins = gnc_plugin_manager_get_plugins (manager);
	g_signal_connect (G_OBJECT (manager), "plugin-added",
			  G_CALLBACK (gnc_main_window_plugin_added), window);
	g_signal_connect (G_OBJECT (manager), "plugin-removed",
			  G_CALLBACK (gnc_main_window_plugin_removed), window);
	g_list_foreach (plugins, gnc_main_window_add_plugin, window);
	g_list_free (plugins);

}

static void
gnc_main_window_add_widget (GtkUIManager *merge,
			    GtkWidget *widget,
			    GncMainWindow *window)
{
	if (GTK_IS_TOOLBAR (widget)) {
		window->priv->toolbar_dock = widget;
	}

	gtk_box_pack_start (GTK_BOX (window->priv->menu_dock), widget, FALSE, FALSE, 0);
	gtk_widget_show (widget);
}

static void
container_remove(GtkWidget *widget, gpointer data)
{
  gtk_container_remove(GTK_CONTAINER(data), widget);
}


static void
gnc_main_window_switch_page_internal (GtkNotebook *notebook,
				      gint pos,
				      GncMainWindow *window)
{
	GtkWidget *child, *summarybar, *summarybar_dock;
	GncPluginPage *page;

	g_return_if_fail (GNC_IS_MAIN_WINDOW (window));

	if (window->priv->current_page != NULL) {
		gnc_plugin_page_unmerge_actions (window->priv->current_page,
						 window->ui_merge);
		gnc_plugin_page_unselected (window->priv->current_page);
	}

	child = gtk_notebook_get_nth_page (notebook, pos);
	page = g_object_get_data (G_OBJECT (child), PLUGIN_PAGE_LABEL);

	window->priv->current_page = page;

	if (page != NULL) {
		/* Update the user interface (e.g. menus and toolbars */
		gnc_plugin_page_merge_actions (page, window->ui_merge);

		/* Remove summarybar for old page */
		summarybar_dock = window->priv->summarybar_dock;
		gtk_container_foreach(GTK_CONTAINER(summarybar_dock),
				      (GtkCallback)container_remove,
				      summarybar_dock);

		/* install new summarybar (if any) */
		summarybar = page->summarybar;
		if (summarybar) {
		  if (GTK_OBJECT_FLOATING(summarybar)) {
		    /* Own this object. This will prevent it from being deleted by
		     * gtk when it is removed from the summarybar. */
		    g_object_ref (summarybar);
		    gtk_object_sink (GTK_OBJECT (summarybar));
		  }

		  if (window->priv->show_summarybar)
		    gtk_widget_show(summarybar);
		  else
		    gtk_widget_hide(summarybar);
		  gtk_box_pack_start(GTK_BOX(summarybar_dock), summarybar,
				     FALSE, TRUE, 0 );
		}

		/* Allow page specific actions */
		gnc_plugin_page_selected (page);
		gnc_window_update_status (GNC_WINDOW(window), page);
	}

	g_signal_emit (window, main_window_signals[PAGE_CHANGED], 0, page);
}

static void
gnc_main_window_switch_page (GtkNotebook *notebook,
			     GtkNotebookPage *notebook_page,
			     gint pos,
			     GncMainWindow *window)
{
	gnc_main_window_switch_page_internal (notebook, pos, window);
}

static void
gnc_main_window_change_current_page (GtkNotebook *notebook,
				     gint pos,
				     GncMainWindow *window)
{
	gnc_main_window_switch_page_internal (notebook, pos, window);
}

static void
gnc_main_window_plugin_added (GncPlugin *manager,
			      GncPlugin *plugin,
			      GncMainWindow *window)
{
	g_return_if_fail (GNC_IS_MAIN_WINDOW (window));
	g_return_if_fail (GNC_IS_PLUGIN (plugin));

	gnc_plugin_add_to_window (plugin, window, window_type);
}

static void
gnc_main_window_plugin_removed (GncPlugin *manager,
				GncPlugin *plugin,
				GncMainWindow *window)
{
	g_return_if_fail (GNC_IS_MAIN_WINDOW (window));
	g_return_if_fail (GNC_IS_PLUGIN (plugin));

	gnc_plugin_remove_from_window (plugin, window, window_type);
}


/* Command callbacks */
static void
gnc_main_window_cmd_file_new (GtkAction *action, GncMainWindow *window)
{
	gnc_file_new ();
	/* FIXME GNOME 2 Port (update the title etc.) */
}

static void
gnc_main_window_cmd_file_open (GtkAction *action, GncMainWindow *window)
{
	gnc_window_set_progressbar_window (GNC_WINDOW(window));
	gnc_file_open ();
	gnc_window_set_progressbar_window (NULL);
	gnc_main_window_update_title (window);
	/* FIXME GNOME 2 Port (update the title etc.) */
}

static void
gnc_main_window_cmd_file_open_new_window (GtkAction *action, GncMainWindow *window)
{
	GncMainWindowPrivate *priv;
	GncMainWindow *new_window;
	GncPluginPage *page;
	GtkNotebook *notebook;
	GtkWidget *tab_widget;
	gint page_num;

	/* Setup */
	priv = window->priv;
	if (priv->current_page == NULL)
		return;
	notebook = GTK_NOTEBOOK (priv->notebook);
	page = priv->current_page;
	tab_widget = gtk_notebook_get_tab_label (notebook, page->notebook_page);

	/* Remove the page from its old window, but hang onto a ref */
	g_object_ref(page);
	g_object_ref(tab_widget);
	g_object_ref(page->notebook_page);
	page_num =  gtk_notebook_page_num(notebook, page->notebook_page);
	gtk_notebook_remove_page (notebook, page_num);
	priv->installed_pages = g_list_remove (priv->installed_pages, page);
	gnc_plugin_page_removed (page);
	gtk_ui_manager_ensure_update (window->ui_merge);
	gnc_window_set_status (GNC_WINDOW(window), page, NULL);

	/* Create the new window */
	new_window = gnc_main_window_new ();
	gtk_widget_show(GTK_WIDGET(new_window));
	priv = new_window->priv;
	notebook = GTK_NOTEBOOK (priv->notebook);

	/* Now add it to the new window */
	page->window = GTK_WIDGET(new_window);
	gtk_notebook_append_page (notebook, page->notebook_page, tab_widget);
	g_object_unref(tab_widget);
	g_object_unref(page->notebook_page);
	gnc_plugin_page_inserted (page);
	gtk_notebook_set_current_page (notebook, -1);
	priv->installed_pages = g_list_append (priv->installed_pages, page);
	g_signal_emit (new_window, main_window_signals[PAGE_ADDED], 0, page);

	g_object_unref(page);
}

static void
gnc_main_window_cmd_file_save (GtkAction *action, GncMainWindow *window)
{
	gnc_window_set_progressbar_window (GNC_WINDOW(window));
	gnc_file_save ();
	gnc_window_set_progressbar_window (NULL);
	/* FIXME GNOME 2 Port (update the title etc.) */
}

static void
gnc_main_window_cmd_file_save_as (GtkAction *action, GncMainWindow *window)
{
	gnc_window_set_progressbar_window (GNC_WINDOW(window));
	gnc_file_save_as ();
	gnc_window_set_progressbar_window (NULL);
	/* FIXME GNOME 2 Port (update the title etc.) */
}

static void
qsf_file_select_ok(GtkWidget *w, GtkFileSelection *fs )
{
	QofSession *qsf_session, *first_session;
	const gchar *filename;
	QofBook *original;

	ENTER (" ");
	gnc_engine_suspend_events();
	filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION (fs));
	gtk_widget_destroy((GtkWidget*) fs);
	first_session = qof_session_get_current_session();
	original = qof_session_get_book(first_session);
	qsf_session = qof_session_new();
	qof_session_begin(qsf_session, filename, TRUE, FALSE);
	qof_session_load(qsf_session, NULL);
	gnc_engine_resume_events();
	gnc_ui_qsf_import_merge_druid(first_session, qsf_session);
	LEAVE (" ");
}

static void
gnc_main_window_cmd_file_qsf_import (GtkAction *action, GncMainWindow *window)
{
	GtkWidget *file_select;

	gnc_window_set_progressbar_window(GNC_WINDOW(window));
	file_select = gtk_file_selection_new("Select the QSF file to import into GnuCash");
	g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (file_select)->ok_button),
		"clicked", G_CALLBACK (qsf_file_select_ok), (gpointer) file_select);
	g_signal_connect_swapped (G_OBJECT (GTK_FILE_SELECTION (file_select)->cancel_button),
		"clicked", G_CALLBACK (gtk_widget_destroy), G_OBJECT (file_select));
	gtk_widget_show (file_select);
	gnc_window_set_progressbar_window(NULL);
}

static void
gnc_main_window_cmd_file_export_accounts (GtkAction *action, GncMainWindow *window)
{
	gnc_window_set_progressbar_window (GNC_WINDOW(window));
	gnc_file_export_file (NULL);
	gnc_window_set_progressbar_window (NULL);
	/* FIXME GNOME 2 Port (update the title etc.) */
	/* gnc_refresh_main_window_info (); */
}

static void
gnc_main_window_cmd_chart_export (GtkAction *action, GncMainWindow *window)
{
	gnc_window_set_progressbar_window (GNC_WINDOW(window));
	gnc_main_window_chart_export();
	gnc_window_set_progressbar_window (NULL);
	/* FIXME GNOME 2 Port (update the title etc.) */
	/* gnc_refresh_main_window_info (); */
}

static void
gnc_main_window_cmd_file_print (GtkAction *action, GncMainWindow *window)
{
}

static void
gnc_main_window_cmd_file_properties (GtkAction *action, GncMainWindow *window)
{
  SCM func = scm_c_eval_string("gnc:main-window-properties-cb");
  if (!SCM_PROCEDUREP (func)) {
      PERR ("not a procedure\n");
      return;
  }
  scm_call_0(func);
}

static void
gnc_main_window_cmd_file_close (GtkAction *action, GncMainWindow *window)
{
	if (window->priv->current_page != NULL) {
		gnc_main_window_close_page (window, window->priv->current_page);
	}
}

static void
gnc_main_window_cmd_file_quit (GtkAction *action, GncMainWindow *window)
{
	gnc_shutdown (0);
}

static void
gnc_main_window_cmd_edit_cut (GtkAction *action, GncMainWindow *window)
{
}

static void
gnc_main_window_cmd_edit_copy (GtkAction *action, GncMainWindow *window)
{
}

static void
gnc_main_window_cmd_edit_paste (GtkAction *action, GncMainWindow *window)
{
}

static void
gnc_main_window_cmd_edit_preferences (GtkAction *action, GncMainWindow *window)
{
	gnc_show_options_dialog ();
}

static void
gnc_main_window_cmd_edit_tax_options (GtkAction *action, GncMainWindow *window)
{
	gnc_tax_info_dialog (GTK_WIDGET (window));
}

static void
gnc_main_window_cmd_view_refresh (GtkAction *action, GncMainWindow *window)
{
}

static void
gnc_main_window_cmd_view_toolbar (GtkAction *action, GncMainWindow *window)
{
	if (gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action))) {
		gtk_widget_show (window->priv->toolbar_dock);
	} else {
		gtk_widget_hide (window->priv->toolbar_dock);
	}
}

static void
gnc_main_window_cmd_view_summary (GtkAction *action, GncMainWindow *window)
{
	if (gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action))) {
		gtk_widget_show (window->priv->summarybar_dock);
	} else {
		gtk_widget_hide (window->priv->summarybar_dock);
	}
}

static void
gnc_main_window_cmd_view_statusbar (GtkAction *action, GncMainWindow *window)
{
	if (gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action))) {
		gtk_widget_show (window->priv->statusbar);
	} else {
		gtk_widget_hide (window->priv->statusbar);
	}
}

static void
gnc_main_window_cmd_actions_scheduled_transaction_editor (GtkAction *action, GncMainWindow *window)
{
	gnc_ui_scheduled_xaction_dialog_create ();
}

static void
gnc_main_window_cmd_actions_since_last_run (GtkAction *action, GncMainWindow *window)
{
	gint ret;
	const char *nothing_to_do_msg =
		_( "There are no Scheduled Transactions to be entered at this time." );
	
	ret = gnc_ui_sxsincelast_dialog_create ();
	if ( ret == 0 ) {
		gnc_info_dialog (GTK_WIDGET(&window->parent), nothing_to_do_msg);
	} else if ( ret < 0 ) {
		gnc_info_dialog (GTK_WIDGET(&window->parent), ngettext
			 /* Translators: %d is the number of transactions. This is a
			    ngettext(3) message. */
			 ("There are no Scheduled Transactions to be entered at this time.\n"
			  "(%d transaction automatically created)",
			  "There are no Scheduled Transactions to be entered at this time.\n"
			  "(%d transactions automatically created)",
			  -(ret)),
			 -(ret));
	} /* else { this else [>0 means dialog was created] intentionally left
	   * blank. } */	       
}

static void
gnc_main_window_cmd_actions_mortgage_loan (GtkAction *action, GncMainWindow *window)
{
	gnc_ui_sx_loan_druid_create ();
}

static void 
gnc_main_window_cmd_actions_budget_workbench(GtkAction *action, GncMainWindow *window)
{
    gnc_budget_list_dialog_create();
}

static void
gnc_main_window_cmd_actions_close_books (GtkAction *action, GncMainWindow *window)
{
	gnc_acct_period_dialog();
}

static void
gnc_main_window_cmd_tools_price_editor (GtkAction *action, GncMainWindow *window)
{
	gnc_prices_dialog (NULL);
}

static void
gnc_main_window_cmd_tools_commodity_editor (GtkAction *action, GncMainWindow *window)
{
	gnc_commodities_dialog (NULL);
}

static void
gnc_main_window_cmd_tools_financial_calculator (GtkAction *action, GncMainWindow *window)
{
	gnc_ui_fincalc_dialog_create();
}

static void
gnc_main_window_cmd_tools_find_transactions (GtkAction *action, GncMainWindow *window)
{
	gnc_ui_find_transactions_dialog_create (NULL);
}

static void
gnc_main_window_cmd_help_tutorial (GtkAction *action, GncMainWindow *window)
{
	gnc_gnome_help (HF_GUIDE, NULL);
}

static void
gnc_main_window_cmd_help_totd (GtkAction *action, GncMainWindow *window)
{
	gnc_totd_dialog(GTK_WINDOW(window), FALSE);
}

static void
gnc_main_window_cmd_help_contents (GtkAction *action, GncMainWindow *window)
{
	gnc_gnome_help (HF_HELP, NULL);
}

static void
gnc_main_window_cmd_test( GtkAction *action, GncMainWindow *window )
{
        GtkWindow *w = GTK_WINDOW(gtk_window_new( GTK_WINDOW_TOPLEVEL ));
        gnc_html *gnchtml = gnc_html_new( w );
        gchar *html = "<html><head><title>testing</title></head><body><h1>testing</h1><h2>testing 2</h2> <p>Tes<br />ting<object classid=\"gnc-guppi-pie\" width=\"300\" height=\"200\">No pie for you!</object></p></body></html>";
        gtk_container_add( GTK_CONTAINER(w), GTK_WIDGET(gnc_html_get_widget(gnchtml)) );

        gnc_html_show_data( gnchtml, html, strlen( html ) );
        
        gtk_widget_show_all( GTK_WIDGET(w) );
}

static void
gnc_main_window_cmd_help_about (GtkAction *action, GncMainWindow *window)
{
	GtkWidget *about;
	const gchar *message = _("The GnuCash personal finance manager.\n"
				 "The GNU way to manage your money!\n"
				 "http://www.gnucash.org/");
	const gchar *copyright = "\xc2\xa9 1998-2002 Linas Vepstas";
	const gchar *authors[] = {
		"Derek Atkins <derek@ihtfp.com>",
		"Rob Browning <rlb@cs.utexas.edu>",
		"Bill Gribble <grib@billgribble.com>",
		"David Hampton <hampton@employees.org>",
		"James LewisMoss <dres@debian.org>",
		"Robert Graham Merkel <rgmerk@mira.net>",
		"Dave Peticolas <dave@krondo.com>",
		"Joshua Sled <jsled@asynchronous.org>",
		"Christian Stimming <stimming@tuhh.de>",
		"Linas Vepstas <linas@linas.org>",
		NULL
	};
	const gchar *documenters[] = {
		NULL
	};
	const gchar *translator_credits = _("translator_credits");
	GdkPixbuf *logo;

	logo = gnc_gnome_get_gdkpixbuf ("appicon.png");

	about = gnome_about_new ("GnuCash", VERSION, copyright, message, authors, documenters,
				 strcmp (translator_credits, "translator_credits") != 0 ? translator_credits : NULL,
				 logo);

	gdk_pixbuf_unref (logo);
	gtk_dialog_run (GTK_DIALOG (about));
}

void
gnc_main_window_update_title (GncMainWindow *window)
{
  const gchar *filename;
  gchar *title;

  filename = gnc_session_get_url (gnc_get_current_session ());

  if (!filename)
    filename = _("<no file>");
  else if (strncmp ("file:", filename, 5) == 0)
    filename += 5;

  title = g_strdup_printf ("Gnucash (%s)", filename);
  gtk_window_set_title (GTK_WINDOW(&window->parent), title);
  g_free(title);
}

static GtkWidget *
gnc_main_window_get_statusbar (GncWindow *window_in)
{
  GncMainWindowPrivate *priv;
  GncMainWindow *window;

  g_return_val_if_fail (GNC_IS_MAIN_WINDOW (window_in), NULL);

  window = GNC_MAIN_WINDOW(window_in);
  priv = window->priv;
  return priv->statusbar;
}

static GtkWidget *
gnc_main_window_get_progressbar (GncWindow *window_in)
{
  GncMainWindowPrivate *priv;
  GncMainWindow *window;

  g_return_val_if_fail (GNC_IS_MAIN_WINDOW (window_in), NULL);

  window = GNC_MAIN_WINDOW(window_in);
  priv = window->priv;
  return priv->progressbar;
}

static void
gnc_window_main_window_init (GncWindowIface *iface)
{
	iface->get_statusbar   = gnc_main_window_get_statusbar;
	iface->get_progressbar = gnc_main_window_get_progressbar;
}

void
gnc_main_window_set_progressbar_window (GncMainWindow *window)
{
  GncWindow *gncwin;
  gncwin = GNC_WINDOW(window);
  gnc_window_set_progressbar_window(gncwin);
}

/********************************************************************
 * gnc_shutdown
 * close down gnucash from the C side...
 ********************************************************************/
void
gnc_shutdown (int exit_status)
{
  /*SCM scm_shutdown = gnc_scm_lookup("gnucash bootstrap", "gnc:shutdown");*/
  SCM scm_shutdown = scm_c_eval_string("gnc:shutdown");

  if(scm_procedure_p(scm_shutdown) != SCM_BOOL_F)
  {
    SCM scm_exit_code = scm_long2num(exit_status);    
    scm_call_1(scm_shutdown, scm_exit_code);
  }
  else
  {
    /* Either guile is not running, or for some reason we
       can't find gnc:shutdown. Either way, just exit. */
    g_warning("couldn't find gnc:shutdown -- exiting anyway.");
    exit(exit_status);
  }
}
