/*
 * dialog-search.c -- Search Dialog
 * Copyright (C) 2002 Derek Atkins
 * Author: Derek Atkins <warlord@MIT.EDU>
 */

#include "config.h"

#include <gnome.h>
#include <glib.h>

#include "dialog-utils.h"
#include "window-help.h"
#include "gnc-component-manager.h"
#include "gnc-ui-util.h"
#include "gnc-ui.h"
#include "gnc-gui-query.h"
#include "global-options.h"
#include "gncObject.h"
#include "QueryNew.h"
#include "QueryObject.h"
#include "QueryCore.h"

#include "Transaction.h"	/* for the SPLIT_* and TRANS_* */

#include "dialog-search.h"
#include "search-core-type.h"
#include "search-param.h"

#define DIALOG_SEARCH_CM_CLASS "dialog-search"

typedef enum {
  GNC_SEARCH_MATCH_ALL = 0,
  GNC_SEARCH_MATCH_ANY = 1
} GNCSearchType;

struct _GNCSearchWindow {
  GtkWidget *	dialog;
  GtkWidget *	criteria_table;
  GtkWidget *	result_hbox;

  /* The "results" sub-window widgets */
  GtkWidget *	result_list;
  gpointer	selected_item;

  /* The search_type radio-buttons */
  GtkWidget *	new_rb;
  GtkWidget *	narrow_rb;
  GtkWidget *	add_rb;
  GtkWidget *	del_rb;

  GtkWidget *	active_only_check;

  /* The Select button */
  GtkWidget *	select_button;

  /* The close/cancel buttons */
  GtkWidget *	close_button;
  GtkWidget *	cancel_button;

  /* Callbacks */
  GNCSearchResultCB result_cb;
  GNCSearchNewItemCB new_item_cb;
  GNCSearchCallbackButton *buttons;
  GNCSearchFree	free_cb;
  gpointer		user_data;

  GNCSearchSelectedCB	selected_cb;
  gpointer		select_arg;
  gboolean		allow_clear;

  /* What we're searching for, and how */
  GNCIdTypeConst search_for;
  GNCSearchType	grouping;	/* Match Any, Match All */
  QueryAccess	get_guid;	/* Function to GetGUID from the object */
  int		search_type;	/* New, Narrow, Add, Delete */

  /* Our query status */
  QueryNew *	q;
  QueryNew *	start_q;	/* The query to start from, if any */

  /* The list of criteria */
  GNCSearchParam * last_param;
  GList *	params_list;	/* List of GNCSearchParams */
  GList *	display_list;	/* List of GNCSearchParams for Display */
  gint		num_cols;	/* Number of Display Columns */
  GList *	crit_list;	/* list of crit_data */

  gint		component_id;
};

struct _crit_data {
  GNCSearchParam *	param;
  GNCSearchCoreType *	element;
  GtkWidget *		elemwidget;
  GtkWidget *		container;
  GtkWidget *		button;
  GnomeDialog *		dialog;
};

static void search_clear_criteria (GNCSearchWindow *sw);
static void gnc_search_dialog_display_results (GNCSearchWindow *sw);

static void
gnc_search_callback_button_execute (GNCSearchCallbackButton *cb,
				    GNCSearchWindow *sw)
{
  if (cb->cb_fcn)
    (cb->cb_fcn)(&(sw->selected_item), sw->user_data);
}

static void
gnc_search_dialog_result_clicked (GtkButton *button, GNCSearchWindow *sw)
{
  GNCSearchCallbackButton *cb;

  cb = gtk_object_get_data (GTK_OBJECT (button), "data");
  gnc_search_callback_button_execute (cb, sw);
}

static void
gnc_search_dialog_select_cb (GtkButton *button, GNCSearchWindow *sw)
{
  g_return_if_fail (sw->selected_cb);

  if (sw->selected_item == NULL && sw->allow_clear == FALSE) {
    char *msg = _("You must select an item from the list");
    gnc_error_dialog_parented (GTK_WINDOW (sw->dialog), msg);
    return;
  }

  (sw->selected_cb)(sw->selected_item, sw->select_arg);
  gnc_search_dialog_destroy (sw);
}

static void
gnc_search_dialog_select_row_cb (GtkCList *clist, gint row, gint column,
				 GdkEventButton *event, gpointer user_data)
{
  GNCSearchWindow *sw = user_data;
  sw->selected_item = gtk_clist_get_row_data (clist, row);

  /* If we double-click an item, then either "select" it, or run it
   * through the first button (which should be view/edit
   */
  if (event && event->type == GDK_2BUTTON_PRESS) {
    if (sw->selected_cb)
      /* Select the time */
      gnc_search_dialog_select_cb (NULL, sw);
    else if (sw->buttons)
      /* Call the first button (usually view/edit) */
      gnc_search_callback_button_execute (sw->buttons, sw);

    /* If we get here, then nothing to do for a double-click */
  }
}

