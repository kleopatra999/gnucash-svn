/********************************************************************\
 * gnc-frequency.c -- GnuCash widget for frequency editing.         *
 * Copyright (C) 2001,2002 Joshua Sled <jsled@asynchronous.org>     *
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

#include "config.h"

#include <glib.h>
#include <math.h>
#include <time.h>

#include "FreqSpec.h"
#include "dialog-utils.h"
#include "gnc-component-manager.h"
#include "gnc-engine-util.h"
#include "gnc-frequency.h"
#include "gnc-ui-util.h"

static short module = MOD_SX;

/** Private Defs ********************/

typedef enum {
  GNCFREQ_CHANGED,
  LAST_SIGNAL
} GNCF_Signals;

static gint gnc_frequency_signals[LAST_SIGNAL] = { 0 };

/** Private Prototypes ********************/

static void gnc_frequency_class_init( GNCFrequencyClass *klass );

static void freq_option_value_changed( GtkMenuShell *b, gpointer d );
static void start_date_changed( GNCDateEdit *gde, gpointer d );
static void spin_changed_helper( GtkAdjustment *adj, gpointer d );

static void weekly_days_changed( GtkButton *b, gpointer d );

static void monthly_sel_changed( GtkButton *b, gpointer d );
static void semimonthly_sel_changed( GtkButton *b, gpointer d );
static void yearly_sel_changed( GtkButton *b, gpointer d );
static void quarterly_sel_changed( GtkButton *b, gpointer d );
static void triyearly_sel_changed( GtkButton *b, gpointer d );
static void semiyearly_sel_changed( GtkButton *b, gpointer d );

static void year_range_sels_changed( GNCFrequency *gf,
                                     int monthsInRange,
                                     GtkWidget *occurW,
                                     GtkWidget *dayOfMonthW );
static void year_range_menu_helper( GtkWidget *dayOptMenu,
                                    GtkWidget *occurOptMenu,
                                    gint monthsInRange,
                                    time_t startDate );

/** Static Inits ********************/

static const struct pageDataTuple PAGES[] = {
        {  0, UIFREQ_NONE,         "None" },
        {  1, UIFREQ_ONCE,         "Once" },
        {  2, UIFREQ_DAILY,        "Daily" },
        {  3, UIFREQ_DAILY_MF,     "Daily [M-F]" },
        {  4, UIFREQ_WEEKLY,       "Weekly" },
        {  5, UIFREQ_BI_WEEKLY,    "Bi-Weekly" },
        {  6, UIFREQ_SEMI_MONTHLY, "Semi-Monthly" },
        {  7, UIFREQ_MONTHLY,      "Monthly" },
        {  8, UIFREQ_QUARTERLY,    "Quarterly" },
        {  9, UIFREQ_TRI_ANUALLY,  "Tri-Anually" },
        { 10, UIFREQ_SEMI_YEARLY,  "Semi-Yearly" },
        { 11, UIFREQ_YEARLY,       "Yearly" },
        { 0, 0, 0 }
};

static const char *CHECKBOX_NAMES[] = {
        "wd_check_sun",
        "wd_check_mon",
        "wd_check_tue",
        "wd_check_wed",
        "wd_check_thu",
        "wd_check_fri",
        "wd_check_sat",
        NULL
};

/** Implementations ********************/

guint
gnc_frequency_get_type()
{
        static guint gncfreq_type = 0;
        if ( ! gncfreq_type ) {
                GtkTypeInfo gncfreq_info =
                        {
                                "GNCFrequency",
                                sizeof(GNCFrequency),
                                sizeof(GNCFrequencyClass),
                                (GtkClassInitFunc)gnc_frequency_class_init,
                                (GtkObjectInitFunc)gnc_frequency_init,
                                (GtkArgSetFunc)NULL,
                                (GtkArgGetFunc)NULL
                        };
    
                gncfreq_type = gtk_type_unique( gtk_vbox_get_type(), &gncfreq_info );
        }
        return gncfreq_type;
}

static void
gnc_frequency_class_init( GNCFrequencyClass *klass )
{
        GtkObjectClass *objectClass;


        objectClass = (GtkObjectClass*)klass;
        gnc_frequency_signals[GNCFREQ_CHANGED] =
                gtk_signal_new( "changed",
                                GTK_RUN_FIRST,
                                objectClass->type,
                                GTK_SIGNAL_OFFSET( GNCFrequencyClass, changed ),
                                gtk_signal_default_marshaller, GTK_TYPE_NONE, 0 );

        gtk_object_class_add_signals( objectClass,
                                      gnc_frequency_signals,
                                      LAST_SIGNAL );

        klass->changed = NULL;
}

