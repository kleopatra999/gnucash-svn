/* 
 * gnc-plugin_page.c -- 
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

/** @addtogroup ContentPlugins
    @{ */
/** @addtogroup ContentPluginBase Common object and functions
    @{ */
/** @file gnc-plugin-page.c
    @brief Functions for adding plugins to a Gnucash window.
    @author Copyright (C) 2003 Jan Arne Petersen
    @author Copyright (C) 2003,2005 David Hampton <hampton@employees.org>
*/

#include "config.h"

#include <gtk/gtk.h>
#include "gnc-plugin.h"
#include "gnc-plugin-page.h"
#include "gnc-gobject-utils.h"

static gpointer         parent_class = NULL;

static void gnc_plugin_page_class_init (GncPluginPageClass *klass);
static void gnc_plugin_page_init       (GncPluginPage *plugin_page,
					GncPluginPageClass *klass);
static void gnc_plugin_page_finalize   (GObject *object);
static void gnc_plugin_page_set_property (GObject         *object,
					  guint            prop_id,
					  const GValue    *value,
					  GParamSpec      *pspec);
static void gnc_plugin_page_get_property (GObject         *object,
					  guint            prop_id,
					  GValue          *value,
					  GParamSpec      *pspec);

enum {
  INSERTED,
  REMOVED,
  SELECTED,
  UNSELECTED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_PAGE_NAME,
  PROP_PAGE_URI,
  PROP_BOOK,
  PROP_STATUSBAR_TEXT,
  PROP_USE_NEW_WINDOW,
  PROP_UI_DESCRIPTION,
  PROP_UI_MERGE,
  PROP_ACTION_GROUP,
};

static guint signals[LAST_SIGNAL] = { 0 };

typedef struct _GncPluginPagePrivate GncPluginPagePrivate;

struct _GncPluginPagePrivate
{
	GtkActionGroup *action_group;
	GtkUIManager *ui_merge;
	guint merge_id;
	char *ui_description;

	GList *books;

	gboolean use_new_window;

	gchar *page_name;
	gchar *uri;
	gchar *statusbar_text;
};

#define GNC_PLUGIN_PAGE_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), GNC_TYPE_PLUGIN_PAGE, GncPluginPagePrivate))

GType
gnc_plugin_page_get_type (void)
{
	static GType gnc_plugin_page_type = 0;

	if (gnc_plugin_page_type == 0) {
		static const GTypeInfo our_info = {

			sizeof (GncPluginPageClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) gnc_plugin_page_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (GncPluginPage),
			0,		/* n_preallocs */
			(GInstanceInitFunc) gnc_plugin_page_init,
		};

		gnc_plugin_page_type = g_type_register_static (G_TYPE_OBJECT,
							       "GncPluginPage",
    							       &our_info, 0);
	}

	return gnc_plugin_page_type;
}

GtkWidget *
gnc_plugin_page_create_widget (GncPluginPage *plugin_page)
{
	GncPluginPageClass *klass;
	GtkWidget *widget;

	g_return_val_if_fail (GNC_IS_PLUGIN_PAGE (plugin_page), NULL);

	klass = GNC_PLUGIN_PAGE_GET_CLASS (plugin_page);
	g_return_val_if_fail (klass != NULL, NULL);
	g_return_val_if_fail (klass->create_widget != NULL, NULL);

	widget = klass->create_widget (plugin_page);

	/*
	 * If there is a destroy function, add a ref so that the
	 * widgets will exists when the destroy function is called.
	 * Otherwise it will be destroyed when it is removed from the
	 * main notebook for the window.
	 */
	if (klass->destroy_widget)
		g_object_ref(widget);

	return widget;
}

void
gnc_plugin_page_destroy_widget (GncPluginPage *plugin_page)
{
	GncPluginPageClass *klass;

	g_return_if_fail (GNC_IS_PLUGIN_PAGE (plugin_page));

	klass = GNC_PLUGIN_PAGE_GET_CLASS (plugin_page);
	g_return_if_fail (klass != NULL);
	g_return_if_fail (klass->destroy_widget != NULL);

	return klass->destroy_widget (plugin_page);
}

