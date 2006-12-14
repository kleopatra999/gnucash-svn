/********************************************************************
 * gnc-backend-gda.c: load and save data to SQL via libgda          *
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
/** @file gnc-backend-gda.c
 *  @brief load and save data to SQL 
 *  @author Copyright (c) 2006 Phil Longstaff <plongstaff@rogers.com>
 *
 * This file implements the top-level QofBackend API for saving/
 * restoring data to/from an SQL db using libgda
 */

#include "config.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <libgda/libgda.h>

#include "qof.h"
#include "qofquery-p.h"
#include "qofquerycore-p.h"
#include "TransLog.h"
#include "gnc-engine.h"

#include "gnc-backend-gda.h"
#include "gnc-gconf-utils.h"

#include "gnc-account-gda.h"
#include "gnc-budget-gda.h"
#include "gnc-commodity-gda.h"
#include "gnc-lots-gda.h"
#include "gnc-price-gda.h"
#include "gnc-schedxaction-gda.h"
#include "gnc-slots-gda.h"
#include "gnc-transaction-gda.h"

static const gchar* convert_search_obj( QofIdType objType );
static void gnc_gda_init_object_handlers( void );
static void add_table_column( GdaServerProvider* server, GdaConnection* cnn,
			xmlNodePtr array_data, const gchar* arg, const gchar* dbms_type,
			gint size, gint flags );

typedef struct {
	QofIdType searchObj;
	gpointer pCompiledQuery;
} gnc_gda_query_info;

/* callback structure */
typedef struct {
	gboolean ok;
	GncGdaBackend* be;
	QofInstance* inst;
	QofQuery* pQuery;
	gpointer pCompiledQuery;
	gnc_gda_query_info* pQueryInfo;
} gda_backend;

static QofLogModule log_module = GNC_MOD_BACKEND;

typedef void (*GNC_GDA_LOAD_FN)( GdaDataModel* pModel, gint row,
								QofSetterFunc setter, gpointer pObject,
								const col_cvt_t* table );
typedef void (*GNC_GDA_CREATE_COL_FN)( GdaServerProvider* server,
						GdaConnection* cnn, xmlNodePtr array_data,
						const col_cvt_t* table_row );
typedef void (*GNC_GDA_GET_GVALUE_QUERY_FN)( GncGdaBackend* be,
				QofIdTypeConst obj_name, gpointer pObject,
				const col_cvt_t* table_row, GdaQuery* query );
typedef GdaQueryCondition* (*GNC_GDA_GET_GVALUE_COND_FN)( GncGdaBackend* be,
				QofIdTypeConst obj_name, gpointer pObject,
				const col_cvt_t* table_row, GdaQuery* query );

typedef struct {
	GNC_GDA_LOAD_FN			load_fn;
	GNC_GDA_CREATE_COL_FN	create_col_fn;
	GNC_GDA_GET_GVALUE_QUERY_FN	get_gvalue_query_fn;
	GNC_GDA_GET_GVALUE_COND_FN	get_gvalue_cond_fn;
} col_type_handler_t;


/* ================================================================= */
static void
add_field_to_query( GdaQuery* query, const gchar* col_name, const GValue* value )
{
	GdaQueryField* field;
	GdaQueryField* field_value;

	field = gda_query_field_field_new( query, col_name );
	gda_query_field_set_visible( field, TRUE );

	field_value = gda_query_field_value_new( query, G_VALUE_TYPE(value) );
	gda_query_field_set_visible( field_value, TRUE );
	gda_query_field_value_set_value( GDA_QUERY_FIELD_VALUE(field_value), value );
	g_object_set( field, "value-provider", field_value, NULL );
	g_object_unref( G_OBJECT(field_value) );

	gda_entity_add_field( GDA_ENTITY(query), GDA_ENTITY_FIELD(field) );
	g_object_unref( G_OBJECT(field) );
}

GdaQueryCondition*
gnc_gda_create_condition_from_field( GdaQuery* query, const gchar* col_name,
								const GValue* value )
{
	GdaQueryCondition* cond;
	GdaQueryField* key;
	GdaQueryField* key_value;

	cond = gda_query_condition_new( query, GDA_QUERY_CONDITION_LEAF_EQUAL );

	key = gda_query_field_field_new( query, col_name );
	gda_query_field_set_visible( key, TRUE );
	gda_query_condition_leaf_set_operator( cond,
											GDA_QUERY_CONDITION_OP_LEFT,
											GDA_QUERY_FIELD(key) );
	g_object_unref( G_OBJECT(key) );

	key_value = gda_query_field_value_new( query, G_VALUE_TYPE(value) );
	gda_query_field_set_visible( key_value, TRUE );
	gda_query_condition_leaf_set_operator( cond, GDA_QUERY_CONDITION_OP_RIGHT,
												GDA_QUERY_FIELD(key_value) );
	g_object_unref( G_OBJECT(key_value) );

	gda_query_field_value_set_value( GDA_QUERY_FIELD_VALUE(key_value), value );

	return cond;
}
/* ----------------------------------------------------------------- */
static QofAccessFunc
get_getter( QofIdTypeConst obj_name, const col_cvt_t* table_row )
{
	QofAccessFunc getter;

	if( table_row->param_name != NULL ) {
		getter = qof_class_get_parameter_getter( obj_name,
												table_row->param_name );
	} else {
		getter = table_row->getter;
	}

	return getter;
}

static void
load_string( GdaDataModel* pModel, gint row,
			QofSetterFunc setter, gpointer pObject,
			const col_cvt_t* table )
{
	const GValue* val;
	const gchar* s;

	val = gda_data_model_get_value_at_col_name( pModel, table->col_name, row );
	if( gda_value_is_null( val ) ) {
		s = NULL;
	} else {
		s = g_value_get_string( val );
	}
	(*setter)( pObject, (const gpointer)s );
}

static void
get_gvalue_string( GncGdaBackend* be, QofIdTypeConst obj_name,
				gpointer pObject, const col_cvt_t* table_row, GValue* value )
{
	QofAccessFunc getter;
	gchar* s;

	memset( value, 0, sizeof( GValue ) );
	getter = get_getter( obj_name, table_row );
	s = (gchar*)(*getter)( pObject, NULL );
	if( s ) {
		g_value_init( value, G_TYPE_STRING );
		g_value_set_string( value, s );
	}
}

static void
get_gvalue_string_for_query( GncGdaBackend* be, QofIdTypeConst obj_name,
				gpointer pObject, const col_cvt_t* table_row, GdaQuery* query )
{
	GValue value;

	get_gvalue_string( be, obj_name, pObject, table_row, &value );
	add_field_to_query( query, table_row->col_name, &value );
}

static GdaQueryCondition*
get_gvalue_string_cond( GncGdaBackend* be, QofIdTypeConst obj_name,
				gpointer pObject, const col_cvt_t* table_row, GdaQuery* query )
{
	GValue value;

	get_gvalue_string( be, obj_name, pObject, table_row, &value );
	return gnc_gda_create_condition_from_field( query, table_row->col_name,
											&value );
}

static void
create_string_col( GdaServerProvider* server, GdaConnection* cnn,
			xmlNodePtr array_data, const col_cvt_t* table_row )
{
	const gchar* dbms_type;

	dbms_type = gda_server_provider_get_default_dbms_type( server,
														cnn, G_TYPE_STRING );
	add_table_column( server, cnn, array_data, table_row->col_name,
					dbms_type, table_row->size, table_row->flags );
}

static col_type_handler_t string_handler
	= { load_string, create_string_col,
		get_gvalue_string_for_query, get_gvalue_string_cond };
/* ----------------------------------------------------------------- */