static void
gnc_search_dialog_unselect_row_cb (GtkCList *clist, gint row, gint column,
				   GdkEventButton *event, gpointer user_data)
{
  GNCSearchWindow *sw = user_data;
  gpointer item = gtk_clist_get_row_data (clist, row);

  if (sw->selected_item == item)
    sw->selected_item = NULL;
}

static void
gnc_search_dialog_init_result_list (GNCSearchWindow *sw)
{
  GList *col;
  gint i;
  char **titles;

  /* How many columns? */
  sw->num_cols = g_list_length (sw->display_list);
  titles = g_new0 (char*, sw->num_cols);

  for (i = 0, col = sw->display_list; col; col = col->next) {
    GNCSearchParam *param = col->data;
    titles[i] = (char *)param->title;
    i++;
  }

  sw->result_list = gtk_clist_new_with_titles (sw->num_cols, titles);
  g_free (titles);
  gtk_clist_set_selection_mode (GTK_CLIST (sw->result_list),
				GTK_SELECTION_SINGLE);

  /* Setup list properties */
  gtk_clist_set_shadow_type(GTK_CLIST(sw->result_list), GTK_SHADOW_IN);
  gtk_clist_column_titles_passive(GTK_CLIST(sw->result_list));
  
  /* Setup column parameters */
  for (i = 0, col = sw->display_list; col; col = col->next) {
    GNCSearchParam *param = col->data;
    gtk_clist_set_column_justification (GTK_CLIST (sw->result_list), i,
					param->justify);
    gtk_clist_set_column_auto_resize (GTK_CLIST (sw->result_list), i, TRUE);
    i++;
  }

  /* Setup the list callbacks */
  gtk_signal_connect (GTK_OBJECT (sw->result_list), "select-row",
		      gnc_search_dialog_select_row_cb, sw);
  gtk_signal_connect (GTK_OBJECT (sw->result_list), "unselect-row",
		      gnc_search_dialog_unselect_row_cb, sw);
}

static void
gnc_search_dialog_prepend_item (GNCSearchWindow *sw, gpointer object)
{
  const GUID *guid = (const GUID *) ((sw->get_guid)(object));
  GList *col;
  char ** row_text;
  gint row, i;

  row_text = g_new0 (char *, sw->num_cols);

  /* Iterate over each column, and compute the particular text
   * to display for that column.
   */
  for (i = 0, col = sw->display_list; col; col = col->next) {
    GNCSearchParam *param = col->data;
    GSList *converters = gnc_search_param_get_converters (param);
    const char *type = gnc_search_param_get_param_type (param);
    gpointer res = object;
    QueryAccess fcn = NULL;

    /* Do all the object conversions */
    for (; converters; converters = converters->next) {
      fcn = converters->data;

      if (converters->next)
	res = fcn (res);
    }

    /* Now convert this to a text value for the row */
    row_text[i++] = gncQueryCoreToString (type, res, fcn);
  }

  row = gtk_clist_prepend (GTK_CLIST (sw->result_list), row_text);
  gtk_clist_set_row_data (GTK_CLIST (sw->result_list), row, object);
  gtk_clist_set_selectable (GTK_CLIST (sw->result_list), row, TRUE);
  
  /* Free up our strings */
  for (i = 0; i < sw->num_cols; i++) {
    if (row_text[i])
      g_free (row_text[i]);
  }
  g_free (row_text);

  /* Watch this item in case it changes */
  gnc_gui_component_watch_entity (sw->component_id, guid,
				  GNC_EVENT_MODIFY | GNC_EVENT_DESTROY);
}

