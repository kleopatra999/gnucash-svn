/* 
 * gnc-tree-model-account-types.h -- GtkTreeModel implementation
 *	to display account types in a GtkTreeView.
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
/** @addtogroup GUI
 *     @{ */
/** @file gnc-tree-model-account-types.h
 *  @brief GtkTreeModel implementation to display account types in a GtkTreeView.
 *  @author Copyright (C) 2003 Jan Arne Petersen
 *  @author: Jan Arne Petersen <jpetersen@uni-bonn.de>
 *
 */

#ifndef __GNC_TREE_MODEL_ACCOUNT_TYPES_H
#define __GNC_TREE_MODEL_ACCOUNT_TYPES_H

#include <gtk/gtktreemodel.h>

G_BEGIN_DECLS

/* type macros */
#define GNC_TYPE_TREE_MODEL_ACCOUNT_TYPES         (gnc_tree_model_account_types_get_type ())
#define GNC_TREE_MODEL_ACCOUNT_TYPES(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GNC_TYPE_TREE_MODEL_ACCOUNT_TYPES, GncTreeModelAccountTypes))
#define GNC_TREE_MODEL_ACCOUNT_TYPES_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GNC_TYPE_TREE_MODEL_ACCOUNT_TYPES, GncTreeModelAccountTypesClass))
#define GNC_IS_TREE_MODEL_ACCOUNT_TYPES(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GNC_TYPE_TREE_MODEL_ACCOUNT_TYPES))
#define GNC_IS_TREE_MODEL_ACCOUNT_TYPES_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GNC_TYPE_TREE_MODEL_ACCOUNT_TYPES))
#define GNC_TREE_MODEL_ACCOUNT_TYPES_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GNC_TYPE_TREE_MODEL_ACCOUNT_TYPES, GncTreeModelAccountTypesClass))

typedef enum {
	GNC_TREE_MODEL_ACCOUNT_TYPES_COL_TYPE,
	GNC_TREE_MODEL_ACCOUNT_TYPES_COL_NAME,
	GNC_TREE_MODEL_ACCOUNT_TYPES_COL_SELECTED,
	GNC_TREE_MODEL_ACCOUNT_TYPES_NUM_COLUMNS
} GncTreeModelAccountTypesColumn;

/* typedefs & structures */
typedef struct GncTreeModelAccountTypesPrivate GncTreeModelAccountTypesPrivate;

typedef struct {
	GObject parent;

	GncTreeModelAccountTypesPrivate *priv;

	int stamp;
} GncTreeModelAccountTypes;

typedef struct {
	GObjectClass parent;
} GncTreeModelAccountTypesClass;

/* function prototypes */
GType           gnc_tree_model_account_types_get_type     (void);

GtkTreeModel   *gnc_tree_model_account_types_new          (guint32 selected);

guint32		gnc_tree_model_account_types_get_selected (GncTreeModelAccountTypes *model);
void		gnc_tree_model_account_types_set_selected (GncTreeModelAccountTypes *model,
							   guint32 selected);


G_END_DECLS

#endif /* __GNC_TREE_MODEL_ACCOUNT_H */

/** @} */