void
gnc_frequency_init( GNCFrequency *gf )
{
        int    i;
        GtkVBox  *vb;
        GtkWidget   *o;
        GtkAdjustment  *adj;

        static const struct optionMenuTuple {
                char *name;
                void (*fn)();
        } optionMenus[] = {
                { "freq_option",        freq_option_value_changed },
                { "semimonthly_first",  semimonthly_sel_changed },
                { "semimonthly_second", semimonthly_sel_changed },
                { "monthly_day",        monthly_sel_changed },
                { "quarterly_occur",    quarterly_sel_changed },
                { "quarterly_day",      quarterly_sel_changed },
                { "triyearly_occur",    triyearly_sel_changed },
                { "triyearly_day",      triyearly_sel_changed },
                { "semiyearly_occur",   semiyearly_sel_changed },
                { "semiyearly_day",     semiyearly_sel_changed },
                { "yearly_month",       yearly_sel_changed },
                { "yearly_day",         yearly_sel_changed },
                { NULL,                 NULL }
        };

        static const struct spinvalTuple {
                char *name;
                void (*fn)();
        } spinVals[] = {
                { "daily_spin",       spin_changed_helper },
                { "dailymf_spin",     spin_changed_helper },
                { "weekly_spin",      spin_changed_helper },
                { "semimonthly_spin", spin_changed_helper },
                { "monthly_spin",     spin_changed_helper },
                { "yearly_spin",      spin_changed_helper },
                { NULL,               NULL }
        };

        gf->gxml = gnc_glade_xml_new( "sched-xact.glade", "gncfreq_vbox" );
        o = glade_xml_get_widget( gf->gxml, "gncfreq_nb" );
        gf->nb = GTK_NOTEBOOK(o);
        o = glade_xml_get_widget( gf->gxml, "freq_option" );
        gf->freqOpt = GTK_OPTION_MENU(o);
        gf->startDate = GNC_DATE_EDIT(gnc_date_edit_new( time(NULL), FALSE, FALSE ));
        /* Add the new widget to the table. */
        {
                GtkWidget *table = glade_xml_get_widget( gf->gxml, "gncfreq_table" );
                gtk_table_attach( GTK_TABLE(table), GTK_WIDGET(gf->startDate),
                                  1, 2, 1, 2,
                                  ( GTK_EXPAND | GTK_FILL ), 0,
                                  0, 0 );
        }
        vb = GTK_VBOX( glade_xml_get_widget( gf->gxml, "gncfreq_vbox" ) );
        gf->vb = vb;
        gtk_container_add( GTK_CONTAINER(&gf->widget), GTK_WIDGET(gf->vb) );

        /* initialize the option menus */
        for ( i=0; optionMenus[i].name != NULL; i++ ) {
                o = glade_xml_get_widget( gf->gxml, optionMenus[i].name );
                gnc_option_menu_init( GTK_WIDGET(o) );
                if ( optionMenus[i].fn != NULL ) {
                        o = gtk_option_menu_get_menu(GTK_OPTION_MENU(o));
                        /* FIXME: having the user-data be a struct of a
                         * calendar name and the GNCFrequency would allow a
                         * single callback fn */
                        gtk_signal_connect( GTK_OBJECT(o), "selection-done",
                                            GTK_SIGNAL_FUNC(optionMenus[i].fn), gf );
                }
        }

        /* initialize the spin buttons */
        for ( i=0; spinVals[i].name != NULL; i++ ) {
                o = glade_xml_get_widget( gf->gxml,
                                          spinVals[i].name );
                if ( spinVals[i].fn != NULL ) {
                        adj = gtk_spin_button_get_adjustment( GTK_SPIN_BUTTON(o) );
                        gtk_signal_connect( GTK_OBJECT(adj), "value_changed",
                                            GTK_SIGNAL_FUNC(spinVals[i].fn), gf );
                }
        }

        /* initialize the weekly::day-of-week checkbox-selection hooks */
        for ( i=0; i<7; i++ ) {
                o = glade_xml_get_widget( gf->gxml, CHECKBOX_NAMES[i] );
                gtk_signal_connect( GTK_OBJECT(o), "clicked",
                                    GTK_SIGNAL_FUNC(weekly_days_changed), gf );
        }

        gtk_widget_show_all( GTK_WIDGET(&gf->widget) );

        /* respond to start date changes */
        gtk_signal_connect( GTK_OBJECT(gf->startDate), "date_changed",
                            GTK_SIGNAL_FUNC(start_date_changed), gf );

}

GtkWidget *
gnc_frequency_new( FreqSpec *fs, GDate *startDate )
{
        GNCFrequency  *toRet;
        toRet = gtk_type_new( gnc_frequency_get_type() );
        gnc_frequency_setup( toRet, fs, startDate );
        return GTK_WIDGET(toRet);
}

