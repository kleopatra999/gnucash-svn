/********************************************************************\
 * druid-utils.c -- utility functions for creating druids           *
 * Copyright (C) 2001 Jeremy Collins                                *
 * Copyright (C) 2001 Dave Peticolas <dave@krondo.com>              *
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
 *                                                                  *
\********************************************************************/

#include "config.h"

#include "dialog-utils.h"
#include "druid-utils.h"
#include "gnc-dir.h"
#include "gnc-engine-util.h"

/********************************************************************\
 * gnc_get_imlib_image                                              *
 *   returns a GdkImlibImage object given a pixmap filename         *
 *                                                                  *
 * Args: none                                                       *
 * Returns: GnomePixmap widget or NULL if there was a problem       *
 \*******************************************************************/
static GdkPixbuf *
gnc_druid_get_pixmap (const char *name)
{
  GdkPixbuf *pixbuf;
  GError *error = NULL;
  char *fullname;

  g_return_val_if_fail (name != NULL, NULL);

  fullname = g_strconcat (GNC_PIXMAP_DIR, "/", name, NULL);
  pixbuf = gdk_pixbuf_new_from_file (fullname, &error);
  if (error != NULL) {
    g_assert (pixbuf == NULL);
    fprintf (stderr, "Pixbuf is NULL: %s\n", error->message);
    g_error_free (error);
  }
  g_free (fullname);

  return pixbuf;
}


void
gnc_druid_set_watermark_images (GnomeDruid *druid,
				const char *top_path,
				const char *side_path)
{
  GdkPixbuf     *top_pixbuf, *side_pixbuf;
  GList         *page_list, *item;
  GtkWidget     *page;

  page_list = gtk_container_get_children(GTK_CONTAINER(druid));
  top_pixbuf = gnc_druid_get_pixmap(top_path);
  side_pixbuf = gnc_druid_get_pixmap(side_path);

  for (item = page_list; item; item = g_list_next(item)) {
    page = item->data;
    if (GNOME_IS_DRUID_PAGE_EDGE (page)) {
      GnomeDruidPageEdge *page_edge;

      page_edge = GNOME_DRUID_PAGE_EDGE (page);
      gnome_druid_page_edge_set_top_watermark (page_edge, top_pixbuf);
      gnome_druid_page_edge_set_watermark (page_edge, side_pixbuf);
    } else {
      GnomeDruidPageStandard *page_standard;

      page_standard = GNOME_DRUID_PAGE_STANDARD (page);
      gnome_druid_page_standard_set_top_watermark (page_standard, top_pixbuf);
    }
  }

  g_object_unref (G_OBJECT(side_pixbuf));
  g_object_unref (G_OBJECT(top_pixbuf));
  g_list_free(page_list);
}

void
gnc_druid_set_logo_image (GnomeDruid *druid, char *image_path)
{
  GdkPixbuf     *logo_pixbuf;
  GList         *page_list, *item;
  GtkWidget     *page;

  page_list = gtk_container_get_children(GTK_CONTAINER(druid));
  logo_pixbuf = gnc_druid_get_pixmap(image_path); 

  for (item = page_list; item; item = g_list_next(item)) {
    page = item->data;
    if (GNOME_IS_DRUID_PAGE_EDGE (page))
    {
      GnomeDruidPageEdge *page_edge;

      page_edge = GNOME_DRUID_PAGE_EDGE (page);
      gnome_druid_page_edge_set_logo (page_edge, logo_pixbuf);
    } else {
      GnomeDruidPageStandard *page_standard;

      page_standard = GNOME_DRUID_PAGE_STANDARD (page);
      gnome_druid_page_standard_set_logo (page_standard, logo_pixbuf);
    }    
  }

  g_object_unref (G_OBJECT(logo_pixbuf));
  g_list_free(page_list);
}

void
gnc_druid_set_colors (GnomeDruid *druid)
{
  GList *pages = gtk_container_get_children (GTK_CONTAINER (druid));
  GdkColor bluish;
  GdkColor white;
  GdkColormap *cm;

  if (!druid) return;
  if (!GNOME_IS_DRUID (druid)) return;

  bluish.red =   (gushort) (.40 * 65535);
  bluish.green = (gushort) (.40 * 65535);
  bluish.blue =  (gushort) (.60 * 65535);

  white.red =   65535;
  white.green = 65535;
  white.blue =  65535;

  cm = gtk_widget_get_colormap (GTK_WIDGET (druid));

  gdk_colormap_alloc_color(cm, &bluish, FALSE, TRUE);
  gdk_colormap_alloc_color(cm, &white, FALSE, TRUE);

  while (pages != NULL)
  {
    GnomeDruidPage *page = GNOME_DRUID_PAGE (pages->data);

    if (GNOME_IS_DRUID_PAGE_EDGE (page))
    {
      GnomeDruidPageEdge *page_edge;

      page_edge = GNOME_DRUID_PAGE_EDGE (page);
      gnome_druid_page_edge_set_bg_color (page_edge, &bluish);
      gnome_druid_page_edge_set_logo_bg_color (page_edge, &bluish);
    }
    else 
    {
      GnomeDruidPageStandard *page_standard;

      page_standard = GNOME_DRUID_PAGE_STANDARD (page);
      gnome_druid_page_standard_set_background (page_standard, &bluish);
      gnome_druid_page_standard_set_logo_background (page_standard, &bluish);
      gnome_druid_page_standard_set_title_foreground (page_standard, &white);
    }    
    pages = g_list_next (pages);
  }
}
