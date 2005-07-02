/********************************************************************\
 * gncEmployee.h -- the Core Employee Interface                     *
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
/** @addtogroup Business
    @{ */
/** @addtogroup Employee
    @{ */
/** @file gncEmployee.h
    @brief Employee Interface
    @author Copyright (C) 2001 Derek Atkins <warlord@MIT.EDU>
*/

#ifndef GNC_EMPLOYEE_H_
#define GNC_EMPLOYEE_H_

typedef struct _gncEmployee GncEmployee;

#include "qofbook.h"
#include "qofid.h"

#include "gncAddress.h"
#include "Account.h"

#define GNC_ID_EMPLOYEE "gncEmployee"
#define GNC_IS_EMPLOYEE(obj)  (QOF_CHECK_TYPE((obj), GNC_ID_EMPLOYEE))
#define GNC_EMPLOYEE(obj)     (QOF_CHECK_CAST((obj), GNC_ID_EMPLOYEE, GncEmployee))

/** @name Create/Destroy Functions 
 @{ */
GncEmployee *gncEmployeeCreate (QofBook *book);
void gncEmployeeDestroy (GncEmployee *employee);
void gncEmployeeBeginEdit (GncEmployee *employee);
void gncEmployeeCommitEdit (GncEmployee *employee);
int gncEmployeeCompare (GncEmployee *a, GncEmployee *b);
/** @} */

/** @name Set Functions 
 @{ */
void gncEmployeeSetID (GncEmployee *employee, const char *id);
void gncEmployeeSetUsername (GncEmployee *employee, const char *username);
void gncEmployeeSetLanguage (GncEmployee *employee, const char *language);
void gncEmployeeSetAcl (GncEmployee *employee, const char *acl);
void gncEmployeeSetWorkday (GncEmployee *employee, gnc_numeric workday);
void gncEmployeeSetRate (GncEmployee *employee, gnc_numeric rate);
void gncEmployeeSetCurrency (GncEmployee *employee, gnc_commodity * currency);
void gncEmployeeSetActive (GncEmployee *employee, gboolean active);
void gncEmployeeSetCCard (GncEmployee *employee, Account* ccard_acc);
void qofEmployeeSetAddr (GncEmployee *employee, QofEntity *addr_ent);

/** @} */

/** @name Get Functions 
 @{ */
QofBook * gncEmployeeGetBook (GncEmployee *employee);
const char * gncEmployeeGetID (GncEmployee *employee);
const char * gncEmployeeGetUsername (GncEmployee *employee);
GncAddress * gncEmployeeGetAddr (GncEmployee *employee);
const char * gncEmployeeGetLanguage (GncEmployee *employee);
const char * gncEmployeeGetAcl (GncEmployee *employee);
gnc_numeric gncEmployeeGetWorkday (GncEmployee *employee);
gnc_numeric gncEmployeeGetRate (GncEmployee *employee);
gnc_commodity * gncEmployeeGetCurrency (GncEmployee *employee);
gboolean gncEmployeeGetActive (GncEmployee *employee);
Account * gncEmployeeGetCCard (GncEmployee *employee);
/** @} */


/** Return a pointer to the instance gncEmployee that is identified
 *  by the guid, and is residing in the book. Returns NULL if the
 *  instance can't be found.
 *  Equivalent function prototype is
 *  GncEmployee * gncEmployeeLookup (QofBook *book, const GUID *guid);
 */
#define gncEmployeeLookup(book,guid)    \
       QOF_BOOK_LOOKUP_ENTITY((book),(guid),GNC_ID_EMPLOYEE, GncEmployee)

gboolean gncEmployeeIsDirty (GncEmployee *employee);

#define EMPLOYEE_ID			"id"
#define EMPLOYEE_USERNAME	"username"
#define EMPLOYEE_ADDR		"addr"
#define EMPLOYEE_LANGUAGE 	"native language"
#define EMPLOYEE_ACL		"acl"
#define EMPLOYEE_WORKDAY	"workday"
#define EMPLOYEE_RATE		"rate"
#define EMPLOYEE_CC    "credit_card_account"

/** deprecated routines */
#define gncEmployeeGetGUID(E) qof_entity_get_guid(QOF_ENTITY(E))
#define gncEmployeeGetBook(E) qof_instance_get_book(QOF_INSTANCE(E))
#define gncEmployeeRetGUID(E) (E ? *(qof_entity_get_guid(QOF_ENTITY(E))) : *(guid_null()))
#define gncEmployeeLookupDirect(G,B) gncEmployeeLookup((B),&(G))

#endif /* GNC_EMPLOYEE_H_ */
/** @} */
/** @} */