void
gnc_frequency_setup( GNCFrequency *gf, FreqSpec *fs, GDate *startDate )
{
        UIFreqType uift;
        int page;
        time_t tmpTT;
        struct tm *tmpTm = NULL;
        GtkWidget *o;
        FreqSpec *subFS;
        GList *list;
        int i;
        const char *str;

        page = -1;
        if ( fs != NULL ) {
                uift = xaccFreqSpecGetUIType( fs );
        } else {
                uift = UIFREQ_NONE;
        }

        for ( i=0; i < UIFREQ_NUM_UI_FREQSPECS+1; i++ ) {
                if ( PAGES[i].uiFTVal == uift ) {
                        page = PAGES[i].idx;
                }
        }
        g_assert( page != -1 );

        gtk_notebook_set_page( gf->nb, page );
        gtk_option_menu_set_history( gf->freqOpt, page );

        if ( fs == NULL ) {
                return;
        }

        /* setup the start date */
        if ( ! g_date_valid(startDate) ) {
                tmpTT = time(NULL);
        } else {
                tmpTm = g_new0( struct tm, 1 );
                g_date_to_struct_tm( startDate, tmpTm );
                tmpTT = mktime( tmpTm );
                g_free( tmpTm );
        }
        gnc_date_edit_set_time( gf->startDate, tmpTT );
 
        switch ( uift ) {
        case UIFREQ_NONE:
                break;
        case UIFREQ_ONCE:
        {
                GDate theDate;
                tmpTm = g_new0( struct tm, 1 );
                /* set the date */
                if ( xaccFreqSpecGetOnce( fs, &theDate ) < 0 ) {
                        PERR( "Inappropriate FreqSpec type "
                              "[gnc-frequency: %d vs. FreqSpec: %d]\n",
                              uift, xaccFreqSpecGetUIType( fs ) );
                        return;
                }
                g_date_to_struct_tm( &theDate, tmpTm );
                gnc_date_edit_set_time( gf->startDate, mktime(tmpTm) );
                g_free( tmpTm );
        }
        break;
        case UIFREQ_DAILY:
        { 
                int dailyMult = -1;
                if ( xaccFreqSpecGetDaily( fs, &dailyMult ) < 0 ) {
                        PERR( "Inappropriate FreqSpec type "
                              "[gnc-frequency: %d vs. FreqSpec: %d]\n",
                              uift, xaccFreqSpecGetUIType( fs ) );
                        return;
                }
                o = glade_xml_get_widget( gf->gxml, "daily_spin" );
                gtk_spin_button_set_value( GTK_SPIN_BUTTON( o ), dailyMult );
        }
        break;
        case UIFREQ_DAILY_MF:
        {
                GList *fsList;
                FreqSpec *subFS;
                int weekMult, dayOfWeek;

                /*  set the mult */
                fsList = xaccFreqSpecCompositeGet( fs );
                if ( g_list_length( fsList ) != 5 ) {
                        PERR( "Invalid Daily[M-F] FreqSpec" );
                        return;
                }
                subFS = (FreqSpec*)fsList->data;
                if ( xaccFreqSpecGetWeekly( subFS, &weekMult, &dayOfWeek ) < 0 ) {
                        PERR( "Inappropriate FreqSpec type "
                              "[gnc-frequency: %d vs. FreqSpec: %d]\n",
                              uift, xaccFreqSpecGetUIType( fs ) );
                        return;
                }
                o = glade_xml_get_widget( gf->gxml, "dailymf_spin" );
                gtk_spin_button_set_value( GTK_SPIN_BUTTON(o), weekMult );
        }
        break;
        case UIFREQ_WEEKLY:
        {
                int weeklyMult = -1;
                int dayOfWeek;

                for ( list = xaccFreqSpecCompositeGet( fs );
                      list; list = g_list_next(list) ) {
                        subFS = (FreqSpec*)(list->data);
                        if ( weeklyMult == -1 ) {
                                if ( subFS == NULL ) {
                                        PERR( "subFS is null\n" );
                                        return;
                                }
                                if ( xaccFreqSpecGetWeekly( subFS,
                                                            &weeklyMult,
                                                            &dayOfWeek ) < 0 ) {
                                        PERR( "Inappropriate FreqSpec type "
                                              "[gnc-frequency: %d, FreqSpec: %d]\n",
                                              uift, xaccFreqSpecGetUIType( fs ) );
                                        return;
                                }
                        } else {
                                int otherWeeklyMult = -1;

                                if ( subFS == NULL ) {
                                        PERR( "subFS is null\n" );
                                        return;
                                }
                                if ( xaccFreqSpecGetWeekly( subFS,
                                                            &otherWeeklyMult,
                                                            &dayOfWeek ) < 0 ) {
                                        PERR( "Inappropriate FreqSpec type "
                                              "[gnc-frequency: %d, FreqSpec: %d]\n",
                                              uift, xaccFreqSpecGetUIType( fs ) );
                                        return;
                                }
                                if ( weeklyMult != otherWeeklyMult ) {
                                        PERR( "Inconsistent weekly FreqSpec "
                                              "multipliers seen "
                                              "[first: %d vs. other: %d]\n",
                                              weeklyMult, otherWeeklyMult );
                                        return;
                                }
                        }
                        if ( dayOfWeek > 6 ) {
                                PERR( "dayOfWeek > 6 [%d]", dayOfWeek );
                                return;
                        }
                        str = CHECKBOX_NAMES[dayOfWeek];
                        o = glade_xml_get_widget( gf->gxml, str );
                        gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(o), TRUE );
                }
                o = glade_xml_get_widget( gf->gxml, "weekly_spin" );
                gtk_spin_button_set_value( GTK_SPIN_BUTTON(o), weeklyMult );
        }
        break;
        case UIFREQ_BI_WEEKLY:
                /*  set the initial date...? */
                break;
        case UIFREQ_SEMI_MONTHLY:
        {
                int monthlyMult;
                int firstDayOfMonth, secondDayOfMonth, monthOffset;
                list = xaccFreqSpecCompositeGet( fs );
                /*  mult */
                o = glade_xml_get_widget( gf->gxml, "semimonthly_spin" );
                subFS = (FreqSpec*)(g_list_nth( list, 0 )->data);
                if ( xaccFreqSpecGetMonthly( subFS, &monthlyMult,
                                             &firstDayOfMonth, &monthOffset ) < 0 ) {
                        PERR( "Inappropriate FreqSpec type "
                              "[gnc-frequency: %d, FreqSpec: %d]\n",
                              uift, xaccFreqSpecGetUIType( fs ) );
                        return;
                }
                gtk_spin_button_set_value( GTK_SPIN_BUTTON(o), monthlyMult );
                /*  first date */
                o = glade_xml_get_widget( gf->gxml, "semimonthly_first" );
                gtk_option_menu_set_history( GTK_OPTION_MENU(o), firstDayOfMonth-1 );
                /*  second date */
                subFS = (FreqSpec*)(g_list_nth( list, 1 )->data);
                o = glade_xml_get_widget( gf->gxml, "semimonthly_second" );
                if ( xaccFreqSpecGetMonthly( subFS, &monthlyMult,
                                             &secondDayOfMonth, &monthOffset ) < 0 ) {
                        PERR( "Inappropriate FreqSpec type\n" );
                        return;
                }
                gtk_option_menu_set_history( GTK_OPTION_MENU(o), secondDayOfMonth-1 );
        }
        break;
        case UIFREQ_MONTHLY:
        {
                int monthlyMult, dayOfMonth, monthOffset;
                if ( xaccFreqSpecGetMonthly( fs, &monthlyMult,
                                             &dayOfMonth, &monthOffset ) < 0 ) {
                        PERR( "Inappropriate FreqSpec type "
                              "[gnc-frequency: %d, FreqSpec: %d]\n",
                              uift, xaccFreqSpecGetUIType( fs ) );
                        return;
                }
                o = glade_xml_get_widget( gf->gxml, "monthly_spin" );
                gtk_spin_button_set_value( GTK_SPIN_BUTTON(o), monthlyMult );
                o = glade_xml_get_widget( gf->gxml, "monthly_day" );
                gtk_option_menu_set_history( GTK_OPTION_MENU(o), dayOfMonth-1 );
                /*  set the day-of-month */
        }
        break;
        case UIFREQ_QUARTERLY:
        {
                int monthlyMult, dayOfMonth, monthOffset;

                if ( xaccFreqSpecGetMonthly( fs, &monthlyMult,
                                             &dayOfMonth, &monthOffset ) < 0 ) {
                        PERR( "Inappropriate FreqSpec type "
                              "[gnc-frequency: %d, FreqSpec: %d]\n",
                              uift, xaccFreqSpecGetUIType( fs ) );
                        return;
                }
                if ( monthlyMult != 3 ) {
                        PERR( "monthly multiplier != 3 [=%d]", monthlyMult );
                        return;
                }
                year_range_menu_helper( glade_xml_get_widget( gf->gxml,
                                                              "quarterly_day" ),
                                        glade_xml_get_widget( gf->gxml,
                                                              "quarterly_occur" ),
                                        3, gnc_date_edit_get_date( gf->startDate ) );
        }
        break;
        case UIFREQ_TRI_ANUALLY:
        {
                int monthlyMult, dayOfMonth, monthOffset;

                if ( xaccFreqSpecGetMonthly( fs, &monthlyMult,
                                             &dayOfMonth, &monthOffset ) < 0 ) {
                        PERR( "Inappropriate FreqSpec type "
                              "[gnc-frequency: %d, FreqSpec: %d]\n",
                              uift, xaccFreqSpecGetUIType( fs ) );
                        return;
                }
                if ( monthlyMult != 4 ) {
                        PERR( "Month-multiplier != 4 [=%d]", monthlyMult );
                        return;
                }
                year_range_menu_helper( glade_xml_get_widget( gf->gxml,
                                                              "triyearly_day" ),
                                        glade_xml_get_widget( gf->gxml,
                                                              "triyearly_occur" ),
                                        4, gnc_date_edit_get_date( gf->startDate ) );
        }
        break;
        case UIFREQ_SEMI_YEARLY:
        {
                int monthlyMult, dayOfMonth, monthOffset;

                if ( xaccFreqSpecGetMonthly( fs, &monthlyMult,
                                             &dayOfMonth, &monthOffset ) < 0 ) {
                        PERR( "Inappropriate FreqSpec type "
                              "[gnc-frequency: %d, FreqSpec: %d]\n",
                              uift, xaccFreqSpecGetUIType( fs ) );
                        return;
                }
                if ( monthlyMult != 6 ) {
                        PERR( "month-mult != 6 [=%d]", monthlyMult );
                        return;
                }
                year_range_menu_helper( glade_xml_get_widget( gf->gxml,
                                                              "semiyearly_day" ),
                                        glade_xml_get_widget( gf->gxml,
                                                              "semiyearly_occur" ),
                                        6, gnc_date_edit_get_date( gf->startDate ) );
        }
        break;
        case UIFREQ_YEARLY:
        {
                int monthlyMult, dayOfMonth, monthOffset;

                if ( xaccFreqSpecGetMonthly( fs, &monthlyMult,
                                             &dayOfMonth, &monthOffset ) < 0 ) {
                        PERR( "Inappropriate FreqSpec type "
                              "[gnc-frequency: %d, FreqSpec: %d]\n",
                              uift, xaccFreqSpecGetUIType( fs ) );
                        return;
                }
                if ( (monthlyMult % 12) != 0) {
                        PERR( "monthly-mult %% 12 != 0 [=%d]", ( monthlyMult % 12 ) );
                        return;
                }

                /* set the mult */
                o = glade_xml_get_widget( gf->gxml, "yearly_spin" );
                gtk_spin_button_set_value( GTK_SPIN_BUTTON(o),
                                           (int)rint(floor(monthlyMult / 12)) );
                tmpTT = gnc_date_edit_get_date( gf->startDate );
                tmpTm = localtime( &tmpTT );
                o = glade_xml_get_widget( gf->gxml, "yearly_month" );
                gtk_option_menu_set_history( GTK_OPTION_MENU(o), monthOffset );
                o = glade_xml_get_widget( gf->gxml, "yearly_day" );
                gtk_option_menu_set_history( GTK_OPTION_MENU(o), dayOfMonth-1 );
        }
        break;
        default:
                PERR( "unknown ui freq type %d [%d, %s]\n",
                      uift, __LINE__, __FILE__ );
                break;
        }

        gtk_signal_emit_by_name( GTK_OBJECT(gf), "changed", NULL );
}

