/********************************************************************\
 * gnucash-item-edit.c -- cell editor cut-n-paste from gnumeric     *
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

/*
 *  An editor for the gnucash sheet.
 *  Cut and pasted from the gnumeric item-edit.c file.
 *
 *  And then substantially rewritten by Dave Peticolas <dave@krondo.com>.
 */


#include "config.h"

#include <gnome.h>

#include "gnucash-color.h"
#include "gnucash-cursor.h"
#include "gnucash-item-edit.h"
#include "gnucash-grid.h"
#include "gnucash-sheet.h"
#include "gnucash-style.h"


/* FIXME GNOME2 Port
 *	- ButtonEvents are not delegated to GtkEntry. 
 *	- Cursor isn't drawn.
 */

/* The arguments we take */
enum {
        PROP_0,
        PROP_SHEET,     /* The sheet property      */
        PROP_EDITOR,     /* The entry property   */
};

/* values for selection info */
enum {
        TARGET_STRING,
        TARGET_TEXT,
        TARGET_COMPOUND_TEXT
};

static GnomeCanvasItemClass *gnc_item_edit_parent_class;
static GdkAtom clipboard_atom = GDK_NONE;
static GdkAtom ctext_atom = GDK_NONE;


typedef struct _TextDrawInfo TextDrawInfo;
struct _TextDrawInfo
{
	PangoLayout *layout;

        GdkRectangle bg_rect;
        GdkRectangle text_rect;
        GdkRectangle hatch_rect;

        GdkColor *fg_color;
        GdkColor *bg_color;

        GdkColor *fg_color2;
        GdkColor *bg_color2;

        gboolean hatching;

	PangoRectangle *cursor;
};


static void gnc_item_edit_show_popup_toggle (GncItemEdit *item_edit,
					 gint x, gint y,
					 gint width, gint height,
					 GtkAnchorType anchor);

/*
 * Returns the coordinates for the editor bounding box
 */
void
gnc_item_edit_get_pixel_coords (GncItemEdit *item_edit,
                            int *x, int *y,
                            int *w, int *h)
{
        GnucashSheet *sheet = item_edit->sheet;
        SheetBlock *block;
        int xd, yd;

        block = gnucash_sheet_get_block (sheet, item_edit->virt_loc.vcell_loc);
        if (block == NULL)
                return;

        xd = block->origin_x;
        yd = block->origin_y;

        gnucash_sheet_style_get_cell_pixel_rel_coords
                (item_edit->style,
                 item_edit->virt_loc.phys_row_offset,
                 item_edit->virt_loc.phys_col_offset,
                 x, y, w, h);

        *x += xd;
        *y += yd;
}


static void
gnc_item_edit_draw_info (GncItemEdit *item_edit, int x, int y, TextDrawInfo *info)
{
        SheetBlock *block;
        SheetBlockStyle *style;
        GtkEditable *editable;
        Table *table;

        gboolean hatching;
        guint32 argb;

        int xd, yd, wd, hd, dx, dy;
        int start_pos, end_pos;
        int toggle_space, cursor_pos;
        const gchar *text;
	PangoRectangle strong_pos;
	PangoAttribute *attr;
	PangoAttrList *attr_list;

        style = item_edit->style;
        table = item_edit->sheet->table;

        block = gnucash_sheet_get_block (item_edit->sheet,
                                         item_edit->virt_loc.vcell_loc);

        argb = gnc_table_get_bg_color (table, item_edit->virt_loc, &hatching);

        info->bg_color = gnucash_color_argb_to_gdk (argb);
        info->hatching = hatching;
        info->fg_color = &gn_black;

        info->bg_color2 = &gn_dark_gray;
        info->fg_color2 = &gn_white;

        editable = GTK_EDITABLE (item_edit->editor);
        cursor_pos = gtk_editable_get_position (editable);
	gtk_editable_get_selection_bounds (editable, &start_pos, &end_pos);

        text = gtk_entry_get_text (GTK_ENTRY (item_edit->editor));

	info->layout = gtk_widget_create_pango_layout (GTK_WIDGET (item_edit->sheet), text);

	/* Selection */
	attr_list = pango_attr_list_new ();

	attr = pango_attr_foreground_new (0xffff, 0xffff, 0xffff);
	attr->start_index = start_pos;
	attr->end_index = end_pos;
	pango_attr_list_insert (attr_list, attr);
	
	attr = pango_attr_background_new (0x0, 0x0, 0x0);
	attr->start_index = start_pos;
	attr->end_index = end_pos;
	pango_attr_list_insert (attr_list, attr);

	pango_layout_set_attributes (info->layout, attr_list);
	pango_attr_list_unref (attr_list);

        gnc_item_edit_get_pixel_coords (item_edit, &xd, &yd, &wd, &hd);

        dx = xd - x;
        dy = yd - y;

        info->bg_rect.x      = dx + CELL_HPADDING;
        info->bg_rect.y      = dy + 1;
        info->bg_rect.width  = wd - (2 * CELL_HPADDING);
        info->bg_rect.height = hd - 2;

        toggle_space = item_edit->is_popup ? 
                item_edit->popup_toggle.toggle_offset : 0;

        info->text_rect.x      = dx;
        info->text_rect.y      = dy + 1;
        info->text_rect.width  = wd - toggle_space;
        info->text_rect.height = hd - 2;

	pango_layout_get_cursor_pos (info->layout, cursor_pos, NULL, &strong_pos);
	info->cursor = &strong_pos;

        if (info->hatching)
        {
                info->hatch_rect.x = dx;
                info->hatch_rect.y = dy;
                info->hatch_rect.width = wd;
                info->hatch_rect.height = hd;
        }
}

static void
gnc_item_edit_free_draw_info_members(TextDrawInfo *info)
{
        if (info == NULL)
                return;

	g_object_unref (G_OBJECT (info->layout));
}

