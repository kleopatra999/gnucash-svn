/*
 * gnc-icons.c -- Functions to create a GtkIconFactory for GnuCash
 * Copyright (C) 2003 Jan Arne Petersen
 * Author: Jan Arne Petersen <jpetersen@uni-bonn.de>
 */

#include <gtk/gtkiconfactory.h>

#include "gnc-icons.h"
#include "gnc-gnome-utils.h"

static void
gnc_add_stock_icon_pair (GtkIconFactory *factory,
			 const char *stock,
			 const char *filename1,
			 const char *filename2)
{
	GtkIconSet *set;
	GtkIconSource *source;
	GdkPixbuf *pixbuf1, *pixbuf2;
	char *fullname1, *fullname2;

	/* Find the complete path names for these files */
	fullname1 = gnc_gnome_locate_file (filename1);
	fullname2 = gnc_gnome_locate_file (filename2);
	g_assert (fullname1 && fullname2);

	/* Load the pixbufs */
	pixbuf1 = gnc_gnome_get_gdkpixbuf (filename1);
	pixbuf2 = gnc_gnome_get_gdkpixbuf (filename2);
	g_assert (pixbuf1 && pixbuf2);

	/* Create the icon set */
	set = gtk_icon_set_new ();
	source = gtk_icon_source_new ();
	gtk_icon_source_set_filename (source, fullname1);
	gtk_icon_source_set_pixbuf (source, pixbuf1);
	gtk_icon_set_add_source (set, source);

	source = gtk_icon_source_new ();
	gtk_icon_source_set_filename (source, fullname2);
	gtk_icon_source_set_pixbuf (source, pixbuf2);
	gtk_icon_source_set_size (source, GTK_ICON_SIZE_MENU);
	gtk_icon_source_set_size_wildcarded (source, FALSE);
	gtk_icon_set_add_source (set, source);

	/* Add it to the factory */
	gtk_icon_factory_add (factory, stock, set);

	/* Cleanup */
	gdk_pixbuf_unref (pixbuf2);
	gdk_pixbuf_unref (pixbuf1);
	g_free(fullname2);
	g_free(fullname1);
	gtk_icon_set_unref (set);
}

void
gnc_load_stock_icons (void)
{
	GtkIconFactory *factory;

	factory = gtk_icon_factory_new ();
	gnc_add_stock_icon_pair (factory, GNC_STOCK_ACCOUNT,
				 "account.png", "account-16.png");
	gnc_add_stock_icon_pair (factory, GNC_STOCK_DELETE_ACCOUNT,
				 "delete-account.png", "delete-account-16.png");
	gnc_add_stock_icon_pair (factory, GNC_STOCK_EDIT_ACCOUNT,
				 "edit-account.png", "edit-account-16.png");
	gnc_add_stock_icon_pair (factory, GNC_STOCK_OPEN_ACCOUNT,
				 "open-account.png", "open-account-16.png");
	gtk_icon_factory_add_default (factory);
}