static void
gnc_search_dialog_display_results (GNCSearchWindow *sw)
{
  GList *list, *node;
  GtkAdjustment *vadjustment;
  gfloat save_value = 0.0;
  gboolean have_list = TRUE;
  gint64 count;
  gdouble max_count;

  /* Check if this is the first time this is called for this window.
   * If so, then build the results sub-window, the scrolled listbox,
   * and the active buttons.
   */
  if (sw->result_list == NULL) {
    GtkWidget *scroller, *button_box, *button;

    have_list = FALSE;

    /* Create the list */
    gnc_search_dialog_init_result_list (sw);

    /* Create the scroller and add the list to the scroller */
    scroller = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroller),
				    GTK_POLICY_AUTOMATIC,
				    GTK_POLICY_AUTOMATIC);
    gtk_widget_set_usize(GTK_WIDGET(scroller), 300, 100);
    gtk_container_add (GTK_CONTAINER (scroller), sw->result_list);

    /* Create the button_box */
    button_box = gtk_vbox_new (FALSE, 3);

    /* ... and add all the buttons */
    if (sw->buttons) {
      int i;

      button = gtk_button_new_with_label (_("Select"));
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  gnc_search_dialog_select_cb, sw);
      gtk_box_pack_start (GTK_BOX (button_box), button, FALSE, FALSE, 3);
      sw->select_button = button;
			  
      for (i = 0; sw->buttons[i].label; i++) {
	button = gtk_button_new_with_label (sw->buttons[i].label);
	gtk_object_set_data (GTK_OBJECT (button), "data", &(sw->buttons[i]));
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    gnc_search_dialog_result_clicked, sw);
	gtk_box_pack_start (GTK_BOX (button_box), button, FALSE, FALSE, 3);
      }
    }

    /* Add the scrolled-list and button-box to the results_box */
    gtk_box_pack_end (GTK_BOX (sw->result_hbox), button_box, FALSE, FALSE, 3);
    gtk_box_pack_end (GTK_BOX (sw->result_hbox), scroller, TRUE, TRUE, 3);

    /* And show the results */
    gtk_widget_show_all (sw->result_hbox);

    /* But maybe hide the select button */
    if (!sw->selected_cb)
      gtk_widget_hide_all (sw->select_button);
  }

  /* Figure out were we were */
  vadjustment = gtk_clist_get_vadjustment (GTK_CLIST (sw->result_list));
  if (vadjustment)
    save_value = vadjustment->value;

  /* Clear out all the items, to insert in a moment */
  gtk_clist_freeze (GTK_CLIST (sw->result_list));
  gtk_clist_clear (GTK_CLIST (sw->result_list));

  /* Clear the watches */
  gnc_gui_component_clear_watches (sw->component_id);

  /* Compute the actual results */
  list = g_list_reverse (gncQueryRun (sw->q));

  /* Add the list of items to the clist */
  for (count = 0, node = list; node; node = node->next, count++) {
    gnc_search_dialog_prepend_item (sw, node->data);
  }

  /* Need to reverse again for internal consistency */
  g_list_reverse (list);

  /* Return to our saved vadjustment */
  if (vadjustment) {
    save_value = CLAMP (save_value, vadjustment->lower,
			vadjustment->upper - vadjustment->page_size);
    gtk_adjustment_set_value (vadjustment, save_value);
  }

  gtk_clist_thaw (GTK_CLIST (sw->result_list));

  /* Figure out what to select */
  {
    gint row = gtk_clist_find_row_from_data (GTK_CLIST (sw->result_list),
					     sw->selected_item);

    if (row < 0)
      row = 0;

    gtk_clist_select_row (GTK_CLIST (sw->result_list), row, 0);

    /* If this row isn't visible, move it to the center */
    if (gtk_clist_row_is_visible (GTK_CLIST (sw->result_list), row) !=
	GTK_VISIBILITY_FULL && have_list)
      gtk_clist_moveto (GTK_CLIST (sw->result_list), row, 0, 0.5, 0);
  }

  /* set 'new search' if fewer than max_count items is returned. */
  max_count = gnc_lookup_number_option ("_+Advanced", "New Search Limit", 0.0);
  if (count < max_count)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (sw->new_rb), TRUE);
}

static void
match_all (GtkWidget *widget, GNCSearchWindow *sw)
{
  sw->grouping = GNC_SEARCH_MATCH_ALL;
}

static void
match_any (GtkWidget *widget, GNCSearchWindow *sw)
{
  sw->grouping = GNC_SEARCH_MATCH_ANY;
}

static void
search_type_cb (GtkToggleButton *button, GNCSearchWindow *sw)
{
  GSList * buttongroup = gtk_radio_button_group (GTK_RADIO_BUTTON (button));

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button))) {
    sw->search_type =
      g_slist_length (buttongroup) - g_slist_index (buttongroup, button) - 1;
  }
}

static void
search_active_only_cb (GtkToggleButton *button, GNCSearchWindow *sw)
{
  gnc_set_boolean_option("__gui", "search_for_active_only",
			 gtk_toggle_button_get_active (button));
}