static void
gnc_item_edit_draw (GnomeCanvasItem *item, GdkDrawable *drawable,
                int x, int y, int width, int height)
{
        GncItemEdit *item_edit = GNC_ITEM_EDIT (item);
        TextDrawInfo info;

        /* be sure we're valid */
        if (item_edit->virt_loc.vcell_loc.virt_row < 0 ||
            item_edit->virt_loc.vcell_loc.virt_col < 0)
                return;

        /* Get the measurements for drawing */
        gnc_item_edit_draw_info (item_edit, x, y, &info);

        item_edit->reset_pos = FALSE;

        /* Draw the background */
        gdk_gc_set_foreground (item_edit->gc, info.bg_color);
        gdk_draw_rectangle (drawable, item_edit->gc, TRUE,
                            info.bg_rect.x, info.bg_rect.y,
                            info.bg_rect.width, info.bg_rect.height);

        if (info.hatching)
                gnucash_draw_hatching (drawable, item_edit->gc,
                                       info.hatch_rect.x,
                                       info.hatch_rect.y,
                                       info.hatch_rect.width,
                                       info.hatch_rect.height);

        /* Draw the foreground text and cursor */
        gdk_gc_set_clip_rectangle (item_edit->gc, &info.text_rect);

        gdk_gc_set_foreground (item_edit->gc, info.fg_color);

	gdk_draw_layout (drawable,
			 item_edit->gc,
			 info.text_rect.x,
			 info.text_rect.y,
			 info.layout);

	gdk_draw_line (drawable, item_edit->gc, info.cursor->x, info.cursor->y,
		       info.cursor->x, info.cursor->y + info.cursor->height);

        gdk_gc_set_clip_rectangle (item_edit->gc, NULL);

        gnc_item_edit_free_draw_info_members (&info);

#ifdef USE_XIM
        if (gdk_im_ready() && item_edit->ic && 
            (gdk_ic_get_style (item_edit->ic) & GDK_IM_PREEDIT_POSITION))
	{
                GnomeCanvasItem *item;
                double winx, winy;

                item = GNOME_CANVAS_ITEM (item_edit);

                gnome_canvas_world_to_window (GNOME_CANVAS (item_edit->sheet),
                                              item->x1, item->y2,
                                              &winx, &winy);

                item_edit->ic_attr->spot_location.x = winx;
                item_edit->ic_attr->spot_location.y = winy;

                gdk_ic_set_attr (item_edit->ic,
                                 item_edit->ic_attr, GDK_IC_SPOT_LOCATION);
	}
#endif 
}


static double
gnc_item_edit_point (GnomeCanvasItem *item, double c_x, double c_y, int cx, int cy,
                 GnomeCanvasItem **actual_item)
{
        int x, y, w, h;

        gnc_item_edit_get_pixel_coords (GNC_ITEM_EDIT (item), &x, &y, &w, &h);

        *actual_item = NULL;
        if ((cx < x) || (cy < y) || (cx > x+w) || (cy > y+w))
                return 10000.0;
 
        *actual_item = item;

        return 0.0;
}


static int
gnc_item_edit_event (GnomeCanvasItem *item, GdkEvent *event)
{
        return 0;
}


int
gnc_item_edit_get_toggle_offset (int row_height)
{
        /* sync with gnc_item_edit_update */
        return row_height - (2 * (CELL_VPADDING + 1)) + 3;
}

static void
gnc_item_edit_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path,
		  int flags)
{
        GncItemEdit *item_edit = GNC_ITEM_EDIT (item);
        gint toggle_x, toggle_y, toggle_width, toggle_height;
        gint x, y, w, h;

        if (GNOME_CANVAS_ITEM_CLASS (gnc_item_edit_parent_class)->update)
                (*GNOME_CANVAS_ITEM_CLASS(gnc_item_edit_parent_class)->update)
			(item, affine, clip_path, flags);

        gnc_item_edit_get_pixel_coords (item_edit, &x, &y, &w, &h);

        item->x1 = x;
        item->y1 = y;
        item->x2 = x + w;
        item->y2 = y + h;

	if (!item_edit->is_popup)
		return;

	toggle_height = h - (2 * (CELL_VPADDING + 1));
	toggle_width  = toggle_height;
	toggle_x      = x + w - (toggle_width + 3);
	toggle_y      = y + (h / 2) - (toggle_height / 2);

        item_edit->popup_toggle.toggle_offset = toggle_width + 3;

        gnc_item_edit_show_popup_toggle (item_edit, toggle_x, toggle_y,
                                     toggle_width, toggle_height,
                                     GTK_ANCHOR_NW);

	if (item_edit->show_popup)
                gnc_item_edit_show_popup (item_edit);
}


static void
gnc_item_edit_realize (GnomeCanvasItem *item)
{
        GnomeCanvas *canvas = item->canvas;
        GdkWindow *window;
        GncItemEdit *item_edit;

        if (GNOME_CANVAS_ITEM_CLASS (gnc_item_edit_parent_class)->realize)
                (*GNOME_CANVAS_ITEM_CLASS
		 (gnc_item_edit_parent_class)->realize) (item);

        item_edit = GNC_ITEM_EDIT (item);
        window = GTK_WIDGET (canvas)->window;

        item_edit->gc = gdk_gc_new (window);

#ifdef USE_XIM
        if (gdk_im_ready () &&
            (item_edit->ic_attr = gdk_ic_attr_new ()) != NULL)
        {
                gint width, height;
                GdkEventMask mask;
                GdkColormap *colormap;
                GdkICAttr *attr = item_edit->ic_attr;
                GdkICAttributesType attrmask = GDK_IC_ALL_REQ;
                GdkIMStyle style;
                GdkIMStyle supported_style =
                        GDK_IM_PREEDIT_NONE |
                        GDK_IM_PREEDIT_NOTHING |
                        GDK_IM_PREEDIT_POSITION |
                        GDK_IM_STATUS_NONE |
                        GDK_IM_STATUS_NOTHING;
                GtkWidget *sheet_widget;

                sheet_widget = GTK_WIDGET (item_edit->sheet);

                if (sheet_widget->style &&
                    gdk_font_from_description (sheet_widget->style->font_desc)
			->type != GDK_FONT_FONTSET)
                        supported_style &= ~GDK_IM_PREEDIT_POSITION;

                attr->style = style = gdk_im_decide_style (supported_style);
                attr->client_window = sheet_widget->window;

                if ((colormap = gtk_widget_get_colormap (sheet_widget)) !=
                    gtk_widget_get_default_colormap ())
                {
                        attrmask |= GDK_IC_PREEDIT_COLORMAP;
                        attr->preedit_colormap = colormap;
                }

                attrmask |= GDK_IC_PREEDIT_FOREGROUND;
                attrmask |= GDK_IC_PREEDIT_BACKGROUND;

                attr->preedit_foreground =
                        sheet_widget->style->fg[GTK_STATE_NORMAL];
                attr->preedit_background =
                        sheet_widget->style->base[GTK_STATE_NORMAL];

                switch (style & GDK_IM_PREEDIT_MASK)
                {
                        case GDK_IM_PREEDIT_POSITION:
                                if (sheet_widget->style &&
                                    gdk_font_from_description (
					sheet_widget->style->font_desc)
					->type != GDK_FONT_FONTSET)
                                {
                                        g_warning ("over-the-spot style "
                                                   "requires fontset");
                                        break;
                                }

                                gdk_window_get_size (attr->client_window,
                                                     &width, &height);

                                attrmask |= GDK_IC_PREEDIT_POSITION_REQ;

                                attr->spot_location.x = 0;
                                attr->spot_location.y = height;
                                attr->preedit_area.x = 0;
                                attr->preedit_area.y = 0;
                                attr->preedit_area.width = width;
                                attr->preedit_area.height = height;
                                attr->preedit_fontset =
                                        gdk_font_from_description (
					sheet_widget->style->font_desc);

                                break;
                }

                item_edit->ic = gdk_ic_new (attr, attrmask);

                if (item_edit->ic == NULL)
                        g_warning ("Can't create input context.");
                else
                {
                        mask = gdk_window_get_events (attr->client_window);
                        mask |= gdk_ic_get_events (item_edit->ic);

                        gdk_window_set_events (attr->client_window, mask);

                        if (GTK_WIDGET_HAS_FOCUS (sheet_widget))
                                gdk_im_begin (item_edit->ic,
                                              attr->client_window);
                }
        }
#endif
}


