/* 
 * gnc-tree-model-selection.c -- GtkTreeModel which supports a selectable column.
 * Copyright (C) 2003 Jan Arne Petersen
 * Author: Jan Arne Petersen <jpetersen@uni-bonn.de>
 */

#include "config.h"

#include <gtk/gtkcellrenderertoggle.h>

#include "gnc-tree-model-selection.h"

static void gnc_tree_model_selection_class_init (GncTreeModelSelectionClass *klass);
static void gnc_tree_model_selection_init (GncTreeModelSelection *model);
static void gnc_tree_model_selection_finalize (GObject *object);

static void gnc_tree_model_selection_tree_model_init (GtkTreeModelIface *iface);
static guint gnc_tree_model_selection_get_flags (GtkTreeModel *tree_model);
static int gnc_tree_model_selection_get_n_columns (GtkTreeModel *tree_model);
static GType gnc_tree_model_selection_get_column_type (GtkTreeModel *tree_model,
						       int index);
static gboolean gnc_tree_model_selection_get_iter (GtkTreeModel *tree_model,
						   GtkTreeIter *iter,
						   GtkTreePath *path);
static GtkTreePath *gnc_tree_model_selection_get_path (GtkTreeModel *tree_model,
						       GtkTreeIter *iter);
static void gnc_tree_model_selection_get_value (GtkTreeModel *tree_model,
						GtkTreeIter *iter,
						int column,
						GValue *value);
static gboolean	gnc_tree_model_selection_iter_next (GtkTreeModel *tree_model,
						    GtkTreeIter *iter);
static gboolean	gnc_tree_model_selection_iter_children (GtkTreeModel *tree_model,
							GtkTreeIter *iter,
							GtkTreeIter *parent);
static gboolean	gnc_tree_model_selection_iter_has_child (GtkTreeModel *tree_model,
							 GtkTreeIter *iter);
static int gnc_tree_model_selection_iter_n_children (GtkTreeModel *tree_model,
						     GtkTreeIter *iter);
static gboolean	gnc_tree_model_selection_iter_nth_child (GtkTreeModel *tree_model,
							 GtkTreeIter *iter,
							 GtkTreeIter *parent,
							 int n);
static gboolean	gnc_tree_model_selection_iter_parent (GtkTreeModel *tree_model,
						      GtkTreeIter *iter,
    						      GtkTreeIter *child);

static void gnc_tree_model_selection_row_changed (GtkTreeModel *tree_model,
						  GtkTreePath *path,
						  GtkTreeIter *iter,
						  GncTreeModelSelection *selection_model);
static void gnc_tree_model_selection_row_inserted (GtkTreeModel *tree_model,
						   GtkTreePath *path,
						   GtkTreeIter *iter,
						   GncTreeModelSelection *selection_model);
static void gnc_tree_model_selection_row_has_child_toggled (GtkTreeModel *tree_model,
							    GtkTreePath *path,
							    GtkTreeIter *iter,
							    GncTreeModelSelection *selection_model);
static void gnc_tree_model_selection_row_deleted (GtkTreeModel *tree_model,
						  GtkTreePath *path,
						  GncTreeModelSelection *selection_model);
static void gnc_tree_model_selection_rows_reordered (GtkTreeModel *tree_model,
						     GtkTreePath *path,
						     GtkTreeIter *iter,
						     gint *new_order,
						     GncTreeModelSelection *selection_model);

static void gnc_tree_model_selection_toggled (GtkCellRendererToggle *toggle,
					      gchar *path,
					      GncTreeModelSelection *model);

struct GncTreeModelSelectionPrivate
{
	GtkTreeModel *child_model;

	GHashTable *selections;
};

static GObjectClass *parent_class = NULL;

