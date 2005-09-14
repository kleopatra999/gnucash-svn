/*
 * gnc-date-format.c -- Date formator widget
 *
 * Copyright (C) 2003 Derek Atkins  <derek@ihtfp.com>
 * All rights reserved.
 *
 * Gnucash is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * as published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * Gnucash is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, contact:
 *
 * Free Software Foundation           Voice:  +1-617-542-5942
 * 59 Temple Place - Suite 330        Fax:    +1-617-542-2652
 * Boston, MA  02111-1307,  USA       gnu@gnu.org
 *
 */
/*
  @NOTATION@
*/

/*
 * Date format widget
 *
 * Authors: Derek Atkins <derek@ihtfp.com>
 */

#include "config.h"

#include <string.h>
#include <stdio.h>

#include "gnc-date-format.h"
#include "dialog-utils.h"

#define MAX_DATE_LEN 80

enum {
  FORMAT_CHANGED,
  LAST_SIGNAL
};

struct _GNCDateFormatPriv {
  GtkWidget*	format_combobox;

  GtkWidget*    label;

  GtkWidget*	months_label;
  GtkWidget*	months_number;
  GtkWidget*	months_abbrev;
  GtkWidget*	months_name;

  GtkWidget*	years_label;
  GtkWidget*	years_button;

  GtkWidget*	custom_label;
  GtkWidget*	custom_entry;

  GtkWidget*	sample_label;
};

static guint date_format_signals [LAST_SIGNAL] = { 0 };


static void gnc_date_format_init         (GNCDateFormat      *gdf);
static void gnc_date_format_class_init   (GNCDateFormatClass *class);
static void gnc_date_format_finalize     (GObject            *object);
static void gnc_date_format_compute_format(GNCDateFormat *gdf);

/* Used by glade_xml_signal_autoconnect_full */
void gnc_ui_date_format_changed_cb(GtkWidget *unused, gpointer user_data);

static GtkHBoxClass *parent_class;

/**
 * gnc_date_format_get_type:
 *
 * Returns the GtkType for the GNCDateFormat widget
 */
GType
gnc_date_format_get_type (void)
{
  static GType date_format_type = 0;

  if (!date_format_type){
    static const GTypeInfo date_format_info = {
      sizeof (GNCDateFormatClass),
      NULL,
      NULL,
      (GClassInitFunc) gnc_date_format_class_init,
      NULL,
      NULL,
      sizeof (GNCDateFormat),
      0,
      (GInstanceInitFunc) gnc_date_format_init,
      NULL,
    };

    date_format_type = g_type_register_static(GTK_TYPE_HBOX,
				        "GNCDateFormat",
					&date_format_info, 0);
  }
	
  return date_format_type;
}


static void
gnc_date_format_class_init (GNCDateFormatClass *klass)
{
  GObjectClass   *gobject_class = (GObjectClass *) klass;

  parent_class = g_type_class_peek_parent(klass);

  date_format_signals [FORMAT_CHANGED] =
    g_signal_new ("format_changed",
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GNCDateFormatClass, format_changed),
		  NULL,
		  NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE,
		  0);

  gobject_class->finalize = gnc_date_format_finalize;
}