static void
gnc_item_edit_unrealize (GnomeCanvasItem *item)
{
        GncItemEdit *item_edit;

        item_edit = GNC_ITEM_EDIT (item);

#ifdef USE_XIM
        if (item_edit->ic)
        {
                gdk_ic_destroy (item_edit->ic);
                item_edit->ic = NULL;
        }

        if (item_edit->ic_attr)
        {
                gdk_ic_attr_destroy (item_edit->ic_attr);
                item_edit->ic_attr = NULL;
        }
#endif

        if (GNOME_CANVAS_ITEM_CLASS (gnc_item_edit_parent_class)->unrealize)
                (*GNOME_CANVAS_ITEM_CLASS
		 (gnc_item_edit_parent_class)->unrealize) (item);
}

void
gnc_item_edit_focus_in (GncItemEdit *item_edit)
{
        g_return_if_fail (item_edit != NULL);
        g_return_if_fail (GNC_IS_ITEM_EDIT(item_edit));

#ifdef USE_XIM
        if (item_edit->ic)
                gdk_im_begin (item_edit->ic,
                              GTK_WIDGET (item_edit->sheet)->window);
#endif
}

void
gnc_item_edit_focus_out (GncItemEdit *item_edit)
{
        g_return_if_fail (item_edit != NULL);
        g_return_if_fail (GNC_IS_ITEM_EDIT(item_edit));

#ifdef USE_XIM
        gdk_im_end ();
#endif
}

void
gnc_item_edit_reset_offset (GncItemEdit *item_edit)
{
        g_return_if_fail (item_edit != NULL);
        g_return_if_fail (GNC_IS_ITEM_EDIT(item_edit));

        item_edit->reset_pos = TRUE;
}

/*
 * Instance initialization
 */
static void
gnc_item_edit_init (GncItemEdit *item_edit)
{
        GnomeCanvasItem *item = GNOME_CANVAS_ITEM (item_edit);

        item->x1 = 0;
        item->y1 = 0;
        item->x2 = 1;
        item->y2 = 1;

        /* Set invalid values so that we know when we have been fully
	   initialized */
        item_edit->sheet = NULL;
	item_edit->parent = NULL;
	item_edit->editor = NULL;
        item_edit->clipboard = NULL;

        item_edit->has_selection = FALSE;
        item_edit->is_popup = FALSE;
        item_edit->show_popup = FALSE;

	item_edit->popup_toggle.toggle_button = NULL;
	item_edit->popup_toggle.toggle_button_item = NULL;
        item_edit->popup_toggle.toggle_offset = 0;
	item_edit->popup_toggle.arrow = NULL;
	item_edit->popup_toggle.signals_connected = FALSE;

        item_edit->popup_item = NULL;
        item_edit->get_popup_height = NULL;
        item_edit->popup_autosize = NULL;
        item_edit->popup_set_focus = NULL;
        item_edit->popup_post_show = NULL;
        item_edit->popup_user_data = NULL;

	item_edit->gc = NULL;
	item_edit->style = NULL;

        item_edit->reset_pos = TRUE;
        item_edit->x_offset = 0;

        item_edit->virt_loc.vcell_loc.virt_row = -1;
        item_edit->virt_loc.vcell_loc.virt_col = -1;
        item_edit->virt_loc.phys_row_offset = -1;
        item_edit->virt_loc.phys_col_offset = -1;

#ifdef USE_XIM
        item_edit->ic = NULL;
        item_edit->ic_attr = NULL;
#endif
}


static void
queue_sync (GncItemEdit *item_edit)
{
        GnomeCanvas *canvas = GNOME_CANVAS_ITEM (item_edit)->canvas;
        int x, y, w, h;

        gnc_item_edit_get_pixel_coords (item_edit, &x, &y, &w, &h);
 
        gnome_canvas_request_redraw (canvas, x, y, x+w+1, y+h+1);
}

void
gnc_item_edit_redraw (GncItemEdit *item_edit)
{
        g_return_if_fail (item_edit != NULL);
        g_return_if_fail (GNC_IS_ITEM_EDIT(item_edit));

        queue_sync (item_edit);
}

static void
entry_changed (GtkEntry *entry, void *data)
{
        queue_sync(GNC_ITEM_EDIT (data));
}