void
gnc_frequency_save_state( GNCFrequency *gf, FreqSpec *fs, GDate *outStartDate )
{
        gint page;
        struct tm *tmpTm;
        gint day;
        GtkWidget *o;
        UIFreqType uift;
        FreqSpec *tmpFS;
        gint tmpInt;
        int i;
        GDate *gd;
        GDate *gd2;
        time_t tmpTimeT;

        /* get the current tab */
        page = gtk_notebook_get_current_page( gf->nb );
        /* save into UIFreqSpec */

        /* We're going to be creating/destroying FreqSpecs, which will cause GUI
           refreshes. :( */
        gnc_suspend_gui_refresh();

        tmpTimeT = gnc_date_edit_get_date( gf->startDate );
        gd = g_date_new();
        g_date_set_time( gd, tmpTimeT );
        if ( outStartDate != NULL ) {
                g_date_set_time( outStartDate, tmpTimeT );
        }
        /*uift = xaccFreqSpecGetUIType( fs );*/
        uift = PAGES[page].uiFTVal;

        /* based on value, parse widget values into FreqSpec */
        switch ( uift ) {
        case UIFREQ_NONE:
                xaccFreqSpecSetNone( fs );
                xaccFreqSpecSetUIType( fs, uift );
                g_date_free( gd );
                break;
        case UIFREQ_ONCE:
                xaccFreqSpecSetOnceDate( fs, gd );
                xaccFreqSpecSetUIType( fs, uift );
                g_date_free( gd );
                break;
        case UIFREQ_DAILY:
                o = glade_xml_get_widget( gf->gxml, "daily_spin" );
                /* FIXME: initial date should be set correctly. */
                {
                        gint foo;

                        foo = gtk_spin_button_get_value_as_int( GTK_SPIN_BUTTON(o) );
                        xaccFreqSpecSetDaily( fs, gd, foo );
                        xaccFreqSpecSetUIType( fs, uift );
                        g_date_free( gd );
                }
                break;
        case UIFREQ_DAILY_MF:
                xaccFreqSpecSetComposite( fs );
                xaccFreqSpecSetUIType( fs, uift );
                o = glade_xml_get_widget( gf->gxml, "dailymf_spin" );
                tmpInt = gtk_spin_button_get_value_as_int( GTK_SPIN_BUTTON(o) );
                /*  Okay.  Assume that the calendar is upgraded to */
                /*  support selecting weeks, returning the Sunday selected. */
                /*  Normalize to sunday. */
                {
                        tmpTm = g_new0( struct tm, 1 );
                        g_date_to_struct_tm( gd, tmpTm );
                        /*  month-day += (week-day - current-week-day ) % 7 */
                        /*  week-day <- 0 */
                        tmpTm->tm_mday -= ( tmpTm->tm_wday ) % 7;
                        g_date_set_time( gd, mktime( tmpTm ) );
                        g_free( tmpTm );
                }

                /*  1 == "mon", 5 == "fri" */
                for ( i=1; i<6; i++ ) {
                        g_date_add_days( gd, 1 );
                        tmpFS = xaccFreqSpecMalloc(gnc_get_current_book ());
                        xaccFreqSpecSetWeekly( tmpFS, gd, tmpInt );
                        xaccFreqSpecCompositeAdd( fs, tmpFS );
                }

                g_date_free( gd );
                break;
        case UIFREQ_WEEKLY:
                xaccFreqSpecSetComposite( fs );
                xaccFreqSpecSetUIType( fs, uift );
                o = glade_xml_get_widget( gf->gxml, "weekly_spin" );
                tmpInt = gtk_spin_button_get_value_as_int( GTK_SPIN_BUTTON(o) );

                /*  assume we have a good calendar that allows week selection. */
                /*  for-now hack: normalize to Sunday. */
                tmpTm = g_new0( struct tm, 1 );
                g_date_to_struct_tm( gd, tmpTm );
                tmpTm->tm_mday -= tmpTm->tm_wday % 7;
                g_date_set_time( gd, mktime( tmpTm ) );

                /*  now, go through the check boxes and add composites based on that date. */
                for ( i=0; CHECKBOX_NAMES[i]!=NULL; i++ ) {
                        o = glade_xml_get_widget( gf->gxml, CHECKBOX_NAMES[i] );
                        if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(o) ) ) {

                                tmpFS = xaccFreqSpecMalloc
                                        (gnc_get_current_book ());
                                xaccFreqSpecSetUIType( tmpFS, uift );

                                gd2 = g_date_new();
                                *gd2 = *gd;
                                /*  Add 'i' days off of Sunday... */
                                g_date_add_days( gd2, i );
                                xaccFreqSpecSetWeekly( tmpFS, gd2, tmpInt );
                                g_date_free( gd2 );
                                xaccFreqSpecCompositeAdd( fs, tmpFS );
                        }
                }
                g_date_free( gd );
                g_free( tmpTm );
                break;
        case UIFREQ_BI_WEEKLY:
                xaccFreqSpecSetUIType( fs, uift );
                o = glade_xml_get_widget( gf->gxml, "biweekly_cal" );
                xaccFreqSpecSetWeekly( fs, gd, 2 );
                g_date_free( gd );
                break;
        case UIFREQ_SEMI_MONTHLY:
                /* FIXME: this is b0rken date calculation for mday>28 */
                xaccFreqSpecSetComposite( fs );
                xaccFreqSpecSetUIType( fs, uift );
    
                o = glade_xml_get_widget( gf->gxml, "semimonthly_spin" );
                tmpInt = gtk_spin_button_get_value_as_int( GTK_SPIN_BUTTON(o) );

                o = glade_xml_get_widget( gf->gxml, "semimonthly_first" );
                day = gnc_option_menu_get_active( GTK_WIDGET(o) )+1;
                tmpFS = xaccFreqSpecMalloc(gnc_get_current_book ());
                tmpTm = g_new0( struct tm, 1 );
                g_date_to_struct_tm( gd, tmpTm );
                if ( day >= tmpTm->tm_mday ) {
                        /* next month */
                        tmpTm->tm_mon += 1;
                }
                /* else, this month */
                tmpTm->tm_mday = day;
                g_date_set_time( gd, mktime( tmpTm ) );
                xaccFreqSpecSetMonthly( tmpFS, gd, tmpInt );
                g_date_free( gd );
                g_free( tmpTm );
                xaccFreqSpecCompositeAdd( fs, tmpFS );

                o = glade_xml_get_widget( gf->gxml, "semimonthly_second" );
                day = gnc_option_menu_get_active( GTK_WIDGET(o) )+1;
                tmpFS = xaccFreqSpecMalloc(gnc_get_current_book ());
                tmpTimeT = gnc_date_edit_get_date( gf->startDate );
                gd = g_date_new();
                g_date_set_time( gd, tmpTimeT );
                tmpTm = g_new0( struct tm, 1 );
                g_date_to_struct_tm( gd, tmpTm );
                if ( day >= tmpTm->tm_mday ) {
                        /* next month */
                        tmpTm->tm_mon += 1;
                }
                /* else, this month */
                tmpTm->tm_mday = day;
                g_date_set_time( gd, mktime( tmpTm ) );
                xaccFreqSpecSetMonthly( tmpFS, gd, tmpInt );
                g_date_free( gd );
                g_free( tmpTm );
                xaccFreqSpecCompositeAdd( fs, tmpFS );

                break;
        case UIFREQ_MONTHLY:
                o = glade_xml_get_widget( gf->gxml, "monthly_spin" );
                tmpInt = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(o));
                tmpTm = g_new0( struct tm, 1 );
                g_date_to_struct_tm( gd, tmpTm );

                o = glade_xml_get_widget( gf->gxml, "monthly_day" );
                day = gnc_option_menu_get_active( GTK_WIDGET(o) ) + 1;
                tmpTm->tm_mday = day;
                g_date_set_time( gd, mktime( tmpTm ) );
                xaccFreqSpecSetMonthly( fs, gd, tmpInt );
                xaccFreqSpecSetUIType( fs, uift );

                g_free( tmpTm );
                g_date_free( gd );
                break;
        case UIFREQ_QUARTERLY:
                xaccFreqSpecSetMonthly( fs, gd, 3 );
                xaccFreqSpecSetUIType( fs, uift );
                g_date_free( gd );
                break;
        case UIFREQ_TRI_ANUALLY:
                xaccFreqSpecSetMonthly( fs, gd, 4 );
                xaccFreqSpecSetUIType( fs, uift );
                g_date_free( gd );
                break;
        case UIFREQ_SEMI_YEARLY:
                xaccFreqSpecSetMonthly( fs, gd, 6 );
                xaccFreqSpecSetUIType( fs, uift );
                g_date_free( gd );
                break;
        case UIFREQ_YEARLY:
                o = glade_xml_get_widget( gf->gxml, "yearly_spin" );
                tmpInt = gtk_spin_button_get_value_as_int( GTK_SPIN_BUTTON(o) );
                xaccFreqSpecSetMonthly( fs, gd, tmpInt * 12 );
                xaccFreqSpecSetUIType( fs, uift );
                g_date_free( gd );
                break;
        default:
                PERR( "Unknown UIFreqType %d [%d, %s]\n",
                      uift, __LINE__, __FILE__ );
                break;
        }
        gnc_resume_gui_refresh();
}