static void
gnc_date_format_init (GNCDateFormat *gdf)
{
  GladeXML *xml;
  GtkWidget *dialog, *table;

  g_return_if_fail(gdf);
  g_return_if_fail(GNC_IS_DATE_FORMAT(gdf));

  gdf->priv = g_new0(GNCDateFormatPriv, 1);

  /* Open up the Glade and set the signals */
  xml = gnc_glade_xml_new("gnc-date-format.glade", "GNC Date Format");
  glade_xml_signal_autoconnect_full(xml, gnc_glade_autoconnect_full_func, gdf);

  /* pull in all the child widgets */
  gdf->priv->label = glade_xml_get_widget(xml, "widget_label");
  gdf->priv->format_combobox = glade_xml_get_widget(xml, "format_combobox");

  gdf->priv->months_label = glade_xml_get_widget(xml, "months_label");
  gdf->priv->months_number = glade_xml_get_widget(xml, "month_number_button");
  gdf->priv->months_abbrev = glade_xml_get_widget(xml, "month_abbrev_button");
  gdf->priv->months_name = glade_xml_get_widget(xml, "month_name_button");

  gdf->priv->years_label = glade_xml_get_widget(xml, "years_label");
  gdf->priv->years_button = glade_xml_get_widget(xml, "years_button");

  gdf->priv->custom_label = glade_xml_get_widget(xml, "format_label");
  gdf->priv->custom_entry = glade_xml_get_widget(xml, "format_entry");

  gdf->priv->sample_label = glade_xml_get_widget(xml, "sample_label");

  /* Set initial format to gnucash default */
  gnc_date_format_set_format(gdf, qof_date_format_get());

  /* pull in the dialog and table widgets and play the reconnect game */
  dialog = glade_xml_get_widget(xml, "GNC Date Format");

  table = glade_xml_get_widget(xml, "date_format_table");
  g_object_ref(G_OBJECT(table));
  gtk_container_remove(GTK_CONTAINER(dialog), table);
  gtk_container_add(GTK_CONTAINER(gdf), table);
  g_object_unref(G_OBJECT(table));

  /* Destroy the now empty window */
  gtk_widget_destroy(dialog);
}

static void
gnc_date_format_finalize (GObject *object)
{
  GNCDateFormat *gdf;

  g_return_if_fail(object != NULL);
  g_return_if_fail(GNC_IS_DATE_FORMAT(object));

  gdf = GNC_DATE_FORMAT(object);

  g_free(gdf->priv);

  if (G_OBJECT_CLASS(parent_class)->finalize)
    (* G_OBJECT_CLASS(parent_class)->finalize) (object);
}


/**
 * gnc_date_format_new:
 *
 * Creates a new GNCDateFormat widget which can be used to provide
 * an easy to use way for entering date formats and seeing the sample.
 * 
 * Returns a GNCDateFormat widget.
 */
GtkWidget *
gnc_date_format_new (void)
{
  return gnc_date_format_new_with_label (NULL);
}

GtkWidget *
gnc_date_format_new_without_label (void)
{
  GtkWidget *widget = gnc_date_format_new_with_label(NULL);
  GNCDateFormat *gdf = GNC_DATE_FORMAT(widget);

  gtk_widget_destroy(gdf->priv->label);
  gdf->priv->label = NULL;

  return widget;
}

/**
 * gnc_date_format_new_with_label:
 * @label: the label to use to define the widget.
 *
 * Creates a new GNCDateFormat widget which can be used to provide
 * an easy to use way for entering date formats and seeing the sample.
 * 
 * Returns a GNCDateFormat widget.
 */
GtkWidget *
gnc_date_format_new_with_label (const char *label)
{
  GNCDateFormat *gdf;

  gdf = g_object_new(GNC_TYPE_DATE_FORMAT, NULL);

  if (label)
    gtk_label_set_text(GTK_LABEL(gdf->priv->label), label);

  gnc_date_format_compute_format(gdf);
  return GTK_WIDGET(gdf);
}

void
gnc_date_format_set_format (GNCDateFormat *gdf, QofDateFormat format)
{
  g_return_if_fail(gdf);
  g_return_if_fail(GNC_IS_DATE_FORMAT(gdf));

  gtk_combo_box_set_active(GTK_COMBO_BOX(gdf->priv->format_combobox), format);
  gnc_date_format_compute_format(gdf);
}

QofDateFormat
gnc_date_format_get_format (GNCDateFormat *gdf)
{
  g_return_val_if_fail (gdf, QOF_DATE_FORMAT_LOCALE);
  g_return_val_if_fail (GNC_IS_DATE_FORMAT(gdf), QOF_DATE_FORMAT_LOCALE);

  return gtk_combo_box_get_active(GTK_COMBO_BOX(gdf->priv->format_combobox));
}