GType
gnc_tree_model_selection_get_type (void)
{
	static GType gnc_tree_model_selection_type = 0;

	if (gnc_tree_model_selection_type == 0) {
		static const GTypeInfo our_info = {
			sizeof (GncTreeModelSelectionClass),
			NULL,
			NULL,
			(GClassInitFunc) gnc_tree_model_selection_class_init,
			NULL,
			NULL,
			sizeof (GncTreeModelSelection),
			0,
			(GInstanceInitFunc) gnc_tree_model_selection_init
		};
		
		static const GInterfaceInfo tree_model_info = {
			(GInterfaceInitFunc) gnc_tree_model_selection_tree_model_init,
			NULL,
			NULL
		};

		gnc_tree_model_selection_type = g_type_register_static (G_TYPE_OBJECT,
								        "GncTreeModelSelection",
								        &our_info, 0);
		
		g_type_add_interface_static (gnc_tree_model_selection_type,
					     GTK_TYPE_TREE_MODEL,
					     &tree_model_info);
	}

	return gnc_tree_model_selection_type;
}

static void
gnc_tree_model_selection_class_init (GncTreeModelSelectionClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = gnc_tree_model_selection_finalize;
}

static void
gnc_tree_model_selection_init (GncTreeModelSelection *model)
{
	while (model->stamp == 0) {
		model->stamp = g_random_int ();
	}

	model->priv = g_new0 (GncTreeModelSelectionPrivate, 1);
	model->priv->selections = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
}

static void
gnc_tree_model_selection_finalize (GObject *object)
{
	GncTreeModelSelection *model;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNC_IS_TREE_MODEL_SELECTION (object));

	model = GNC_TREE_MODEL_SELECTION (object);

	g_return_if_fail (model->priv != NULL);

	g_object_unref (model->priv->child_model);
	g_hash_table_destroy (model->priv->selections);
	g_free (model->priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkTreeModel *
gnc_tree_model_selection_new (GtkTreeModel *child_model)
{
	GncTreeModelSelection *model;

	model = g_object_new (GNC_TYPE_TREE_MODEL_SELECTION, NULL);

	model->priv->child_model = child_model;
	g_object_ref (child_model);
	g_signal_connect (G_OBJECT (child_model), "row_changed",
			  G_CALLBACK (gnc_tree_model_selection_row_changed), model);
	g_signal_connect (G_OBJECT (child_model), "row_inserted",
			  G_CALLBACK (gnc_tree_model_selection_row_inserted), model);
	g_signal_connect (G_OBJECT (child_model), "row_has_child_toggled",
			  G_CALLBACK (gnc_tree_model_selection_row_has_child_toggled), model);
	g_signal_connect (G_OBJECT (child_model), "row_deleted",
			  G_CALLBACK (gnc_tree_model_selection_row_deleted), model);
	g_signal_connect (G_OBJECT (child_model), "rows_reordered",
			  G_CALLBACK (gnc_tree_model_selection_rows_reordered), model);

	return GTK_TREE_MODEL (model);
}

GtkTreeModel *
gnc_tree_model_selection_get_model (GncTreeModelSelection *model)
{
	g_return_val_if_fail (GNC_IS_TREE_MODEL_SELECTION (model), NULL);

	return model->priv->child_model;
}

void
gnc_tree_model_selection_convert_child_iter_to_iter (GncTreeModelSelection *model,
						     GtkTreeIter *selection_iter,
						     GtkTreeIter *child_iter)
{
	g_return_if_fail (GNC_IS_TREE_MODEL_SELECTION (model));
	g_return_if_fail (child_iter != NULL);
	g_return_if_fail (selection_iter != NULL);

	selection_iter->stamp = model->stamp;

	selection_iter->user_data = gtk_tree_iter_copy (child_iter);
}

void
gnc_tree_model_selection_convert_iter_to_child_iter (GncTreeModelSelection *model,
						     GtkTreeIter *child_iter,
						     GtkTreeIter *selection_iter)
{
	g_return_if_fail (GNC_IS_TREE_MODEL_SELECTION (model));
	g_return_if_fail (selection_iter != NULL);
	g_return_if_fail (GNC_TREE_MODEL_SELECTION (model)->stamp == selection_iter->stamp);
	g_return_if_fail (selection_iter->user_data != NULL);
	g_return_if_fail (child_iter != NULL);

	child_iter->stamp = ((GtkTreeIter *) selection_iter->user_data)->stamp;
	child_iter->user_data = ((GtkTreeIter *) selection_iter->user_data)->user_data;
	child_iter->user_data2 = ((GtkTreeIter *) selection_iter->user_data)->user_data2;
	child_iter->user_data3 = ((GtkTreeIter *) selection_iter->user_data)->user_data3;
}

gint
gnc_tree_model_selection_get_selection_row (GncTreeModelSelection *model)
{
	g_return_val_if_fail (GNC_IS_TREE_MODEL_SELECTION (model), 0);

	return gtk_tree_model_get_n_columns (model->priv->child_model);
}

GtkTreeViewColumn *
gnc_tree_model_selection_create_tree_view_column (GncTreeModelSelection *model,
						  const gchar *title)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	renderer = gtk_cell_renderer_toggle_new ();
	g_object_set (G_OBJECT (renderer), "activatable", TRUE);
	g_signal_connect (G_OBJECT (renderer), "toggled",
			  G_CALLBACK (gnc_tree_model_selection_toggled), model);

	column = gtk_tree_view_column_new_with_attributes (title,
							   renderer,
							   "active",
							   gnc_tree_model_selection_get_selection_row (model),
							   NULL);

	return column;
}