static void
gnc_item_edit_dispose (GObject *object)
{
	GncItemEdit *item_edit = GNC_ITEM_EDIT (object);

	g_signal_handlers_disconnect_matched (G_OBJECT (item_edit->editor), G_SIGNAL_MATCH_DATA,
					      0, 0, NULL, NULL, item_edit);

        G_OBJECT_CLASS (gnc_item_edit_parent_class)->dispose (object);
}

static void
gnc_item_edit_finalize (GObject *object)
{
        GncItemEdit *item_edit = GNC_ITEM_EDIT (object);

        if (item_edit->clipboard != NULL) {
                g_free (item_edit->clipboard);
        	item_edit->clipboard = NULL;
	}


        gdk_gc_destroy (item_edit->gc);

        G_OBJECT_CLASS (gnc_item_edit_parent_class)->finalize (object);
}


gboolean
gnc_item_edit_set_cursor_pos (GncItemEdit *item_edit,
                          VirtualLocation virt_loc, int x,
                          gboolean changed_cells,
                          gboolean extend_selection)
{
        GtkEditable *editable;
        TextDrawInfo info;
        Table *table;
        gint pos = 0;
        gint o_x, o_y;
        CellDimensions *cd;
        gint cell_row, cell_col;
        SheetBlockStyle *style;
/*        GdkWChar *text;*/

        g_return_val_if_fail (GNC_IS_ITEM_EDIT(item_edit), FALSE);

        table = item_edit->sheet->table;

	cell_row = virt_loc.phys_row_offset;
	cell_col = virt_loc.phys_col_offset;

        style = gnucash_sheet_get_style (item_edit->sheet, virt_loc.vcell_loc);

        cd = gnucash_style_get_cell_dimensions (style, cell_row, cell_col);

        o_x = cd->origin_x;
        o_y = cd->origin_y;

        if (!virt_loc_equal (virt_loc, item_edit->virt_loc))
                return FALSE;

        editable = GTK_EDITABLE (item_edit->editor);

        if (changed_cells)
        {
                CellAlignment align;

                align = gnc_table_get_align (table, item_edit->virt_loc);

                if (align == CELL_ALIGN_RIGHT)
                        gtk_editable_set_position(editable, -1);
                else
                        gtk_editable_set_position(editable, 0);

                if (item_edit->is_popup)
                        x -= item_edit->popup_toggle.toggle_offset;
        }

        gnc_item_edit_draw_info (item_edit, o_x, o_y, &info);

        if (extend_selection)
        {
                gint current_pos, start_sel, end_sel;

                current_pos = gtk_editable_get_position (editable);
		gtk_editable_get_selection_bounds (editable, &start_sel, &end_sel);

                if (start_sel == end_sel)
                {
                        start_sel = current_pos;
                        end_sel = pos;
                }
                else if (current_pos == start_sel)
                        start_sel = pos;
                else
                        end_sel = pos;

                gtk_editable_select_region(editable, start_sel, end_sel);
        }
        else
                gtk_editable_select_region(editable, 0, 0);

        gtk_editable_set_position (editable, pos);

        queue_sync (item_edit);

        gnc_item_edit_free_draw_info_members(&info);

        return TRUE;
}


static int
entry_event (GtkEntry *entry, GdkEvent *event, GncItemEdit *item_edit)
{
        switch (event->type)
        {
		case GDK_KEY_PRESS:
		case GDK_KEY_RELEASE:
		case GDK_BUTTON_PRESS:
			queue_sync (item_edit);
                        break;

		default:
			break;
        }

        return FALSE;
}


static void
gnc_item_edit_set_editor (GncItemEdit *item_edit, void *data)
{
        item_edit->editor = GTK_WIDGET (data);

        g_signal_connect (G_OBJECT (item_edit->editor), "changed",
                          G_CALLBACK (entry_changed), item_edit);

        g_signal_connect_after (G_OBJECT (item_edit->editor), "event",
                                G_CALLBACK (entry_event), item_edit);
}


void
gnc_item_edit_configure (GncItemEdit *item_edit)
{
        GnucashSheet *sheet = item_edit->sheet;
        GnucashItemCursor *cursor;

        cursor = GNUCASH_ITEM_CURSOR
		(GNUCASH_CURSOR(sheet->cursor)->cursor[GNUCASH_CURSOR_BLOCK]);

        if (item_edit->virt_loc.vcell_loc.virt_row != cursor->row)
        {
                item_edit->virt_loc.vcell_loc.virt_row = cursor->row;
                item_edit->reset_pos = TRUE;
        }

        if (item_edit->virt_loc.vcell_loc.virt_col != cursor->col)
        {
                item_edit->virt_loc.vcell_loc.virt_col = cursor->col;
                item_edit->reset_pos = TRUE;
        }

        item_edit->style =
                gnucash_sheet_get_style (item_edit->sheet,
                                         item_edit->virt_loc.vcell_loc);

        cursor = GNUCASH_ITEM_CURSOR
		(GNUCASH_CURSOR(sheet->cursor)->cursor[GNUCASH_CURSOR_CELL]);

        if (item_edit->virt_loc.phys_row_offset != cursor->row)
        {
                item_edit->virt_loc.phys_row_offset = cursor->row;
                item_edit->reset_pos = TRUE;
        }

        if (item_edit->virt_loc.phys_col_offset != cursor->col)
        {
                item_edit->virt_loc.phys_col_offset = cursor->col;
                item_edit->reset_pos = TRUE;
        }

        if (!gnc_table_is_popup (item_edit->sheet->table, item_edit->virt_loc))
                gnc_item_edit_set_popup (item_edit, NULL, NULL, NULL,
                                     NULL, NULL, NULL, NULL);

        gnc_item_edit_update (GNOME_CANVAS_ITEM(item_edit), NULL, NULL, 0);

#ifdef USE_XIM
        if (item_edit->ic &&
            (gdk_ic_get_style (item_edit->ic) & GDK_IM_PREEDIT_POSITION))
	{
                GnomeCanvasItem *item;

                item = GNOME_CANVAS_ITEM (item_edit);

                item_edit->ic_attr->preedit_area.x = item->x1;
                item_edit->ic_attr->preedit_area.y = item->y1;
                item_edit->ic_attr->preedit_area.width = item->x2 - item->x1;
                item_edit->ic_attr->preedit_area.height = item->y2 - item->y1;

                gdk_ic_set_attr (item_edit->ic, item_edit->ic_attr,
                                 GDK_IC_PREEDIT_AREA);
	}
#endif
}


