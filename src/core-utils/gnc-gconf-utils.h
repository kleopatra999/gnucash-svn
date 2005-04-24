/********************************************************************\
 * gnc-gconf-utils.h -- utility functions for storing/retrieving    *
 *              data in the GConf database for GnuCash              *
 * Copyright (C) 2005 David Hampton <hampton@employees.org>         *
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

/** @addtogroup Object
    @{ */
/** @addtogroup GConf

    The API in this file is designed to make it easy to use the GConf
    system from within Gnucash.  GConf is a shared key/value storage
    system.

    The main benefits of these routines are that they 1) maintain a
    GConfClient object, 2) convert gnucash internal section names into
    full gconf pathnames, and 3) optionally take care of error
    checking on return values.

    @{ */
/** @file gnc-gconf-utils.h
 *  @brief GConf helper routines.
 *  @author Copyright (C) 2005 David Hampton <hampton@employees.org>
 */


#ifndef GNC_GCONF_UTILS_H
#define GNC_GCONF_UTILS_H

#include <gconf/gconf-client.h>

/** @name GConf Miscellaneous Functions */
/** @{ */

/** Convert a local key name to a full gconf path name.
 *
 *  This function takes a gconf key name and converts it into a fully
 *  qualified gconf path name.  It does this by prepending the
 *  standard path for all gnucash keys.  It the key is already fully
 *  qualified (i.e. begins with a '/' character), this routine does
 *  not change the key.
 *
 *  @param A partial gconf key or section name.  This name is added to
 *  the standard prefix to produce a fully qualified key name.
 *
 *  @return This function returns a string pointer to the fully
 *  qualified path name of the gconf key.  It is the caller's
 *  responsibility to free this string.
 */
char *gnc_gconf_section_name (const char *name);

/** Tell GConf to propagate changes.
 *
 *  This function tells gconf that changes have been made and that is
 *  should propagate its internal state to permanent storage and any
 *  other clients.  This function is a suggestion to gconf, not a
 *  directive, and is therefore should be considered optional.  Doesn't
 *  hurt to call it though if you've made numerous changes to gconf in
 *  a short period of time.
 */
void gnc_gconf_suggest_sync (void);

/** @} */



/** @name GConf Get Functions */
/** @{ */

/** Get a boolean value from GConf.
 *
 *  Retrieve a TRUE/FALSE value from GConf.  The section and key names
 *  provided as arguments are combined with the standard gnucash key
 *  prefix to produce a fully qualified key name.  Either name (but
 *  not both) may be a fully qualified key path name, in which case it
 *  is used as is, without the standard gnucash prefix.  This allows
 *  the program to access keys like standard desktop settings.  Either
 *  name (but not both) may be NULL.
 *
 *  @param section This string provides a grouping of keys within the
 *  GnuCash section of the gconf database.  It can be a simple string
 *  as in "history" for settings that are common to many areas of
 *  gnucash, or it can be a partial path name as in
 *  "dialogs/business/invoice" for setting that only apply to one
 *  specific area of the program.
 *
 *  @param name This string is the name of the particular key within
 *  the named section of gconf.
 *
 *  @param error An optional pointer to a GError structure.  If NULL,
 *  this function will check for any errors returned by GConf and will
 *  display an error message via stdout.  If present, this function
 *  will pass any error back to the calling function for it to handle.
 *
 *  @return This function returns the TRUE or FALSE value stored at
 *  the requested key in the gconf database.  If the key has never
 *  been set, this function passes on the default value returned by
 *  GConf as specified in the schema for this key.  If there is an
 *  error in processing, this function passed on the value of FALSE as
 *  returned by GConf.
 */
gboolean gnc_gconf_get_bool (const gchar *section,
			     const gchar *name,
			     GError **error);

/** Get a boolean value from GConf with no error argument.
 *
 *  Retrieve a TRUE/FALSE value from GConf.  The section and key names
 *  provided as arguments are combined with the standard gnucash key
 *  prefix to produce a fully qualified key name.  Either name (but
 *  not both) may be a fully qualified key path name, in which case it
 *  is used as is, without the standard gnucash prefix.  This allows
 *  the program to access keys like standard desktop settings.  Either
 *  name (but not both) may be NULL.
 *
 *  @param section This string provides a grouping of keys within the
 *  GnuCash section of the gconf database.  It can be a simple string
 *  as in "history" for settings that are common to many areas of
 *  gnucash, or it can be a partial path name as in
 *  "dialogs/business/invoice" for setting that only apply to one
 *  specific area of the program.
 *
 *  @param name This string is the name of the particular key within
 *  the named section of gconf.
 *
 *  @return This function returns the TRUE or FALSE value stored at
 *  the requested key in the gconf database.  If the key has never
 *  been set, this function passes on the default value returned by
 *  GConf as specified in the schema for this key.  If there is an
 *  error in processing, this function passed on the value of FALSE as
 *  returned by GConf.
 *
 * @note This function was intended for use only by the guile wrapper
 * functions.  It should not be called from C code.
 */
