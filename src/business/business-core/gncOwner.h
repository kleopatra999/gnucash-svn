/********************************************************************\
 * gncOwner.h -- Business Interface:  Object OWNERs                 *
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
 * Copyright (C) 2001, 2002 Derek Atkins
 * Author: Derek Atkins <warlord@MIT.EDU>
 */

#ifndef GNC_OWNER_H_
#define GNC_OWNER_H_

typedef struct gnc_owner_s GncOwner;

#define GNC_ID_OWNER   "gncOwner"
#define GNC_IS_OWNER(obj)  (QOF_CHECK_TYPE((obj), GNC_ID_OWNER))
#define GNC_OWNER(obj)     (QOF_CHECK_CAST((obj), GNC_ID_OWNER, GncOwner))

#include "gncCustomer.h"
#include "gncJob.h"
#include "gncVendor.h"
#include "gncEmployee.h"
#include "gnc-lot.h" 

typedef enum {
  GNC_OWNER_NONE,
  GNC_OWNER_UNDEFINED,
  GNC_OWNER_CUSTOMER,
  GNC_OWNER_JOB,
  GNC_OWNER_VENDOR,
  GNC_OWNER_EMPLOYEE
} GncOwnerType;

struct gnc_owner_s {
  GncOwnerType     type;
  union {
    gpointer       undefined;
    GncCustomer *  customer;
    GncJob *       job;
    GncVendor *    vendor;
    GncEmployee *  employee;
  } owner;
};

void gncOwnerInitUndefined (GncOwner *owner, gpointer obj);
void gncOwnerInitCustomer (GncOwner *owner, GncCustomer *customer);
void gncOwnerInitJob (GncOwner *owner, GncJob *job);
void gncOwnerInitVendor (GncOwner *owner, GncVendor *vendor);
void gncOwnerInitEmployee (GncOwner *owner, GncEmployee *employee);

GncOwnerType gncOwnerGetType (const GncOwner *owner);
gpointer gncOwnerGetUndefined (const GncOwner *owner);
GncCustomer * gncOwnerGetCustomer (const GncOwner *owner);
GncJob * gncOwnerGetJob (const GncOwner *owner);
GncVendor * gncOwnerGetVendor (const GncOwner *owner);
GncEmployee * gncOwnerGetEmployee (const GncOwner *owner);

void gncOwnerCopy (const GncOwner *src, GncOwner *dest);
gboolean gncOwnerEqual (const GncOwner *a, const GncOwner *b);
int gncOwnerCompare (const GncOwner *a, const GncOwner *b);

const char * gncOwnerGetName (GncOwner *owner);
gnc_commodity * gncOwnerGetCurrency (GncOwner *owner);

/* Get the GUID of the immediate owner */
const GUID * gncOwnerGetGUID (GncOwner *owner);
GUID gncOwnerRetGUID (GncOwner *owner);

gboolean gncOwnerIsValid (GncOwner *owner);

/*
 * Get the "parent" Owner or GUID thereof.  The "parent" owner
 * is the Customer or Vendor, or the Owner of a Job
 */
GncOwner * gncOwnerGetEndOwner (GncOwner *owner);
const GUID * gncOwnerGetEndGUID (GncOwner *owner);

/* attach an owner to a lot */
void gncOwnerAttachToLot (GncOwner *owner, GNCLot *lot);

/* Get the owner from the lot.  If an owner is found in the lot,
 * fill in "owner" and return TRUE.  Otherwise return FALSE.
 */
gboolean gncOwnerGetOwnerFromLot (GNCLot *lot, GncOwner *owner);

#define OWNER_TYPE	"type"
#define OWNER_CUSTOMER	"customer"
#define OWNER_JOB	"job"
#define OWNER_VENDOR	"vendor"
#define OWNER_EMPLOYEE	"employee"
#define OWNER_PARENT	"parent"
#define OWNER_PARENTG	"parent-guid"
#define OWNER_NAME	"name"

#define OWNER_FROM_LOT	"owner-from-lot"

/*
 * These two functions are mainly for the convenience of scheme code.
 * Normal C code has no need to ever use these two functions, and rather
 * can just use a GncOwner directly and just pass around a pointer to it.
 */
GncOwner * gncOwnerCreate (void);
void gncOwnerDestroy (GncOwner *owner);

#endif /* GNC_OWNER_H_ */