gboolean
gnc_tree_model_selection_is_selected  (GncTreeModelSelection *model,
				       GtkTreeIter *iter)
{
	gchar *path;
	gboolean selected;
	
	path = gtk_tree_model_get_string_from_iter (GTK_TREE_MODEL (model), iter);
	selected = g_hash_table_lookup (model->priv->selections, path) != NULL;
	g_free (path);

	return selected;
}

void
gnc_tree_model_selection_set_selected (GncTreeModelSelection *model,
				       GtkTreeIter *iter,
				       gboolean selected)
{
	gchar *path_string;
	GtkTreePath *path;
	
	path_string = gtk_tree_model_get_string_from_iter (GTK_TREE_MODEL (model), iter);

	if (selected == (g_hash_table_lookup (model->priv->selections, path_string) != NULL)) {
		g_free (path_string);
		return;
	}

	if (selected) {
		g_hash_table_insert (model->priv->selections, g_strdup (path_string), GINT_TO_POINTER (1));
	} else {
		g_hash_table_remove (model->priv->selections, path_string);
	}

	path = gtk_tree_path_new_from_string (path_string);
	gtk_tree_model_row_changed (GTK_TREE_MODEL (model), path, iter);
	gtk_tree_path_free (path);

	g_free (path_string);
}

static void
gnc_tree_model_selection_tree_model_init (GtkTreeModelIface *iface)
{
	iface->get_flags       = gnc_tree_model_selection_get_flags;
	iface->get_n_columns   = gnc_tree_model_selection_get_n_columns;
	iface->get_column_type = gnc_tree_model_selection_get_column_type;
	iface->get_iter        = gnc_tree_model_selection_get_iter;
	iface->get_path        = gnc_tree_model_selection_get_path;
	iface->get_value       = gnc_tree_model_selection_get_value;
	iface->iter_next       = gnc_tree_model_selection_iter_next;
	iface->iter_children   = gnc_tree_model_selection_iter_children;
	iface->iter_has_child  = gnc_tree_model_selection_iter_has_child;
	iface->iter_n_children = gnc_tree_model_selection_iter_n_children;
	iface->iter_nth_child  = gnc_tree_model_selection_iter_nth_child;
	iface->iter_parent     = gnc_tree_model_selection_iter_parent;
}

static guint
gnc_tree_model_selection_get_flags (GtkTreeModel *tree_model)
{
	GncTreeModelSelection *model;

	g_return_val_if_fail (GNC_IS_TREE_MODEL_SELECTION (tree_model), 0);

	model = GNC_TREE_MODEL_SELECTION (tree_model);

	return gtk_tree_model_get_flags (model->priv->child_model);
}

