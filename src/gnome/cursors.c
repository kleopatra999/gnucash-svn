/********************************************************************\
 * cursor.c -- functions for changing cursors                       *
 *                                                                  *
 * Copyright (C) 1997 Robin D. Clark <rclark@cs.hmc.edu>            *
 * Copyright (C) 1998-2000 Linas Vepstas <linas@linas.org>          *
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
 * 59 Temple Place - Suite 330        Fax:    +1-617-542-2652       *
 * Boston, MA  02111-1307,  USA       gnu@gnu.org                   *
\********************************************************************/

#include "config.h"

#include <gnome.h>

#include "gnc-ui.h"
#include "cursors.h"


/********************************************************************\
 * gnc_ui_set_cursor                                                * 
 *   sets the cursor to the specified type                          * 
 *                                                                  * 
 * Args: w    - the widget over which to change the cursor          *
 *       type - the type of cursor to make                          *
 * Return: none                                                     * 
\********************************************************************/
static void
gnc_ui_set_cursor (GdkWindow *win, int type)
{
  GdkCursor *cursor = NULL;

  if (win == NULL)
    return;

  if (type != GNC_CURSOR_NORMAL)
    cursor = gdk_cursor_new(type);

  gdk_window_set_cursor (win, cursor);

  while (gtk_events_pending())
    gtk_main_iteration();

  if (type != GNC_CURSOR_NORMAL)
    gdk_cursor_destroy(cursor);
}


static void
set_cursor_helper(gpointer window, gpointer data)
{
  GtkWidget *widget = GTK_WIDGET(window);
  int type = GPOINTER_TO_INT(data);

  gnc_ui_set_cursor(widget->window, type);
}


/********************************************************************\
 * gnc_set_busy_cursor                                              * 
 *   sets the cursor to the busy watch for the given window.        * 
 *   if the window is null, sets the cursor for all toplevel windows*
 *                                                                  * 
 * Args:   w - the widget over which to make cursor busy            * 
 * Return: none                                                     * 
\********************************************************************/
void 
gnc_set_busy_cursor(GtkWidget *w)
{
  if (w != NULL)
    gnc_ui_set_cursor(w->window, GNC_CURSOR_BUSY);
  else
    g_list_foreach(gtk_container_get_toplevels(),
                   set_cursor_helper,
                   GINT_TO_POINTER(GNC_CURSOR_BUSY));
}


/********************************************************************\
 * gnc_unset_busy_cursor                                            * 
 *   sets the cursor to the default cursor for the given window.    * 
 *   if the window is null, sets the cursor for all toplevel windows*
 *                                                                  * 
 * Args:   w - the widget over which to make cursor normal          * 
 * Return: none                                                     * 
\********************************************************************/
void 
gnc_unset_busy_cursor(GtkWidget *w)
{
  if (w != NULL)
    gnc_ui_set_cursor(w->window, GNC_CURSOR_NORMAL);
  else
    g_list_foreach(gtk_container_get_toplevels(),
                   set_cursor_helper,
                   GINT_TO_POINTER(GNC_CURSOR_NORMAL));
}

/************************* END OF FILE ******************************\
\********************************************************************/
