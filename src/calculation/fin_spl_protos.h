/***************************************************************************
 *              -------------------
 *    create   : Tue Jul 11 20:21:18 2000
 *    copyright: (C) 2000 by Terry D. Boldt
 *    email    : tboldt@attglobal.net
 *              -------------------
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/***************************************************************************
 *  Global Function Prototypes
 *  Tue Jul 11 20:21:18 2000
 *
 ***************************************************************************/

#ifndef __FIN_SPL_PROTOS_H__
#define __FIN_SPL_PROTOS_H__

/*==================================================*/
/* expression_parser.c */
/* Line Number:  344 */
parser_env_ptr init_parser(
                           var_store_ptr  predefined_vars,
                           char  radix_point,
                           char  group_char,
                           void          *trans_numeric(char  *digit_str,
                                                        char   radix_point,
                                                        char   group_char,
                                                        char **rstr),
                           void          *numeric_ops(char  op_sym,
                                                      void *left_value,
                                                      void *right_value),
                           void          *negate_numeric(void *value),
                           void           free_numeric(void *numeric_value));

#endif