static void
search_update_query (GNCSearchWindow *sw)
{
  static GSList *active_params = NULL;
  QueryNew *q, *q2, *new_q;
  GList *node;
  QueryOp op;
  QueryPredData_t pdata;

  if (sw->grouping == GNC_SEARCH_MATCH_ANY)
    op = QUERY_OR;
  else
    op = QUERY_AND;

  if (active_params == NULL)
    active_params = g_slist_prepend (NULL, QUERY_PARAM_ACTIVE);

  /* Make sure we supply a book! */
  if (sw->start_q == NULL) {
    sw->start_q = gncQueryCreateFor (sw->search_for);
    gncQuerySetBook (sw->start_q, gnc_get_current_book ());
  } else {
    /* We've got a query -- purge it of any "active" parameters */
    gncQueryPurgeTerms (sw->start_q, active_params);
  }

  /* Now create a new query to work from */
  q = gncQueryCreateFor (sw->search_for);

  /* Walk the list of criteria */
  for (node = sw->crit_list; node; node = node->next) {
    struct _crit_data *data = node->data;

    pdata = gnc_search_core_type_get_predicate (data->element);
    if (pdata)
      gncQueryAddTerm (q, gnc_search_param_get_param_path (data->param),
		       pdata, op);
  }

  /* Now combine this query with the existing query, depending on
   * what we want to do...  We can assume that cases 1, 2, and 3
   * already have sw->q being valid!
   */

  switch (sw->search_type) {
  case 0:			/* New */
    new_q = gncQueryMerge (sw->start_q, q, QUERY_AND);
    gncQueryDestroy (q);
    break;
  case 1:			/* Refine */
    new_q = gncQueryMerge (sw->q, q, QUERY_AND);
    gncQueryDestroy (q);
    break;
  case 2:			/* Add */
    new_q = gncQueryMerge (sw->q, q, QUERY_OR);
    gncQueryDestroy (q);
    break;
  case 3:			/* Delete */
    q2 = gncQueryInvert (q);
    new_q = gncQueryMerge (sw->q, q2, QUERY_AND);
    gncQueryDestroy (q2);
    gncQueryDestroy (q);
    break;
  default:
    g_warning ("bad search type: %d", sw->search_type);
    new_q = q;
    break;
  }

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (sw->active_only_check))) {
    gncQueryAddBooleanMatch (new_q, active_params, TRUE, QUERY_AND);
    active_params = NULL;
  }

  /* Destroy the old query */
  if (sw->q)
    gncQueryDestroy (sw->q);

  /* And save the new one */
  sw->q = new_q;
}

static void
gnc_search_dialog_show_close_cancel (GNCSearchWindow *sw)
{
  if (sw->selected_cb) {
    gtk_widget_show_all (sw->cancel_button);
    gtk_widget_hide_all (sw->close_button);
  } else {
    gtk_widget_hide_all (sw->cancel_button);
    gtk_widget_show_all (sw->close_button);
  }
}

static void
gnc_search_dialog_reset_widgets (GNCSearchWindow *sw)
{
  gboolean sens = (sw->q != NULL);

  gtk_widget_set_sensitive(GTK_WIDGET(sw->narrow_rb), sens);
  gtk_widget_set_sensitive(GTK_WIDGET(sw->add_rb), sens);
  gtk_widget_set_sensitive(GTK_WIDGET(sw->del_rb), sens);

  if (sw->q) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (sw->new_rb), FALSE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (sw->narrow_rb), TRUE);
  }
}

static gboolean
gnc_search_dialog_crit_ok (GNCSearchWindow *sw)
{
  struct _crit_data *data;
  GList *l;
  gboolean ret;

  l = g_list_last (sw->crit_list);
  data = l->data;
  ret = gnc_search_core_type_validate (data->element);

  if (ret)
    sw->last_param = data->param;

  return ret;
}

static void
search_find_cb (GtkButton *button, GNCSearchWindow *sw)
{
  if (!gnc_search_dialog_crit_ok (sw))
    return;

  search_update_query (sw);
  search_clear_criteria (sw);
  gnc_search_dialog_reset_widgets (sw);

  if (sw->result_cb)
    (sw->result_cb)(sw->q, sw->user_data, &(sw->selected_item));
  else
    gnc_search_dialog_display_results (sw);
}

static void
search_new_item_cb (GtkButton *button, GNCSearchWindow *sw)
{
  gpointer res;

  g_return_if_fail (sw->new_item_cb);

  res = (sw->new_item_cb)(sw->user_data);

  if (res) {
    const GUID *guid = (const GUID *) ((sw->get_guid)(res));
    QueryOp op = QUERY_OR;

    if (!sw->q) {
      if (!sw->start_q) {
	sw->start_q = gncQueryCreateFor (sw->search_for);
	gncQuerySetBook (sw->start_q, gnc_get_current_book ());
      }
      sw->q = gncQueryCopy (sw->start_q);
      op = QUERY_AND;
    }

    gncQueryAddGUIDMatch (sw->q, g_slist_prepend (NULL, QUERY_PARAM_GUID),
			  guid, op);

    /* Watch this entity so we'll refresh once it's actually changed */
    gnc_gui_component_watch_entity (sw->component_id, guid, GNC_EVENT_MODIFY);
  }
}

static void
search_cancel_cb (GtkButton *button, GNCSearchWindow *sw)
{
  /* Don't select anything */
  sw->selected_item = NULL;
  gnc_search_dialog_destroy (sw);
}

static void
search_help_cb (GtkButton *button, GNCSearchWindow *sw)
{
  helpWindow (NULL, NULL, HH_FIND_TRANSACTIONS);
}