void
gnc_plugin_page_merge_actions (GncPluginPage *page,
			       GtkUIManager *ui_merge)
{
	GncPluginPagePrivate *priv;
	
	g_return_if_fail (GNC_IS_PLUGIN_PAGE(page));

	priv = GNC_PLUGIN_PAGE_GET_PRIVATE(page);
	priv->ui_merge = ui_merge;
	priv->merge_id = gnc_plugin_add_actions(priv->ui_merge,
						priv->action_group,
						priv->ui_description);
}

void
gnc_plugin_page_unmerge_actions (GncPluginPage *page,
				 GtkUIManager *ui_merge)
{
	GncPluginPagePrivate *priv;

	priv = GNC_PLUGIN_PAGE_GET_PRIVATE(page);

	g_return_if_fail (GNC_IS_PLUGIN_PAGE (page));
	g_return_if_fail (priv->merge_id != 0);
	g_return_if_fail (priv->action_group != NULL);

	gtk_ui_manager_remove_ui(ui_merge, priv->merge_id);
	gtk_ui_manager_remove_action_group(ui_merge, priv->action_group);

	priv->ui_merge = NULL;
	priv->merge_id = 0;
}

const gchar *
gnc_plugin_page_get_name (GncPluginPage *plugin_page)
{
	GncPluginPageClass *klass;

	g_return_val_if_fail (GNC_IS_PLUGIN_PAGE (plugin_page), NULL);

	klass = GNC_PLUGIN_PAGE_GET_CLASS (plugin_page);
	g_return_val_if_fail (klass != NULL, NULL);

	return (klass->plugin_name);
}


/* Signals */
void
gnc_plugin_page_inserted (GncPluginPage *plugin_page)
{
	g_return_if_fail (GNC_IS_PLUGIN_PAGE (plugin_page));

	g_signal_emit (G_OBJECT (plugin_page), signals[INSERTED], 0);
}

void
gnc_plugin_page_removed (GncPluginPage *plugin_page)
{
	g_return_if_fail (GNC_IS_PLUGIN_PAGE (plugin_page));

	g_signal_emit (G_OBJECT (plugin_page), signals[REMOVED], 0);
}

void
gnc_plugin_page_selected (GncPluginPage *plugin_page)
{
	g_return_if_fail (GNC_IS_PLUGIN_PAGE (plugin_page));

	g_signal_emit (G_OBJECT (plugin_page), signals[SELECTED], 0);
}

void
gnc_plugin_page_unselected (GncPluginPage *plugin_page)
{
	g_return_if_fail (GNC_IS_PLUGIN_PAGE (plugin_page));

	g_signal_emit (G_OBJECT (plugin_page), signals[UNSELECTED], 0);
}