static void
load_int( GdaDataModel* pModel, gint row,
			QofSetterFunc setter, gpointer pObject,
			const col_cvt_t* table )
{
	const GValue* val;
	gint int_value;

	val = gda_data_model_get_value_at_col_name( pModel, table->col_name, row );
	if( gda_value_is_null( val ) ) {
		int_value = 0;
	} else {
		int_value = g_value_get_int( val );
	}
	(*setter)( pObject, (gpointer)int_value );
}

static void
get_gvalue_int( GncGdaBackend* be, QofIdTypeConst obj_name,
				gpointer pObject, const col_cvt_t* table_row, GValue* value )
{
	gint int_value;
	QofAccessFunc getter;

	memset( value, 0, sizeof( GValue ) );

	getter = get_getter( obj_name, table_row );
	int_value = (gint)(*getter)( pObject, NULL );
	g_value_init( value, G_TYPE_INT );
	g_value_set_int( value, int_value );
}

static void
get_gvalue_int_for_query( GncGdaBackend* be, QofIdTypeConst obj_name,
				gpointer pObject, const col_cvt_t* table_row, GdaQuery* query )
{
	GValue value;

	get_gvalue_int( be, obj_name, pObject, table_row, &value );
	add_field_to_query( query, table_row->col_name, &value );
}

static GdaQueryCondition*
get_gvalue_int_cond( GncGdaBackend* be, QofIdTypeConst obj_name,
				gpointer pObject, const col_cvt_t* table_row, GdaQuery* query )
{
	GValue value;

	get_gvalue_int( be, obj_name, pObject, table_row, &value );
	return gnc_gda_create_condition_from_field( query, table_row->col_name, &value );
}

static void
create_int_col( GdaServerProvider* server, GdaConnection* cnn,
			xmlNodePtr array_data, const col_cvt_t* table_row )
{
	const gchar* dbms_type;

	dbms_type = gda_server_provider_get_default_dbms_type( server,
														cnn, G_TYPE_INT );
	add_table_column( server, cnn, array_data, table_row->col_name,
					dbms_type, table_row->size, table_row->flags );
}

static col_type_handler_t int_handler =
		{ load_int, create_int_col,
			get_gvalue_int_for_query, get_gvalue_int_cond };
/* ----------------------------------------------------------------- */

static void
load_int64( GdaDataModel* pModel, gint row,
			QofSetterFunc setter, gpointer pObject,
			const col_cvt_t* table )
{
	const GValue* val;
	gint64 i64_value;

	val = gda_data_model_get_value_at_col_name( pModel, table->col_name, row );
	if( gda_value_is_null( val ) ) {
		(*setter)( pObject, NULL );
	} else {	
		i64_value = g_value_get_int64( val );
		(*setter)( pObject, (gpointer)&i64_value );
	}
}

static void
get_gvalue_int64( GncGdaBackend* be, QofIdTypeConst obj_name, gpointer pObject,
				const col_cvt_t* table_row, GValue* value )
{
	gint64* pInt64;
	gint64 i64_value;
	QofAccessFunc getter;

	memset( value, 0, sizeof( GValue ) );
	getter = get_getter( obj_name, table_row );
	pInt64 = (*getter)( pObject, NULL );
	if( pInt64 != NULL ) {
		gchar* s;

		i64_value = *pInt64;
		g_value_init( value, G_TYPE_INT64 );
		g_value_set_int64( value, i64_value );
	}
}

static void
get_gvalue_int64_for_query( GncGdaBackend* be, QofIdTypeConst obj_name,
				gpointer pObject, const col_cvt_t* table_row, GdaQuery* query )
{
	GValue value;

	get_gvalue_int64( be, obj_name, pObject, table_row, &value );
	add_field_to_query( query, table_row->col_name, &value );
}

static GdaQueryCondition*
get_gvalue_int64_cond( GncGdaBackend* be, QofIdTypeConst obj_name,
				gpointer pObject, const col_cvt_t* table_row, GdaQuery* query )
{
	GValue value;

	get_gvalue_int64( be, obj_name, pObject, table_row, &value );
	return gnc_gda_create_condition_from_field( query, table_row->col_name, &value );
}

static void
create_int64_col( GdaServerProvider* server, GdaConnection* cnn,
			xmlNodePtr array_data, const col_cvt_t* table_row )
{
	const gchar* dbms_type;

	dbms_type = gda_server_provider_get_default_dbms_type( server,
														cnn, G_TYPE_INT64 );
	add_table_column( server, cnn, array_data, table_row->col_name,
					dbms_type, table_row->size, table_row->flags );
}

static col_type_handler_t int64_handler =
		{ load_int64, create_int64_col,
			get_gvalue_int64_for_query, get_gvalue_int64_cond };
/* ----------------------------------------------------------------- */

static void
load_double( GdaDataModel* pModel, gint row,
			QofSetterFunc setter, gpointer pObject,
			const col_cvt_t* table )
{
	const GValue* val;
	gdouble d_value;

	val = gda_data_model_get_value_at_col_name( pModel, table->col_name, row );
	if( gda_value_is_null( val ) ) {
		(*setter)( pObject, (gpointer)NULL );
	} else {
		d_value = g_value_get_double( val );
		(*setter)( pObject, (gpointer)&d_value );
	}
}

static void
get_gvalue_double( GncGdaBackend* be, QofIdTypeConst obj_name, gpointer pObject,
				const col_cvt_t* table_row, GValue* value )
{
	QofAccessFunc getter;
	gdouble* pDouble;
	gdouble d_value;

	memset( value, 0, sizeof( GValue ) );

	getter = get_getter( obj_name, table_row );
	pDouble = (*getter)( pObject, NULL );
	if( pDouble != NULL ) {
		d_value = *pDouble;
		g_value_init( value, G_TYPE_DOUBLE );
		g_value_set_double( value, d_value );
	}
}

static void
get_gvalue_double_for_query( GncGdaBackend* be, QofIdTypeConst obj_name,
				gpointer pObject, const col_cvt_t* table_row, GdaQuery* query )
{
	GValue value;

	get_gvalue_double( be, obj_name, pObject, table_row, &value );
	add_field_to_query( query, table_row->col_name, &value );
}

static GdaQueryCondition*
get_gvalue_double_cond( GncGdaBackend* be, QofIdTypeConst obj_name,
				gpointer pObject, const col_cvt_t* table_row, GdaQuery* query )
{
	GValue value;

	get_gvalue_double( be, obj_name, pObject, table_row, &value );
	return gnc_gda_create_condition_from_field( query, table_row->col_name, &value );
}

static void
create_double_col( GdaServerProvider* server, GdaConnection* cnn,
			xmlNodePtr array_data, const col_cvt_t* table_row )
{
	const gchar* dbms_type;

	dbms_type = gda_server_provider_get_default_dbms_type( server,
														cnn, G_TYPE_INT64 );
	add_table_column( server, cnn, array_data, table_row->col_name,
					dbms_type, table_row->size, table_row->flags );
}

static col_type_handler_t double_handler =
		{ load_double, create_double_col,
			get_gvalue_double_for_query, get_gvalue_double_cond };
/* ----------------------------------------------------------------- */

static void
load_guid( GdaDataModel* pModel, gint row,
			QofSetterFunc setter, gpointer pObject,
			const col_cvt_t* table )
{
	const GValue* val;
	GUID guid;
	const GUID* pGuid;

	val = gda_data_model_get_value_at_col_name( pModel, table->col_name, row );
	if( gda_value_is_null( val ) ) {
		pGuid = NULL;
	} else {
		string_to_guid( g_value_get_string( val ), &guid );
		pGuid = &guid;
	}
	(*setter)( pObject, (gpointer)pGuid );
}

static void
get_gvalue_guid( GncGdaBackend* be, QofIdTypeConst obj_name, gpointer pObject,
				const col_cvt_t* table_row, GValue* value )
{
	QofAccessFunc getter;
	const GUID* guid;
	gchar guid_buf[GUID_ENCODING_LENGTH+1];

	memset( value, 0, sizeof( GValue ) );

	getter = get_getter( obj_name, table_row );
	guid = (*getter)( pObject, NULL );
	if( guid != NULL ) {
		(void)guid_to_string_buff( guid, guid_buf );
		g_value_init( value, G_TYPE_STRING );
		g_value_set_string( value, guid_buf );
	}
}

