/********************************************************************\
 * date.c -- utility functions to handle the date (adjusting, get   * 
 *           current date, etc.) for xacc (X-Accountant)            *
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
 *                                                                  *
 *                                                                  * 
 * TODO: - for now, every year is leap year                         * 
 *                                                                  * 
\********************************************************************/

#include <time.h>

#include "config.h"
#include "date.h"

#define False 0
#define True  1

/** PROTOTYPES ******************************************************/
static int    validateDate( Date *date );

/** GLOBALS *********************************************************/
/** DEFAULT FOR FEBRUARY IS 28 VICE 29 (and patched by year in validateDate() **/
static
char days[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };

static int been_here = 0;

/********************************************************************\
 * adjustDay                                                        *
 *   adds adj to the current day of the month... the resulting day  *
 *   of the month is a valid day of the month                       *
 *                                                                  * 
 * Args:   date - the date structure to edit                        *
 *         adj  - the amount to adjust the day of the month by      *
 * Return: none                                                     *
\********************************************************************/
void    
adjustDay( Date *date, int adj )
  {
  date->day += adj;
  validateDate(date);
  }

/********************************************************************\
 * adjustMonth                                                      *
 *   adds adj to the current month of the year.. the resulting      *
 *   month of the year is corrected to be in the range [1..12],     *
 *   incrementing/decrementing the year if necessary.               * 
 *                                                                  * 
 * Args:   date - the date structure to edit                        *
 *         adj  - the amount to adjust the day of the month by      *
 * Return: none                                                     *
\********************************************************************/
void    
adjustMonth( Date *date, int adj )
  {
  date->month += adj;
  validateDate(date);
  }

/********************************************************************\
 * validateDate                                                     *
 *   ensures that all the fields in date are in the valid range     *
 *   (ie month = [1..12], day is not less than 1, and not greater   *
 *   than last day of the month.).  If fields are not in the valid  * 
 *   range, they are adjusted.                                      * 
 *                                                                  * 
 * Args:   date - the date structure to edit                        *
 * Return: True if date was changed, otherwise False                *
\********************************************************************/
static int
validateDate( Date *date )
  {
  int valid = True;

  /* the "been here" flag prevents infinite recursion */
  if (1 == been_here) return valid;
  been_here = 1;

  /* adjust days in february for leap year */
  if ( ( ( date->year % 4 == 0 ) && ( date->year % 100 != 0 ) )
       || ( date->year % 400 == 0 ) )
    days[1] = 29;
  else 
    days[1] = 28;
  
  /* the "% 12" business is because month might not be valid!*/

  while( date->day > days[(date->month+11) % 12] )
    {
    valid = False;
    date->day -= days[(date->month+11) % 12];
    date->month++;
    }
  while( date->day < 1 )
    {
    valid = False;
    date->month--;
    date->day += days[(date->month+11) % 12];
    }
  while( date->month > 12 )
    {
    valid = False;
    date->month -= 12;
    date->year++;
    }
  while( date->month < 1 )
    {
    valid = False;
    date->month += 12;
    date->year--;
    }
  
  /* try again, in case a year change messed up leap year calcs. */
  validateDate (date);
  been_here = 0;

  return valid;
  }

/********************************************************************\
 * todaysDate                                                       *
 *   takes a (Date *) and fills it in with today's date             *
 *                                                                  * 
 * Args:   date - the struct to be filled in                        *
 * Return: today's date                                             *
\********************************************************************/
Date*   
todaysDate( Date *date )
  {
  time_t t;
  struct tm *theTime;
  
  time(&t);
  theTime = localtime(&t);
  
  date->day   = theTime->tm_mday;
  date->month = theTime->tm_mon + 1;
  date->year  = theTime->tm_year + 1900;
  
  return date;
  }

/********************************************************************\
 * daysInMonth                                                      *
 *   returns the number of days in month. The argument "year" is    *
 *   required to take into account leap years                       *
 *                                                                  * 
 * Args:   month - the current month                                *
 *         year  - the current year                                 *
 * Return: the number of days in month                              *
\********************************************************************/