static void
remove_element (GtkWidget *button, GNCSearchWindow *sw)
{
  GtkWidget *element;
  struct _elem_data *data;

  if (g_list_length (sw->crit_list) < 2)
    return;

  element = gtk_object_get_data (GTK_OBJECT (button), "element");
  data = gtk_object_get_data (GTK_OBJECT (element), "data");

  /* remove the element from the list */
  sw->crit_list = g_list_remove (sw->crit_list, data);

  /* and from the display */
  gtk_container_remove (GTK_CONTAINER (sw->criteria_table), element);
  gtk_container_remove (GTK_CONTAINER (sw->criteria_table), button);
}

static void
attach_element (GtkWidget *element, GNCSearchWindow *sw, int row)
{
  GtkWidget *remove;
  struct _crit_data *data;

  data = gtk_object_get_data (GTK_OBJECT (element), "data");

  gtk_table_attach (GTK_TABLE (sw->criteria_table), element, 0, 1, row, row+1,
		    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  
  remove = gtk_button_new_from_stock (GTK_STOCK_REMOVE);
  gtk_object_set_data (GTK_OBJECT (remove), "element", element);
  gtk_signal_connect (GTK_OBJECT (remove), "clicked", remove_element, sw);
  gtk_table_attach (GTK_TABLE (sw->criteria_table), remove, 1, 2, row, row+1,
		    0, 0, 0, 0);
  gtk_widget_show (remove);
  data->button = remove;	/* Save the button for later */
}

static void
option_activate (GtkMenuItem *item, struct _crit_data *data)
{
  GNCSearchParam *param = gtk_object_get_data (GTK_OBJECT (item), "param");
  GNCSearchCoreType *newelem;

  if (gnc_search_param_type_match (param, data->param)) {
    /* The param type is the same, just save the new param */
    data->param = param;
    return;
  }
  data->param = param;

  /* OK, let's do a widget shuffle, throw away the old widget/element,
   * and create another one here.  No need to change the crit_list --
   * the pointer to data stays the same.
   */
  if (data->elemwidget)
    gtk_container_remove (GTK_CONTAINER (data->container), data->elemwidget);
  gtk_object_destroy (GTK_OBJECT (data->element));

  newelem = gnc_search_core_type_new_type_name
    (gnc_search_param_get_param_type (param));
  data->element = newelem;
  data->elemwidget = gnc_search_core_type_get_widget (newelem);
  if (data->elemwidget) {
    gtk_box_pack_start (GTK_BOX (data->container), data->elemwidget,
			FALSE, FALSE, 0);
  }

  /* Make sure it's visible */
  gtk_widget_show_all (data->container);

  /* Make sure we widen up if necessary */
  gtk_widget_queue_resize (GTK_WIDGET (data->dialog));

  /* And grab focus */
  gnc_search_core_type_grab_focus (newelem);
  gnc_search_core_type_editable_enters (newelem, data->dialog);
}

static void
search_clear_criteria (GNCSearchWindow *sw)
{
  GList *node;

  for (node = sw->crit_list; node; ) {
    GList *tmp = node->next;
    struct _crit_data *data = node->data;
    g_object_ref (GTK_OBJECT(data->button));
    remove_element (data->button, sw);
    node = tmp;
  }
}

static GtkWidget *
get_element_widget (GNCSearchWindow *sw, GNCSearchCoreType *element)
{
  GtkWidget *menu, *item, *omenu, *hbox, *p;
  GList *l;
  struct _crit_data *data;
  int index = 0, current = 0;

  data = g_new0 (struct _crit_data, 1);
  data->element = element;
  data->dialog = GNOME_DIALOG (sw->dialog);

  hbox = gtk_hbox_new (FALSE, 0);
  /* only set to automaticaly clean up the memory */
  gtk_object_set_data_full (GTK_OBJECT (hbox), "data", data, g_free);

  p = gnc_search_core_type_get_widget (element);
  data->elemwidget = p;
  data->container = hbox;
  data->param = sw->last_param;

  menu = gtk_menu_new ();
  for (l = sw->params_list; l; l = l->next) {
    GNCSearchParam *param = l->data;
    item = gtk_menu_item_new_with_label (_(param->title));
    gtk_object_set_data (GTK_OBJECT (item), "param", param);
    gtk_signal_connect (GTK_OBJECT (item), "activate", option_activate, data);
    gtk_menu_append (GTK_MENU (menu), item);
    gtk_widget_show (item);

    if (param == sw->last_param) /* is this the right parameter to start? */
      current = index;

    index++;
  }

  omenu = gtk_option_menu_new ();
  gtk_option_menu_set_menu (GTK_OPTION_MENU (omenu), menu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (omenu), current);
  gtk_widget_show (omenu);

  gtk_box_pack_start (GTK_BOX (hbox), omenu, FALSE, FALSE, 0);
  if (p)
    gtk_box_pack_start (GTK_BOX (hbox), p, FALSE, FALSE, 0);
  gtk_widget_show_all (hbox);

  return hbox;
}

static void
gnc_search_dialog_add_criterion (GNCSearchWindow *sw)
{
  GNCSearchCoreType *new;

  /* First, make sure that the last criterion is ok */
  if (sw->crit_list) {
    if (!gnc_search_dialog_crit_ok (sw))
      return;
  } else
    sw->last_param = sw->params_list->data;

  /* create a new criterion element */

  new = gnc_search_core_type_new_type_name
    (gnc_search_param_get_param_type (sw->last_param));

  if (new) {
    struct _crit_data *data;
    GtkWidget *w;
    int rows;

    w = get_element_widget (sw, new);
    data = gtk_object_get_data (GTK_OBJECT (w), "data");
    sw->crit_list = g_list_append (sw->crit_list, data);

    rows = GTK_TABLE (sw->criteria_table)->nrows;
    gtk_table_resize (GTK_TABLE (sw->criteria_table), rows+1, 2);
    attach_element (w, sw, rows);

    gnc_search_core_type_grab_focus (new);
    gnc_search_core_type_editable_enters (new, GNOME_DIALOG (sw->dialog));
  }
}

static void
add_criterion (GtkWidget *button, GNCSearchWindow *sw)
{
  gnc_search_dialog_add_criterion (sw);
}

static int
gnc_search_dialog_close_cb (GnomeDialog *dialog, GNCSearchWindow *sw)
{
  g_return_val_if_fail (sw, TRUE);

  gnc_unregister_gui_component (sw->component_id);

  /* XXX: Clear the params_list? */
  g_list_free (sw->crit_list);

  /* Destroy the queries */
  if (sw->q) gncQueryDestroy (sw->q);
  if (sw->start_q) gncQueryDestroy (sw->start_q);

  /* Destroy the user_data */
  if (sw->free_cb)
    (sw->free_cb)(sw->user_data);

  /* Destroy and exit */
  g_free (sw);
  return FALSE;
}

static void
refresh_handler (GHashTable *changes, gpointer data)
{
  GNCSearchWindow * sw = data;

  g_return_if_fail (sw);
  gnc_search_dialog_display_results (sw);
}

static void
close_handler (gpointer data)
{
  GNCSearchWindow * sw = data;

  g_return_if_fail (sw);
  gnome_dialog_close (GNOME_DIALOG (sw->dialog));
}

static void
gnc_search_dialog_init_widgets (GNCSearchWindow *sw)
{
  GladeXML *xml;
  GtkWidget *label, *add, *box;
  GtkWidget *menu, *item, *omenu;
  GtkWidget *new_item_button;
  const char * type_label;

  xml = gnc_glade_xml_new ("search.glade", "Search Dialog");

  /* Grab the dialog, save the dialog info */
  sw->dialog = glade_xml_get_widget (xml, "Search Dialog");
  gtk_object_set_data (GTK_OBJECT (sw->dialog), "dialog-info", sw);

  /* grab the result hbox */
  sw->result_hbox = glade_xml_get_widget (xml, "result_hbox");

  /* Grab the search-table widget */
  sw->criteria_table = glade_xml_get_widget (xml, "criteria_table");

  /* Set the type label */
  label = glade_xml_get_widget (xml, "type_label");
  type_label = gncObjectGetTypeLabel (sw->search_for);
  gtk_label_set_text (GTK_LABEL (label), type_label);

  /* Set the 'add criterion' button */
  add = gtk_button_new_from_stock (GTK_STOCK_ADD);
 
  gtk_signal_connect (GTK_OBJECT (add), "clicked", add_criterion, sw);
  box = glade_xml_get_widget (xml, "add_button_box");
  gtk_box_pack_start (GTK_BOX (box), add, FALSE, FALSE, 3);
  
  /* Set the match-type menu */
  menu = gtk_menu_new ();

  item = gtk_menu_item_new_with_label (_("all criteria are met"));
  gtk_signal_connect (GTK_OBJECT (item), "activate", match_all, sw);
  gtk_menu_append (GTK_MENU (menu), item);
  gtk_widget_show (item);
	
  item = gtk_menu_item_new_with_label (_("any criteria are met"));
  gtk_signal_connect (GTK_OBJECT (item), "activate", match_any, sw);
  gtk_menu_append (GTK_MENU (menu), item);
  gtk_widget_show (item);
	
  omenu = gtk_option_menu_new ();
  gtk_option_menu_set_menu (GTK_OPTION_MENU (omenu), menu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (omenu), sw->grouping);

  gtk_widget_show (omenu);
  box = glade_xml_get_widget (xml, "type_menu_box");
  gtk_box_pack_start (GTK_BOX (box), omenu, FALSE, FALSE, 3);

  /* if there's no original query, make the narrow, add, delete 
   * buttons inaccessible */
  sw->new_rb = glade_xml_get_widget (xml, "new_search_radiobutton");
  sw->narrow_rb = glade_xml_get_widget (xml, "narrow_search_radiobutton");
  sw->add_rb = glade_xml_get_widget (xml, "add_search_radiobutton");
  sw->del_rb = glade_xml_get_widget (xml, "delete_search_radiobutton");

  sw->active_only_check = glade_xml_get_widget (xml, "active_only_check");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sw->active_only_check),
				gnc_lookup_boolean_option ("__gui",
							   "search_for_active_only",
							   TRUE));

  /* Figure out if we this object-type has an "active" parameter, and
   * if not, then set the active-check button insensitive
   */
  if (gncQueryObjectGetParameter (sw->search_for, QUERY_PARAM_ACTIVE) == NULL)
    gtk_widget_set_sensitive (sw->active_only_check, FALSE);

  /* Deal with the cancel button */
  sw->cancel_button = glade_xml_get_widget (xml, "cancel_button");
  sw->close_button = glade_xml_get_widget (xml, "close_button");

  /* Deal with the new_item button */
  new_item_button = glade_xml_get_widget (xml, "new_item_button");
  {
    char *desc =
      /* Translators: %s is either "item" or the name of some other
       * item, e.g. "Customer" or "Invoice". */
      g_strdup_printf (_("New %s"), type_label ? type_label : _("item"));
    gtk_label_set_text (GTK_LABEL (GTK_BIN (new_item_button)->child), desc);
    g_free (desc);
  }
  /* add the first criterion */
  gnc_search_dialog_add_criterion (sw);

  /* show it all */
  gtk_widget_show_all (sw->dialog);

  /* Hide the 'new' button if there is no new_item_cb */
  if (!sw->new_item_cb)
    gtk_widget_hide_all (new_item_button);

  /* Connect XML signals */

  glade_xml_signal_connect_data (xml, "gnc_ui_search_type_cb",
				 GTK_SIGNAL_FUNC (search_type_cb), sw);
  
  glade_xml_signal_connect_data (xml, "gnc_ui_search_active_cb",
				 GTK_SIGNAL_FUNC (search_active_only_cb), sw);

  glade_xml_signal_connect_data (xml, "gnc_ui_search_new_cb",
				 GTK_SIGNAL_FUNC (search_new_item_cb), sw);
  
  glade_xml_signal_connect_data (xml, "gnc_ui_search_find_cb",
				 GTK_SIGNAL_FUNC (search_find_cb), sw);
  
  glade_xml_signal_connect_data (xml, "gnc_ui_search_cancel_cb",
				 GTK_SIGNAL_FUNC (search_cancel_cb), sw);

  glade_xml_signal_connect_data (xml, "gnc_ui_search_close_cb",
				 GTK_SIGNAL_FUNC (search_cancel_cb), sw);
  
  glade_xml_signal_connect_data (xml, "gnc_ui_search_help_cb",
				 GTK_SIGNAL_FUNC (search_help_cb), sw);

  /* Register ourselves */
  sw->component_id = gnc_register_gui_component (DIALOG_SEARCH_CM_CLASS,
						 refresh_handler,
						 close_handler, sw);

  /* And setup the close callback */
  gtk_signal_connect (GTK_OBJECT (sw->dialog), "close",
		      GTK_SIGNAL_FUNC (gnc_search_dialog_close_cb), sw);

  gnc_search_dialog_reset_widgets (sw);
  gnc_search_dialog_show_close_cancel (sw);
}