void
gnc_date_format_set_months (GNCDateFormat *gdf, GNCDateMonthFormat months)
{
  GtkWidget *button = NULL;

  g_return_if_fail(gdf);
  g_return_if_fail(GNC_IS_DATE_FORMAT(gdf));

  switch (months) {
  case GNCDATE_MONTH_NUMBER:
    button = gdf->priv->months_number;
    break;
  case GNCDATE_MONTH_ABBREV:
    button = gdf->priv->months_abbrev;
    break;
  case GNCDATE_MONTH_NAME:
    button = gdf->priv->months_name;
    break;
  default:
    break;
  }

  g_return_if_fail(button);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
  gnc_date_format_compute_format(gdf);
}

GNCDateMonthFormat
gnc_date_format_get_months (GNCDateFormat *gdf)
{
  g_return_val_if_fail(gdf, GNCDATE_MONTH_NUMBER);
  g_return_val_if_fail(GNC_IS_DATE_FORMAT(gdf), GNCDATE_MONTH_NUMBER);

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gdf->priv->months_number)))
    return GNCDATE_MONTH_NUMBER;
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gdf->priv->months_abbrev)))
    return GNCDATE_MONTH_ABBREV;
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gdf->priv->months_name)))
    return GNCDATE_MONTH_ABBREV;

  /* We should never reach this point */
  g_assert(FALSE);
  return GNCDATE_MONTH_NUMBER;
}

void
gnc_date_format_set_years (GNCDateFormat *gdf, gboolean include_century)
{
  g_return_if_fail(gdf);
  g_return_if_fail(GNC_IS_DATE_FORMAT(gdf));

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gdf->priv->years_button),
			       include_century);
  gnc_date_format_compute_format(gdf);
}

gboolean
gnc_date_format_get_years (GNCDateFormat *gdf)
{
  g_return_val_if_fail(gdf, FALSE);
  g_return_val_if_fail(GNC_IS_DATE_FORMAT(gdf), FALSE);

  return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gdf->priv->years_button));
}

void
gnc_date_format_set_custom (GNCDateFormat *gdf, const char *format)
{
  g_return_if_fail(gdf);
  g_return_if_fail(GNC_IS_DATE_FORMAT(gdf));

  if (format == NULL || *format == '\0')
    return;

  gtk_entry_set_text(GTK_ENTRY(gdf->priv->custom_entry), format);
  gnc_date_format_compute_format(gdf);
}

const char *
gnc_date_format_get_custom (GNCDateFormat *gdf)
{
  g_return_val_if_fail(gdf, "");
  g_return_val_if_fail(GNC_IS_DATE_FORMAT(gdf), "");

  return gtk_entry_get_text(GTK_ENTRY(gdf->priv->custom_entry));
}

/**
 * gnc_date_format_editable_enters:
 * @dialog: The gnome dialog this date formator lives in
 * @gdf: The date formator to modity
 * 
 * Extracts the formatable field from a GNCDateFormat widget, and sets it
 * up so that pressing the Enter key in this field as the same as
 * clicking the button that has the default.
 **/
void
gnc_date_format_editable_enters (GnomeDialog *dialog, GNCDateFormat *gdf)
{
  g_return_if_fail(dialog);
  g_return_if_fail(gdf);
  g_return_if_fail(GNC_IS_DATE_FORMAT(gdf));

  gtk_entry_set_activates_default(GTK_ENTRY(gdf->priv->custom_entry), TRUE);
}

void
gnc_ui_date_format_changed_cb(GtkWidget *unused, gpointer user_data)
{
  GNCDateFormat * gdf = user_data;

  gnc_date_format_compute_format(gdf);
}

static void
gnc_date_format_enable_month (GNCDateFormat *gdf, gboolean sensitive)
{
  gtk_widget_set_sensitive(gdf->priv->months_label, sensitive);
  gtk_widget_set_sensitive(gdf->priv->months_number, sensitive);
  gtk_widget_set_sensitive(gdf->priv->months_abbrev, sensitive);
  gtk_widget_set_sensitive(gdf->priv->months_name, sensitive);
}