static void
get_gvalue_guid_for_query( GncGdaBackend* be, QofIdTypeConst obj_name,
				gpointer pObject, const col_cvt_t* table_row, GdaQuery* query )
{
	GValue value;

	get_gvalue_guid( be, obj_name, pObject, table_row, &value );
	add_field_to_query( query, table_row->col_name, &value );
}

static GdaQueryCondition*
get_gvalue_guid_cond( GncGdaBackend* be, QofIdTypeConst obj_name,
				gpointer pObject, const col_cvt_t* table_row, GdaQuery* query )
{
	GValue value;

	get_gvalue_guid( be, obj_name, pObject, table_row, &value );
	return gnc_gda_create_condition_from_field( query, table_row->col_name, &value );
}

static void
create_guid_col( GdaServerProvider* server, GdaConnection* cnn,
			xmlNodePtr array_data, const col_cvt_t* table_row )
{
	add_table_column( server, cnn, array_data, table_row->col_name,
					"char", GUID_ENCODING_LENGTH, table_row->flags );
}

static col_type_handler_t guid_handler =
		{ load_guid, create_guid_col,
			get_gvalue_guid_for_query, get_gvalue_guid_cond };
/* ----------------------------------------------------------------- */
static void
load_timespec( GdaDataModel* pModel, gint row,
			QofSetterFunc setter, gpointer pObject,
			const col_cvt_t* table )
{
	const GValue* val;
	GDate* date;
	Timespec ts;

	val = gda_data_model_get_value_at_col_name( pModel, table->col_name, row );
	if( gda_value_is_null( val ) ) {
		(*setter)( pObject, NULL );
	} else {
		date = (GDate*)g_value_get_boxed( val );
		if( date != NULL ) {
			ts = gnc_dmy2timespec( g_date_get_day( date ),
								g_date_get_month( date ),
								g_date_get_year( date ) );
			(*setter)( pObject, &ts );
		}
	}
}

static void
get_gvalue_timespec( GncGdaBackend* be, QofIdTypeConst obj_name,
				gpointer pObject, const col_cvt_t* table_row, GValue* value )
{
	QofAccessFunc getter;
	Timespec* pTimespec;

	memset( value, 0, sizeof( GValue ) );

	getter = get_getter( obj_name, table_row );
	pTimespec = (Timespec*)(*getter)( pObject, NULL );
	if( pTimespec != NULL ) {
		GDate* date;
		gchar* s;
		gint y, m, d;
		gchar iso8601_buf[33];

		date = g_date_new();
		(void)gnc_timespec_to_iso8601_buff( *pTimespec, iso8601_buf );
		sscanf( iso8601_buf, "%d-%d-%d", &y, &m, &d );
		g_date_set_dmy( date, d, m, y );
		g_value_init( value, G_TYPE_DATE );
		g_value_set_boxed( value, date );
	}
}

static void
get_gvalue_timespec_for_query( GncGdaBackend* be, QofIdTypeConst obj_name,
				gpointer pObject, const col_cvt_t* table_row, GdaQuery* query )
{
	GValue value;

	get_gvalue_timespec( be, obj_name, pObject, table_row, &value );
	add_field_to_query( query, table_row->col_name, &value );
}

static GdaQueryCondition*
get_gvalue_timespec_cond( GncGdaBackend* be, QofIdTypeConst obj_name,
				gpointer pObject, const col_cvt_t* table_row, GdaQuery* query )
{
	GValue value;

	get_gvalue_timespec( be, obj_name, pObject, table_row, &value );
	return gnc_gda_create_condition_from_field( query, table_row->col_name, &value );
}

static void
create_timespec_col( GdaServerProvider* server, GdaConnection* cnn,
			xmlNodePtr array_data, const col_cvt_t* table_row )
{
	const gchar* dbms_type;

	dbms_type = gda_server_provider_get_default_dbms_type( server,
														cnn, G_TYPE_DATE );
	add_table_column( server, cnn, array_data, table_row->col_name,
					dbms_type, table_row->size, table_row->flags );
}

static col_type_handler_t timespec_handler =
		{ load_timespec, create_timespec_col,
			get_gvalue_timespec_for_query, get_gvalue_timespec_cond };
/* ----------------------------------------------------------------- */
static void
load_date( GdaDataModel* pModel, gint row,
			QofSetterFunc setter, gpointer pObject,
			const col_cvt_t* table )
{
	const GValue* val;
	GDate* date;

	val = gda_data_model_get_value_at_col_name( pModel, table->col_name, row );
	if( gda_value_is_null( val ) ) {
		(*setter)( pObject, NULL );
	} else {
		date = (GDate*)g_value_get_boxed( val );
		if( date != NULL ) {
			(*setter)( pObject, date );
		}
	}
}

static void
get_gvalue_date( GncGdaBackend* be, QofIdTypeConst obj_name, gpointer pObject,
				const col_cvt_t* table_row, GValue* value )
{
	GDate* date;
	QofAccessFunc getter;

	memset( value, 0, sizeof( GValue ) );

	getter = get_getter( obj_name, table_row );
	date = (GDate*)(*getter)( pObject, NULL );
	if( date != NULL ) {
		g_value_init( value, G_TYPE_DATE );
		g_value_set_boxed( value, date );
	}
}

static void
get_gvalue_date_for_query( GncGdaBackend* be, QofIdTypeConst obj_name,
				gpointer pObject, const col_cvt_t* table_row, GdaQuery* query )
{
	GValue value;

	get_gvalue_date( be, obj_name, pObject, table_row, &value );
	add_field_to_query( query, table_row->col_name, &value );
}

static GdaQueryCondition*
get_gvalue_date_cond( GncGdaBackend* be, QofIdTypeConst obj_name,
				gpointer pObject, const col_cvt_t* table_row, GdaQuery* query )
{
	GValue value;

	get_gvalue_date( be, obj_name, pObject, table_row, &value );
	return gnc_gda_create_condition_from_field( query, table_row->col_name, &value );
}

static col_type_handler_t date_handler =
		{ load_date, create_timespec_col,
			get_gvalue_date_for_query, get_gvalue_date_cond };
/* ----------------------------------------------------------------- */
static void
load_numeric( GdaDataModel* pModel, gint row,
			QofSetterFunc setter, gpointer pObject,
			const col_cvt_t* table )
{
	const GValue* val;
	gchar* buf;
	gint64 num, denom;
	gnc_numeric n;
	gboolean isNull = FALSE;

	buf = g_strdup_printf( "%s_num", table->col_name );
	val = gda_data_model_get_value_at_col_name( pModel, buf, row );
	g_free( buf );
	if( gda_value_is_null( val ) ) {
		isNull = TRUE;
		num = 0;
	} else {
		num = g_value_get_int64( val );
	}
	buf = g_strdup_printf( "%s_denom", table->col_name );
	val = gda_data_model_get_value_at_col_name( pModel, buf, row );
	g_free( buf );
	if( gda_value_is_null( val ) ) {
		isNull = TRUE;
		denom = 1;
	} else {
		denom = g_value_get_int64( val );
	}
	n = gnc_numeric_create( num, denom );
	if( isNull ) {
		(*setter)( pObject, NULL );
	} else {
		(*setter)( pObject, &n );
	}
}

static void
get_gvalue_numeric( GncGdaBackend* be, QofIdTypeConst obj_name, gpointer pObject,
				const col_cvt_t* table_row, GValue* value )
{
	QofAccessFunc getter;
	gnc_numeric* n;

	memset( value, 0, sizeof( GValue ) );

	getter = get_getter( obj_name, table_row );
	n = (gnc_numeric*)(*getter)( pObject, NULL );
	if( n != NULL ) {
		g_value_init( value, gnc_numeric_get_type() );
		g_value_set_boxed( value, n );
	}
}