static void
spin_changed_helper( GtkAdjustment *adj, gpointer d )
{
        gtk_signal_emit_by_name( GTK_OBJECT(GNC_FREQUENCY(d)), "changed", NULL );
}

static void
weekly_days_changed( GtkButton *b, gpointer d )
{
        GNCFrequency *gf;

        gf = GNC_FREQUENCY(d);
        gtk_signal_emit_by_name( GTK_OBJECT(gf), "changed", NULL );
}

static void
monthly_sel_changed( GtkButton *b, gpointer d )
{
        GNCFrequency  *gf;
        GtkWidget  *o;
        guint    dayOfMonth;
        struct tm  *tmptm;
        time_t    tmptt;
  
        gf = (GNCFrequency*)d;

        o = glade_xml_get_widget( ((GNCFrequency*)d)->gxml,
                                  "monthly_day" );
        dayOfMonth = gnc_option_menu_get_active( GTK_WIDGET(o) ) + 1;

        tmptt = gnc_date_edit_get_date( gf->startDate );
        tmptm = localtime( &tmptt );
        while ( ! g_date_valid_dmy( dayOfMonth,
                                    tmptm->tm_mon + 1,
                                    tmptm->tm_year+1900 ) ) {
                dayOfMonth -= 1;
        }
        tmptm->tm_mday = dayOfMonth;
        tmptt = mktime( tmptm );
        gnc_date_edit_set_time( gf->startDate, tmptt );

        gtk_signal_emit_by_name( GTK_OBJECT(d), "changed", NULL );
}