int    daysInMonth( int month , int year )
  {
  /* adjust for leap year */
  if( month == 2 ) {  
    if ( ( ( 0 == year % 4 ) && ( 0 != year % 100 ) )
         || ( 0 == year % 400 ) ) 
      days[1] = 29;
    else 
      days[1] = 28;
  }
  return days[month-1];
  }

/********************************************************************\
 * datecmp                                                          *
 *   compares date1 and date2                                       *
 *   If you tink you need this function, you probably don't.        *
 *   You should probably use xaccTransOrder instead.  Some          *
 *   if the algorithms used here depend on the superior ordering    *
 *   that xaccTransOrder provides.                                  *
 *                                                                  * 
 * Args:   date1 - the first date to look at                        *
 *         date2 - the second date in the comparison                * 
 * Return: < 0 if date1<date2, == 0 if date1==date2, > 0 otherwise  *
\********************************************************************/
int
datecmp( Date *date1, Date *date2 )
  {
  if( date1 == NULL )
    return 0;
  else if( date2 == NULL )
    return 0;
  else
    {
#ifdef  OLD_DATE_COMPARISON_CODE
    /* to sort properly, must have 32 > days in month, and 500 > 12*32 */
    unsigned int d1 = date1->day + (32 * date1->month) + (500 * date1->year);
    unsigned int d2 = date2->day + (32 * date2->month) + (500 * date2->year);
    
    if( d1 < d2 )
      return -1;
    if( d1 == d2 )
      return 0;
    else
      return 1;
#endif /* OLD_DATE_COMPARISON_CODE */

    /* this date comparison code executes faster than multiply
     * code above, according to Dave Freese of USCG */
    if ( date1->year > date2->year ) return 1;
    if ( date1->year < date2->year ) return -1;
    if ( date1->month > date2->month ) return 1;
    if ( date1->month < date2->month ) return -1;
    if ( date1->day > date2->day ) return 1;
    if ( date1->day < date2->day ) return -1;
    return 0;
    }
  }

/********************************************************************\
 * sprintdate                                                       *
 *   format a date and write it into the given string               *
 *                                                                  *
 * Args:   buf -- the buffer to write the rendered date into        *
 *         date -- the date to render                               *
 *         flag -- indicates whether to render the whole of the     *
 *                 or a portion. Valid values are:                  *
                   DATE_SHORT, DATE_YEAR and DATE_FULL              *
 * Return: number of characters written to string                   *
\********************************************************************/
int
sprintdate( char *buf, Date *date, int flag )
{
  /* by using these variables and not the day, month, and year directly */
  /* it allows us to rearrange the order of them more easily            */
  int a,b,c;

#ifdef UK_DATES
  a=date->day; b=date->month; c=date->year;
#else
  a=date->month; b=date->day; c=date->year;
#endif

  switch (flag) 
    {
    case DATE_SHORT: 
      return sprintf( buf, "%2d/%2d", a, b ); 
    case DATE_YEAR: 
      return sprintf( buf, "%2d", c);
    case DATE_FULL: 
      return sprintf( buf, "%2d/%2d/%4d", a, b, c );
    }
}

/********************************************************************\
 * sscandate                                                        *
 *   parses a date from a given string                              *
 *                                                                  *
 * Args:   in_string -- the string to parse                         *
 *         date -- the date into which the parsed date is placed    *
 *         flag -- indicates whether to parse the whole of the      *
 *                 or a portion. Valid values are:                  *
                   DATE_SHORT, DATE_YEAR and DATE_FULL              *
 * Return: number of characters read from string                    *
\********************************************************************/
int
sscandate( const char *in_string, Date *date, int flags )
{
  int *a,*b,*c; /* pointers to address of day, month and year vars */
  int ret;

  if (!in_string) return 0;

#ifdef UK_DATES
  a=&date->day; b=&date->month; c=&date->year;
#else
  a=&date->month; b=&date->day; c=&date->year;
#endif

  switch (flags)
    {
    case DATE_SHORT:
      ret=sscanf( in_string, "%d/%d", a, b ); break;
    case DATE_YEAR:
      ret=sscanf( in_string, "%d", c ); break;
    case DATE_FULL:
      ret=sscanf( in_string, "%d/%d/%d", a, b, c); break;
    }
  return ret;
}

/********************** END OF FILE *********************************\
\********************************************************************/