static void
get_gvalue_numeric_for_query( GncGdaBackend* be, QofIdTypeConst obj_name,
				gpointer pObject, const col_cvt_t* table_row, GdaQuery* query )
{
	GValue value;
	GValue num_value;
	GValue denom_value;
	gnc_numeric* n;
	gchar* s;

	memset( &value, 0, sizeof( GValue ) );
	memset( &num_value, 0, sizeof( GValue ) );
	memset( &denom_value, 0, sizeof( GValue ) );

	get_gvalue_numeric( be, obj_name, pObject, table_row, &value );
	if( G_VALUE_TYPE(&value) != 0 ) {
		n = g_value_get_boxed( &value );
		g_value_init( &num_value, G_TYPE_INT64 );
		g_value_set_int64( &num_value, gnc_numeric_num( *n ) );
		g_value_init( &denom_value, G_TYPE_INT64 );
		g_value_set_int64( &denom_value, gnc_numeric_denom( *n ) );
	}

	s = g_strdup_printf( "%s_num", table_row->col_name );
	add_field_to_query( query, s, &num_value );
	g_free( s );
	s = g_strdup_printf( "%s_denom", table_row->col_name );
	add_field_to_query( query, s, &denom_value );
	g_free( s );
}

static GdaQueryCondition*
get_gvalue_numeric_cond( GncGdaBackend* be, QofIdTypeConst obj_name,
				gpointer pObject, const col_cvt_t* table_row, GdaQuery* query )
{
	GValue value;
	GValue num_value;
	GValue denom_value;
	gnc_numeric* n;
	gint64 num;
	gint64 denom;
	gchar* s;
	GdaQueryCondition* num_cond;
	GdaQueryCondition* denom_cond;
	GdaQueryCondition* cond;

	memset( &value, 0, sizeof( GValue ) );
	memset( &num_value, 0, sizeof( GValue ) );
	memset( &denom_value, 0, sizeof( GValue ) );

	get_gvalue_numeric( be, obj_name, pObject, table_row, &value );
	if( G_VALUE_TYPE(&value) != 0 ) {
		n = g_value_get_boxed( &value );
		g_value_init( &num_value, G_TYPE_INT64 );
		g_value_set_int64( &num_value, gnc_numeric_num( *n ) );
		g_value_init( &denom_value, G_TYPE_INT64 );
		g_value_set_int64( &denom_value, gnc_numeric_denom( *n ) );
	}

	s = g_strdup_printf( "%s_num", table_row->col_name );
	num_cond = gnc_gda_create_condition_from_field( query, table_row->col_name, &value );
	g_free( s );
	s = g_strdup_printf( "%s_denom", table_row->col_name );
	denom_cond = gnc_gda_create_condition_from_field( query, table_row->col_name, &value );
	g_free( s );

	cond = gda_query_condition_new( query, GDA_QUERY_CONDITION_NODE_AND );
	gda_query_condition_node_add_child( cond, num_cond, NULL );
	gda_query_condition_node_add_child( cond, denom_cond, NULL );

	return cond;
}

static void
create_numeric_col( GdaServerProvider* server, GdaConnection* cnn,
			xmlNodePtr array_data, const col_cvt_t* table_row )
{
	const gchar* dbms_type;
	gchar* buf;

	dbms_type = gda_server_provider_get_default_dbms_type( server, cnn,
															G_TYPE_INT64 );
	buf = g_strdup_printf( "%s_num", table_row->col_name );
	add_table_column( server, cnn, array_data, buf, dbms_type,
						table_row->size, table_row->flags );
	g_free( buf );
	buf = g_strdup_printf( "%s_denom", table_row->col_name );
	add_table_column( server, cnn, array_data, buf, dbms_type,
						table_row->size, table_row->flags );
	g_free( buf );
}

static col_type_handler_t numeric_handler =
		{ load_numeric, create_numeric_col,
			get_gvalue_numeric_for_query, get_gvalue_numeric_cond };
/* ================================================================= */

static col_type_handler_t*
get_handler( int col_type )
{
	col_type_handler_t* pHandler;

	switch( col_type ) {
		case CT_STRING:
			pHandler = &string_handler;
			break;

		case CT_BOOLEAN:
		case CT_INT:
			pHandler = &int_handler;
			break;

		case CT_INT64:
			pHandler = &int64_handler;
			break;

		case CT_DOUBLE:
			pHandler = &double_handler;
			break;

		case CT_GUID:
			pHandler = &guid_handler;
			break;
				
		case CT_TIMESPEC:
			pHandler = &timespec_handler;
			break;

		case CT_GDATE:
			pHandler = &date_handler;
			break;

		case CT_NUMERIC:
			pHandler = &numeric_handler;
			break;

		default:	/* undefined col type */
			g_assert( FALSE );
	}

	return pHandler;
}

static void retrieve_guid( gpointer pObject, gpointer pValue );

static void 
retrieve_guid( gpointer pObject, gpointer pValue )
{
	GUID** ppGuid = (GUID**)pObject;
	GUID* guid = (GUID*)pValue;

	*ppGuid = guid;
}


// Table to retrieve just the guid
static col_cvt_t guid_table[] =
{
	{ "guid", CT_GUID, 0, 0, NULL, NULL, retrieve_guid },
	{ NULL }
};

const GUID*
gnc_gda_load_guid( GdaDataModel* pModel, gint row )
{
	const GUID* guid;

	gnc_gda_load_object( pModel, row, NULL, &guid, guid_table );

	return guid;
}

void
gnc_gda_load_object( GdaDataModel* pModel, gint row,
					QofIdTypeConst obj_name, gpointer pObject,
					const col_cvt_t* table )
{
	int col;
	QofSetterFunc setter;
	col_type_handler_t* pHandler;

	for( col = 0; table[col].col_name != NULL; col++ ) {
		if( table[col].param_name != NULL ) {
			setter = qof_class_get_parameter_setter( obj_name,
													table[col].param_name );
		} else {
			setter = table[col].setter;
		}
		pHandler = get_handler( table[col].col_type );
		pHandler->load_fn( pModel, row, setter, pObject, &table[col] );
	}
}

/* ================================================================= */
GdaQuery*
gnc_gda_create_select_query( const GncGdaBackend* be, const gchar* table_name )
{
	GdaQuery* query;
	GdaQueryTarget* target;
	GdaQueryField* allFields;

	/* SELECT */
	query = gda_query_new( be->pDict );
	gda_query_set_query_type( query, GDA_QUERY_TYPE_SELECT );

	/* FROM */
	target = gda_query_target_new( query, table_name );
	gda_query_add_target( query, target, NULL );
	g_object_unref( G_OBJECT(target) );

	/* all fields */
	allFields = gda_query_field_all_new( query, table_name );
	gda_query_field_set_visible( allFields, TRUE );
	gda_entity_add_field( GDA_ENTITY(query), GDA_ENTITY_FIELD(allFields) );
	g_object_unref( G_OBJECT(allFields) );

	return query;
}

/* ================================================================= */
GdaObject*
gnc_gda_execute_query( GncGdaBackend* be, GdaQuery* query )
{
	GError* error = NULL;

	GdaObject* ret;

	ret = gda_query_execute( query, NULL, FALSE, &error );

	if( error != NULL ) {
		printf( "SQL error: %s\n", error->message );
	}

	return ret;
}

GdaObject*
gnc_gda_execute_sql( GncGdaBackend* be, const gchar* sql )
{
	GError* error = NULL;

	GdaQuery* query;

	query = gda_query_new_from_sql( be->pDict, sql, &error );
	if( query == NULL ) {
		printf( "SQL error: %s\n", error->message );
		return NULL;
	}
	return gnc_gda_execute_query( be, query );
}