void
gnc_item_edit_claim_selection (GncItemEdit *item_edit, guint32 time)
{
        GtkEditable *editable;
        gint start_sel, end_sel;

        g_return_if_fail(item_edit != NULL);
        g_return_if_fail(GNC_IS_ITEM_EDIT(item_edit));

        editable = GTK_EDITABLE (item_edit->editor);

	gtk_editable_get_selection_bounds (editable, &start_sel, &end_sel);

        if (start_sel != end_sel)
        {
                gtk_selection_owner_set (GTK_WIDGET(item_edit->sheet),
                                         GDK_SELECTION_PRIMARY, time);
                item_edit->has_selection = TRUE;
        }
        else
        {
                GdkWindow *owner;

                owner = gdk_selection_owner_get (GDK_SELECTION_PRIMARY);
                if (owner == GTK_WIDGET(item_edit->sheet)->window)
                        gtk_selection_owner_set (NULL, GDK_SELECTION_PRIMARY,
                                                 time);
                item_edit->has_selection = FALSE;
        }
}


static void
gnc_item_edit_cut_copy_clipboard (GncItemEdit *item_edit, guint32 time, gboolean cut)
{
        GtkEditable *editable;
        gint start_sel, end_sel;
        gchar *clip;

        g_return_if_fail(item_edit != NULL);
        g_return_if_fail(GNC_IS_ITEM_EDIT(item_edit));

        editable = GTK_EDITABLE (item_edit->editor);

	if (!gtk_editable_get_selection_bounds (editable, &start_sel, &end_sel))
		return;

        g_free(item_edit->clipboard);

        if (gtk_selection_owner_set (GTK_WIDGET(item_edit->sheet),
                                     clipboard_atom, time))
                clip = gtk_editable_get_chars (editable, start_sel, end_sel);
        else
                clip = NULL;

        item_edit->clipboard = clip;

        if (!cut)
                return;

        gtk_editable_delete_text(editable, start_sel, end_sel);
        gtk_editable_select_region(editable, 0, 0);
        gtk_editable_set_position(editable, start_sel);
}


void
gnc_item_edit_cut_clipboard (GncItemEdit *item_edit, guint32 time)
{
        gnc_item_edit_cut_copy_clipboard(item_edit, time, TRUE);
}


void
gnc_item_edit_copy_clipboard (GncItemEdit *item_edit, guint32 time)
{
        gnc_item_edit_cut_copy_clipboard(item_edit, time, FALSE);
}


void
gnc_item_edit_paste_clipboard (GncItemEdit *item_edit, guint32 time)
{
        g_return_if_fail(item_edit != NULL);
        g_return_if_fail(GNC_IS_ITEM_EDIT(item_edit));

        if (ctext_atom == GDK_NONE)
                ctext_atom = gdk_atom_intern ("COMPOUND_TEXT", FALSE);

        gtk_selection_convert(GTK_WIDGET(item_edit->sheet), 
                              clipboard_atom, ctext_atom, time);
}


void
gnc_item_edit_paste_primary (GncItemEdit *item_edit, guint32 time)
{
        g_return_if_fail(item_edit != NULL);
        g_return_if_fail(GNC_IS_ITEM_EDIT(item_edit));

        if (ctext_atom == GDK_NONE)
                ctext_atom = gdk_atom_intern ("COMPOUND_TEXT", FALSE);

        gtk_selection_convert(GTK_WIDGET(item_edit->sheet), 
                              GDK_SELECTION_PRIMARY, ctext_atom, time);
}


static void
gnc_item_edit_show_popup_toggle (GncItemEdit *item_edit,
                             gint x, gint y,
                             gint width, gint height,
                             GtkAnchorType anchor)
{
	g_return_if_fail (GNC_IS_ITEM_EDIT (item_edit));

	gnome_canvas_item_raise_to_top
                (item_edit->popup_toggle.toggle_button_item);

	gnome_canvas_item_set (item_edit->popup_toggle.toggle_button_item,
                               "x", (gdouble) x,
                               "y", (gdouble) y,
                               "width", (gdouble) width,
                               "height", (gdouble) height,
                               "anchor", anchor,
                               NULL);
}


static void
gnc_item_edit_hide_popup_toggle (GncItemEdit *item_edit)
{
	g_return_if_fail (GNC_IS_ITEM_EDIT(item_edit));

	/* safely out of the way */
	gnome_canvas_item_set (item_edit->popup_toggle.toggle_button_item,
                               "x", -10000.0, NULL);
}


static gboolean
key_press_popup_cb (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	GncItemEdit *item_edit = GNC_ITEM_EDIT (data);

	gtk_signal_emit_stop_by_name (GTK_OBJECT(widget), "key_press_event");

	gtk_widget_event (GTK_WIDGET(item_edit->sheet), (GdkEvent *) event);

	return TRUE;
}


static void
gnc_item_edit_popup_toggled (GtkToggleButton *button, gpointer data)
{
	GncItemEdit *item_edit = GNC_ITEM_EDIT (data);
        gboolean show_popup;

	show_popup = gtk_toggle_button_get_active (button);
        if (show_popup)
        {
                Table *table;
                VirtualLocation virt_loc;

                table = item_edit->sheet->table;
                virt_loc = table->current_cursor_loc;

                if (!gnc_table_confirm_change (table, virt_loc))
                {
                        gtk_signal_handler_block_by_data
                                (GTK_OBJECT (button), data);

                        gtk_toggle_button_set_active (button, FALSE);

                        gtk_signal_handler_unblock_by_data
                                (GTK_OBJECT (button), data);

                        return;
                }
        }

        item_edit->show_popup = show_popup;

        if (!item_edit->show_popup)
                gnc_item_edit_hide_popup (item_edit);

	gnc_item_edit_configure (item_edit);
}


static void
block_toggle_signals(GncItemEdit *item_edit)
{
        GtkObject *obj;

        if (!item_edit->popup_toggle.signals_connected)
                return;

        obj = GTK_OBJECT (item_edit->popup_toggle.toggle_button);

        gtk_signal_handler_block_by_data (obj, item_edit);
}