void
gnc_search_dialog_destroy (GNCSearchWindow *sw)
{
  if (!sw) return;
  gnc_close_gui_component (sw->component_id);
}

void
gnc_search_dialog_raise (GNCSearchWindow *sw)
{
  if (!sw) return;
  gtk_window_present (GTK_WINDOW(sw->dialog));
}

GNCSearchWindow *
gnc_search_dialog_create (GNCIdTypeConst obj_type, GList *param_list,
			  GList *display_list,
			  QueryNew *start_query, QueryNew *show_start_query,
			  GNCSearchCallbackButton *callbacks,
			  GNCSearchResultCB result_callback,
			  GNCSearchNewItemCB new_item_cb,
			  gpointer user_data, GNCSearchFree free_cb)
{
  GNCSearchWindow *sw = g_new0 (GNCSearchWindow, 1);

  g_return_val_if_fail (obj_type, NULL);
  g_return_val_if_fail (*obj_type != '\0', NULL);
  g_return_val_if_fail (param_list, NULL);

  /* Make sure the caller supplies callbacks xor result_callback */
  g_return_val_if_fail ((callbacks && !result_callback) ||
			(!callbacks && result_callback), NULL);

  if (callbacks)
    g_return_val_if_fail (display_list, NULL);

  sw->search_for = obj_type;
  sw->params_list = param_list;
  sw->display_list = display_list;
  sw->buttons = callbacks;
  sw->result_cb = result_callback;
  sw->new_item_cb = new_item_cb;
  sw->user_data = user_data;
  sw->free_cb = free_cb;

  /* Grab the get_guid function */
  sw->get_guid = gncQueryObjectGetParameterGetter (sw->search_for,
						   QUERY_PARAM_GUID);
  if (start_query)
    sw->start_q = gncQueryCopy (start_query);
  sw->q = show_start_query;

  gnc_search_dialog_init_widgets (sw);

  /* Maybe display the original query results? */
  if (callbacks && show_start_query) {
    gnc_search_dialog_reset_widgets (sw);
    gnc_search_dialog_display_results (sw);
  }

  return sw;
}