static void
gnc_plugin_page_class_init (GncPluginPageClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);
	gobject_class->finalize = gnc_plugin_page_finalize;
	gobject_class->set_property = gnc_plugin_page_set_property;
	gobject_class->get_property = gnc_plugin_page_get_property;

	klass->tab_icon    = NULL;
	klass->plugin_name = NULL;

	g_type_class_add_private(klass, sizeof(GncPluginPagePrivate));

        g_object_class_install_property
	  (gobject_class,
	   PROP_PAGE_NAME,
	   g_param_spec_string ("page-name",
				"Page Name",
				"The name of this page.  This value is "
				"used to generate the notebook tab and "
				"menu items, and also the window title "
				"when this page is visible.",
				NULL,
				G_PARAM_READWRITE));

        g_object_class_install_property
	  (gobject_class,
	   PROP_PAGE_URI,
	   g_param_spec_string ("page-uri",
				"Page URI",
				"The uri for this page.",
				NULL,
				G_PARAM_READWRITE));

        g_object_class_install_property
	  (gobject_class,
	   PROP_STATUSBAR_TEXT,
	   g_param_spec_string ("statusbar-text",
				"Statusbar Text",
				"The text to be displayed in the statusbar "
				"at the bottom of the window when this page "
				"is visible.",
				NULL,
				G_PARAM_READWRITE));

        g_object_class_install_property
	  (gobject_class,
	   PROP_USE_NEW_WINDOW,
	   g_param_spec_boolean ("use-new-window",
				 "Use New Window",
				 "When TRUE a new top level window will be "
				 "created to hold this page.",
				 FALSE,
				 G_PARAM_READWRITE));

        g_object_class_install_property
	  (gobject_class,
	   PROP_UI_DESCRIPTION,
	   g_param_spec_string ("ui-description",
				"UI Description File",
				"The filename containing the XML data that "
				"describes this pages menus and toolbars.",
				NULL,
				G_PARAM_READWRITE));

        g_object_class_install_property
	  (gobject_class,
	   PROP_UI_MERGE,
	   g_param_spec_object ("ui-merge",
				"UI Merge",
				"A pointer to the GtkUIManager object that "
				"represents this pages menu hierarchy.",
				GTK_TYPE_UI_MANAGER,
				G_PARAM_READABLE));

        g_object_class_install_property
	  (gobject_class,
	   PROP_ACTION_GROUP,
	   g_param_spec_object ("action-group",
				"Action Group",
				"A pointer to the GtkActionGroup object that "
				"represents this pages available menu/toolbar "
				"actions.",
				GTK_TYPE_ACTION_GROUP,
				G_PARAM_READABLE));




 	signals[INSERTED] = g_signal_new ("inserted",
					  G_OBJECT_CLASS_TYPE (klass),
					  G_SIGNAL_RUN_FIRST,
					  G_STRUCT_OFFSET (GncPluginPageClass, inserted),
					  NULL, NULL,
					  g_cclosure_marshal_VOID__VOID,
					  G_TYPE_NONE,
					  0);
	signals[REMOVED] = g_signal_new ("removed",
					 G_OBJECT_CLASS_TYPE (klass),
					 G_SIGNAL_RUN_FIRST,
					 G_STRUCT_OFFSET (GncPluginPageClass, removed),
					 NULL, NULL,
					 g_cclosure_marshal_VOID__VOID,
					 G_TYPE_NONE,
					 0);
	signals[SELECTED] = g_signal_new ("selected",
					  G_OBJECT_CLASS_TYPE (klass),
					  G_SIGNAL_RUN_FIRST,
					  G_STRUCT_OFFSET (GncPluginPageClass, selected),
					  NULL, NULL,
					  g_cclosure_marshal_VOID__VOID,
					  G_TYPE_NONE,
					  0);
	signals[UNSELECTED] = g_signal_new ("unselected",
					    G_OBJECT_CLASS_TYPE (klass),
					    G_SIGNAL_RUN_FIRST,
					    G_STRUCT_OFFSET (GncPluginPageClass, unselected),
					    NULL, NULL,
					    g_cclosure_marshal_VOID__VOID,
					    G_TYPE_NONE,
					    0);
}

static void
gnc_plugin_page_init (GncPluginPage *page, GncPluginPageClass *klass)
{
	GncPluginPagePrivate *priv;

	priv = GNC_PLUGIN_PAGE_GET_PRIVATE(page);
	priv->page_name   = NULL;
	priv->uri         = NULL;

	page->window      = NULL;
	page->summarybar  = NULL;

	gnc_gobject_tracking_remember(G_OBJECT(page), G_OBJECT_CLASS(klass));
}