static int
gnc_tree_model_selection_get_n_columns (GtkTreeModel *tree_model)
{
	GncTreeModelSelection *model;

	g_return_val_if_fail (GNC_IS_TREE_MODEL_SELECTION (tree_model), 0);

	model = GNC_TREE_MODEL_SELECTION (tree_model);

	return gtk_tree_model_get_n_columns (model->priv->child_model) + 1;
}

static GType
gnc_tree_model_selection_get_column_type (GtkTreeModel *tree_model,
					  int index)
{
	GncTreeModelSelection *model;
	gint columns = gnc_tree_model_selection_get_n_columns (tree_model);
	
	g_return_val_if_fail (GNC_IS_TREE_MODEL_SELECTION (tree_model), G_TYPE_INVALID);
	g_return_val_if_fail ((index >= 0) && (index < columns), G_TYPE_INVALID);

	model = GNC_TREE_MODEL_SELECTION (tree_model);

	if (index < columns - 1) {
		return gtk_tree_model_get_column_type (model->priv->child_model, index);
	} else {
		return G_TYPE_BOOLEAN;
	}
}

static gboolean
gnc_tree_model_selection_get_iter (GtkTreeModel *tree_model,
				   GtkTreeIter *iter,
				   GtkTreePath *path)
{
	GncTreeModelSelection *model;
	GtkTreeIter child_iter;

	g_return_val_if_fail (GNC_IS_TREE_MODEL_SELECTION (tree_model), FALSE);

	model = GNC_TREE_MODEL_SELECTION (tree_model);

	if (!gtk_tree_model_get_iter (model->priv->child_model, &child_iter, path)) {
		return FALSE;
	}

	gnc_tree_model_selection_convert_child_iter_to_iter (model, iter, &child_iter);

	return TRUE;
}

static GtkTreePath *
gnc_tree_model_selection_get_path (GtkTreeModel *tree_model,
				   GtkTreeIter *iter)
{
	GncTreeModelSelection *model;
	GtkTreeIter child_iter;

	g_return_val_if_fail (GNC_IS_TREE_MODEL_SELECTION (tree_model), NULL);
	g_return_val_if_fail (iter != NULL, NULL);
	g_return_val_if_fail (iter->stamp == GNC_TREE_MODEL_SELECTION (tree_model)->stamp, NULL);

	model = GNC_TREE_MODEL_SELECTION (tree_model);

	gnc_tree_model_selection_convert_iter_to_child_iter (model, &child_iter, iter);

	return gtk_tree_model_get_path (model->priv->child_model, &child_iter);
}

static void
gnc_tree_model_selection_get_value (GtkTreeModel *tree_model,
				    GtkTreeIter *iter,
				    int column,
				    GValue *value)
{
	gint columns = gnc_tree_model_selection_get_n_columns (tree_model);
	GncTreeModelSelection *model;
	GtkTreeIter child_iter;
	gchar *path;

	g_return_if_fail (GNC_IS_TREE_MODEL_SELECTION (tree_model));
	g_return_if_fail ((column >= 0) && (column < columns));
	g_return_if_fail (iter != NULL);
	g_return_if_fail (iter->stamp == GNC_TREE_MODEL_SELECTION (tree_model)->stamp);

	model = GNC_TREE_MODEL_SELECTION (tree_model);

	gnc_tree_model_selection_convert_iter_to_child_iter (model, &child_iter, iter);

	if (column < columns - 1) {
		gtk_tree_model_get_value (model->priv->child_model, &child_iter, column, value);
	} else {
		g_value_init (value, G_TYPE_BOOLEAN);

		path = gtk_tree_model_get_string_from_iter (model->priv->child_model, &child_iter);
		g_value_set_boolean (value, g_hash_table_lookup (model->priv->selections, path) != NULL);
		g_free (path);
	}
}

