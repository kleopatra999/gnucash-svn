/* 
 * gnc-plugin-ofx.c -- 
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

#include "gnc-plugin-ofx.h"
#include "gnc-plugin-manager.h"

#include "gnc-ofx-import.h"
#include "messages.h"

static GList *active_plugins = NULL;

static void gnc_plugin_ofx_class_init (GncPluginOfxClass *klass);
static void gnc_plugin_ofx_init (GncPluginOfx *plugin);
static void gnc_plugin_ofx_finalize (GObject *object);

/* Command callbacks */
static void gnc_plugin_ofx_cmd_import (GtkAction *action, GncMainWindowActionData *data);


#define PLUGIN_ACTIONS_NAME "gnc-plugin-ofx-actions"
#define PLUGIN_UI_FILENAME  "gnc-plugin-ofx-ui.xml"

static GtkActionEntry gnc_plugin_actions [] = {
  { "OfxImportAction", GTK_STOCK_CONVERT, N_("Import OFX/QFX"), NULL,
    N_("Process an OFX/QFX response file"),
    G_CALLBACK (gnc_plugin_ofx_cmd_import) },
};
static guint gnc_plugin_n_actions = G_N_ELEMENTS (gnc_plugin_actions);

struct GncPluginOfxPrivate
{
  gpointer dummy;
};

static GObjectClass *parent_class = NULL;

GType
gnc_plugin_ofx_get_type (void)
{
  static GType gnc_plugin_ofx_type = 0;

  if (gnc_plugin_ofx_type == 0) {
    static const GTypeInfo our_info = {
		sizeof (GncPluginOfxClass),
		NULL,		/* base_init */
		NULL,		/* base_finalize */
		(GClassInitFunc) gnc_plugin_ofx_class_init,
		NULL,		/* class_finalize */
		NULL,		/* class_data */
		sizeof (GncPluginOfx),
		0,		/* n_preallocs */
		(GInstanceInitFunc) gnc_plugin_ofx_init,
    };

    gnc_plugin_ofx_type = g_type_register_static (GNC_TYPE_PLUGIN,
						  "GncPluginOfx",
						  &our_info, 0);
  }

  return gnc_plugin_ofx_type;
}

GncPlugin *
gnc_plugin_ofx_new (void)
{
  return GNC_PLUGIN (g_object_new (GNC_TYPE_PLUGIN_OFX, NULL));
}

#if DEBUG_REFERENCE_COUNTING
static void
dump_model (GncPluginOfx *plugin, gpointer dummy)
{
  g_warning("GncPluginOfx %p still exists.", plugin);
}

static gint
gnc_plugin_ofx_report_references (void)
{
  g_list_foreach(active_plugins, (GFunc)dump_model, NULL);
  return 0;
}
#endif

static void
gnc_plugin_ofx_class_init (GncPluginOfxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GncPluginClass *plugin_class = GNC_PLUGIN_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gnc_plugin_ofx_finalize;

  /* plugin info */
  plugin_class->plugin_name  = GNC_PLUGIN_OFX_NAME;

  /* widget addition/removal */
  plugin_class->actions_name = PLUGIN_ACTIONS_NAME;
  plugin_class->actions      = gnc_plugin_actions;
  plugin_class->n_actions    = gnc_plugin_n_actions;
  plugin_class->ui_filename  = PLUGIN_UI_FILENAME;

#if DEBUG_REFERENCE_COUNTING
  gtk_quit_add (0,
		(GtkFunction)gnc_plugin_ofx_report_references,
		NULL);
#endif
}

static void
gnc_plugin_ofx_init (GncPluginOfx *plugin)
{
  plugin->priv = g_new0 (GncPluginOfxPrivate, 1);

  active_plugins = g_list_append (active_plugins, plugin);
}

static void
gnc_plugin_ofx_finalize (GObject *object)
{
  GncPluginOfx *plugin;

  g_return_if_fail (GNC_IS_PLUGIN_OFX (object));

  plugin = GNC_PLUGIN_OFX (object);
  active_plugins = g_list_remove (active_plugins, plugin);

  g_return_if_fail (plugin->priv != NULL);

  g_free (plugin->priv);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/************************************************************
 *              Plugin Function Implementation              *
 ************************************************************/

/************************************************************
 *                    Command Callbacks                     *
 ************************************************************/

static void
gnc_plugin_ofx_cmd_import (GtkAction *action,
			   GncMainWindowActionData *data)
{
  gnc_file_ofx_import ();
}


/************************************************************
 *                    Plugin Bootstrapping                   *
 ************************************************************/

void
gnc_plugin_ofx_create_plugin (void)
{
  GncPlugin *plugin = gnc_plugin_ofx_new ();

  gnc_plugin_manager_add_plugin (gnc_plugin_manager_get (), plugin);
}