int
gnc_gda_execute_select_get_count( GncGdaBackend* be, const gchar* sql )
{
	GError* error = NULL;
	int count = 0;

	GdaObject* ret;

	ret = gnc_gda_execute_sql( be, sql );
	if( GDA_IS_DATA_MODEL(ret) ) {
		GdaDataModel* pModel = GDA_DATA_MODEL(ret);
		count = gda_data_model_get_n_rows( pModel );
	}

	if( error != NULL ) {
		printf( "SQL error: %s\n", error->message );
	}

	return count;
}

int
gnc_gda_execute_query_get_count( GncGdaBackend* be, GdaQuery* query )
{
	int count = 0;

	GdaObject* ret;

	ret = gnc_gda_execute_query( be, query );
	if( GDA_IS_DATA_MODEL(ret) ) {
		GdaDataModel* pModel = GDA_DATA_MODEL(ret);
		count = gda_data_model_get_n_rows( pModel );
	}

	return count;
}
/* ================================================================= */
static void
get_col_gvalue_for_query( GncGdaBackend* be, QofIdTypeConst obj_name,
						gpointer pObject, const col_cvt_t* table_row,
						GdaQuery* query )
{
	GValue value;
	col_type_handler_t* pHandler;

	pHandler = get_handler( table_row->col_type );
	pHandler->get_gvalue_query_fn( be, obj_name, pObject, table_row, query );
}

static void
get_col_gvalue_for_condition( GncGdaBackend* be, QofIdTypeConst obj_name,
						gpointer pObject, const col_cvt_t* table_row,
						GdaQuery* query )
{
	GValue value;
	col_type_handler_t* pHandler;
	GdaQueryCondition* cond;

	pHandler = get_handler( table_row->col_type );
	cond = pHandler->get_gvalue_cond_fn( be, obj_name, pObject, table_row, query );

	gda_query_set_condition( query, cond );
	g_object_unref( G_OBJECT(cond) );
}

gboolean
gnc_gda_object_is_it_in_db( GncGdaBackend* be, const gchar* table_name,
					QofIdTypeConst obj_name, gpointer pObject,
					const col_cvt_t* table )
{
	GdaQuery* query;
	GValue value;
	int count;

	/* SELECT * FROM */
	query = gnc_gda_create_select_query( be, table_name );

	/* WHERE */
	get_col_gvalue_for_condition( be, obj_name, pObject, &table[0], query );

	count = gnc_gda_execute_query_get_count( be, query );
	if( count == 0 ) {
		return FALSE;
	} else {
		return TRUE;
	}
}

gboolean
gnc_gda_do_db_operation( GncGdaBackend* be,
						E_DB_OPERATION op,
						const gchar* table_name,
						QofIdTypeConst obj_name, gpointer pObject,
						const col_cvt_t* table )
{
	GdaQuery* pQuery;

	if( op == OP_DB_ADD_OR_UPDATE ) {
		if( gnc_gda_object_is_it_in_db( be, table_name, obj_name, pObject, table ) ) {
			pQuery = gnc_gda_build_update_query( be, table_name, obj_name, pObject, table );
		} else {
			pQuery = gnc_gda_build_insert_query( be, table_name, obj_name, pObject, table );
		}
	} else if( op == OP_DB_DELETE ) {
		pQuery = gnc_gda_build_delete_query( be, table_name, obj_name, pObject, table );
	} else if( op == OP_DB_ADD ) {
		pQuery = gnc_gda_build_insert_query( be, table_name, obj_name, pObject, table );
	} else {
		g_assert( FALSE );
	}
	if( pQuery != NULL ) {
		gnc_gda_execute_query( be, pQuery );
		g_object_unref( G_OBJECT(pQuery) );

		return TRUE;
	} else {
		return FALSE;
	}
}

#define INITIAL_SQL_BUF_LEN 500

GdaQuery*
gnc_gda_build_insert_query( GncGdaBackend* be,
							const gchar* table_name,
							QofIdTypeConst obj_name, gpointer pObject,
							const col_cvt_t* table )
{
	GdaQuery* query;
	int col;

	GdaQueryTarget* target;

	/* INSERT */
	query = gda_query_new( be->pDict );
	gda_query_set_query_type( query, GDA_QUERY_TYPE_INSERT );

	/* INTO */
	target = gda_query_target_new( query, table_name );
	gda_query_add_target( query, target, NULL );

	/* VALUES */
	for( col = 0; table[col].col_name != NULL; col++ ) {
		get_col_gvalue_for_query( be, obj_name, pObject, &table[col], query );
	}

	return query;
}

GdaQuery*
gnc_gda_build_update_query( GncGdaBackend* be,
							const gchar* table_name,
							QofIdTypeConst obj_name, gpointer pObject,
							const col_cvt_t* table )
{
	GdaQuery* query;
	int col;

	GdaQueryTarget* target;

	/* UPDATE */
	query = gda_query_new( be->pDict );
	gda_query_set_query_type( query, GDA_QUERY_TYPE_UPDATE );
	target = gda_query_target_new( query, table_name );
	gda_query_add_target( query, target, NULL );

	/* SET */
	for( col = 1; table[col].col_name != NULL; col++ ) {
		get_col_gvalue_for_query( be, obj_name, pObject, &table[col], query );
	}

	/* WHERE */
	get_col_gvalue_for_condition( be, obj_name, pObject, &table[0], query );

	return query;
}

GdaQuery*
gnc_gda_build_delete_query( GncGdaBackend* be,
							const gchar* table_name,
							QofIdTypeConst obj_name, gpointer pObject,
							const col_cvt_t* table )
{
	GdaQuery* query;
	GdaQueryTarget* target;
	GdaQueryField* key;
	GdaQueryField* key_value;

	/* DELETE */
	query = gda_query_new( be->pDict );
	gda_query_set_query_type( query, GDA_QUERY_TYPE_DELETE );

	/* FROM */
	target = gda_query_target_new( query, table_name );
	gda_query_add_target( query, target, NULL );
	g_object_unref( G_OBJECT(target) );

	/* WHERE */
	get_col_gvalue_for_condition( be, obj_name, pObject, &table[0], query );

	return query;
}

/* ================================================================= */
static void
add_table_column( GdaServerProvider* server, GdaConnection* cnn,
			xmlNodePtr array_data, const gchar* arg, const gchar* dbms_type,
			gint size, gint flags )
{
	xmlNodePtr array_row, array_value;
	gchar* buf;

	array_row = xmlNewChild( array_data, NULL, "gda_array_row", NULL );
	array_value = xmlNewChild( array_row, NULL, "gda_array_value", arg );
	xmlSetProp( array_value, "colid", "COLUMN_NAME" );
	array_value = xmlNewChild( array_row, NULL, "gda_array_value", dbms_type );
	xmlSetProp( array_value, "colid", "COLUMN_TYPE" );
	if( size != 0 ) {
		buf = g_strdup_printf( "%d", size );
		array_value = xmlNewChild( array_row, NULL, "gda_array_value", buf );
		xmlSetProp( array_value, "colid", "COLUMN_SIZE" );
		g_free( buf );
	}
	if( (flags & COL_PKEY) != 0 ) {
		array_value = xmlNewChild( array_row, NULL, "gda_array_value", "TRUE" );
		xmlSetProp( array_value, "colid", "COLUMN_PKEY" );
	}
	if( (flags & COL_NNUL) != 0 ) {
		array_value = xmlNewChild( array_row, NULL, "gda_array_value", "TRUE" );
		xmlSetProp( array_value, "colid", "COLUMN_NNUL" );
	}
	if( (flags & COL_AUTOINC) != 0 ) {
		array_value = xmlNewChild( array_row, NULL, "gda_array_value", "TRUE" );
		xmlSetProp( array_value, "colid", "COLUMN_AUTOINC" );
	}
	if( (flags & COL_UNIQUE) != 0 ) {
		array_value = xmlNewChild( array_row, NULL, "gda_array_value", "TRUE" );
		xmlSetProp( array_value, "colid", "COLUMN_UNIQUE" );
	}
}

