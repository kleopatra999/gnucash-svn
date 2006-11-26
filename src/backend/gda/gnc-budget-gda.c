/********************************************************************
 * gnc-budget-gda.c: load and save data to SQL via libgda           *
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
/** @file gnc-budget-gda.c
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
#include "gnc-budget.h"

#include "gnc-backend-gda.h"

#include "gnc-budget-gda.h"
#include "gnc-slots-gda.h"

#include "gnc-budget.h"

#define BUDGET_TABLE "budgets"

static QofLogModule log_module = GNC_MOD_BACKEND;

static void retrieve_guid( gpointer pObject, const gpointer pValue );

#define BUDGET_MAX_NAME_LEN 50
#define BUDGET_MAX_DESCRIPTION_LEN 500

static col_cvt_t col_table[] =
{
	{ "guid",			CT_GUID,	0, COL_NNUL|COL_PKEY,	NULL,
			(GNC_GDA_FN_GETTER)qof_entity_get_guid,
			(GNC_GDA_FN_SETTER)qof_entity_set_guid },
	{ "name",			CT_STRING,	BUDGET_MAX_NAME_LEN, COL_NNUL,	"name" },
	{ "description",	CT_STRING,	BUDGET_MAX_DESCRIPTION_LEN, 0,	"description" },
	{ "num_periods",	CT_INT,		0, COL_NNUL, "num_periods" },
	{ NULL }
};

// Table to retrieve just the guid
static col_cvt_t guid_table[] =
{
	{ "guid", CT_GUID, 0, 0, NULL, NULL, retrieve_guid },
	{ NULL }
};


/* ================================================================= */
static void 
retrieve_guid( gpointer pObject, const gpointer pValue )
{
	GUID** ppGuid = (GUID**)pObject;
	GUID* guid = (GUID*)pValue;

	*ppGuid = guid;
}

/* ================================================================= */
static GncBudget*
load_budget( GncGdaBackend* be, GdaDataModel* pModel, int row,
			GncBudget* pBudget )
{
	const GUID* guid;
	GUID budget_guid;

	gnc_gda_load_object( pModel, row, GNC_ID_BUDGET, &guid, guid_table );
	budget_guid = *guid;

	if( pBudget == NULL ) {
		pBudget = gnc_budget_lookup( &budget_guid, be->primary_book );
		if( pBudget == NULL ) {
			pBudget = gnc_budget_new( be->primary_book );
		}
	}

	gnc_gda_load_object( pModel, row, GNC_ID_BUDGET, pBudget, col_table );
	gnc_gda_slots_load( be, gnc_budget_get_guid( pBudget ),
							qof_instance_get_slots( (QofInstance*)pBudget ) );

	qof_instance_mark_clean( (QofInstance*)pBudget );

	return pBudget;
}

static void
load_budgets( GncGdaBackend* be )
{
	gchar* buf;
	GdaObject* ret;
	QofBook* pBook = be->primary_book;

	buf = g_strdup_printf( "SELECT * FROM %s", BUDGET_TABLE );
	ret = gnc_gda_execute_sql( be, buf );
	g_free( buf );
	if( GDA_IS_DATA_MODEL( ret ) ) {
		GdaDataModel* pModel = (GdaDataModel*)ret;
		int numRows = gda_data_model_get_n_rows( pModel );
		int r;

		for( r = 0; r < numRows; r++ ) {
			(void)load_budget( be, pModel, r, NULL );
		}
	}
}

/* ================================================================= */
static void
create_budget_tables( GncGdaBackend* be )
{
	gnc_gda_create_table_if_needed( be, BUDGET_TABLE, col_table );
}

/* ================================================================= */
static void
commit_budget( GncGdaBackend* be, QofInstance* inst )
{
	GncBudget* pBudget = (GncBudget*)inst;
	const GUID* guid;

	(void)gnc_gda_do_db_operation( be,
							(inst->do_free ? OP_DB_DELETE : OP_DB_ADD_OR_UPDATE ),
							BUDGET_TABLE,
							GNC_ID_BUDGET, pBudget,
							col_table );

	// Delete old slot info
	guid = qof_instance_get_guid( inst );

	gnc_gda_slots_delete( be, guid );

	// Now, commit any slots
	if( !inst->do_free ) {
		gnc_gda_slots_save( be, guid, qof_instance_get_slots( inst ) );
	}
}

/* ================================================================= */
void
gnc_gda_init_budget_handler( void )
{
	static GncGdaDataType_t be_data =
	{
		GNC_GDA_BACKEND_VERSION,
		GNC_ID_BUDGET,
		commit_budget,				/* commit */
		load_budgets,				/* initial_load */
		create_budget_tables		/* create_tables */
	};

	qof_object_register_backend( GNC_ID_BUDGET, GNC_GDA_BACKEND, &be_data );
}
/* ========================== END OF FILE ===================== */
