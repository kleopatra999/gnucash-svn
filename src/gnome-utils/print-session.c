/********************************************************************\
 * print-session.c -- simple printing manager for gnucash           *
 * Copyright (C) 2000 Bill Gribble <grib@billgribble.com>           *
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

#define _GNU_SOURCE

#include "config.h"

#include <gnome.h>
#include <libgnomeprint/gnome-font.h>
#if GNOME_PRINT_VER == 2
#include <libgnomeprintui/gnome-print-job-preview.h>
#else
#include <libgnomeprintui/gnome-print-master-preview.h>
#endif
#include <stdio.h>

#include "gnc-ui.h"
#include "messages.h"
#include "print-session.h"
#include "window-help.h"


PrintSession * 
gnc_print_session_create(gboolean hand_built_pages)
{
  PrintSession * ps = g_new0(PrintSession, 1);
  GnomePrintConfig *config;

  /* this is about the most basic we can get */
#if GNOME_PRINT_VER==2
  ps->master       = gnome_print_job_new(NULL);
  config = gnome_print_job_get_config(ps->master);
#else
  ps->master       = gnome_print_master_new();
  config = gnome_print_master_get_config(ps->master);
#endif
  ps->meta         = gnome_print_context_new(config);
  ps->default_font = gnome_font_find("Courier", 12);

  if (hand_built_pages) {
    gnome_print_beginpage(GNOME_PRINT_CONTEXT(ps->meta), "");
    gnome_print_setrgbcolor(GNOME_PRINT_CONTEXT(ps->meta),
			    0.0, 0.0, 0.0);
    gnome_print_setfont(GNOME_PRINT_CONTEXT(ps->meta), 
			GNOME_FONT(ps->default_font));
  }
  return ps;
}

void 
gnc_print_session_destroy(PrintSession * ps)
{
  g_object_unref(ps->meta);
  g_object_unref(ps->master);
  g_object_unref(ps->default_font);

  g_free(ps);
}

void 
gnc_print_session_moveto(PrintSession * ps, double x, double y)
{
  gnome_print_moveto(GNOME_PRINT_CONTEXT(ps->meta), x, y);
}


void 
gnc_print_session_text(PrintSession * ps, const char * text)
{
  gnome_print_show(GNOME_PRINT_CONTEXT(ps->meta), text);  
}


void
gnc_print_session_done(PrintSession * ps, gboolean hand_built_pages)
{
  if (hand_built_pages) {
    gnome_print_showpage(GNOME_PRINT_CONTEXT(ps->meta));
  }
  gnome_print_context_close(GNOME_PRINT_CONTEXT(ps->meta));

}

void
gnc_print_session_rotate(PrintSession *ps, double theta_in_degrees)
{
  gnome_print_rotate(GNOME_PRINT_CONTEXT(ps->meta), theta_in_degrees);
}

void
gnc_print_session_translate(PrintSession *ps, double x, double y)
{
  gnome_print_translate(GNOME_PRINT_CONTEXT(ps->meta), x, y);
}

void
gnc_print_session_gsave(PrintSession *ps)
{
  gnome_print_gsave(GNOME_PRINT_CONTEXT(ps->meta));
}

void
gnc_print_session_grestore(PrintSession *ps)
{
  gnome_print_grestore(GNOME_PRINT_CONTEXT(ps->meta));
}

void
gnc_print_session_print(PrintSession * ps)
{
#if GNOME_PRINT_VER==2
  GtkWidget * dialog    =
    gnome_print_dialog_new(ps->master,_("Print GnuCash Document"), 0);
#else
  GtkWidget * dialog    =
    gnome_print_dialog_new(_("Print GnuCash Document"), 0);
#endif
  int button            = gnome_dialog_run(GNOME_DIALOG(dialog));
  GnomePrintConfig *config;

  switch(button) {
  case 0: 
    /* print button */
    if(ps->master) {
      g_object_unref(ps->master);
      ps->master = NULL;
    }
    config = gnome_print_dialog_get_config (GNOME_PRINT_DIALOG (dialog));
#if GNOME_PRINT_VER==2
    ps->master = gnome_print_job_new(config);
#else
    ps->master = gnome_print_master_new_from_config(config);
#endif
    gnome_dialog_close(GNOME_DIALOG(dialog));
    gnc_print_session_render(ps);
    break;
    
  case 1:
    if(ps->master) {
      g_object_unref(ps->master);
      ps->master = NULL;
    }
    config = gnome_print_dialog_get_config (GNOME_PRINT_DIALOG (dialog));
#if GNOME_PRINT_VER==2
    ps->master = gnome_print_job_new(config);
#else
    ps->master = gnome_print_master_new_from_config(config);
#endif
    gnome_dialog_close(GNOME_DIALOG(dialog));
    gnc_print_session_preview(ps);    
    break;

  case 2:
    gnome_dialog_close(GNOME_DIALOG(dialog));
    break;
  }
}


void 
gnc_print_session_render(PrintSession * ps)
{
#if GNOME_PRINT_VER==2
  gnome_print_job_render (ps->master, ps->meta);
  gnome_print_job_close (ps->master);
  gnome_print_job_print(ps->master);
#else
  gnome_print_master_render (ps->master, ps->meta);
  gnome_print_master_close (ps->master);
  gnome_print_master_print(ps->master);
#endif
}

void 
gnc_print_session_preview(PrintSession * ps)
{
  GtkWidget * preview;
#if GNOME_PRINT_VER==2
  gnome_print_job_render (ps->master, ps->meta);
  gnome_print_job_close (ps->master);
  preview = gnome_print_job_preview_new(ps->master, _("Print Preview"));
#else
  gnome_print_master_render (ps->master, ps->meta);
  gnome_print_master_close (ps->master);
  preview = gnome_print_master_preview_new(ps->master, _("Print Preview"));
#endif
  gtk_widget_show_all(preview);
}
