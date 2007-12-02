/********************************************************************
 * gnc-recurrence-gda.h: load and save data to SQL via libgda       *
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
 * 51 Franklin Street, Fifth Floor    Fax:    +1-617-542-2652       *
 * Boston, MA  02110-1301,  USA       gnu@gnu.org                   *
\********************************************************************/
/** @file gnc-recurrence-gda.h
 *  @brief load and save accounts data to SQL via libgda
 *  @author Copyright (c) 2006-2007 Phil Longstaff <plongstaff@rogers.com>
 *
 * This file implements the top-level QofBackend API for saving/
 * restoring data to/from an SQL database via libgda
 */

#ifndef GNC_RECURRENCE_GDA_H_
#define GNC_RECURRENCE_GDA_H_

#include "qof.h"
#include <gmodule.h>

void gnc_gda_recurrence_save( GncGdaBackend* be, const GUID* guid, const Recurrence* pRecurrence );
void gnc_gda_recurrence_save_list( GncGdaBackend* be, const GUID* guid, GList* schedule );
void gnc_gda_recurrence_delete( GncGdaBackend* be, const GUID* guid );
void gnc_gda_recurrence_load( GncGdaBackend* be, const GUID* guid, Recurrence* pRecurrence );
void gnc_gda_recurrence_load_list( GncGdaBackend* be, const GUID* guid, GList** pSchedule );

void gnc_gda_init_recurrence_handler( void );

#endif /* GNC_RECURRENCE_GDA_H_ */