static void
unblock_toggle_signals(GncItemEdit *item_edit)
{
        GtkObject *obj;

        if (!item_edit->popup_toggle.signals_connected)
                return;

        obj = GTK_OBJECT (item_edit->popup_toggle.toggle_button);

        gtk_signal_handler_unblock_by_data (obj, item_edit);
}


static void
connect_popup_toggle_signals (GncItemEdit *item_edit)
{
        GtkObject *object;

	g_return_if_fail(GNC_IS_ITEM_EDIT(item_edit));

	if (item_edit->popup_toggle.signals_connected)
		return;

        object = GTK_OBJECT(item_edit->popup_toggle.toggle_button);

        gtk_signal_connect (object, "toggled",
                            GTK_SIGNAL_FUNC(gnc_item_edit_popup_toggled),
                            item_edit);

	gtk_signal_connect (object, "key_press_event",
                            GTK_SIGNAL_FUNC(key_press_popup_cb),
                            item_edit);

	item_edit->popup_toggle.signals_connected = TRUE;
}


static void
disconnect_popup_toggle_signals (GncItemEdit *item_edit)
{
	g_return_if_fail(GNC_IS_ITEM_EDIT(item_edit));

	if (!item_edit->popup_toggle.signals_connected)
		return;

	gtk_signal_disconnect_by_data
                (GTK_OBJECT(item_edit->popup_toggle.toggle_button), item_edit);

	item_edit->popup_toggle.signals_connected = FALSE;
}

static void
gnc_item_edit_get_property (GObject *object,
			guint param_id,
			GValue *value,
			GParamSpec *pspec)
{
	GncItemEdit *item_edit = GNC_ITEM_EDIT (object);

	switch (param_id) {
		case PROP_SHEET:
			g_value_set_object (value, item_edit->sheet);
			break;
		case PROP_EDITOR:
			g_value_set_object (value, item_edit->editor);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
			break;
	}
}

static void
gnc_item_edit_set_property (GObject *object,
			guint param_id,
			const GValue *value,
			GParamSpec *pspec)
{
	GncItemEdit *item_edit = GNC_ITEM_EDIT (object);

	switch (param_id) {
		case PROP_SHEET:
			item_edit->sheet = GNUCASH_SHEET (g_value_get_object (value));
			break;
		case PROP_EDITOR:
			gnc_item_edit_set_editor (item_edit, GTK_ENTRY (g_value_get_object (value)));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
			break;
	}
}

/*
 * GncItemEdit class initialization
 */
static void
gnc_item_edit_class_init (GncItemEditClass *gnc_item_edit_class)
{
        GObjectClass  *object_class;
        GnomeCanvasItemClass *item_class;

        gnc_item_edit_parent_class = g_type_class_peek_parent (gnc_item_edit_class);
 
        object_class = G_OBJECT_CLASS (gnc_item_edit_class);
        item_class = GNOME_CANVAS_ITEM_CLASS (gnc_item_edit_class);

        object_class->get_property = gnc_item_edit_get_property;
        object_class->set_property = gnc_item_edit_set_property;
        object_class->dispose = gnc_item_edit_dispose;
        object_class->finalize = gnc_item_edit_finalize;

	g_object_class_install_property (object_class,
					 PROP_SHEET,
					 g_param_spec_object ("sheet",
						 	      "Sheet Value",
							      "Sheet Value",
							      GNUCASH_TYPE_SHEET,
							      G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_EDITOR,
					 g_param_spec_object ("editor",
						 	      "Editor Value",
							      "Editor Value",
							      GTK_TYPE_ENTRY,
							      G_PARAM_READWRITE));
 
        /* GnomeCanvasItem method overrides */
        item_class->update      = gnc_item_edit_update;
        item_class->draw        = gnc_item_edit_draw;
        item_class->point       = gnc_item_edit_point;
        item_class->realize     = gnc_item_edit_realize;
        item_class->unrealize   = gnc_item_edit_unrealize;
        item_class->event       = gnc_item_edit_event;
}


GtkType
gnc_item_edit_get_type (void)
{
        static GtkType gnc_item_edit_type = 0;

        if (!gnc_item_edit_type) {
                GtkTypeInfo gnc_item_edit_info = {
                        "GncItemEdit",
                        sizeof (GncItemEdit),
                        sizeof (GncItemEditClass),
                        (GtkClassInitFunc) gnc_item_edit_class_init,
                        (GtkObjectInitFunc) gnc_item_edit_init,
                        NULL, /* reserved_1 */
                        NULL, /* reserved_2 */
                        (GtkClassInitFunc) NULL
                };

                gnc_item_edit_type =
			gtk_type_unique(gnome_canvas_item_get_type (),
					&gnc_item_edit_info);
        }

        return gnc_item_edit_type;
}


static void
create_popup_toggle(GnomeCanvasGroup *parent, PopupToggle *pt)
{
        GtkWidget *button, *arrow;

        arrow = gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_IN);
        gtk_misc_set_alignment(GTK_MISC(arrow), 0.5, 0.5);
        pt->arrow = GTK_ARROW(arrow);

        button = gtk_toggle_button_new();
        pt->toggle_button = GTK_TOGGLE_BUTTON(button);
        gtk_container_add(GTK_CONTAINER(button), arrow);

        pt->toggle_button_item =
                gnome_canvas_item_new(parent, gnome_canvas_widget_get_type(),
                                      "widget", button,
                                      "size_pixels", TRUE,
                                      NULL);
}


GnomeCanvasItem *
gnc_item_edit_new (GnomeCanvasGroup *parent, GnucashSheet *sheet, GtkWidget *entry)
{
        static const GtkTargetEntry targets[] = {
                { "STRING", 0, TARGET_STRING },
                { "TEXT",   0, TARGET_TEXT }, 
                { "COMPOUND_TEXT", 0, TARGET_COMPOUND_TEXT }
        };
        static const gint n_targets = sizeof(targets) / sizeof(targets[0]);

        GnomeCanvasItem *item;
        GncItemEdit *item_edit;

        item = gnome_canvas_item_new (parent,
				      GNC_TYPE_ITEM_EDIT,
				      "sheet", sheet,
				      "editor", sheet->entry,
				      NULL);

        item_edit = GNC_ITEM_EDIT(item);

	item_edit->parent = parent;

	create_popup_toggle (parent, &item_edit->popup_toggle);

        if (clipboard_atom == GDK_NONE)
		clipboard_atom = gdk_atom_intern ("CLIPBOARD", FALSE);

        gtk_selection_add_targets (GTK_WIDGET(sheet),
                                   GDK_SELECTION_PRIMARY,
                                   targets, n_targets);

        gtk_selection_add_targets (GTK_WIDGET(sheet),
                                   clipboard_atom,
                                   targets, n_targets);

        return item;
}