gboolean gnc_gconf_get_bool_no_error (const gchar *section,
				      const gchar *name);

/** Get an integer value from GConf.
 *
 *  Retrieve an integer value from GConf.  The section and key names
 *  provided as arguments are combined with the standard gnucash key
 *  prefix to produce a fully qualified key name.  Either name (but
 *  not both) may be a fully qualified key path name, in which case it
 *  is used as is, without the standard gnucash prefix.  This allows
 *  the program to access keys like standard desktop settings.  Either
 *  name (but not both) may be NULL.
 *
 *  @param section This string provides a grouping of keys within the
 *  GnuCash section of the gconf database.  It can be a simple string
 *  as in "history" for settings that are common to many areas of
 *  gnucash, or it can be a partial path name as in
 *  "dialogs/business/invoice" for setting that only apply to one
 *  specific area of the program.
 *
 *  @param name This string is the name of the particular key within
 *  the named section of gconf.
 *
 *  @param error An optional pointer to a GError structure.  If NULL,
 *  this function will check for any errors returned by GConf and will
 *  display an error message via stdout.  If present, this function
 *  will pass any error back to the calling function for it to handle.
 *
 *  @return This function returns the integer value stored at the
 *  requested key in the gconf database.  If the key has never been
 *  set, this function passes on the default value returned by GConf
 *  as specified in the schema for this key.  If there is an error in
 *  processing, this function passed on the value of zero as returned
 *  by GConf.
 */
gint gnc_gconf_get_int (const gchar *section,
			const gchar *name,
			GError **error);

/** Get a string value from GConf.
 *
 *  Retrieve an string value from GConf.  The section and key names
 *  provided as arguments are combined with the standard gnucash key
 *  prefix to produce a fully qualified key name.  Either name (but
 *  not both) may be a fully qualified key path name, in which case it
 *  is used as is, without the standard gnucash prefix.  This allows
 *  the program to access keys like standard desktop settings.  Either
 *  name (but not both) may be NULL.
 *
 *  @param section This string provides a grouping of keys within the
 *  GnuCash section of the gconf database.  It can be a simple string
 *  as in "history" for settings that are common to many areas of
 *  gnucash, or it can be a partial path name as in
 *  "dialogs/business/invoice" for setting that only apply to one
 *  specific area of the program.
 *
 *  @param name This string is the name of the particular key within
 *  the named section of gconf.
 *
 *  @param error An optional pointer to a GError structure.  If NULL,
 *  this function will check for any errors returned by GConf and will
 *  display an error message via stdout.  If present, this function
 *  will pass any error back to the calling function for it to handle.
 *
 *  @return This function returns the string value stored at the
 *  requested key in the gconf database.  If the key has never been
 *  set, this function passes on the default value returned by GConf
 *  as specified in the schema for this key.  If there is an error in
 *  processing, this function passed on the NULL value as returned by
 *  GConf.  It is the callers responsibility to free any string
 *  returned by this function.
 */
char *gnc_gconf_get_string (const gchar *section,
			    const gchar *name,
			    GError **error);

/** Get a list of values from GConf.
 *
 *  Retrieve a list of values from GConf.  This list may be of any
 *  kind of value understoof by GConf, but all values in the list will
 *  be of the same type.  The section and key names provided as
 *  arguments are combined with the standard gnucash key prefix to
 *  produce a fully qualified key name.  Either name (but not both)
 *  may be a fully qualified key path name, in which case it is used
 *  as is, without the standard gnucash prefix.  This allows the
 *  program to access keys like standard desktop settings.  Either
 *  name (but not both) may be NULL.
 *
 *  @param section This string provides a grouping of keys within the
 *  GnuCash section of the gconf database.  It can be a simple string
 *  as in "history" for settings that are common to many areas of
 *  gnucash, or it can be a partial path name as in
 *  "dialogs/business/invoice" for setting that only apply to one
 *  specific area of the program.
 *
 *  @param name This string is the name of the particular key within
 *  the named section of gconf.
 *
 *  @param list_type This enum indicates the type of each item in the
 *  returned list.  This type must match the type off the stored
 *  items.
 *
 *  @param error An optional pointer to a GError structure.  If NULL,
 *  this function will check for any errors returned by GConf and will
 *  display an error message via stdout.  If present, this function
 *  will pass any error back to the calling function for it to handle.
 *
 *  @return This function returns a list of value stored at the
 *  requested key in the gconf database.  If the key has never been
 *  set, this function passes on the default value returned by GConf
 *  as specified in the schema for this key.  If there is an error in
 *  processing, this function passed on the NULL value as returned by
 *  GConf.  It is the callers responsibility to free any memory
 *  returned by this function.  This include the list itself, and any
 *  list data that are string values.
 */