static gboolean
gnc_tree_model_selection_iter_next (GtkTreeModel *tree_model,
				    GtkTreeIter *iter)
{
	GncTreeModelSelection *model;
	GtkTreeIter child_iter;

	g_return_val_if_fail (GNC_IS_TREE_MODEL_SELECTION (tree_model), FALSE);
	g_return_val_if_fail (iter != NULL, FALSE);
	g_return_val_if_fail (iter->stamp == GNC_TREE_MODEL_SELECTION (tree_model)->stamp, FALSE);

	model = GNC_TREE_MODEL_SELECTION (tree_model);

	gnc_tree_model_selection_convert_iter_to_child_iter (model, &child_iter, iter);

	if (!gtk_tree_model_iter_next (model->priv->child_model, &child_iter)) {
		return FALSE;
	} else {
		gnc_tree_model_selection_convert_child_iter_to_iter (model, iter, &child_iter);

		return TRUE;
	}
}

static gboolean
gnc_tree_model_selection_iter_children (GtkTreeModel *tree_model,
					GtkTreeIter *iter,
					GtkTreeIter *parent)
{
	GncTreeModelSelection *model;
	GtkTreeIter child_iter;
	GtkTreeIter child_parent;

	g_return_val_if_fail (GNC_IS_TREE_MODEL_SELECTION (tree_model), FALSE);

	model = GNC_TREE_MODEL_SELECTION (tree_model);

	if (parent == NULL) {
		if (!gtk_tree_model_iter_children (model->priv->child_model, &child_iter, NULL))
			return FALSE;
	} else {
		g_return_val_if_fail (iter != NULL, FALSE);
		g_return_val_if_fail (iter->stamp == model->stamp, FALSE);

		gnc_tree_model_selection_convert_iter_to_child_iter (model, &child_parent, parent);

		if (!gtk_tree_model_iter_children (model->priv->child_model, &child_iter, &child_parent))
			return FALSE;
	}

	gnc_tree_model_selection_convert_child_iter_to_iter (model, iter, &child_iter);

	return TRUE;
}

static gboolean
gnc_tree_model_selection_iter_has_child (GtkTreeModel *tree_model,
					 GtkTreeIter *iter)
{
	GncTreeModelSelection *model;
	GtkTreeIter child_iter;

	g_return_val_if_fail (GNC_IS_TREE_MODEL_SELECTION (tree_model), FALSE);
	g_return_val_if_fail (iter != NULL, FALSE);
	g_return_val_if_fail (iter->stamp == GNC_TREE_MODEL_SELECTION (tree_model)->stamp, FALSE);

	model = GNC_TREE_MODEL_SELECTION (tree_model);

	gnc_tree_model_selection_convert_iter_to_child_iter (model, &child_iter, iter);

	return gtk_tree_model_iter_has_child (model->priv->child_model, &child_iter);
}

static int
gnc_tree_model_selection_iter_n_children (GtkTreeModel *tree_model,
					  GtkTreeIter *iter)
{
	GncTreeModelSelection *model;
	GtkTreeIter child_iter;

	g_return_val_if_fail (GNC_IS_TREE_MODEL_SELECTION (tree_model), 0);

	model = GNC_TREE_MODEL_SELECTION (tree_model);

	if (iter == NULL) {
		return gtk_tree_model_iter_n_children (model->priv->child_model, NULL);
	} else {
		g_return_val_if_fail (iter != NULL, 0);
		g_return_val_if_fail (iter->stamp == model->stamp, 0);

		gnc_tree_model_selection_convert_iter_to_child_iter (model, &child_iter, iter);

		return gtk_tree_model_iter_n_children (model->priv->child_model, &child_iter);
	}
}

static gboolean
gnc_tree_model_selection_iter_nth_child (GtkTreeModel *tree_model,
					 GtkTreeIter *iter,
					 GtkTreeIter *parent,
					 int n)
{
	GncTreeModelSelection *model;
	GtkTreeIter child_iter;
	GtkTreeIter child_parent;

	g_return_val_if_fail (GNC_IS_TREE_MODEL_SELECTION (tree_model), FALSE);

	model = GNC_TREE_MODEL_SELECTION (tree_model);

	if (parent == NULL) {
		if (!gtk_tree_model_iter_nth_child (model->priv->child_model, &child_iter, NULL, n))
			return FALSE;
	} else {
		g_return_val_if_fail (iter != NULL, FALSE);
		g_return_val_if_fail (iter->stamp == model->stamp, FALSE);

		gnc_tree_model_selection_convert_iter_to_child_iter (model, &child_parent, parent);

		if (!gtk_tree_model_iter_nth_child (model->priv->child_model, &child_iter, &child_parent, n))
			return FALSE;
	}

	gnc_tree_model_selection_convert_child_iter_to_iter (model, iter, &child_iter);

	return TRUE;
}