static void
semimonthly_sel_changed( GtkButton *b, gpointer d )
{
        GNCFrequency  *gf;
        GtkWidget  *o;
        gint    tmpint;
        time_t    tmptt;
        struct tm  *tmptm;

        gf = (GNCFrequency*)d;

        tmptt = gnc_date_edit_get_date( gf->startDate );
        tmptm = localtime( &tmptt );

        o = glade_xml_get_widget( gf->gxml, "semimonthly_first" );
        tmpint = gnc_option_menu_get_active( GTK_WIDGET(o) )+1;
        o = glade_xml_get_widget( gf->gxml, "semimonthly_second" );
        if ( tmpint > gnc_option_menu_get_active( GTK_WIDGET(o) )+1 ) {
                tmpint = gnc_option_menu_get_active( GTK_WIDGET(o) )+1;
        }

        tmptm->tm_mday = tmpint;
        while ( ! g_date_valid_dmy( tmptm->tm_mday,
                                    tmptm->tm_mon+1,
                                    tmptm->tm_year+1900 ) ) {
                tmptm->tm_mday -= 1;
        }
        tmptt = mktime( tmptm );
        gnc_date_edit_set_time( gf->startDate, tmptt );

        gtk_signal_emit_by_name( GTK_OBJECT(gf), "changed", NULL );
}