GSList *gnc_gconf_get_list (const gchar *section,
			    const gchar *name,
			    GConfValueType list_type,
			    GError **error);
/** @} */

/** @name GConf Set/Unset Functions */
/** @{ */


/** Store a boolean value into GConf.
 *
 *  Store a boolean value into GConf.  The section and key names
 *  provided as arguments are combined with the standard gnucash key
 *  prefix to produce a fully qualified key name.  Either name (but
 *  not both) may be a fully qualified key path name, in which case it
 *  is used as is, without the standard gnucash prefix.  This allows
 *  the program to access keys like standard desktop settings.  Either
 *  name (but not both) may be NULL.
 *
 *  @param section This string provides a grouping of keys within the
 *  GnuCash section of the gconf database.  It can be a simple string
 *  as in "history" for settings that are common to many areas of
 *  gnucash, or it can be a partial path name as in
 *  "dialogs/business/invoice" for setting that only apply to one
 *  specific area of the program.
 *
 *  @param name This string is the name of the particular key within
 *  the named section of gconf.
 *
 *  @value The TRUE/FALSE value to be stored.
 *
 *  @param error An optional pointer to a GError structure.  If NULL,
 *  this function will check for any errors returned by GConf and will
 *  display an error message via stdout.  If present, this function
 *  will pass any error back to the calling function for it to handle.
 */
void gnc_gconf_set_bool (const gchar *section,
			 const gchar *name,
			 const gboolean value,
			 GError **error);

/** Store an integer value into GConf.
 *
 *  Store an integer into GConf.  The section and key names provided
 *  as arguments are combined with the standard gnucash key prefix to
 *  produce a fully qualified key name.  Either name (but not both)
 *  may be a fully qualified key path name, in which case it is used
 *  as is, without the standard gnucash prefix.  This allows the
 *  program to access keys like standard desktop settings.  Either
 *  name (but not both) may be NULL.
 *
 *  @param section This string provides a grouping of keys within the
 *  GnuCash section of the gconf database.  It can be a simple string
 *  as in "history" for settings that are common to many areas of
 *  gnucash, or it can be a partial path name as in
 *  "dialogs/business/invoice" for setting that only apply to one
 *  specific area of the program.
 *
 *  @param name This string is the name of the particular key within
 *  the named section of gconf.
 *
 *  @value The number to be stored.
 *
 *  @param error An optional pointer to a GError structure.  If NULL,
 *  this function will check for any errors returned by GConf and will
 *  display an error message via stdout.  If present, this function
 *  will pass any error back to the calling function for it to handle.
 */
void gnc_gconf_set_int (const gchar *section,
			const gchar *name,
			const gint value,
			GError **error);

/** Store a string into GConf.
 *
 *  Store a single string into GConf.  The section and key names
 *  provided as arguments are combined with the standard gnucash key
 *  prefix to produce a fully qualified key name.  Either name (but
 *  not both) may be a fully qualified key path name, in which case it
 *  is used as is, without the standard gnucash prefix.  This allows
 *  the program to access keys like standard desktop settings.  Either
 *  name (but not both) may be NULL.
 *
 *  @param section This string provides a grouping of keys within the
 *  GnuCash section of the gconf database.  It can be a simple string
 *  as in "history" for settings that are common to many areas of
 *  gnucash, or it can be a partial path name as in
 *  "dialogs/business/invoice" for setting that only apply to one
 *  specific area of the program.
 *
 *  @param name This string is the name of the particular key within
 *  the named section of gconf.
 *
 *  @value The string to be stored.  GConf will make a copy of this
 *  string, so it is the callers responsibility to free the space used
 *  by this string (if necessary).
 *
 *  @param error An optional pointer to a GError structure.  If NULL,
 *  this function will check for any errors returned by GConf and will
 *  display an error message via stdout.  If present, this function
 *  will pass any error back to the calling function for it to handle.
 */
void gnc_gconf_set_string (const gchar *section,
			   const gchar *name,
			   const gchar *value,
			   GError **error);

