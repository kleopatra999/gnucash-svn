/********************************************************************\
 * gncTaxTableP.h -- the Gnucash Tax Table private interface        *
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
 * Copyright (C) 2002 Derek Atkins
 * Copyright (C) 2003 Linas Vepstas <linas@linas.org>
 * Author: Derek Atkins <warlord@MIT.EDU>
 */

#ifndef GNC_TAXTABLEP_H_
#define GNC_TAXTABLEP_H_

#include "gncTaxTable.h"

gboolean gncTaxTableRegister (void);

void gncTaxTableSetGUID (GncTaxTable *table, const GUID *guid);
void gncTaxTableSetParent (GncTaxTable *table, GncTaxTable *parent);
void gncTaxTableSetChild (GncTaxTable *table, GncTaxTable *child);
void gncTaxTableSetRefcount (GncTaxTable *table, gint64 refcount);
void gncTaxTableMakeInvisible (GncTaxTable *table);

gboolean gncTaxTableGetInvisible (GncTaxTable *table);

/** The gncCloneTaxTable() routine makes a copy of the indicated
 *  tax table, placing it in the indicated book.  It copies
 *  the etc.
 *  It does not copy parent/child relationships ???
 * XXX the above need fixin....
 *  It then adds a pair of 'gemini' kvp pointers so that each copy
 *  can be found from the other.
 */

GncTaxTable * gncCloneTaxTable (GncTaxTable *from, QofBook *book);

/** The gncTaxTableObtainTwin() will find the 'twin' of the
 *  indicated tax table in the indicated book.  If the twin doesn't
 *  yet exist in the book, it will be created (by calling
 *  gncCloneTaxTable()) and placed into the book.
 *
 * We called this routine 'Obtain' instead of "Get" to distinguish
 * it from the other Get routines, which work in fundamentally
 * different ways.
 */
GncTaxTable * gncTaxTableObtainTwin (GncTaxTable *from, QofBook *book);
                                                                                
#endif /* GNC_TAXTABLEP_H_ */