static gboolean
gnc_tree_model_selection_iter_parent (GtkTreeModel *tree_model,
				      GtkTreeIter *iter,
				      GtkTreeIter *child)
{
	GncTreeModelSelection *model;
	GtkTreeIter child_child;
	GtkTreeIter child_iter;

	g_return_val_if_fail (GNC_IS_TREE_MODEL_SELECTION (tree_model), FALSE);
	g_return_val_if_fail (child != NULL, FALSE);
	g_return_val_if_fail (child->stamp == GNC_TREE_MODEL_SELECTION (tree_model)->stamp, FALSE);

	model = GNC_TREE_MODEL_SELECTION (tree_model);

	gnc_tree_model_selection_convert_iter_to_child_iter (model, &child_child, child);

	if (!gtk_tree_model_iter_parent (model->priv->child_model, &child_iter, &child_child)) {
		return FALSE;
	} else {
		gnc_tree_model_selection_convert_child_iter_to_iter (model, iter, &child_iter);

		return TRUE;
	}
}

static void
gnc_tree_model_selection_row_changed (GtkTreeModel *tree_model,
				      GtkTreePath *path,
				      GtkTreeIter *iter,
				      GncTreeModelSelection *selection_model)
{
	GtkTreeIter selection_iter;

	gnc_tree_model_selection_convert_child_iter_to_iter (selection_model, &selection_iter, iter);

	gtk_tree_model_row_changed (GTK_TREE_MODEL (selection_model), path, &selection_iter);
}

static void
gnc_tree_model_selection_row_inserted (GtkTreeModel *tree_model,
				       GtkTreePath *path,
				       GtkTreeIter *iter,
				       GncTreeModelSelection *selection_model)
{
	GtkTreeIter selection_iter;

	gnc_tree_model_selection_convert_child_iter_to_iter (selection_model, &selection_iter, iter);

	gtk_tree_model_row_inserted (GTK_TREE_MODEL (selection_model), path, &selection_iter);
}

static void
gnc_tree_model_selection_row_has_child_toggled (GtkTreeModel *tree_model,
						GtkTreePath *path,
						GtkTreeIter *iter,
						GncTreeModelSelection *selection_model)
{
	GtkTreeIter selection_iter;

	gnc_tree_model_selection_convert_child_iter_to_iter (selection_model, &selection_iter, iter);

	gtk_tree_model_row_has_child_toggled (GTK_TREE_MODEL (selection_model), path, &selection_iter);
}

static void
gnc_tree_model_selection_row_deleted (GtkTreeModel *tree_model,
				      GtkTreePath *path,
				      GncTreeModelSelection *selection_model)
{
	gtk_tree_model_row_deleted (GTK_TREE_MODEL (selection_model), path);
}

static void
gnc_tree_model_selection_rows_reordered (GtkTreeModel *tree_model,
					 GtkTreePath *path,
					 GtkTreeIter *iter,
					 gint *new_order,
					 GncTreeModelSelection *selection_model)
{
	GtkTreeIter selection_iter;

	gnc_tree_model_selection_convert_child_iter_to_iter (selection_model, &selection_iter, iter);

	gtk_tree_model_rows_reordered (GTK_TREE_MODEL (selection_model), path, &selection_iter, new_order);
}

static void
gnc_tree_model_selection_toggled (GtkCellRendererToggle *toggle,
				  gchar *path,
				  GncTreeModelSelection *model)
{
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (model), &iter, path)) {
		gnc_tree_model_selection_set_selected (model, &iter, !gtk_cell_renderer_toggle_get_active (toggle));
	}
}
