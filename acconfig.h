/********************************************************************\
 * config.h -- configuration defines for xacc                       *
 * Copyright (C) 1997 Robin D. Clark                                *
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
 * along with this program; if not, write to the Free Software      *
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.        *
 *                                                                  *
 *   Author: Rob Clark                                              *
 * Internet: rclark@cs.hmc.edu                                      *
 *  Address: 609 8th Street                                         *
 *           Huntington Beach, CA 92648-4632                        *
\********************************************************************/


#ifndef __XACC_CONFIG_H__
#define __XACC_CONFIG_H__

/* Package name and version number */
#undef PACKAGE
#undef VERSION

/* Are we bigendian -- needed for reading binary file format */
#undef  WORDS_BIGENDIAN     

/* Are we compiling for GNOME?  The answer right now is always yes... */
#undef GNOME

/* Do some memory debugging stuff */
#define  DEBUG_MEMORY        

/* Enable debugging stuff */
#define  USE_DEBUG           

/* Standard C headers present */
#undef STDC_HEADERS

/* limits.h header present */
#undef HAVE_LIMITS_H

/* memcpy present */
#undef HAVE_MEMCPY

/* check for stpcpy for Solaris */
#undef HAVE_STPCPY

/* Enable quickfill in register window */
#define  USE_QUICKFILL       

/* Don't color the balance depending on whether positive
 * or negative */
#define  USE_NO_COLOR        

/* If configure found libXpm, then use it */
#define  HAVE_XPM            

/* Use the new XmHTML widdget instead of the old htmlw widget */
#define  HAVE_LIBXMHTML 1

/* use gnomeprint if it's available */
#undef HAVE_LIBGNOMEPRINT

/* misc image and compression libs needed by html widget */
#undef HAVE_ZLIB               
#undef HAVE_PNG             
#undef HAVE_JPEG            

#undef HAVE_PLOTUTILS

/* Configure found the function malloc_usable_size */
#define  HAVE_MALLOC_USABLE_SIZE  

/*** Begin i18n ***/

/* internationalization with gettext */ 
#undef HAVE_GETTEXT

/* internationalization with catgets */ 
#undef HAVE_CATGETS

/* locale.h contains LC_MESSAGES */ 
#undef HAVE_LC_MESSAGES

/* specific locale directory */ 
#undef HAVE_LOCALE_DIR

/* defined if NLS is available */
#undef ENABLE_NLS

/*** End i18n ***/

#endif