static void
quarterly_sel_changed( GtkButton *b, gpointer d )
{
        GNCFrequency *gf;
        gf = (GNCFrequency*)d;
        year_range_sels_changed( gf, 3,
                                 glade_xml_get_widget( gf->gxml, "quarterly_occur" ),
                                 glade_xml_get_widget( gf->gxml, "quarterly_day" ) );
}

static void
triyearly_sel_changed( GtkButton *b, gpointer d )
{
        GNCFrequency *gf;
        gf = (GNCFrequency*)d;
        year_range_sels_changed( gf, 4,
                                 glade_xml_get_widget( gf->gxml, "triyearly_occur" ),
                                 glade_xml_get_widget( gf->gxml, "triyearly_day" ) );
}

static void
semiyearly_sel_changed( GtkButton *b, gpointer d )
{
        GNCFrequency *gf;
        gf = (GNCFrequency*)d;
        year_range_sels_changed( gf, 6,
                                 glade_xml_get_widget( gf->gxml, "semiyearly_occur" ),
                                 glade_xml_get_widget( gf->gxml, "semiyearly_day" ) );
}

static void
year_range_sels_changed( GNCFrequency *gf,
                         int monthsInRange,
                         GtkWidget *occurW,
                         GtkWidget *dayOfMonthW )
{
        int occur, day;
        time_t tmpTT;
        struct tm *tmpTm;

        occur = gnc_option_menu_get_active( occurW );
        day = gnc_option_menu_get_active( dayOfMonthW ) + 1;

        tmpTT = gnc_date_edit_get_date( gf->startDate );
        tmpTm = localtime( &tmpTT );
        tmpTm->tm_mday = day;
        // jump the month to the closest appropriate month.
        // FIXME: this could probably be made more "appropriate".
        tmpTm->tm_mon += occur - (tmpTm->tm_mon % monthsInRange);
        tmpTT = mktime( tmpTm );
        gnc_date_edit_set_time( gf->startDate, tmpTT );

        gtk_signal_emit_by_name( GTK_OBJECT(gf), "changed", NULL );
}

static void
yearly_sel_changed( GtkButton *b, gpointer d )
{
        GNCFrequency  *gf;
        GtkWidget  *o;
        time_t    tmptt;
        struct tm  *tmptm;

        gf = (GNCFrequency*)d;

        tmptt = gnc_date_edit_get_date( gf->startDate );
        tmptm = localtime( &tmptt );

        o = glade_xml_get_widget( gf->gxml, "yearly_month" );
        tmptm->tm_mon = gnc_option_menu_get_active( GTK_WIDGET(o) );
        o = glade_xml_get_widget( gf->gxml, "yearly_day" );
        tmptm->tm_mday = gnc_option_menu_get_active( GTK_WIDGET(o) )+1;

        /* FIXME: correct for
           option_menu_selected_day > min(31,correct_days_in_month)
           problem */
        while ( ! g_date_valid_dmy( tmptm->tm_mday,
                                    tmptm->tm_mon+1,
                                    tmptm->tm_year+1900 ) ) {
                tmptm->tm_mday -= 1;
        }

        tmptt = mktime( tmptm );
        gnc_date_edit_set_time( gf->startDate, tmptt );

        gtk_signal_emit_by_name( GTK_OBJECT(gf), "changed", NULL );
}

static inline guint32 min( guint32 a, guint32 b )
{
        return a > b ? b : a;
}

static inline guint32 max( guint32 a, guint32 b )
{
        return a > b ? a : b;
}

static void
freq_option_value_changed( GtkMenuShell *b, gpointer d )
{
        GNCFrequency *gf = (GNCFrequency*)d;
        int optIdx;
        UIFreqType uift;
        time_t startDate, tmpDate;
        struct tm *tmpTm;
        GtkWidget *o;

        /* Set the new page. */
        optIdx = gnc_option_menu_get_active( GTK_WIDGET(((GNCFrequency*)d)->freqOpt) );
        gtk_notebook_set_page( ((GNCFrequency*)d)->nb, optIdx );

        /* setup initial values for new page, as possible. */
        uift = PAGES[optIdx].uiFTVal;
        startDate = gnc_date_edit_get_date( gf->startDate );
        tmpTm = localtime( &startDate );

        switch ( uift ) {
        case UIFREQ_SEMI_MONTHLY:
        {
                gint tmpDayOfMonth;
                /* first on the <startdate_dom>, then on the <startdate_dom+2w> */
                o = glade_xml_get_widget( gf->gxml, "semimonthly_first" );
                tmpDayOfMonth = tmpTm->tm_mday;
                tmpTm->tm_mday += 14;
                tmpDate = mktime( tmpTm );
                tmpTm = localtime( &tmpDate );
                gtk_option_menu_set_history( GTK_OPTION_MENU(o),
                                             min( tmpTm->tm_mday, tmpDayOfMonth ) - 1 );
                o = glade_xml_get_widget( gf->gxml, "semimonthly_second" );
                gtk_option_menu_set_history( GTK_OPTION_MENU(o),
                                             max( tmpTm->tm_mday, tmpDayOfMonth ) - 1 );
        }
        break;
        case UIFREQ_MONTHLY:
                /* on the <startdate_dom> */
                o = glade_xml_get_widget( gf->gxml, "monthly_day" );
                gtk_option_menu_set_history( GTK_OPTION_MENU(o),
                                             tmpTm->tm_mday - 1 );
                break;
        case UIFREQ_QUARTERLY:
                /* on the <startdate_dom> */
                o = glade_xml_get_widget( gf->gxml, "quarterly_day" );
                gtk_option_menu_set_history( GTK_OPTION_MENU(o),
                                             tmpTm->tm_mday - 1 );
                break;
        case UIFREQ_TRI_ANUALLY:
                /* on the <startdate_dom> */
                o = glade_xml_get_widget( gf->gxml, "triyearly_day" );
                gtk_option_menu_set_history( GTK_OPTION_MENU(o),
                                             tmpTm->tm_mday - 1 );
                break;
        case UIFREQ_SEMI_YEARLY:
                /* on the <startdate_dom> */
                o = glade_xml_get_widget( gf->gxml, "semiyearly_day" );
                gtk_option_menu_set_history( GTK_OPTION_MENU(o),
                                             tmpTm->tm_mday - 1 );
                break;
        case UIFREQ_YEARLY:
                /* on the <startdate_mon>, <startdate_dom> */
                o = glade_xml_get_widget( gf->gxml, "yearly_month" );
                gtk_option_menu_set_history( GTK_OPTION_MENU(o),
                                             tmpTm->tm_mon );
                o = glade_xml_get_widget( gf->gxml, "yearly_day" );
                gtk_option_menu_set_history( GTK_OPTION_MENU(o),
                                             tmpTm->tm_mday - 1 );
                break;
        default:
                /* nuttin can be done, for whatever reason. */
                break;
        }
        gtk_signal_emit_by_name( GTK_OBJECT(gf), "changed", NULL );
}