/* Register an on-close signal with the Search Dialog */
guint gnc_search_dialog_connect_on_close (GNCSearchWindow *sw,
					  GtkSignalFunc func,
					  gpointer user_data)
{
  g_return_val_if_fail (sw, 0);
  g_return_val_if_fail (func, 0);
  g_return_val_if_fail (user_data, 0);

  return gtk_signal_connect (GTK_OBJECT (sw->dialog), "close",
			     func, user_data);

}

/* Un-register the signal handlers with the Search Dialog */
void gnc_search_dialog_disconnect (GNCSearchWindow *sw, gpointer user_data)
{
  g_return_if_fail (sw);
  g_return_if_fail (user_data);

  gtk_signal_disconnect_by_data (GTK_OBJECT (sw->dialog), user_data);  
}

/* Clear all callbacks with this Search Window */
void gnc_search_dialog_set_select_cb (GNCSearchWindow *sw,
				      GNCSearchSelectedCB selected_cb,
				      gpointer user_data,
				      gboolean allow_clear)
{
  g_return_if_fail (sw);

  sw->selected_cb = selected_cb;
  sw->select_arg = user_data;
  sw->allow_clear = allow_clear;

  /* Show or hide the select button */
  if (sw->select_button) {
    if (selected_cb)
      gtk_widget_show_all (sw->select_button);
    else
      gtk_widget_hide_all (sw->select_button);
  }

  /* Show the proper close/cancel button */
  gnc_search_dialog_show_close_cancel (sw);
}