gboolean
gnc_gda_create_table( GdaConnection* cnn, const gchar* table_name,
					col_cvt_t* col_table, GError** error )
{
	GdaServerOperation *op;
	GdaServerProvider *server;
	
	g_return_val_if_fail( GDA_IS_CONNECTION(cnn), FALSE );
	g_return_val_if_fail( gda_connection_is_opened(cnn), FALSE );
	
	server = gda_connection_get_provider_obj( cnn );
	
	op = gda_server_provider_create_operation( server, cnn, 
						   GDA_SERVER_OPERATION_CREATE_TABLE, NULL, error );
	if( GDA_IS_SERVER_OPERATION(op) ) {
		gint col;
		GType type;
		xmlDocPtr parameters;
		xmlNodePtr root;
		xmlNodePtr table, op_data, array_data, array_row, array_value;
		
		if( table_name == NULL ) {
			g_message( "Table name is NULL!" );      
			g_set_error( error,
					GDA_GENERAL_ERROR, GDA_GENERAL_OBJECT_NAME_ERROR, 
				    "Couldn't create table with a NULL string" );
			return FALSE;    
		}
		
	
		/* Initation of the xmlDoc */
		parameters = xmlNewDoc( "1.0" );
		
		root = xmlNewDocNode( parameters, NULL, "serv_op_data", NULL );
		xmlDocSetRootElement( parameters, root );
		table = xmlNewChild( root, NULL, "op_data", table_name );
		xmlSetProp( table, "path", "/TABLE_DEF_P/TABLE_NAME" );

		op_data = xmlNewChild( root, NULL, "op_data", NULL );
		xmlSetProp( op_data, "path", "/FIELDS_A" );
		array_data = xmlNewChild( op_data, NULL, "gda_array_data", NULL );

		type = 0;
		
		for( col = 0; col_table[col].col_name != NULL; col++ ) {
			gchar* buf;
			gchar *dbms_type = NULL;
			const gchar *arg;
			gint size;
			gint flags;
			col_type_handler_t* pHandler;

			pHandler = get_handler( col_table[col].col_type );

			pHandler->create_col_fn( server, cnn, array_data, &col_table[col] );
		}
		
		if( !gda_server_operation_load_data_from_xml(op, root, error ) ) {
			/* error */
			g_set_error( error, GDA_GENERAL_ERROR, GDA_GENERAL_OPERATION_ERROR, 
				     "The XML operation doesn't exist or could't be loaded" );
			g_object_unref( op );
			xmlFreeDoc( parameters );
			return FALSE;
		} else {
			if( !gda_server_provider_perform_operation( server, cnn, op, error ) ) {
				/* error */
				g_set_error( error,
					GDA_GENERAL_ERROR, GDA_GENERAL_OPERATION_ERROR, 
					"The Server couldn't perform the CREATE TABLE operation!" );
				g_object_unref( op );
				xmlFreeDoc( parameters );
				return FALSE;
			}
		}

		g_object_unref( op );
		xmlFreeDoc( parameters );
	} else {
		g_set_error( error, GDA_GENERAL_ERROR, GDA_GENERAL_OBJECT_NAME_ERROR, 
			    "The Server doesn't support the CREATE TABLE operation!" );
		return FALSE;
	}
	return TRUE;
}

void gnc_gda_create_table_if_needed( GncGdaBackend* be,
						const gchar* table_name, col_cvt_t* col_table )
{
	GdaDictTable* table;
	GError* error = NULL;
	GdaDictDatabase* db;
	
	db = gda_dict_get_database( be->pDict );
	table = gda_dict_database_get_table_by_name( db, table_name );
	if( !GDA_IS_DICT_TABLE(table) ) {
		gnc_gda_create_table( be->pConnection, table_name, col_table, &error );
		if( error != NULL ) {
			printf( "Error creating table: %s\n", error->message );
		}
	}
}
/* ================================================================= */

static void
create_tables_cb( const gchar* type, gpointer data_p, gpointer be_data_p )
{
	GncGdaDataType_t* pData = data_p;
	gda_backend* be_data = be_data_p;

	g_return_if_fail( type != NULL && pData != NULL && be_data != NULL );
	g_return_if_fail( pData->version == GNC_GDA_BACKEND_VERSION );

	if( pData->create_tables != NULL ) {
		(pData->create_tables)( be_data->be );
	}
}

static void
gnc_gda_session_begin(QofBackend *be_start, QofSession *session, 
                   const gchar *book_id,
                   gboolean ignore_lock, gboolean create_if_nonexistent)
{
    GncGdaBackend *be = (GncGdaBackend*) be_start;
	GError* error = NULL;
	gda_backend be_data;
	gchar* book_info;
	gchar* dsn;
	gchar* username;
	gchar* password;

    ENTER (" ");

	be->pClient = gda_client_new();

	/* Split book_id into provider and connection string */
	book_info = g_strdup( book_id );
	dsn = strchr( book_info, ':' );
	*dsn = '\0';
	dsn += 3;
	username = strchr( dsn, ':' );
	if( username != NULL ) {
		*username++ = '\0';
	} else {
		username = "";
	}
	password = strchr( username, ':' );
	if( password != NULL ) {
		*password++ = '\0';
	} else {
		password = "";
	}

	be->pConnection = gda_client_open_connection( be->pClient,
						dsn,
						username, password,
						0,
						&error );
	g_free( book_info );

	if( be->pConnection == NULL ) {
		printf( "SQL error: %s\n", error->message );
		qof_backend_set_error( be_start, ERR_BACKEND_NO_SUCH_DB );
		return;
	}

	// Set up the dictionary
	be->pDict = gda_dict_new();
	gda_dict_set_connection( be->pDict, be->pConnection );
	gda_dict_update_dbms_meta_data( be->pDict, 0, NULL, &error );
	if( error != NULL ) {
		printf( "gda_dict_update_dbms_meta_data() error: %s\n", error->message );
	}

	// Call all object backends to create any required tables
	be_data.ok = FALSE;
	be_data.be = be;
	be_data.inst = NULL;
	qof_object_foreach_backend( GNC_GDA_BACKEND, create_tables_cb, &be_data );

	// Update the dictionary because new tables may exist
	gda_dict_update_dbms_meta_data( be->pDict, 0, NULL, &error );
	if( error != NULL ) {
		printf( "gda_dict_update_dbms_meta_data() error: %s\n", error->message );
	}

	be->pStrHandler = gda_handler_string_new();
	be->pNumHandler = gda_handler_numerical_new();
	be->pDateHandler = gda_handler_time_new_no_locale();

    LEAVE (" ");
}

/* ================================================================= */

static void
gnc_gda_session_end(QofBackend *be_start)
{
    GncGdaBackend *be = (GncGdaBackend*)be_start;
    ENTER (" ");

	if( be->pDict != NULL ) {
		g_object_unref( G_OBJECT(be->pDict) );
		be->pDict = NULL;
	}
	if( be->pConnection != NULL && gda_connection_is_opened( be->pConnection ) ) {
		gda_connection_close( be->pConnection );
	}
	be->pConnection = NULL;
	if( be->pClient != NULL ) {
		g_object_unref( G_OBJECT(be->pClient ) );
		be->pClient = NULL;
	}
	if( be->pStrHandler != NULL ) {
		g_object_unref( G_OBJECT(be->pStrHandler) );
	}
	if( be->pNumHandler != NULL ) {
		g_object_unref( G_OBJECT(be->pNumHandler) );
	}
	if( be->pDateHandler != NULL ) {
		g_object_unref( G_OBJECT(be->pDateHandler) );
	}

    LEAVE (" ");
}

