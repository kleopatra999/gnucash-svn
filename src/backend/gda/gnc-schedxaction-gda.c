/********************************************************************
 * gnc-schedxaction-gda.c: load and save data to SQL via libgda     *
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
/** @file gnc-schedxaction-gda.c
 *  @brief load and save data to SQL 
 *  @author Copyright (c) 2006 Phil Longstaff <plongstaff@rogers.com>
 *
 * This file implements the top-level QofBackend API for saving/
 * restoring data to/from an SQL db using libgda
 */

#include "config.h"

#include <glib.h>
#include <libgda/libgda.h>

#include "qof.h"

#include "gnc-backend-gda.h"

#include "gnc-schedxaction-gda.h"
#include "gnc-slots-gda.h"

#include "SchedXaction.h"

#define SCHEDXACTION_TABLE "schedxactions"

static QofLogModule log_module = GNC_MOD_BACKEND;

#define SX_MAX_NAME_LEN 50

static gpointer get_autocreate( gpointer pObject );
static void set_autocreate( gpointer pObject, const gpointer pValue );
static gpointer get_autonotify( gpointer pObject );
static void set_autonotify( gpointer pObject, const gpointer pValue );

static col_cvt_t col_table[] =
{
	{ "guid",			CT_GUID,	0, COL_NNUL|COL_PKEY,	NULL,
			(GNC_GDA_FN_GETTER)qof_entity_get_guid,
			(GNC_GDA_FN_SETTER)qof_entity_set_guid },
	{ "name",			CT_STRING, SX_MAX_NAME_LEN, COL_NNUL, GNC_SX_NAME },
	{ "start_date",		CT_GDATE,	0, COL_NNUL, GNC_SX_START_DATE },
	{ "last_occur",		CT_GDATE,	0, COL_NNUL, GNC_SX_LAST_DATE },
	{ "num_occur",		CT_INT,		0, COL_NNUL, GNC_SX_NUM_OCCUR },
	{ "rem_occur",		CT_INT,		0, COL_NNUL, GNC_SX_REM_OCCUR },
	{ "auto_create",	CT_BOOLEAN,	0, COL_NNUL, NULL,
			get_autocreate, set_autocreate },
	{ "auto_notify",	CT_BOOLEAN,	0, COL_NNUL, NULL,
			get_autonotify, set_autonotify },
	{ "adv_creation",	CT_INT,		0, COL_NNUL, NULL,
			(GNC_GDA_FN_GETTER)xaccSchedXactionGetAdvanceCreation,
			(GNC_GDA_FN_SETTER)xaccSchedXactionSetAdvanceCreation },
	{ "adv_notify",	CT_INT,		0, COL_NNUL, NULL,
			(GNC_GDA_FN_GETTER)xaccSchedXactionGetAdvanceReminder,
			(GNC_GDA_FN_SETTER)xaccSchedXactionSetAdvanceReminder },
	{ NULL }
};

/* ================================================================= */

static gpointer
get_autocreate( gpointer pObject )
{
	const SchedXaction* pSx = GNC_SX(pObject);
	gboolean autoCreate;
	gboolean autoNotify;

	xaccSchedXactionGetAutoCreate( pSx, &autoCreate, &autoNotify );
	return (gpointer)autoCreate;
}

static void 
set_autocreate( gpointer pObject, const gpointer pValue )
{
	SchedXaction* pSx = GNC_SX(pObject);
	gboolean autoCreate;
	gboolean autoNotify;

	xaccSchedXactionGetAutoCreate( pSx, &autoCreate, &autoNotify );
	autoCreate = (gboolean)pValue;
	xaccSchedXactionSetAutoCreate( pSx, autoCreate, autoNotify );
}

static gpointer
get_autonotify( gpointer pObject )
{
	const SchedXaction* pSx = GNC_SX(pObject);
	gboolean autoCreate;
	gboolean autoNotify;

	xaccSchedXactionGetAutoCreate( pSx, &autoCreate, &autoNotify );
	return (gpointer)autoNotify;
}

static void 
set_autonotify( gpointer pObject, const gpointer pValue )
{
	SchedXaction* pSx = GNC_SX(pObject);
	gboolean autoCreate;
	gboolean autoNotify;

	xaccSchedXactionGetAutoCreate( pSx, &autoCreate, &autoNotify );
	autoNotify = (gboolean)pValue;
	xaccSchedXactionSetAutoCreate( pSx, autoCreate, autoNotify );
}

/* ================================================================= */
static SchedXaction*
load_sx( GncGdaBackend* be, GdaDataModel* pModel, int row,
			SchedXaction* pSx )
{
	const GUID* guid;
	GUID sx_guid;

	guid = gnc_gda_load_guid( pModel, row );
	sx_guid = *guid;

	if( pSx == NULL ) {
		pSx = xaccSchedXactionMalloc( be->primary_book );
	}

	gnc_gda_load_object( pModel, row, GNC_ID_SCHEDXACTION, pSx, col_table );
	gnc_gda_slots_load( be, qof_entity_get_guid( QOF_ENTITY(pSx) ),
							qof_instance_get_slots( QOF_INSTANCE(pSx) ) );

	qof_instance_mark_clean( QOF_INSTANCE(pSx) );

	return pSx;
}

static void
load_sxes( GncGdaBackend* be )
{
	static GdaQuery* query;
	GdaObject* ret;
	QofBook* pBook = be->primary_book;

	if( query == NULL ) {
		query = gnc_gda_create_select_query( be, SCHEDXACTION_TABLE );
	}
	ret = gnc_gda_execute_query( be, query );
	if( GDA_IS_DATA_MODEL( ret ) ) {
		GdaDataModel* pModel = GDA_DATA_MODEL(ret);
		int numRows = gda_data_model_get_n_rows( pModel );
		int r;

		for( r = 0; r < numRows; r++ ) {
			(void)load_sx( be, pModel, r, NULL );
		}
	}
}

/* ================================================================= */
static void
create_sx_tables( GncGdaBackend* be )
{
	gnc_gda_create_table_if_needed( be, SCHEDXACTION_TABLE, col_table );
}

/* ================================================================= */
static void
commit_sx( GncGdaBackend* be, QofInstance* inst )
{
	SchedXaction* pSx = GNC_SX(inst);
	const GUID* guid;

	(void)gnc_gda_do_db_operation( be,
						(inst->do_free ? OP_DB_DELETE : OP_DB_ADD_OR_UPDATE ),
						SCHEDXACTION_TABLE,
						GNC_ID_SCHEDXACTION, pSx,
						col_table );

	// Delete old slot info
	guid = qof_instance_get_guid( inst );

	// Now, commit any slots
	if( !inst->do_free ) {
		gnc_gda_slots_save( be, guid, qof_instance_get_slots( inst ) );
	} else {
		gnc_gda_slots_delete( be, guid );
	}
}

/* ================================================================= */
void
gnc_gda_init_schedxaction_handler( void )
{
	static GncGdaDataType_t be_data =
	{
		GNC_GDA_BACKEND_VERSION,
		GNC_ID_SCHEDXACTION,
		commit_sx,				/* commit */
		load_sxes,				/* initial_load */
		create_sx_tables		/* create_tables */
	};

	qof_object_register_backend( GNC_ID_SCHEDXACTION, GNC_GDA_BACKEND, &be_data );
}
/* ========================== END OF FILE ===================== */