GncItemList *
gnc_item_edit_new_list (GncItemEdit *item_edit)
{
        GncItemList *item_list;

	g_return_val_if_fail (GNC_IS_ITEM_EDIT(item_edit), NULL);

        item_list = GNC_ITEM_LIST (gnc_item_list_new (item_edit->parent));

	return item_list;
}

GNCDatePicker *
gnc_item_edit_new_date_picker (GncItemEdit *item_edit)
{
        GNCDatePicker *gdp;

	g_return_val_if_fail (GNC_IS_ITEM_EDIT (item_edit), NULL);

        gdp = GNC_DATE_PICKER (gnc_date_picker_new (item_edit->parent));

	return gdp;
}


void
gnc_item_edit_show_popup (GncItemEdit *item_edit)
{
        GtkToggleButton *toggle;
        GtkAnchorType popup_anchor;
        GnucashSheet *sheet;
        gint x, y, w, h;
        gint y_offset;
        gint popup_x, popup_y;
        gint popup_width;
        gint popup_height;
        gint popup_max_width;
        gint view_height;
        gint view_width;
        gint up_height;
        gint down_height;

        g_return_if_fail (item_edit != NULL);
	g_return_if_fail (GNC_IS_ITEM_EDIT(item_edit));

        if (!item_edit->is_popup)
                return;

        sheet = item_edit->sheet;

        view_height = GTK_WIDGET (sheet)->allocation.height;
        view_width  = GTK_WIDGET (sheet)->allocation.width;

        gnome_canvas_get_scroll_offsets (GNOME_CANVAS(sheet), NULL, &y_offset);
        gnc_item_edit_get_pixel_coords (item_edit, &x, &y, &w, &h);

	popup_x = x;

        up_height = y - y_offset;
        down_height = view_height - (up_height + h);

	if (up_height > down_height)
        {
		popup_y = y;
		popup_anchor = GTK_ANCHOR_SW;
                popup_height = up_height;
	}
	else
        {
                popup_y = y + h;
		popup_anchor = GTK_ANCHOR_NW;
                popup_height = down_height;
        }

        popup_max_width = view_width - popup_x;

        if (item_edit->get_popup_height)
                popup_height = item_edit->get_popup_height
                        (item_edit->popup_item, popup_height, h,
                         item_edit->popup_user_data);

        if (item_edit->popup_autosize)
                popup_width =
                        item_edit->popup_autosize (item_edit->popup_item,
                                                   popup_max_width,
                                                   item_edit->popup_user_data);
        else
                popup_width = 0;

        if (popup_width > 0)
                gnome_canvas_item_set (item_edit->popup_item,
                                       "x", (gdouble) popup_x,
                                       "y", (gdouble) popup_y,
                                       "height", (gdouble) popup_height,
                                       "width", (gdouble) popup_width,
                                       "anchor", popup_anchor,
                                       NULL);
        else
                gnome_canvas_item_set (item_edit->popup_item,
                                       "x", (gdouble) popup_x,
                                       "y", (gdouble) popup_y,
                                       "height", (gdouble) popup_height,
                                       "anchor", popup_anchor,
                                       NULL);

        toggle = item_edit->popup_toggle.toggle_button;

        if (!gtk_toggle_button_get_active (toggle))
        {
                block_toggle_signals (item_edit);
                gtk_toggle_button_set_active (toggle, TRUE);
                unblock_toggle_signals (item_edit);
        }

	gtk_arrow_set (item_edit->popup_toggle.arrow,
                       GTK_ARROW_UP, GTK_SHADOW_OUT);

        if (item_edit->popup_set_focus)
                item_edit->popup_set_focus (item_edit->popup_item,
                                            item_edit->popup_user_data);

        if (item_edit->popup_post_show)
                item_edit->popup_post_show (item_edit->popup_item,
                                            item_edit->popup_user_data);

        if (item_edit->popup_get_width)
        {
                int popup_width;

                popup_width = item_edit->popup_get_width
                        (item_edit->popup_item,
                         item_edit->popup_user_data);

                if (popup_width > popup_max_width)
                {
                        popup_x -= popup_width - popup_max_width;
                        popup_x = MAX (0, popup_x);

                        gnome_canvas_item_set (item_edit->popup_item,
                                               "x", (gdouble) popup_x,
                                               NULL);
                }
        }
}


void
gnc_item_edit_hide_popup (GncItemEdit *item_edit)
{
        g_return_if_fail(item_edit != NULL);
	g_return_if_fail(GNC_IS_ITEM_EDIT(item_edit));

        if (!item_edit->is_popup)
                return;

        gnome_canvas_item_set (item_edit->popup_item, "x", -10000.0, NULL);

	gtk_arrow_set (item_edit->popup_toggle.arrow,
                       GTK_ARROW_DOWN, GTK_SHADOW_IN);

        gtk_toggle_button_set_active
                (item_edit->popup_toggle.toggle_button, FALSE);

        gtk_widget_grab_focus (GTK_WIDGET (item_edit->sheet));
}

void
gnc_item_edit_set_popup (GncItemEdit        *item_edit,
                     GnomeCanvasItem *popup_item,
                     GetPopupHeight   get_popup_height,
                     PopupAutosize    popup_autosize,
                     PopupSetFocus    popup_set_focus,
                     PopupPostShow    popup_post_show,
                     PopupGetWidth    popup_get_width,
                     gpointer         popup_user_data)
{
	g_return_if_fail (GNC_IS_ITEM_EDIT(item_edit));

        if (item_edit->is_popup)
                gnc_item_edit_hide_popup (item_edit);

        item_edit->is_popup = popup_item != NULL;

        item_edit->popup_item       = popup_item;
        item_edit->get_popup_height = get_popup_height;
        item_edit->popup_autosize   = popup_autosize;
        item_edit->popup_set_focus  = popup_set_focus;
        item_edit->popup_post_show  = popup_post_show;
        item_edit->popup_get_width  = popup_get_width;
        item_edit->popup_user_data  = popup_user_data;

        if (item_edit->is_popup)
                connect_popup_toggle_signals (item_edit);
        else
        {
                disconnect_popup_toggle_signals (item_edit);

                gnc_item_edit_hide_popup (item_edit);
                gnc_item_edit_hide_popup_toggle (item_edit);
        }

        gnc_item_edit_update (GNOME_CANVAS_ITEM (item_edit), NULL, NULL, 0);
}