static void
gnc_date_format_enable_year (GNCDateFormat *gdf, gboolean sensitive)
{
  gtk_widget_set_sensitive(gdf->priv->years_label, sensitive);
  gtk_widget_set_sensitive(gdf->priv->years_button, sensitive);
}

static void
gnc_date_format_enable_format (GNCDateFormat *gdf, gboolean sensitive)
{
  gtk_widget_set_sensitive(gdf->priv->custom_label, sensitive);
  gtk_widget_set_sensitive(gdf->priv->custom_entry, sensitive);
}

void
gnc_date_format_refresh (GNCDateFormat *gdf)
{
  int sel_option;
  gboolean enable_year, enable_month, enable_custom, check_modifiers;
  static gchar *format, *c;
  gchar date_string[MAX_DATE_LEN];
  time_t secs_now;
  struct tm today;

  g_return_if_fail(gdf);
  g_return_if_fail(GNC_IS_DATE_FORMAT(gdf));

  sel_option =
    gtk_combo_box_get_active(GTK_COMBO_BOX(gdf->priv->format_combobox));

  switch (sel_option) {
   case QOF_DATE_FORMAT_CUSTOM:
    format = g_strdup(gtk_entry_get_text(GTK_ENTRY(gdf->priv->custom_entry)));
    enable_year = enable_month = check_modifiers = FALSE;
    enable_custom = TRUE;
    break;

   case QOF_DATE_FORMAT_LOCALE:
   case QOF_DATE_FORMAT_UTC:
    format = g_strdup(qof_date_format_get_string(sel_option));
    enable_year = enable_month = check_modifiers = enable_custom = FALSE;
    break;

   case QOF_DATE_FORMAT_ISO:
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gdf->priv->months_number), TRUE);
    enable_year = check_modifiers = TRUE;
    enable_month = enable_custom = FALSE;
    break;

   default:
    enable_year = enable_month = check_modifiers = TRUE;
    enable_custom = FALSE;
    break;
  }

  /* Tweak widget sensitivities, as appropriate. */
  gnc_date_format_enable_year(gdf, enable_year);
  gnc_date_format_enable_month(gdf, enable_month);
  gnc_date_format_enable_format(gdf, enable_custom);

  /* Update the format string based upon the user's preferences */
  if (check_modifiers) {
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gdf->priv->months_number))) {
      format = g_strdup(qof_date_format_get_string(sel_option));
    } else {
      format = g_strdup(qof_date_text_format_get_string(sel_option));
      if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gdf->priv->months_name))) {
	c = strchr(format, 'b');
	if (c)
	  *c = 'B';
      }
    }
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gdf->priv->years_button))){
      c = strchr(format, 'y');
      if (c)
	*c = 'Y';
    }
  }

  /*
   * Give feedback on the format string so users can see how it works
   * without having to read the strftime man page. Prevent recursive
   * signals.
   */
  gtk_signal_handler_block_by_data(GTK_OBJECT(gdf->priv->custom_entry), gdf);
  gtk_entry_set_text(GTK_ENTRY(gdf->priv->custom_entry), format);
  gtk_signal_handler_unblock_by_data(GTK_OBJECT(gdf->priv->custom_entry), gdf);
  
  /* Visual feedback on what the date will look like. */
  secs_now = time(NULL);
  localtime_r(&secs_now, &today);
  strftime(date_string, MAX_DATE_LEN, format, &today);
  gtk_label_set_text(GTK_LABEL(gdf->priv->sample_label), date_string);
  g_free(format);
}

static void
gnc_date_format_compute_format(GNCDateFormat *gdf)
{
  g_return_if_fail(gdf);
  g_return_if_fail(GNC_IS_DATE_FORMAT(gdf));

  /* refresh the widget */
  gnc_date_format_refresh(gdf);

  /* Emit a signal that we've changed */
  g_signal_emit(G_OBJECT(gdf), date_format_signals[FORMAT_CHANGED], 0);
}