static void
gnc_gda_destroy_backend(QofBackend *be)
{
    g_free(be);
}

/* ================================================================= */

static void
initial_load_cb( const gchar* type, gpointer data_p, gpointer be_data_p )
{
	GncGdaDataType_t* pData = data_p;
	gda_backend* be_data = be_data_p;

	g_return_if_fail( type != NULL && pData != NULL && be_data != NULL );
	g_return_if_fail( pData->version == GNC_GDA_BACKEND_VERSION );

	if( pData->initial_load != NULL ) {
		(pData->initial_load)( be_data->be );
	}
}

static void
gnc_gda_load(QofBackend* be_start, QofBook *book)
{
    GncGdaBackend *be = (GncGdaBackend*)be_start;
	gda_backend be_data;

    ENTER (" ");

	g_assert( be->primary_book == NULL );
	be->primary_book = book;

	/* Load any initial stuff */
	be->loading = TRUE;

	be_data.ok = FALSE;
	be_data.be = be;
	be_data.inst = NULL;
	qof_object_foreach_backend( GNC_GDA_BACKEND, initial_load_cb, &be_data );

	be->loading = FALSE;

	LEAVE( "" );
}

/* ================================================================= */

static void
gnc_gda_sync_all(QofBackend* be, QofBook *book)
{
    GncGdaBackend *fbe = (GncGdaBackend *) be;
    ENTER ("book=%p, primary=%p", book, fbe->primary_book);

    LEAVE ("book=%p", book);
}

/* ================================================================= */
/* Routines to deal with the creation of multiple books. */


static void
gnc_gda_begin_edit (QofBackend *be, QofInstance *inst)
{
}

static void
gnc_gda_rollback_edit (QofBackend *be, QofInstance *inst)
{
}

static void
commit_cb( const gchar* type, gpointer data_p, gpointer be_data_p )
{
	GncGdaDataType_t* pData = data_p;
	gda_backend* be_data = be_data_p;

	g_return_if_fail( type != NULL && pData != NULL && be_data != NULL );
	g_return_if_fail( pData->version == GNC_GDA_BACKEND_VERSION );

	/* If this has already been handled, or is not the correct handler, return */
	g_return_if_fail( strcmp( pData->type_name, be_data->inst->entity.e_type ) == 0 );
	g_return_if_fail( !be_data->ok );

	if( pData->commit != NULL ) {
		(pData->commit)( be_data->be, be_data->inst );
		be_data->ok = TRUE;
	}
}

/* Commit_edit handler - find the correct backend handler for this object
 * type and call its commit handler
 */
static void
gnc_gda_commit_edit (QofBackend *be_start, QofInstance *inst)
{
    GncGdaBackend *be = (GncGdaBackend*)be_start;
	gda_backend be_data;

	/* During initial load where objects are being created, don't commit
	anything */

	if( be->loading ) return;

	printf( "gda_commit_edit(): %s dirty = %d, do_free=%d\n", inst->entity.e_type, inst->dirty, inst->do_free );

	if( !inst->dirty && !inst->do_free && GNC_IS_TRANS(inst) ) {
		gnc_gda_transaction_commit_splits( be, GNC_TRANS(inst) );
	}

	if( !inst->dirty && !inst->do_free ) return;

	be_data.ok = FALSE;
	be_data.be = be;
	be_data.inst = inst;
	qof_object_foreach_backend( GNC_GDA_BACKEND, commit_cb, &be_data );

	if( !be_data.ok ) {
		printf( "gnc_gda_commit_edit(): Unknown object type %s\n",
				inst->entity.e_type );
		return;
	}

	inst->dirty = FALSE;
	qof_book_mark_saved( be->primary_book );
}
/* ---------------------------------------------------------------------- */

/* Query processing */

static const gchar*
convert_search_obj( QofIdType objType )
{
	return (gchar*)objType;
}

static void
handle_and_term( QofQueryTerm* pTerm, gchar* sql )
{
	GSList* pParamPath = qof_query_term_get_param_path( pTerm );
	QofQueryPredData* pPredData = qof_query_term_get_pred_data( pTerm );
	gboolean isInverted = qof_query_term_is_inverted( pTerm );
	GSList* name;
	gchar val[GUID_ENCODING_LENGTH+1];

	strcat( sql, "(" );
	if( isInverted ) {
		strcat( sql, "!" );
	}

	for( name = pParamPath; name != NULL; name = name->next ) {
		if( name != pParamPath ) strcat( sql, "." );
		strcat( sql, name->data );
	}

	if( pPredData->how == QOF_COMPARE_LT ) {
		strcat( sql, "<" );
	} else if( pPredData->how == QOF_COMPARE_LTE ) {
		strcat( sql, "<=" );
	} else if( pPredData->how == QOF_COMPARE_EQUAL ) {
		strcat( sql, "=" );
	} else if( pPredData->how == QOF_COMPARE_GT ) {
		strcat( sql, ">" );
	} else if( pPredData->how == QOF_COMPARE_GTE ) {
		strcat( sql, ">=" );
	} else if( pPredData->how == QOF_COMPARE_NEQ ) {
		strcat( sql, "~=" );
	} else {
		strcat( sql, "??" );
	}

	if( strcmp( pPredData->type_name, "string" ) == 0 ) {
		query_string_t pData = (query_string_t)pPredData;
		strcat( sql, "'" );
		strcat( sql, pData->matchstring );
		strcat( sql, "'" );
	} else if( strcmp( pPredData->type_name, "date" ) == 0 ) {
		query_date_t pData = (query_date_t)pPredData;

		(void)gnc_timespec_to_iso8601_buff( pData->date, val );
		strcat( sql, "'" );
		strncat( sql, val, 4+1+2+1+2 );
		strcat( sql, "'" );
	} else if( strcmp( pPredData->type_name, "numeric" ) == 0 ) {
		query_numeric_t pData = (query_numeric_t)pPredData;

		strcat( sql, "numeric" );
	} else if( strcmp( pPredData->type_name, "guid" ) == 0 ) {
		query_guid_t pData = (query_guid_t)pPredData;
		(void)guid_to_string_buff( pData->guids->data, val );
		strcat( sql, "'" );
		strcat( sql, val );
		strcat( sql, "'" );
	} else if( strcmp( pPredData->type_name, "gint32" ) == 0 ) {
		query_int32_t pData = (query_int32_t)pPredData;

		sprintf( val, "%d", pData->val );
		strcat( sql, val );
	} else if( strcmp( pPredData->type_name, "gint64" ) == 0 ) {
		query_int64_t pData = (query_int64_t)pPredData;

		sprintf( val, "%lld", pData->val );
		strcat( sql, val );
	} else if( strcmp( pPredData->type_name, "double" ) == 0 ) {
		query_double_t pData = (query_double_t)pPredData;

		sprintf( val, "%f", pData->val );
		strcat( sql, val );
	} else if( strcmp( pPredData->type_name, "boolean" ) == 0 ) {
		query_boolean_t pData = (query_boolean_t)pPredData;

		sprintf( val, "%d", pData->val );
		strcat( sql, val );
	} else {
		g_assert( FALSE );
	}

	strcat( sql, ")" );
}

static void
compile_query_cb( const gchar* type, gpointer data_p, gpointer be_data_p )
{
	GncGdaDataType_t* pData = data_p;
	gda_backend* be_data = be_data_p;

	g_return_if_fail( type != NULL && pData != NULL && be_data != NULL );
	g_return_if_fail( pData->version == GNC_GDA_BACKEND_VERSION );

	g_return_if_fail( strcmp( type, be_data->pQueryInfo->searchObj ) == 0 );
	g_return_if_fail( !be_data->ok );

	if( pData->compile_query != NULL ) {
		be_data->pQueryInfo->pCompiledQuery = (pData->compile_query)(
															be_data->be,
															be_data->pQuery );
		be_data->ok = TRUE;
	}
}