void
gnc_item_edit_set_has_selection (GncItemEdit *item_edit, gboolean has_selection)
{
        g_return_if_fail (item_edit != NULL);
        g_return_if_fail (GNC_IS_ITEM_EDIT (item_edit));

        item_edit->has_selection = has_selection;
}

gboolean
gnc_item_edit_selection_clear (GncItemEdit          *item_edit,
                           GdkEventSelection *event)
{
        g_return_val_if_fail(item_edit != NULL, FALSE);
        g_return_val_if_fail(GNC_IS_ITEM_EDIT(item_edit), FALSE);
        g_return_val_if_fail(event != NULL, FALSE);

        /* Let the selection handling code know that the selection
         * has been changed, since we've overriden the default handler */
        if (!gtk_selection_clear (GTK_WIDGET(item_edit->sheet), event))
                return FALSE;

        if (event->selection == GDK_SELECTION_PRIMARY)
        {
                if (item_edit->has_selection)
                {
                        item_edit->has_selection = FALSE;
                        /* TODO: redraw differently? */
                }
        }
        else if (event->selection == clipboard_atom)
        {
                g_free (item_edit->clipboard);
                item_edit->clipboard = NULL;
        }

        return TRUE;
}


void
gnc_item_edit_selection_get (GncItemEdit         *item_edit,
			     GtkSelectionData *selection_data,
			     guint             info,
			     guint             time)
{
        GtkEditable *editable;

        gint start_pos;
        gint end_pos;

        gchar *str;
        gint length;

        g_return_if_fail(item_edit != NULL);
        g_return_if_fail(GNC_IS_ITEM_EDIT(item_edit));

        editable = GTK_EDITABLE (item_edit->editor);

        if (selection_data->selection == GDK_SELECTION_PRIMARY)
        {
		gtk_editable_get_selection_bounds (editable, &start_pos, &end_pos);

                str = gtk_editable_get_chars(editable, start_pos, end_pos);
        }
        else /* CLIPBOARD */
                str = item_edit->clipboard;

        if (str == NULL)
                return;

        length = strlen(str);
  
        if (info == TARGET_STRING)
        {
                gtk_selection_data_set (selection_data,
                                        GDK_SELECTION_TYPE_STRING,
                                        8 * sizeof(gchar), (guchar *) str,
                                        length);
        }
        else if ((info == TARGET_TEXT) || (info == TARGET_COMPOUND_TEXT))
        {
                guchar *text;
                gchar c;
                GdkAtom encoding;
                gint format;
                gint new_length;

                c = str[length];
                str[length] = '\0';

                gdk_string_to_compound_text(str, &encoding, &format,
                                            &text, &new_length);

                gtk_selection_data_set(selection_data, encoding,
                                       format, text, new_length);

                gdk_free_compound_text(text);

                str[length] = c;
        }

        if (str != item_edit->clipboard)
                g_free(str);
}


void
gnc_item_edit_selection_received (GncItemEdit       *item_edit,
				  GtkSelectionData  *selection_data,
				  guint              time)
{
        GtkEditable *editable;
        gboolean reselect;
        gint old_pos;
        gint tmp_pos;
	gint start_sel, end_sel;
        enum {INVALID, STRING, CTEXT} type;

        g_return_if_fail(item_edit != NULL);
        g_return_if_fail(GNC_IS_ITEM_EDIT(item_edit));

        editable = GTK_EDITABLE(item_edit->editor);

        if (selection_data->type == GDK_TARGET_STRING)
                type = STRING;
        else if ((selection_data->type ==
                  gdk_atom_intern("COMPOUND_TEXT", FALSE)) ||
                 (selection_data->type == gdk_atom_intern("TEXT", FALSE)))
                type = CTEXT;
        else
                type = INVALID;

        if (type == INVALID || selection_data->length < 0)
        {
                /* avoid infinite loop */
                if (selection_data->target != GDK_TARGET_STRING)
                        gtk_selection_convert(GTK_WIDGET(item_edit->sheet),
                                              selection_data->selection,
                                              GDK_TARGET_STRING, time);
                return;
        }

        reselect = FALSE;

        if (gtk_editable_get_selection_bounds (editable, &start_sel, &end_sel) && 
            (!item_edit->has_selection || 
             (selection_data->selection == clipboard_atom)))
        {
                reselect = TRUE;

                gtk_editable_delete_text(editable, start_sel, end_sel);
        }

        tmp_pos = old_pos = gtk_editable_get_position (editable);

        switch (type)
        {
                case STRING:
                        selection_data->data[selection_data->length] = 0;

                        gtk_editable_insert_text
                                (editable, (gchar *) selection_data->data,
                                 strlen((gchar *)selection_data->data),
                                 &tmp_pos);

                        gtk_editable_set_position(editable, tmp_pos);
                        break;
                case CTEXT: {
                        gchar **list;
                        gint count;
                        gint i;

                        count = gdk_text_property_to_text_list
                                (selection_data->type, selection_data->format, 
                                 selection_data->data, selection_data->length,
                                 &list);

                        for (i = 0; i < count; i++) 
                        {
                                gtk_editable_insert_text(editable,
                                                         list[i],
                                                         strlen(list[i]),
                                                         &tmp_pos);

                                gtk_editable_set_position(editable, tmp_pos);
                        }

                        if (count > 0)
                                gdk_free_text_list(list);
                }
                break;
                case INVALID: /* quiet compiler */
                        break;
        }

        if (!reselect)
                return;

        gtk_editable_select_region(editable, old_pos, gtk_editable_get_position (editable));
}


/*
  Local Variables:
  c-basic-offset: 8
  End:
*/