static void
year_range_menu_helper( GtkWidget *dayOptMenu,
                        GtkWidget *occurOptMenu,
                        gint monthsInRange,
                        time_t startDate )
{
        struct tm  *tmpTm;
        tmpTm = localtime( &startDate );
        gtk_option_menu_set_history( GTK_OPTION_MENU(occurOptMenu),
                                     tmpTm->tm_mon % monthsInRange );
        gtk_option_menu_set_history( GTK_OPTION_MENU(dayOptMenu),
                                     tmpTm->tm_mday - 1 );
}

static void
start_date_changed( GNCDateEdit *gde, gpointer d )
{
        GNCFrequency  *gf;
        GtkWidget  *o;
        struct tm  *tmpTm;
        time_t    dateFromGDE;
        gint    page;
        UIFreqType  uift;
  
        gf = (GNCFrequency*)d;

        dateFromGDE = gnc_date_edit_get_date( gde );

        page = gtk_notebook_get_current_page( gf->nb );
        uift = PAGES[page].uiFTVal;

        o = NULL;

        switch (uift) {
        case UIFREQ_ONCE:      /* FALLTHROUGH */
        case UIFREQ_DAILY:     /* FALLTHROUGH */
        case UIFREQ_DAILY_MF:  /* FALLTHROUGH */
        case UIFREQ_WEEKLY:    /* FALLTHROUGH */
        case UIFREQ_BI_WEEKLY:
                break;

        case UIFREQ_SEMI_MONTHLY:
        {
                gint first_day;
                o = glade_xml_get_widget( gf->gxml, "semimonthly_first" );
                first_day = gnc_option_menu_get_active( GTK_WIDGET(o) )+1;
                o = glade_xml_get_widget( gf->gxml, "semimonthly_second" );
                if ( first_day < gnc_option_menu_get_active(
                             GTK_WIDGET(o) )+1 ) {
                        o = glade_xml_get_widget( gf->gxml,
                                                  "semimonthly_first" );
                }

                tmpTm = localtime( &dateFromGDE );
                gtk_option_menu_set_history( GTK_OPTION_MENU(o),
                                             tmpTm->tm_mday-1 );
        }
        break;
        case UIFREQ_MONTHLY:
                o = glade_xml_get_widget( gf->gxml, "monthly_day" );
                tmpTm = localtime( &dateFromGDE );
                gtk_option_menu_set_history( GTK_OPTION_MENU(o),
                                             (tmpTm->tm_mday-1) );
                break;
        case UIFREQ_QUARTERLY:
                year_range_menu_helper( glade_xml_get_widget(
                                                gf->gxml, "quarterly_day" ),
                                        glade_xml_get_widget(
                                                gf->gxml, "quarterly_occur" ),
                                        3, dateFromGDE );
                break;
        case UIFREQ_TRI_ANUALLY:
                year_range_menu_helper( glade_xml_get_widget(
                                                gf->gxml, "triyearly_day" ),
                                        glade_xml_get_widget(
                                                gf->gxml, "triyearly_occur" ),
                                        4, dateFromGDE );
                break;
        case UIFREQ_SEMI_YEARLY:
                year_range_menu_helper( glade_xml_get_widget(
                                                gf->gxml, "semiyearly_day" ),
                                        glade_xml_get_widget(
                                                gf->gxml, "semiyearly_occur" ),
                                        6, dateFromGDE );
                break;
        case UIFREQ_YEARLY:
                o = glade_xml_get_widget( gf->gxml, "yearly_month" );
                tmpTm = localtime( &dateFromGDE );
                gtk_option_menu_set_history( GTK_OPTION_MENU(o),
                                             tmpTm->tm_mon );
                o = glade_xml_get_widget( gf->gxml, "yearly_day" );
                gtk_option_menu_set_history( GTK_OPTION_MENU(o),
                                             tmpTm->tm_mday-1 );
                break;
        default:
                PERR( "unknown uift value %d\n", uift );
                break;
        }
        gtk_signal_emit_by_name( GTK_OBJECT(gf), "changed", NULL );
}