static gpointer
gnc_gda_compile_query(QofBackend* pBEnd, QofQuery* pQuery)
{
    GncGdaBackend *be = (GncGdaBackend*)pBEnd;
	GList* pBookList;
	QofIdType searchObj;
	gchar sql[1000];
	gda_backend be_data;
	gnc_gda_query_info* pQueryInfo;

	searchObj = qof_query_get_search_for( pQuery );

	pQueryInfo = g_malloc( sizeof( gnc_gda_query_info ) );

	// Try various objects first
	be_data.ok = FALSE;
	be_data.be = be;
	be_data.pQuery = pQuery;
	pQueryInfo->searchObj = searchObj;
	be_data.pQueryInfo = pQueryInfo;

	qof_object_foreach_backend( GNC_GDA_BACKEND, compile_query_cb, &be_data );
	if( be_data.ok ) {
		return be_data.pQueryInfo;
	}

	pBookList = qof_query_get_books( pQuery );

	/* Convert search object type to table name */
	sprintf( sql, "SELECT * from %s", convert_search_obj( searchObj ) );
	if( !qof_query_has_terms( pQuery ) ) {
		strcat( sql, ";" );
	} else {
		GList* pOrTerms = qof_query_get_terms( pQuery );
		GList* orTerm;

		strcat( sql, " WHERE " );

		for( orTerm = pOrTerms; orTerm != NULL; orTerm = orTerm->next ) {
			GList* pAndTerms = (GList*)orTerm->data;
			GList* andTerm;

			if( orTerm != pOrTerms ) strcat( sql, " OR " );
			strcat( sql, "(" );
			for( andTerm = pAndTerms; andTerm != NULL; andTerm = andTerm->next ) {
				if( andTerm != pAndTerms ) strcat( sql, " AND " );
				handle_and_term( (QofQueryTerm*)andTerm->data, sql );
			}
			strcat( sql, ")" );
		}
	}

	printf( "Compiled: %s\n", sql );
	pQueryInfo->pCompiledQuery =  g_strdup( sql );

	return pQueryInfo;
}

static void
free_query_cb( const gchar* type, gpointer data_p, gpointer be_data_p )
{
	GncGdaDataType_t* pData = data_p;
	gda_backend* be_data = be_data_p;

	g_return_if_fail( type != NULL && pData != NULL && be_data != NULL );
	g_return_if_fail( pData->version == GNC_GDA_BACKEND_VERSION );
	g_return_if_fail( strcmp( type, be_data->pQueryInfo->searchObj ) == 0 );

	g_return_if_fail( !be_data->ok );

	if( pData->free_query != NULL ) {
		(pData->free_query)( be_data->be, be_data->pCompiledQuery );
		be_data->ok = TRUE;
	}
}

static void
gnc_gda_free_query(QofBackend* pBEnd, gpointer pQuery)
{
    GncGdaBackend *be = (GncGdaBackend*)pBEnd;
	gnc_gda_query_info* pQueryInfo = (gnc_gda_query_info*)pQuery;
	gda_backend be_data;

	// Try various objects first
	be_data.ok = FALSE;
	be_data.be = be;
	be_data.pCompiledQuery = pQuery;
	be_data.pQueryInfo = pQueryInfo;

	qof_object_foreach_backend( GNC_GDA_BACKEND, free_query_cb, &be_data );
	if( be_data.ok ) {
		return;
	}

	printf( "gda_free_query(): %s\n", (gchar*)pQueryInfo->pCompiledQuery );
	g_free( pQueryInfo->pCompiledQuery );
	g_free( pQueryInfo );
}

static void
run_query_cb( const gchar* type, gpointer data_p, gpointer be_data_p )
{
	GncGdaDataType_t* pData = data_p;
	gda_backend* be_data = be_data_p;

	g_return_if_fail( type != NULL && pData != NULL && be_data != NULL );
	g_return_if_fail( pData->version == GNC_GDA_BACKEND_VERSION );
	g_return_if_fail( strcmp( type, be_data->pQueryInfo->searchObj ) == 0 );

	g_return_if_fail( !be_data->ok );

	if( pData->run_query != NULL ) {
		(pData->run_query)( be_data->be, be_data->pCompiledQuery );
		be_data->ok = TRUE;
	}
}

static void
gnc_gda_run_query(QofBackend* pBEnd, gpointer pQuery)
{
    GncGdaBackend *be = (GncGdaBackend*)pBEnd;
	gnc_gda_query_info* pQueryInfo = (gnc_gda_query_info*)pQuery;
	gda_backend be_data;

	g_return_if_fail( !be->in_query );

	be->loading = TRUE;
	be->in_query = TRUE;

	qof_event_suspend();

	// Try various objects first
	be_data.ok = FALSE;
	be_data.be = be;
	be_data.pCompiledQuery = pQueryInfo->pCompiledQuery;
	be_data.pQueryInfo = pQueryInfo;

	qof_object_foreach_backend( GNC_GDA_BACKEND, run_query_cb, &be_data );
	be->loading = FALSE;
	be->in_query = FALSE;
	qof_event_resume();
	if( be_data.ok ) {
		return;
	}

	printf( "gda_run_query(): %s\n", (gchar*)pQueryInfo->pCompiledQuery );
}

/* ================================================================= */
static void
gnc_gda_init_object_handlers( void )
{
	gnc_gda_init_commodity_handler();
	gnc_gda_init_account_handler();
	gnc_gda_init_budget_handler();
	gnc_gda_init_price_handler();
	gnc_gda_init_transaction_handler();
	gnc_gda_init_slots_handler();
	gnc_gda_init_schedxaction_handler();
	gnc_gda_init_lot_handler();
}

/* ================================================================= */

static QofBackend*
gnc_gda_backend_new(void)
{
	GncGdaBackend *gnc_be;
	QofBackend *be;

	gnc_be = g_new0(GncGdaBackend, 1);
	be = (QofBackend*) gnc_be;
	qof_backend_init(be);

	be->session_begin = gnc_gda_session_begin;
	be->session_end = gnc_gda_session_end;
	be->destroy_backend = gnc_gda_destroy_backend;

	be->load = gnc_gda_load;
	be->save_may_clobber_data = NULL;

	/* The gda backend treats accounting periods transactionally. */
	be->begin = gnc_gda_begin_edit;
	be->commit = gnc_gda_commit_edit;
	be->rollback = gnc_gda_rollback_edit;

	/* The gda backend uses queries to load data ... */
	be->compile_query = gnc_gda_compile_query;
	be->free_query = gnc_gda_free_query;
	be->run_query = gnc_gda_run_query;

	be->counter = NULL;

	/* The gda backend will not be multi-user (for now)... */
	be->events_pending = NULL;
	be->process_events = NULL;

	be->sync = gnc_gda_sync_all;
	be->load_config = NULL;
	be->get_config = NULL;

    be->export = NULL;

	gnc_be->primary_book = NULL;

	return be;
}

static void
gnc_gda_provider_free (QofBackendProvider *prov)
{
    prov->provider_name = NULL;
    prov->access_method = NULL;
    g_free (prov);
}

G_MODULE_EXPORT const gchar *
g_module_check_init(GModule *module)
{
	QofBackendProvider *prov;

	prov = g_new0 (QofBackendProvider, 1);
    prov->provider_name = "GnuCash LibGDA Backend (MySQL)";
    prov->access_method = "gda";
    prov->partial_book_supported = FALSE;
    prov->backend_new = gnc_gda_backend_new;
    prov->provider_free = gnc_gda_provider_free;
	prov->check_data_type = NULL;
    qof_backend_register_provider (prov);

	gda_init( "gnucash", "2.0", 0, NULL );

	gnc_gda_init_object_handlers();
    return NULL;
}

/* ========================== END OF FILE ===================== */
