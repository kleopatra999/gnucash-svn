/* 
 * gnc-plugin-transactions.h -- 
 *
 * Copyright (C) 2006 Chris Shoemaker <c.shoemaker@cox.net>
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
 * 51 Franklin Street, Fifth Floor    Fax:    +1-617-542-2652
 * Boston, MA  02110-1301,  USA       gnu@gnu.org
 */

#ifndef __GNC_PLUGIN_TRANSACTIONS_H
#define __GNC_PLUGIN_TRANSACTIONS_H

#include "gnc-plugin.h"

G_BEGIN_DECLS

/* type macros */
#define GNC_TYPE_PLUGIN_TRANSACTIONS            (gnc_plugin_transactions_get_type ())
#define GNC_PLUGIN_TRANSACTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNC_TYPE_PLUGIN_TRANSACTIONS, GncPluginTransactions))
#define GNC_PLUGIN_TRANSACTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GNC_TYPE_PLUGIN_TRANSACTIONS, GncPluginTransactionsClass))
#define GNC_IS_PLUGIN_TRANSACTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNC_TYPE_PLUGIN_TRANSACTIONS))
#define GNC_IS_PLUGIN_TRANSACTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GNC_TYPE_PLUGIN_TRANSACTIONS))
#define GNC_PLUGIN_TRANSACTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GNC_TYPE_PLUGIN_TRANSACTIONS, GncPluginTransactionsClass))

#define GNC_PLUGIN_TRANSACTIONS_NAME "gnc-plugin-transactions"

/* typedefs & structures */
typedef struct {
	GncPlugin gnc_plugin;
} GncPluginTransactions;

typedef struct {
	GncPluginClass gnc_plugin;
} GncPluginTransactionsClass;

/* function prototypes */
GType      gnc_plugin_transactions_get_type (void);

GncPlugin *gnc_plugin_transactions_new      (void);

G_END_DECLS

#endif /* __GNC_PLUGIN_TRANSACTIONS_H */