/** Store a list of values into GConf.
 *
 *  Store a list of values into GConf.  This list may be of any kind
 *  of value understoof by GConf, but all values in the list must be
 *  of the same type.  The section and key names provided as arguments
 *  are combined with the standard gnucash key prefix to produce a
 *  fully qualified key name.  Either name (but not both) may be a
 *  fully qualified key path name, in which case it is used as is,
 *  without the standard gnucash prefix.  This allows the program to
 *  access keys like standard desktop settings.  Either name (but not
 *  both) may be NULL.
 *
 *  @param section This string provides a grouping of keys within the
 *  GnuCash section of the gconf database.  It can be a simple string
 *  as in "history" for settings that are common to many areas of
 *  gnucash, or it can be a partial path name as in
 *  "dialogs/business/invoice" for setting that only apply to one
 *  specific area of the program.
 *
 *  @param name This string is the name of the particular key within
 *  the named section of gconf.
 *
 *  @param list_type This enum indicates the type of each item in the
 *  list to be stored.
 *
 *  @value The list of items to be stored.  Each item in the list must
 *  be of the type specified.  E.G. If the list_type is
 *  GCONF_VALUE_STRING, then the data field of each element in the
 *  list must be a string pointer.
 *
 *  @param error An optional pointer to a GError structure.  If NULL,
 *  this function will check for any errors returned by GConf and will
 *  display an error message via stdout.  If present, this function
 *  will pass any error back to the calling function for it to handle.
 */
void gnc_gconf_set_list (const gchar *section,
			 const gchar *name,
			 GConfValueType list_type,
			 GSList *value,
			 GError **error);

/** Delete a value from GConf.
 *
 *  Complete remove a value from GConf.  The next attempt to read this
 *  value will return the default as specified in the GConf schema for
 *  this key.  The section and key names provided as arguments are
 *  combined with the standard gnucash key prefix to produce a fully
 *  qualified key name.  Either name (but not both) may be a fully
 *  qualified key path name, in which case it is used as is, without
 *  the standard gnucash prefix.  This allows the program to access
 *  keys like standard desktop settings.  Either name (but not both)
 *  may be NULL.
 *
 *  @param section This string provides a grouping of keys within the
 *  GnuCash section of the gconf database.  It can be a simple string
 *  as in "history" for settings that are common to many areas of
 *  gnucash, or it can be a partial path name as in
 *  "dialogs/business/invoice" for setting that only apply to one
 *  specific area of the program.
 *
 *  @param name This string is the name of the particular key within
 *  the named section of gconf.
 *
 *  @param error An optional pointer to a GError structure.  If NULL,
 *  this function will check for any errors returned by GConf and will
 *  display an error message via stdout.  If present, this function
 *  will pass any error back to the calling function for it to handle.
 */
void gnc_gconf_unset (const gchar *section,
		      const gchar *name,
		      GError **error);

/** @} */

/** @name GConf Notification Functions */
/** @{ */

/** Add a notification callback to GConf.
 *
 *  Add a function that will be called whenever a value within the
 *  specified section of the GConf tree changes.  The section name
 *  provided as an argument is combined with the standard gnucash key
 *  prefix to produce a fully qualified key name.  This name may be a
 *  fully qualified key path name, in which case it is used as is,
 *  without the standard gnucash prefix.  This allows the object to
 *  respond to keys like standard desktop settings.
 *
 *  @param section This string provides a grouping of keys within the
 *  GnuCash section of the gconf database.  It can be a simple string
 *  as in "history" for settings that are common to many areas of
 *  gnucash, or it can be a partial path name as in
 *  "dialogs/business/invoice" for setting that only apply to one
 *  specific area of the program.  Any key changes within this section
 *  will invoke the notification function.
 *
 *  @param callback The function to call when a value changes.  This
 *  function will receive the key/value pair as one argument, and the
 *  'object' argument to this function as another of its arguments.
 */
void gnc_gconf_add_notification (GObject *object,
				 const gchar *section,
				 GConfClientNotifyFunc callback);

/** Remove a callback from GConf.
 *
 *  Remove a GConf callback function previously added with the
 *  gnc_gconf_add_notification function.  The section name must be the
 *  same string provided when the callback function was added.  This
 *  name is used to find/remove the callback.
 *
 *  @param section This string is used to find the correct
 *  notification function to remove from GConf.
 */
void gnc_gconf_remove_notification (GObject *object,
				    const gchar *section);
/** @} */

#endif /* GNC_GCONF_UTILS_H */
/** @} */
/** @} */
