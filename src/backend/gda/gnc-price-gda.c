/********************************************************************
 * gnc-price-gda.c: load and save data to SQL via libgda            *
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
/** @file gnc-price-gda.c
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
#include "gnc-pricedb.h"

#include "gnc-backend-gda.h"

#include "gnc-price-gda.h"

static QofLogModule log_module = GNC_MOD_BACKEND;

#define TABLE_NAME "prices"

static gpointer get_value( gpointer pObject );
static void set_value( gpointer pObject, const gpointer pValue );
static gpointer get_date( gpointer pObject );
static void set_date( gpointer pObject, const gpointer pValue );
static gpointer get_currency( gpointer pObject );
static void set_currency( gpointer pObject, const gpointer pValue );
static gpointer get_commodity( gpointer pObject );
static void set_commodity( gpointer pObject, const gpointer pValue );

static col_cvt_t col_table[] =
{
	{ "guid",			CT_GUID,	0, COL_NNUL|COL_PKEY,	NULL,
			(GNC_GDA_FN_GETTER)qof_entity_get_guid,
			(GNC_GDA_FN_SETTER)qof_entity_set_guid },
	{ "commodity_guid",	CT_GUID,	0, COL_NNUL, NULL,
			get_commodity, set_commodity },
	{ "currency_guid",	CT_GUID,	0, COL_NNUL, NULL,
			get_currency, set_currency },
	{ "date",			CT_TIMESPEC,	0, COL_NNUL, NULL,
			get_date, set_date },
	{ "source",			CT_STRING,	50, 0, PRICE_SOURCE },
	{ "type",			CT_STRING,	50, 0, PRICE_TYPE },
	{ "value",			CT_NUMERIC,	COL_NNUL, 0, NULL, get_value, set_value },
	{ NULL }
};

/* ================================================================= */
static gpointer
get_value( gpointer pObject )
{
	GNCPrice* pPrice = (GNCPrice*)pObject;
	static gnc_numeric v;

	v = gnc_price_get_value( pPrice );
	return &v;
}

static void
set_value( gpointer pObject, const gpointer pValue )
{
	GNCPrice* pPrice = (GNCPrice*)pObject;
	const gnc_numeric* pNumeric = (const gnc_numeric*)pValue;

	gnc_price_set_value( pPrice, *pNumeric );
}

static gpointer
get_date( gpointer pObject )
{
	GNCPrice* pPrice = (GNCPrice*)pObject;
	static Timespec t;

	t = gnc_price_get_time( pPrice );
	return &t;
}

static void
set_date( gpointer pObject, const gpointer pValue )
{
	GNCPrice* pPrice = (GNCPrice*)pObject;
	const Timespec* pTimespec = (const Timespec*)pValue;

	gnc_price_set_time( pPrice, *pTimespec );
}

static gpointer
get_currency( gpointer pObject )
{
	GNCPrice* pPrice = (GNCPrice*)pObject;

	return (gpointer)qof_instance_get_guid(
							(QofInstance*)gnc_price_get_currency( pPrice ) );
}

static void 
set_currency( gpointer pObject, const gpointer pValue )
{
	GNCPrice* pPrice = (GNCPrice*)pObject;
	QofBook* pBook = qof_instance_get_book( (QofInstance*)pPrice );
	gnc_commodity* pCurrency;
	GUID* guid = (GUID*)pValue;

	pCurrency = gnc_commodity_find_commodity_by_guid( guid, pBook );
	gnc_price_set_currency( pPrice, pCurrency );
}

static gpointer
get_commodity( gpointer pObject )
{
	GNCPrice* pPrice = (GNCPrice*)pObject;

	return (gpointer)qof_instance_get_guid(
						(QofInstance*)gnc_price_get_commodity( pPrice ) );
}

static void 
set_commodity( gpointer pObject, const gpointer pValue )
{
	GNCPrice* pPrice = (GNCPrice*)pObject;
	QofBook* pBook = qof_instance_get_book( (QofInstance*)pPrice );
	gnc_commodity* pCommodity;
	GUID* guid = (GUID*)pValue;

	pCommodity = gnc_commodity_find_commodity_by_guid( guid, pBook );
	gnc_price_set_commodity( pPrice, pCommodity );
}

static GNCPrice*
load_price( GncGdaBackend* be, GdaDataModel* pModel, int row )
{
	GNCPrice* pPrice;

	pPrice = gnc_price_create( be->primary_book );

	gnc_gda_load_object( pModel, row, GNC_ID_PRICE, pPrice, col_table );

	return pPrice;
}

static void
load_prices( GncGdaBackend* be )
{
	GError* error = NULL;
	gchar* buf;
	GdaQuery* query;
	GdaObject* ret;
	QofBook* pBook = be->primary_book;
	GNCPriceDB* pPriceDB = gnc_book_get_pricedb( pBook );

	buf = g_strdup_printf( "SELECT * FROM %s", TABLE_NAME );
	query = gda_query_new_from_sql( be->pDict, buf, &error );
	g_free( buf );
	if( query == NULL ) {
		printf( "SQL error: %s\n", error->message );
		return;
	}
	error = NULL;
	ret = gda_query_execute( query, NULL, FALSE, &error );

	if( error != NULL ) {
		printf( "SQL error: %s\n", error->message );
	}
	if( GDA_IS_DATA_MODEL( ret ) ) {
		GdaDataModel* pModel = (GdaDataModel*)ret;
		int numRows = gda_data_model_get_n_rows( pModel );
		int r;
		GNCPrice* pPrice;

		for( r = 0; r < numRows; r++ ) {

			pPrice = load_price( be, pModel, r );

			if( pPrice != NULL ) {
				gnc_pricedb_add_price( pPriceDB, pPrice );
			}
		}
	}
}

/* ================================================================= */
static void
create_prices_tables( GncGdaBackend* be )
{
	gnc_gda_create_table_if_needed( be, TABLE_NAME, col_table );
}

/* ================================================================= */

static void
commit_price( GncGdaBackend* be, QofInstance* inst )
{
	GNCPrice* pPrice = (GNCPrice*)inst;

	(void)gnc_gda_do_db_operation( be,
							(inst->do_free ? OP_DB_DELETE : OP_DB_ADD_OR_UPDATE ),
							TABLE_NAME,
							GNC_ID_PRICE, pPrice,
							col_table );
}

/* ================================================================= */
void
gnc_gda_init_price_handler( void )
{
	static GncGdaDataType_t be_data =
	{
		GNC_GDA_BACKEND_VERSION,
		GNC_ID_PRICE,
		commit_price,			/* commit */
		load_prices,			/* initial_load */
		create_prices_tables	/* create tables */
	};

	qof_object_register_backend( GNC_ID_PRICE, GNC_GDA_BACKEND, &be_data );
}

/* ========================== END OF FILE ===================== */