/* TEST CODE BELOW HERE */

static GList *
get_params_list (GNCIdTypeConst type)
{
  GList *list = NULL;

  list = gnc_search_param_prepend (list, "Txn: All Accounts",
				   ACCOUNT_MATCH_ALL_TYPE,
				   type, SPLIT_TRANS, TRANS_SPLITLIST,
				   SPLIT_ACCOUNT_GUID, NULL);
  list = gnc_search_param_prepend (list, "Split Account", GNC_ID_ACCOUNT,
				   type, SPLIT_ACCOUNT, QUERY_PARAM_GUID,
				   NULL);
  list = gnc_search_param_prepend (list, "Split->Txn->Void?", NULL, type,
				   SPLIT_TRANS, TRANS_VOID_STATUS, NULL);
  list = gnc_search_param_prepend (list, "Split Int64", NULL, type,
				   "d-share-int64", NULL);
  list = gnc_search_param_prepend (list, "Split Amount (double)", NULL, type,
				   "d-share-amount", NULL);
  list = gnc_search_param_prepend (list, "Split Value (debcred)", NULL, type,
				   SPLIT_VALUE, NULL);
  list = gnc_search_param_prepend (list, "Split Amount (numeric)", NULL, type,
				   SPLIT_AMOUNT, NULL);
  list = gnc_search_param_prepend (list, "Date Reconciled (date)", NULL, type,
				   SPLIT_DATE_RECONCILED, NULL);
  list = gnc_search_param_prepend (list, "Split Memo (string)", NULL, type,
				   SPLIT_MEMO, NULL);

  return list;
}

static GList *
get_display_list (GNCIdTypeConst type)
{
  GList *list = NULL;

  list = gnc_search_param_prepend (list, "Amount", NULL, type, SPLIT_AMOUNT,
				   NULL);
  list = gnc_search_param_prepend (list, "Memo", NULL, type, SPLIT_MEMO, NULL);
  list = gnc_search_param_prepend (list, "Date", NULL, type, SPLIT_TRANS,
				   TRANS_DATE_POSTED, NULL);

  return list;
}

static void
do_nothing (gpointer *a, gpointer b)
{
  return;
}

void
gnc_search_dialog_test (void)
{
  GNCSearchWindow *sw;
  static GList *params = NULL;
  static GList *display = NULL;
  static GNCSearchCallbackButton buttons[] = {
    /* Don't mark these as translatable since these are only test strings! */ 
    { ("View Split"), do_nothing },
    { ("New Split"), do_nothing },
    { ("Do Something"), do_nothing },
    { ("Do Nothing"), do_nothing },
    { ("Who Cares?"), do_nothing },
    { NULL }
  };

  if (params == NULL)
    params = get_params_list (GNC_ID_SPLIT);

  if (display == NULL)
    display = get_display_list (GNC_ID_SPLIT);

  sw = gnc_search_dialog_create (GNC_ID_SPLIT, params, display,
				 NULL, NULL, buttons, NULL, NULL, NULL, NULL);
}