static void
gnc_plugin_page_finalize (GObject *object)
{
  GncPluginPagePrivate *priv;
  GncPluginPage *page;
  GList *item;

  page = GNC_PLUGIN_PAGE (object);

  priv = GNC_PLUGIN_PAGE_GET_PRIVATE(page);
  if (priv->page_name)
	g_free(priv->page_name);
  if (priv->uri)
	g_free(priv->uri);
  if (priv->statusbar_text)
	g_free(priv->statusbar_text);

  if (priv->books) {
    for (item = priv->books; item; item = g_list_next(item)) {
      guid_free (item->data);
    }
    g_list_free(priv->books);
    priv->books = NULL;
  }

  if (page->summarybar) {
    g_object_unref(G_OBJECT(page->summarybar));
    page->summarybar = NULL;
  }
  page->window = NULL; // Don't need to free it.

  gnc_gobject_tracking_forget(object);
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/************************************************************/
/*                g_object other functions                  */
/************************************************************/

/** Retrieve a property specific to this GncPluginPage object.  This is
 *  nothing more than a dispatch function for routines that can be
 *  called directly.  It has the nice feature of allowing a single
 *  function call to retrieve multiple properties.
 *
 *  @internal
 */
static void
gnc_plugin_page_get_property (GObject     *object,
			      guint        prop_id,
			      GValue      *value,
			      GParamSpec  *pspec)
{
  GncPluginPage *page;
  GncPluginPagePrivate *priv;

  g_return_if_fail(GNC_IS_PLUGIN_PAGE(object));

  page = GNC_PLUGIN_PAGE(object);
  priv = GNC_PLUGIN_PAGE_GET_PRIVATE(page);
  switch (prop_id)
    {
    case PROP_PAGE_NAME:
      g_value_set_string (value, priv->page_name);
      break;
    case PROP_PAGE_URI:
      g_value_set_string (value, priv->uri);
      break;
    case PROP_STATUSBAR_TEXT:
      g_value_set_string (value, priv->statusbar_text);
      break;
    case PROP_USE_NEW_WINDOW:
      g_value_set_boolean (value, priv->use_new_window);
      break;
    case PROP_UI_DESCRIPTION:
      g_value_set_string (value, priv->ui_description);
      break;
    case PROP_UI_MERGE:
      g_value_set_object (value, priv->ui_merge);
      break;
    case PROP_ACTION_GROUP:
      g_value_set_object (value, priv->action_group);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


/** Set a property specific to this GncPluginPage object.  This is
 *  nothing more than a dispatch function for routines that can be
 *  called directly.  It has the nice feature of allowing a new view
 *  to be created with a varargs list specifying the properties,
 *  instead of having to explicitly call each property function.
 *
 *  @internal
 */
static void
gnc_plugin_page_set_property (GObject      *object,
			      guint         prop_id,
			      const GValue *value,
			      GParamSpec   *pspec)
{
  GncPluginPage *page;

  g_return_if_fail(GNC_IS_PLUGIN_PAGE(object));

  page = GNC_PLUGIN_PAGE(object);
  
  switch (prop_id)
    {
    case PROP_PAGE_NAME:
      gnc_plugin_page_set_page_name(page, g_value_get_string(value));
      break;
    case PROP_PAGE_URI:
      gnc_plugin_page_set_uri(page, g_value_get_string(value));
      break;
    case PROP_STATUSBAR_TEXT:
      gnc_plugin_page_set_statusbar_text(page, g_value_get_string(value));
      break;
    case PROP_USE_NEW_WINDOW:
      gnc_plugin_page_set_use_new_window(page, g_value_get_boolean(value));
      break;
    case PROP_UI_DESCRIPTION:
      gnc_plugin_page_set_ui_description(page, g_value_get_string(value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/************************************************************/
/*                                                          */
/************************************************************/

void
gnc_plugin_page_add_book (GncPluginPage *page, QofBook *book)
{
  GncPluginPagePrivate *priv;
  GUID *guid;

  g_return_if_fail (GNC_IS_PLUGIN_PAGE (page));
  g_return_if_fail (book != NULL);

  priv = GNC_PLUGIN_PAGE_GET_PRIVATE(page);

  guid = guid_malloc();
  *guid = *qof_book_get_guid(book);
  priv->books = g_list_append(priv->books, guid);
}

gboolean
gnc_plugin_page_has_book (GncPluginPage *page, GUID *entity)
{
  GncPluginPagePrivate *priv;
  GList *item;

  g_return_val_if_fail (GNC_IS_PLUGIN_PAGE (page), FALSE);
  g_return_val_if_fail (entity != NULL, FALSE);

  priv = GNC_PLUGIN_PAGE_GET_PRIVATE(page);
  for (item = priv->books; item; item = g_list_next(item)) {
    if (guid_equal((GUID*)item->data, entity)) {
      return TRUE;
    }
  }
  return FALSE;
}

gboolean
gnc_plugin_page_has_books (GncPluginPage *page)
{
  GncPluginPagePrivate *priv;

  g_return_val_if_fail (GNC_IS_PLUGIN_PAGE (page), FALSE);

  priv = GNC_PLUGIN_PAGE_GET_PRIVATE(page);
  return (priv->books != NULL);
}

GtkWidget *
gnc_plugin_page_get_window (GncPluginPage *page)
{
  g_return_val_if_fail (GNC_IS_PLUGIN_PAGE (page), NULL);

  return page->window;
}

const gchar *
gnc_plugin_page_get_page_name (GncPluginPage *page)
{
  GncPluginPagePrivate *priv;

  g_return_val_if_fail (GNC_IS_PLUGIN_PAGE (page), NULL);

  priv = GNC_PLUGIN_PAGE_GET_PRIVATE(page);
  return priv->page_name;
}

void
gnc_plugin_page_set_page_name (GncPluginPage *page, const gchar *name)
{
  GncPluginPagePrivate *priv;

  g_return_if_fail (GNC_IS_PLUGIN_PAGE (page));

  priv = GNC_PLUGIN_PAGE_GET_PRIVATE(page);
  if (priv->page_name)
    g_free(priv->page_name);
  priv->page_name = g_strdup(name);
}

const gchar *
gnc_plugin_page_get_uri (GncPluginPage *page)
{
  GncPluginPagePrivate *priv;

  g_return_val_if_fail (GNC_IS_PLUGIN_PAGE (page), NULL);

  priv = GNC_PLUGIN_PAGE_GET_PRIVATE(page);
  return priv->uri;
}

void
gnc_plugin_page_set_uri (GncPluginPage *page, const gchar *name)
{
  GncPluginPagePrivate *priv;

  g_return_if_fail (GNC_IS_PLUGIN_PAGE (page));

  priv = GNC_PLUGIN_PAGE_GET_PRIVATE(page);
  if (priv->uri)
    g_free(priv->uri);
  priv->uri = g_strdup(name);
}

const gchar *
gnc_plugin_page_get_statusbar_text (GncPluginPage *page)
{
  GncPluginPagePrivate *priv;

  g_return_val_if_fail (GNC_IS_PLUGIN_PAGE (page), NULL);

  priv = GNC_PLUGIN_PAGE_GET_PRIVATE(page);
  return priv->statusbar_text;
}

void
gnc_plugin_page_set_statusbar_text (GncPluginPage *page, const gchar *message)
{
  GncPluginPagePrivate *priv;

  g_return_if_fail (GNC_IS_PLUGIN_PAGE (page));

  priv = GNC_PLUGIN_PAGE_GET_PRIVATE(page);
  if (priv->statusbar_text)
    g_free(priv->statusbar_text);
  priv->statusbar_text = g_strdup(message);
}

gboolean
gnc_plugin_page_get_use_new_window (GncPluginPage *page)
{
  GncPluginPagePrivate *priv;

  g_return_val_if_fail (GNC_IS_PLUGIN_PAGE (page), FALSE);

  priv = GNC_PLUGIN_PAGE_GET_PRIVATE(page);
  return priv->use_new_window;
}

void
gnc_plugin_page_set_use_new_window (GncPluginPage *page, gboolean use_new)
{
  GncPluginPagePrivate *priv;

  g_return_if_fail (GNC_IS_PLUGIN_PAGE (page));

  priv = GNC_PLUGIN_PAGE_GET_PRIVATE(page);
  priv->use_new_window = use_new;
}

const gchar *
gnc_plugin_page_get_ui_description (GncPluginPage *page)
{
  GncPluginPagePrivate *priv;

  g_return_val_if_fail (GNC_IS_PLUGIN_PAGE (page), FALSE);

  priv = GNC_PLUGIN_PAGE_GET_PRIVATE(page);
  return priv->ui_description;
}

void
gnc_plugin_page_set_ui_description (GncPluginPage *page,
				    const char *ui_filename)
{
  GncPluginPagePrivate *priv;

  g_return_if_fail(GNC_IS_PLUGIN_PAGE(page));

  priv = GNC_PLUGIN_PAGE_GET_PRIVATE(page);
  if (priv->ui_description)
    g_free(priv->ui_description);
  priv->ui_description = g_strdup(ui_filename);
}

GtkUIManager *
gnc_plugin_page_get_ui_merge (GncPluginPage *page)
{
  GncPluginPagePrivate *priv;

  g_return_val_if_fail(GNC_IS_PLUGIN_PAGE(page), NULL);

  priv = GNC_PLUGIN_PAGE_GET_PRIVATE(page);
  return priv->ui_merge;
}

GtkActionGroup *
gnc_plugin_page_get_action_group(GncPluginPage *page)
{
  GncPluginPagePrivate *priv;

  g_return_val_if_fail (GNC_IS_PLUGIN_PAGE (page), NULL);
  priv = GNC_PLUGIN_PAGE_GET_PRIVATE(page);
  return priv->action_group;
}

GtkActionGroup *
gnc_plugin_page_create_action_group (GncPluginPage *page, const gchar *group_name)
{
  GncPluginPagePrivate *priv;

  priv = GNC_PLUGIN_PAGE_GET_PRIVATE(page);
  priv->action_group = gtk_action_group_new(group_name);
  return priv->action_group;
}

/** @} */
/** @} */
