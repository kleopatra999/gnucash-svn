/* 
 * gnc-plugin.c -- 
 *
 * Copyright (C) 2003 Jan Arne Petersen
 * Author: Jan Arne Petersen <jpetersen@uni-bonn.de>
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

#include "gnc-plugin.h"
#include "gnc-trace.h"
#include "gnc-gconf-utils.h"

static gpointer parent_class = NULL;
static short module = MOD_GUI;

static void gnc_plugin_class_init (GncPluginClass *klass);
static void gnc_plugin_init       (GncPlugin *plugin_page);
static void gnc_plugin_finalize   (GObject *object);

struct  GncPluginPrivate {
	gpointer dummy;
};

GType
gnc_plugin_get_type (void)
{
	static GType gnc_plugin_type = 0;

	if (gnc_plugin_type == 0) {
		static const GTypeInfo our_info = {
			sizeof (GncPluginClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) gnc_plugin_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (GncPlugin),
			0,		/* n_preallocs */
			(GInstanceInitFunc) gnc_plugin_init,
		};

		gnc_plugin_type = g_type_register_static (G_TYPE_OBJECT,
							  "GncPlugin",
							   &our_info, 0);
	}

	return gnc_plugin_type;
}

static void
gnc_plugin_class_init (GncPluginClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);
	gobject_class->finalize = gnc_plugin_finalize;
}

static void
gnc_plugin_init (GncPlugin *plugin_page)
{
	GncPluginPrivate *priv;

	priv = plugin_page->priv = g_new0 (GncPluginPrivate, 1);
}

static void
gnc_plugin_finalize (GObject *object)
{
	GncPlugin *plugin;

	g_return_if_fail (GNC_IS_PLUGIN (object));

	plugin = GNC_PLUGIN (object);
	g_return_if_fail (plugin->priv != NULL);

	g_free (plugin->priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


void
gnc_plugin_add_to_window (GncPlugin *plugin,
			  GncMainWindow *window,
			  GQuark type)
{
	GncPluginClass *class;

	g_return_if_fail (GNC_IS_PLUGIN (plugin));
	class = GNC_PLUGIN_GET_CLASS (plugin);
	ENTER (": plugin %s(%p), window %p", gnc_plugin_get_name(plugin),
	       plugin, window);

	/*
	 * Update window with additional UI items
	 */
	if (class->actions_name) {
	  DEBUG ("%s: %d actions to merge with gui from %s",
		 class->actions_name, class->n_actions, class->ui_filename);
	  gnc_main_window_merge_actions (window, class->actions_name,
					 class->actions, class->n_actions,
					 class->ui_filename, plugin);
	}

	/*
	 * Setup gconf notifications if requested
	 */
	if (class->gconf_section && class->gconf_notifications) {
	  DEBUG ("Requesting notification for section %s", class->gconf_section);
	  gnc_gconf_add_notification(G_OBJECT(window), class->gconf_section,
				     class->gconf_notifications);
	}

	/*
	 * Do plugin specific actions.
	 */
	if (GNC_PLUGIN_GET_CLASS (plugin)->add_to_window) {
	  DEBUG ("Calling child class function %p", GNC_PLUGIN_GET_CLASS (plugin)->add_to_window);
	  GNC_PLUGIN_GET_CLASS (plugin)->add_to_window (plugin, window, type);
	}
	LEAVE ("");
}

void
gnc_plugin_remove_from_window (GncPlugin *plugin,
			       GncMainWindow *window,
			       GQuark type)
{
	GncPluginClass *class;

	g_return_if_fail (GNC_IS_PLUGIN (plugin));
	class = GNC_PLUGIN_GET_CLASS (plugin);
	ENTER (": plugin %s(%p), window %p", gnc_plugin_get_name(plugin),
	       plugin, window);

	/*
	 * Do plugin specific actions.
	 */
	if (GNC_PLUGIN_GET_CLASS (plugin)->remove_from_window) {
	  DEBUG ("Calling child class function %p",
		 GNC_PLUGIN_GET_CLASS (plugin)->remove_from_window);
	  GNC_PLUGIN_GET_CLASS (plugin)->remove_from_window (plugin, window, type);
	}

	/*
	 * Remove any gconf notifications
	 */
	if (class->gconf_section && class->gconf_notifications) {
	  DEBUG ("Remove notification for section %s", class->gconf_section);
	  gnc_gconf_remove_notification (G_OBJECT(window), class->gconf_section);
	}

	/*
	 * Update window to remove UI items
	 */
	if (class->actions_name) {
	  DEBUG ("%s: %d actions to unmerge",
		 class->actions_name, class->n_actions);
	  gnc_main_window_unmerge_actions (window, class->actions_name);
	}
	LEAVE ("");
}

GncPluginPage *
gnc_plugin_create_page (GncPlugin *plugin,
			const gchar *uri)
{
	g_return_val_if_fail (GNC_IS_PLUGIN (plugin), NULL);

	if (!GNC_PLUGIN_GET_CLASS (plugin)->create_page)
	  return NULL;
	return GNC_PLUGIN_GET_CLASS (plugin)->create_page (plugin, uri);
}

const gchar *
gnc_plugin_get_name (GncPlugin *plugin)
{
	g_return_val_if_fail (GNC_IS_PLUGIN (plugin), NULL);
	return (GNC_PLUGIN_GET_CLASS(plugin)->plugin_name);
}

#if 0
static void
gnc_plugin_base_init (gpointer klass)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		initialized = TRUE;

		signals[MERGE_ACTIONS] = g_signal_new ("merge-actions",
						       G_OBJECT_CLASS_TYPE (klass),
						       G_SIGNAL_RUN_FIRST,
						       G_STRUCT_OFFSET (GncPluginClass, merge_actions),
						       NULL, NULL,
						       g_cclosure_marshal_VOID__POINTER,
						       G_TYPE_NONE,
						       1,
						       GTK_TYPE_MENU_MERGE);
		signals[UNMERGE_ACTIONS] = g_signal_new ("unmerge-actions",
							 G_OBJECT_CLASS_TYPE (klass),
							 G_SIGNAL_RUN_FIRST,
							 G_STRUCT_OFFSET (GncPluginClass, unmerge_actions),
							 NULL, NULL,
							 g_cclosure_marshal_VOID__POINTER,
							 G_TYPE_NONE,
							 1,
							 GTK_TYPE_MENU_MERGE);
	}
}
#endif
