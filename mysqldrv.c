/*
 *	$Id: mysqldrv.c,v 1.7 2000/05/10 17:51:56 inan Exp $
 * Mysqldrv - A MySQL driver for MCAL
 * Copyright (C) 2000 Lauren Matheson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 *
 * Contact Information:
 *
 * Lauren Matheson
 * inan@canada.com
 *
 * http://mcal.sourceforge.net
 * http://mcal.chek.com
 * mcal@lists.chek.com
 */


#include <stdlib.h>
#include <string.h>
#include "mcal.h"
#include "mysqldrv.h"
#include "/usr/include/mysql/mysql.h"

#define MYSQL_VER "0.0"
//#define	DATA_T	struct st_mysql
#define	DATA	((MYSQL*) stream->data)

#define VEVENT_TABLE "_VEVENT (
  ID                      INTEGER UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  CLASS                   ENUM(\"PUBLIC\", \"PRIVATE\", \"CONFIDENTIAL\", \"<other>\") NOT NULL,  /* Max 1 per VEVENT*/
  CLASS_OTHER_VALUE       TEXT,
  CLASS_XPARAM            INTEGER,                /* VALUE_KEY */ 
  CREATED                 TIMESTAMP(14),          /* Max 1 per VEVENT*/
  CREATED_VALUETYPE       ENUM(\"DATE-TIME\",\"DATE\") DEFAULT \"DATE-TIME\",
  CREATED_TZID            INTEGER,                /* VALUE_KEY */
  CREATED_XPARAM          INTEGER,                /* VALUE_KEY */ 
  DESCRIPTION             TEXT,                   /* Max 1 per VEVENT*/ 
  DESCRIPTION_ALTREP      VARCHAR(255),                
  DESCRIPTION_LANGUAGE    VARCHAR(255),                
  DESCRIPTION_XPARAM      INTEGER,                /* VALUE_KEY */
  DTEND                   TIMESTAMP(14),          /* Max 1 per VEVENT no DURATION*/
  DTEND_VALUETYPE         ENUM(\"DATE-TIME\",\"DATE\") DEFAULT \"DATE-TIME\",
  DTEND_TZID              TEXT,
  DTEND_XPARAM            INTEGER,                /* VALUE_KEY */   
  DTSTAMP                 TIMESTAMP(14),          /* Max 1 per VEVENT*/
  DTSTAMP_XPARAM          INTEGER,                /* VALUE_KEY */ 
  DTSTART                 TIMESTAMP(14),          /* Max 1 per VEVENT*/
  DTSTART_VALUETYPE       ENUM(\"DATE-TIME\",\"DATE\") DEFAULT \"DATE-TIME\",
  DTSTART_TZID            INTEGER,                /* VALUE_KEY */
  DTSTART_XPARAM          INTEGER,                /* VALUE_KEY */
  DURATION                INTEGER,                /* Max 1 per VEVENT no DTEND*/
  DURATION_XPARAM         INTEGER,                /* VALUE_KEY */
  GEO                     TINYINT,                /* Max 1 per VEVENT, a marker so that NULL means no GEO info*/
  GEO_LATITUDE            FLOAT(7,3),
  GEO_LONGITUDE           FLOAT(7,3),
  GEO_XPARAM              INTEGER,                /* VALUE_KEY */
  LAST_MODIFIED           TIMESTAMP(14),          /* Max 1 per VEVENT*/
  LAST_MODIFIED_XPARAM    INTEGER,                /* VALUE_KEY */ 
  LOCATION                TEXT,                   /* Max 1 per VEVENT*/ 
  LOCATION_ALTREP         VARCHAR(255),
  LOCATION_LANGUAGE       VARCHAR(255),
  LOCATION_XPARAM         INTEGER,                /*VALUE_KEY*/
  ORGANIZER               TEXT,                   /* Max 1 per VEVENT*/
  ORGANIZER_CN            INTEGER,                /* VALUE_KEY */
  ORGANIZER_DIR           INTEGER,                /* VALUE_KEY */
  ORGANIZER_SENT_BY       INTEGER,                /* VALUE_KEY */
  ORGANIZER_LANGUAGE      INTEGER,                /* VALUE_KEY */
  ORGANIZER_XPARAM        INTEGER,                /* VALUE_KEY */
  PRIORITY                INTEGER,                /* Max 1 per VEVENT*/
  PRIORITY_XPARAM         INTEGER,                /* VALUE_KEY */
  RECURRENCE_ID           TIMESTAMP(14),          /* Max 1 per VEVENT*/
  RECURRENCE_ID_VALUETYPE INTEGER,                /* VALUE_KEY */
  RECURRENCE_ID_RANGE     ENUM(\"THISANDPRIOR\", \"THISANDFUTURE\"),
  RECURRENCE_ID_TZID      INTEGER,                /* VALUE_KEY */
  RECURRENCE_ID_XPARAM    INTEGER,                 /* VALUE_KEY */
  SUMMARY                 TINYTEXT,               /* Max 1 per VEVENT*/
  SUMMARY_ALTREP          INTEGER,                /* VALUE_KEY */
  SUMMARY_LANGUAGE        INTEGER,                /* VALUE_KEY */
  SUMMARY_XPARAM          INTEGER,                /* VALUE_KEY */
  SEQUENCE                INTEGER,                /* Max 1 per VEVENT*/
  SEQUENCE_XPARAM         INTEGER,                /* VALUE_KEY */
  STATUS                  ENUM(\"TENTATIVE\",\"CONFIRMED\",\"CANCELLED\",\"NEEDS-ACTION\",
                               \"COMPLETED\",\"IN-PROCESS\",\"DRAFT\",\"FINAL\"), /* Max 1 per VEVENT*/
  STATUS_XPARAM                   INTEGER,                 /* VALUE_KEY */
  TRANSP                  ENUM(\"OPQQUE\",\"TRANSPARENT\",\"OPAQUE-NOCONFLICTS\",
                               \"TRANSPARENT-NOCONFLICTS\") NOT NULL DEFAULT \"TRANSPARENT\", /* Max 1 per VEVENT*/
  TRANSP_XPARAM           INTEGER,                /* VALUE_KEY */
  UID                     VARCHAR(255) NOT NULL,  /* Max 1 per VEVENT*/
  UID_XPARAM              INTEGER,                /* VALUE_KEY */
  URL                     TEXT,                /* Max 1 per VEVENT*/
  URL_XPARAM              INTEGER,                 /* VALUE_KEY */                 
  UNIQUE KEY UID(UID)
  )"
/* would also like to index on: but they can be null... hmmm...
 * KEY DTSTART(DTSTART),
 * KEY DTEND(DTEND)
 */

/* These are the VEVENT elements of unlimited number, so will have seperate tables joined to VEVENT.ID
  ATTACH                  
  ATTENDEE                
  CATEGORIES              Already included
  COMMENT                 
  CONTACT                 
  EXDATE                  
  EXRULE                  
  METHOD                  
  RDATE_KEY               
  RELATED_TO              
  RESOURCES               
  RRULE                   Already included
  X_PROP_KEY              Already included       
  ALARM_KEY
                 
  The only other table is XPARAM which lists parameter extensions for most fields.  This is included, but not yet used
*/


#define CATEGORIES_TABLE "_CATEGORIES (
           ID                      INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY,
           VALUE_KEY               INTEGER NOT NULL,
           VALUE                   TEXT,
           LANGUAGE                INTEGER,                /* VALUE_KEY */
           XPARAM                  INTEGER                 /* VALUE_KEY */
   )"
#define X_PROP_TABLE "_X_PROP (
           ID                       INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY,
           VALUE_KEY                INTEGER NOT NULL,
           VALUE                    TEXT NOT NULL,
           NAME                     VARCHAR(252) NOT NULL,
           PARAMS                   INTEGER,             /* VALUE_KEY (XPARAM)*/
           KEY  VALUE_KEY(VALUE_KEY),
           UNIQUE UNIQUE_KEY(VALUE_KEY, NAME)
   )"
#define XPARAM_TABLE "_XPARAM (
           ID                       INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY,
           VALUE_KEY                INTEGER NOT NULL,
           VALUE                    TEXT NOT NULL,
           NAME                     VARCHAR(252) NOT NULL,
           KEY  VALUE_KEY(VALUE_KEY),
           UNIQUE UNIQUE_KEY(VALUE_KEY, NAME)
   )"
#define RRULE_TABLE "_RRULE (
           ID                       INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY,
           VALUE_KEY                INTEGER NOT NULL,
           FREQ                  ENUM(\"SECONDLY\", \"MINUTELY\", \"HOURLY\",
                                   \"DAILY\", \"WEEKLY\", \"MONTHLY\", \"YEARLY\"),
           UNTIL                 TIMESTAMP(14),
           COUNT                 INTEGER,
           INTERVAL_VAL          INTEGER,
           BYSECOND              SET(\"1\", \"2\", \"3\", \"4\", \"5\", \"6\", \"7\", \"8\",
                                     \"9\", \"10\", \"11\", \"12\", \"13\", \"14\", \"15\",
                                     \"16\", \"17\", \"18\", \"19\", \"20\", \"21\", \"22\",
                                     \"23\", \"24\", \"25\", \"26\", \"27\", \"28\", \"29\",
                                     \"30\", \"31\", \"32\", \"33\", \"34\", \"35\", \"36\",
                                     \"37\", \"38\", \"39\", \"40\", \"41\", \"42\", \"43\",
                                     \"44\", \"45\", \"46\", \"47\", \"47\", \"48\", \"49\",
                                     \"50\", \"51\", \"52\", \"53\", \"54\", \"55\", \"56\",
                                     \"57\", \"58\", \"59\"),
           BYMINUTE              SET(\"1\", \"2\", \"3\", \"4\", \"5\", \"6\", \"7\", \"8\",
                                     \"9\", \"10\", \"11\", \"12\", \"13\", \"14\", \"15\",
                                     \"16\", \"17\", \"18\", \"19\", \"20\", \"21\", \"22\",
                                     \"23\", \"24\", \"25\", \"26\", \"27\", \"28\", \"29\",
                                     \"30\", \"31\", \"32\", \"33\", \"34\", \"35\", \"36\",
                                     \"37\", \"38\", \"39\", \"40\", \"41\", \"42\", \"43\",
                                     \"44\", \"45\", \"46\", \"47\", \"47\", \"48\", \"49\",
                                     \"50\", \"51\", \"52\", \"53\", \"54\", \"55\", \"56\",
                                     \"57\", \"58\", \"59\"),
           BYHOUR                SET(\"1\", \"2\", \"3\", \"4\", \"5\", \"6\", \"7\", \"8\",
                                     \"9\", \"10\", \"11\", \"12\", \"13\", \"14\", \"15\",
                                     \"16\", \"17\", \"18\", \"19\", \"20\", \"21\", \"22\",
                                     \"23\"),
           BYDAY                 TINYTEXT,
           BYMONTHDAY            SET(\"1\", \"2\", \"3\", \"4\", \"5\", \"6\", \"7\", \"8\",
                                     \"9\", \"10\", \"11\", \"12\", \"13\", \"14\", \"15\",
                                     \"16\", \"17\", \"18\", \"19\", \"20\", \"21\", \"22\",
                                     \"23\", \"24\", \"25\", \"26\", \"27\", \"28\", \"29\",
                                     \"30\", \"31\", \"-1\", \"-2\", \"-3\", \"-4\", \"-5\",
                                     \"-6\", \"-7\", \"-8\", \"-9\", \"-10\", \"-11\",
                                     \"-12\", \"-13\", \"-14\", \"-15\", \"-16\", \"-17\",
                                     \"-18\", \"-19\", \"-20\", \"-21\", \"-22\", \"-23\",
                                     \"-24\", \"-25\", \"-26\", \"-27\", \"-28\", \"-29\",
                                     \"-30\", \"-31\"),
           BYYEARDAY             TINYTEXT,
           BYWEEKNO              TINYTEXT,
           BYMONTH               SET(\"1\", \"2\", \"3\", \"4\", \"5\", \"6\", \"7\", \"8\",
                                     \"9\", \"10\", \"11\", \"12\"),
           BYSETPOS              TINYTEXT,
           WKST                  SET(\"SU\", \"MO\", \"TU\", \"WE\", \"TH\", \"FR\", \"SA\"),
           XPARAM                INTEGER,                 /* VALUE_KEY */
           KEY  VALUE_KEY(VALUE_KEY)
   )"

static void		mysqldrv_freestream(CALSTREAM *stream);
static unsigned long mysqldrv_mysql_query(CALSTREAM *stream, const char *query);
static void         mysqldrv_make_timestamp ( char *output, const datetime_t *value );
static void         mysqldrv_set_recur (CALEVENT *event, const char *freq, const char *enddate, const char *interval, const char *byday);
//static bool		mysqldrv_validuser(const char *username,const char *password);
//static bool		mysqldrv_userexists(const char *username);

static bool		mysqldrv_valid ( const CALADDR *addr );
static CALSTREAM*	mysqldrv_open (	CALSTREAM *stream,
                                    const CALADDR *addr,
                                    long options );
static CALSTREAM*	mysqldrv_close( CALSTREAM *stream,
                                    long options );
static bool		mysqldrv_ping ( CALSTREAM *stream );
static bool		mysqldrv_create ( CALSTREAM *stream,
                                        const char *calendar );
static bool		mysqldrv_search_range(	CALSTREAM *stream,
						const datetime_t *start,
						const datetime_t *end);
static bool		mysqldrv_search_alarm(	CALSTREAM *stream,
						const datetime_t *when);
static bool		mysqldrv_fetch(	CALSTREAM *stream,
						unsigned long id,
						CALEVENT **event);
static bool		mysqldrv_append(	CALSTREAM *stream,
						const CALADDR *addr,
						unsigned long *id,
						CALEVENT *event);
static bool		mysqldrv_remove(	CALSTREAM *stream,
						unsigned long id);
static bool		mysqldrv_snooze(	CALSTREAM *stream,
					unsigned long id);
static bool		mysqldrv_store(   CALSTREAM *stream, 
					CALEVENT *event);

CALDRIVER mysqldrv_driver =
{
	mysqldrv_valid,
	mysqldrv_open,
	mysqldrv_close,
	mysqldrv_ping,
	mysqldrv_create,
	mysqldrv_search_range,
	mysqldrv_search_alarm,
	mysqldrv_fetch,
	mysqldrv_append,
	mysqldrv_remove,
	mysqldrv_snooze,
	mysqldrv_store,
};

void
mysqldrv_freestream(CALSTREAM *stream)
{
	if (stream) {
		if (DATA) {
			mysql_close( DATA );
			free(DATA);
		}
		free(stream);
	}
}

/* returns unique index, 0 or null -- silly me, for an integer null is 0.. 
 */
unsigned long
mysqldrv_mysql_query(CALSTREAM *stream, const char *query) {
    unsigned long output;

    output = mysql_query(DATA,query);

//    cc_vlog (query,"");
//    cc_vlog (mysql_error(DATA),"");
    if (output == 0) {
//        output = mysql_insert_id(DATA);
        output = true;
    } else {
        output = false;
    }
    
    return output;
}

bool
mysqldrv_valid(const CALADDR *addr)
{
    bool output;
    
    if (!addr->proto || strcasecmp(addr->proto, "mysql")) {
        output = false;
    } else {
        output = true;
    }
    
    return output;
}

bool
mysqldrv_ping(CALSTREAM *stream)
{
    bool output;

    if (mysql_ping( DATA ) == 0) {
        output = true;
    } else {
        output = false;
    }
    
    return output;
}


#define CREATE_TABLE(tablename) {        \
    strcpy (buffer, "CREATE TABLE ");    \
    strcat (buffer, calendar);           \
    strcat (buffer, tablename);          \
    mysqldrv_mysql_query(stream, buffer);\
}
bool
mysqldrv_create( CALSTREAM *stream, const char *calendar ) {
    bool output;
    char buffer[6000];

    CREATE_TABLE( CATEGORIES_TABLE );
    CREATE_TABLE( VEVENT_TABLE );
    CREATE_TABLE( X_PROP_TABLE );
    CREATE_TABLE( XPARAM_TABLE );
    CREATE_TABLE( RRULE_TABLE );

    return output;
}
#undef CREATE_TABLE


/* Clears memory allocated in _open.
 */
CALSTREAM*
mysqldrv_close(CALSTREAM *stream, long options)
{
	mysqldrv_freestream(stream);
	return NULL;
}



/* Must have a valid address 
 * Allocates memory for:
 *  - stream - pointer is *stream parameter
 *  - DATA ((*MYSQL)stream->data) - pointer is stream->data
 * This memory is realeased in _freestream called by a failed open here, 
 *  or by _close. 
 */
CALSTREAM*
mysqldrv_open(CALSTREAM *stream, const CALADDR *addr, long options) {
	const char	*username = NULL;
	const char	*password = NULL;

	/* get valid user */
	cc_login(&username, &password);
	if (username == NULL) {
		goto fail;
	}
//	add user/password validation, reusable streams.
	
//	if (!reopen) {
		if ((stream = calloc(1, sizeof(*stream))) == NULL)
			goto fail;
		if ((DATA = calloc(1, sizeof(*DATA))) == NULL)
			goto fail;
//	}	
	
	mysql_init( DATA );
	if (!mysql_real_connect(DATA,addr->host,username,password,"mcal",0,NULL,0)) {
		goto fail;
	}
	
	return stream;
fail:
	mysqldrv_freestream(stream);
	return NULL;
}

void
mysqldrv_make_timestamp ( char *output, const datetime_t *value ) {
    char temp_str[6];
    sprintf (output ,"%04u%02u%02u", value->year, value->mon, value->mday);
    if (dt_hastime(value)) {
        sprintf(temp_str,	"%02u%02u%02u", value->hour, value->min, value->sec);
        strcat (output, temp_str);
    }
}

void
mysqldrv_set_recur (CALEVENT *event, const char *freq, const char *enddate, const char *interval, const char *byday) {
    recur_t temp_recur;
    byday_t temp_byday;

    ical_get_recur_freq(&temp_recur, freq);
    event->recur_type = temp_recur;
    switch(temp_recur){
        case RECUR_WEEKLY:
            ical_get_byday(&temp_byday, byday);
            event->recur_data.weekly_wday = temp_byday.weekdays;
            break;
        case RECUR_MONTHLY_WDAY:
            //even though we've stored extra monthly_wday info it is ignored by mcal right now
        case RECUR_NONE:
        case RECUR_DAILY:
        case RECUR_MONTHLY_MDAY:
        case RECUR_YEARLY:
        case NUM_RECUR_TYPES:
            break;
    }
    cal_decode_dt(&(event)->recur_enddate, enddate);
    event->recur_interval = atol(interval);
}


bool
mysqldrv_search_range(	CALSTREAM *stream, const datetime_t *start, const datetime_t *end) {
    bool error = false;
    char buffer [10000];
    char folder [100];
    char startstamp [14];
    char endstamp [14];
    char temp_str [1000];
    MYSQL_RES *query_result;
    MYSQL_ROW row;
    CALEVENT *event;
       
    strcpy( folder, DATA->user ); 
    mysqldrv_make_timestamp( startstamp, start );
    mysqldrv_make_timestamp( endstamp, end );
    
    sprintf (buffer, "SELECT %s_VEVENT.UID, %s_RRULE.ID, DATE_FORMAT(%s_VEVENT.DTSTART,'Ymd'), %s_RRULE.FREQ, DATE_FORMAT(%s_RRULE.UNTIL,'Ymd'), %s_RRULE.INTERVAL_VAL, %s_RRULE.BYDAY FROM %s_VEVENT LEFT JOIN %s_RRULE ON %s_VEVENT.ID = %s_RRULE.VALUE_KEY ", folder, folder, folder, folder, folder, folder, folder, folder, folder, folder, folder);
    sprintf (temp_str, "WHERE (%s_VEVENT.DTSTART <= %s) AND ( (%s_VEVENT.DTEND >= %s) OR (%s_VEVENT.DTSTART >= %s) OR (%s_RRULE.UNTIL >= %s) )", folder, endstamp, folder, startstamp, folder, startstamp, folder, startstamp);
    strcat (buffer, temp_str);

    if ( mysqldrv_mysql_query(stream, buffer) ) {
        // the records exist
        query_result = mysql_store_result( DATA );
        while ((row = mysql_fetch_row( query_result ))) {

            if ( row[1]==0x0 ) {
                //_RRULE.ID is null => no recurrence to worry about, just include it.
                cc_searched(atol(row[0]));
            } else {
                datetime_t  clamp = DT_INIT;

                event = calevent_new();
                cal_decode_dt(&(event)->start, row[2]);
                mysqldrv_set_recur (event, row[3], row[4], row[5], row[6]);
                calevent_next_recurrence(event, &clamp, stream->startofweek);

                if (!start) {
                    dt_setdate(&clamp, 1, JANUARY, 1);
                } else {
                    dt_setdate(&clamp, start->year, start->mon, start->mday);
                }

                if ( dt_hasdate(&clamp) && !(end && dt_compare(&clamp, end) > 0))	{
                    cc_searched(atol(row[0]));
                }
                calevent_free(event);
            }
        }
        mysql_free_result( query_result );
    }
    return !error;
}

bool
mysqldrv_search_alarm(	CALSTREAM *stream, const datetime_t *when) {
    bool error = false;
    char buffer [10000];
    char folder [100];
    char whenstamp [14];
    char temp_str [1000];
    MYSQL_RES *query_result;
    MYSQL_ROW row;
       
    strcpy( folder, DATA->user ); 
    mysqldrv_make_timestamp( whenstamp, when );
    
    sprintf (buffer, "SELECT %s_VEVENT.UID FROM %s_VEVENT LEFT JOIN %s_X_PROP ON %s_VEVENT.ID = %s_X_PROP.VALUE_KEY ", folder, folder, folder, folder, folder);
    sprintf (temp_str, "WHERE (DATE_SUB(%s_VEVENT.DTSTART, INTERVAL %s_X_PROP.VALUE MINUTE) <= %s) AND ( (%s_VEVENT.DTEND >= %s) OR (%s_VEVENT.DTSTART >= %s) )", folder, folder, whenstamp, folder, whenstamp, folder, whenstamp);
    strcat (buffer, temp_str);

    if ( mysqldrv_mysql_query(stream, buffer) ) {
        // the records exist
        query_result = mysql_store_result( DATA );
        while ((row = mysql_fetch_row( query_result ))) {
            cc_searched(atol(row[0]));
        }
        mysql_free_result( query_result );
    }
    return !error;
}

#define ERRTEST error = error || !
bool
mysqldrv_fetch(	CALSTREAM *stream, unsigned long uid, CALEVENT **event){
    bool error = false;
    char *id;
    char buffer [10000];
    char temp_string [1000];
    char folder[40];
    MYSQL_RES *query_result;
    MYSQL_ROW row;

    strcpy (folder, DATA->user);
    // get record id from uid
    strcpy ( buffer, "SELECT \
        ID, \
        CLASS LIKE 'PUBLIC', \
        DESCRIPTION,\
        CONCAT( DATE_FORMAT(DTEND,'Ymd'), IF(DTEND_VALUETYPE='DATE-TIME',CONCAT('T',DATE_FORMAT(DTEND,'His'),'Z'),'') ),\
        CONCAT( DATE_FORMAT(DTSTART,'Ymd'), IF(DTSTART_VALUETYPE='DATE-TIME',CONCAT('T',DATE_FORMAT(DTSTART,'His'),'Z'),'') ),\
        SUMMARY, \
        UID");
    sprintf( temp_string," FROM %s_VEVENT WHERE UID=%lu", folder, uid );
    strcat (buffer, temp_string);

    if ( mysqldrv_mysql_query(stream, buffer) ) {
        // the record exists in VEVENT
        query_result = mysql_store_result( DATA );
        if (query_result->row_count == 0) {
	    // no data
	    error = true;
        } else {
            row = mysql_fetch_row( query_result );

            id = strdup (row[0]);        
            (*event)->public = atoi(row[1]);
            (*event)->description = strdup(row[2]);
            cal_decode_dt(&(*event)->start, row[3]);
    	    //dt_hasdate(&(*event)->start);
            cal_decode_dt(&(*event)->end, row[4]);
	    //dt_hasdate(&(*event)->end);
            (*event)->title = strdup(row[5]);
            (*event)->id = atol(row[6]);
            // query_result freed at the very end

            sprintf (buffer, "SELECT NAME, VALUE FROM %s_X_PROP WHERE VALUE_KEY=%s",folder, id);
            if ( mysqldrv_mysql_query(stream, buffer) ) {
                // there are x prop's for this event
                query_result = mysql_store_result( DATA );
                
                while ((row = mysql_fetch_row( query_result ))) {
                    if (strcmp(row[0],"X-ALARM") == 0) {
                        (*event)->alarm = atol(row[1]);
                    } else {
                        calevent_setattr(*event, row[0], row[1]);
                    }
                }
                // query_result freed at the very end
            }
        
            sprintf (buffer, "SELECT FREQ,DATE_FORMAT(UNTIL,'Ymd'), INTERVAL_VAL, BYDAY FROM %s_RRULE WHERE VALUE_KEY=%s", folder, id);
            if ( mysqldrv_mysql_query(stream, buffer) ) {
                query_result = mysql_store_result( DATA );
                if (query_result->row_count != 0) {
                    // there is recurrence for this event
                    row = mysql_fetch_row( query_result );
                    mysqldrv_set_recur (*event, row[0], row[1], row[2], row[3]);
	        }
                // query_result freed at the very end
            }
        }
        mysql_free_result( query_result );
    } else {
        error = true;
    }

    return !error;
}
#undef ERRTEST

#define	STORE_STRING(var, field) {      \
    if ( var ) {                        \
        strcat ( fieldbuffer, (field) );\
        strcat ( fieldbuffer, ", " );   \
        strcat ( valbuffer, "\'" );     \
        strcat ( valbuffer, var);       \
        strcat ( valbuffer, "\', " );   \
    }                                   \
}
#define	STORE_LONG(var, field) {             \
    if ( var ) {                             \
        strcat ( fieldbuffer, (field) );     \
        strcat ( fieldbuffer, ", " );        \
        sprintf ( temp_string, "%lu", var);  \
        strcat ( valbuffer, temp_string);    \
        strcat ( valbuffer, ", " );          \
    }                                        \
}
#define	STORE_INTEGER(var, field) {          \
    if ( var ) {                             \
        strcat ( fieldbuffer, (field) );     \
        strcat ( fieldbuffer, ", " );        \
        sprintf ( temp_string, "%i", var);   \
        strcat ( valbuffer, temp_string);    \
        strcat ( valbuffer, ", " );          \
    }                                        \
}
#define	STORE_DATE(var, field, hasvaluetype) {                                                 \
    if ( dt_hasdate(&var)) {                                                                   \
        strcat ( fieldbuffer, (field) );                                                       \
        strcat ( fieldbuffer, ", " );                                                          \
        strcat ( valbuffer, "\'");                                                             \
        sprintf ( temp_string, "%04u-%02u-%02u", (&var)->year, (&var)->mon, (&var)->mday );    \
        strcat ( valbuffer, temp_string );                                                     \
        if ( dt_hastime(&var) ) {                                                              \
            sprintf ( temp_string, " %02u:%02u:%02u", (&var)->hour, (&var)->min, (&var)->sec );\
            strcat ( valbuffer, temp_string );                                                 \
        }                                                                                      \
        strcat (valbuffer,"\', ");                                                             \
                                                                                               \
        if ( hasvaluetype ) {                                                                  \
            strcat ( fieldbuffer, (field) );                                                   \
            strcat ( fieldbuffer, "_VALUETYPE, " );                                            \
            if (dt_hastime(&var)) {                                                            \
                strcat ( valbuffer, "'DATE-TIME', ");                                          \
            } else {                                                                           \
                strcat ( valbuffer, "'DATE', " );                                              \
            }                                                                                  \
        }                                                                                      \
    }                                                                                          \
}
bool
mysqldrv_append( CALSTREAM *stream, const CALADDR *addr, unsigned long *id, CALEVENT *event) {
    /* I'm using buffers in this function both for simplicity and to cut down the number of mallocs
     * this function is in danger of memory overruns if the query size grows alot without changing the 
     * buffer sizes, strcat doesn't do any memory checking.
     */
    char buffer [10000] = "";
    char fieldbuffer [1000] = "";
    char valbuffer [9000] = "";
    bool error = false;
    char error_text [100] = "";
    char temp_string[100] = "";
    char folder[40];
    unsigned long auto_id = false;
    CALATTR *attr;
    char event_class[10];
    int     start_day;
    byday_t byday = BYDAY_INIT;
    
    

    /*
     * SECTION 1: SET UP THE DATA
     * In this section any data integrety checks and filling in field values are done
     */
    strcpy (folder, DATA->user);
    
    if (!event->id) {
        // set the UID, this isn't the best way to do it though, not overly unique.
        event->id = time(NULL);
    }
    
    if (event->public) {
        strcpy ( event_class, "PUBLIC" );
    } else {
        strcpy ( event_class, "PRIVATE" );
    }

    if (!dt_hasdate(&event->start)) {
        error = true;
        strcat (error_text,"6.x DTSTART required\n");
    }

    /* set end date if time but not date present */ 
    if (dt_hastime(&event->end) && !dt_hasdate(&event->end)) {
        if (dt_hasdate(&event->start)) {
            dt_setdate(&event->end,	event->start.year, event->start.mon, event->start.mday);
        } else {
            error = true;
            strcat (error_text,"6x DTEND date required\n");
        }
    } 


    /*
     * SECTION 2: SET UP THE VEVENT QUERY
     */
    
    STORE_LONG   ( event->id, "UID" );
    STORE_STRING ( event_class , "CLASS" );
    STORE_DATE   ( event->start, "DTSTART", true);
    STORE_DATE   ( event->end, "DTEND", true);
    STORE_STRING ( event->title, "SUMMARY" );
    STORE_STRING ( event->description, "DESCRIPTION" );

    if ( strlen(valbuffer) > 2 ) {
        /* cut ', ' off the ends of buffers.*/
        valbuffer[ strlen(valbuffer)-2 ] = '\0';
        fieldbuffer[ strlen(fieldbuffer)-2 ] = '\0';    
    } else {
        error = true;
        strcat (error_text, "No data to append to VEVENT\n");
    }

    sprintf ( buffer, "INSERT INTO %s_VEVENT ( %s ) VALUES ( %s )", folder, fieldbuffer, valbuffer );
    if ( mysqldrv_mysql_query(stream, buffer)) {
        auto_id = mysql_insert_id(DATA);
    } else {
        error = true;
        strcat (error_text, "VEVENT query error\n");
    }
    
    /*
     * SECTION 3: SET UP CONNECTED TABLES QUERIES
     */

    if (event->category && !error) {
        // this line will have to be expanded to pass through a token string once we support more than one category
        sprintf ( buffer, "INSERT INTO %s_CATEGORIES (VALUE_KEY, VALUE) VALUES (%lu, '%s')", folder, auto_id, event->category);
        mysqldrv_mysql_query( stream, buffer );
    }

    if (event->alarm && !error) {
        sprintf ( buffer, "INSERT INTO %s_X_PROP (VALUE_KEY, NAME, VALUE) VALUES (%lu, '%s', %ld)", folder, auto_id, "X-ALARM", event->alarm);
        mysqldrv_mysql_query( stream, buffer );
    }
    
    if (event->attrlist && !error) {
        sprintf( buffer, "INSERT INTO %s_X_PROP (VALUE_KEY, NAME, VALUE) VALUES", folder);
        for (attr = event->attrlist; attr; attr = attr->next) {
            strcpy ( temp_string, buffer );
	    sprintf ( buffer, "%s (%lu, '%s', '%s')", temp_string, auto_id, attr->name, attr->value);
	}
        mysqldrv_mysql_query( stream, buffer );
    }
    
    if ((event->recur_type) && (event->recur_type != RECUR_NONE) && !error) {       
        fieldbuffer[0] = '\0';
        valbuffer[0] = '\0';

	STORE_LONG( auto_id, "VALUE_KEY" );
        STORE_INTEGER((int)event->recur_interval,"INTERVAL_VAL");
        STORE_DATE(event->recur_enddate,"UNTIL", false);
        switch (event->recur_type) {
            case RECUR_DAILY:
                STORE_STRING ("DAILY", "FREQ");
                break;
            case RECUR_WEEKLY:
                STORE_STRING ("WEEKLY", "FREQ");
                byday.weekdays = event->recur_data.weekly_wday;
		ical_set_byday (temp_string, &byday );
                STORE_STRING ( temp_string ,"BYDAY");
                break;
            case RECUR_MONTHLY_MDAY:
                STORE_STRING ("MONTHLY", "FREQ");
                break;
            case RECUR_MONTHLY_WDAY:
                STORE_STRING ("MONTHLY", "FREQ");
                start_day = dt_dayofweek( &event->start );
                byday.weekdays = 0x1 << start_day;               // this is a bit field
                byday.ordwk[start_day] = dt_orderofmonth( &event->start, DT_FORWARD );
                ical_set_byday ( temp_string, &byday );
                STORE_STRING ( temp_string, "BYDAY");
                break;
            case RECUR_YEARLY:
                STORE_STRING ("YEARLY", "FREQ");
                break;
            case RECUR_NONE:
            case NUM_RECUR_TYPES:
                // just here to avoid compiler warnings
        }        

        /* cut ', ' off the ends of buffers.  Assumes lengths to be >=2 (they have to be) */
        valbuffer[ strlen(valbuffer)-2 ] = '\0';
        fieldbuffer[ strlen(fieldbuffer)-2 ] = '\0';    

        sprintf ( buffer, "INSERT INTO %s_RRULE ( %s ) VALUES ( %s )", folder, fieldbuffer, valbuffer );
        if (mysqldrv_mysql_query(stream, buffer)) {
	    auto_id = mysql_insert_id(DATA);
	} else {
            error = true;
            strcat (error_text, "RRULE query error\n");
        }
    }

    return !error;    
}
#undef STORE_STRING
#undef STORE_LONG
#undef STORE_DATE
#undef STORE_INTEGER

/*
 * this function should have the field fetching tidyed up, mysql returns fixed length, 
 * not null terminated, but this works for now.
 */
bool
mysqldrv_remove( CALSTREAM *stream, unsigned long uid){
    bool error = false;
    char *id;
    char buffer [10000];
    char folder[40];
    MYSQL_RES *query_result;
    MYSQL_ROW temp_row;

    strcpy (folder, DATA->user);
        
    // get record id from uid
    sprintf ( buffer, "SELECT ID, DTSTART_VALUETYPE FROM %s_VEVENT WHERE UID=%lu", folder, uid );

    if ( mysqldrv_mysql_query(stream, buffer) ) {
        // the record exists in VEVENT
        query_result = mysql_store_result( DATA );
        temp_row = mysql_fetch_row( query_result );
        id = strdup (temp_row[0]);        
        mysql_free_result( query_result );
        
        sprintf (buffer, "DELETE FROM %s_VEVENT WHERE ID = %s", folder, id );
        if (!mysqldrv_mysql_query( stream, buffer )) {
            error = true;
        }
        sprintf (buffer, "DELETE FROM %s_CATEGORIES WHERE VALUE_KEY = %s", folder, id );
        mysqldrv_mysql_query( stream, buffer );
        sprintf (buffer, "DELETE FROM %s_X_PROP WHERE VALUE_KEY = %s", folder, id );
        mysqldrv_mysql_query( stream, buffer );
        sprintf (buffer, "DELETE FROM %s_RRULE WHERE VALUE_KEY = %s", folder, id );
        mysqldrv_mysql_query( stream, buffer );
    } else {
        // the record doesn't exist
        error = true;
    
    }
    return !error;
}

bool
mysqldrv_snooze( CALSTREAM *stream, unsigned long uid) {
    bool error = false;
    char buffer [10000];
    char folder [100];
    char *id;
    MYSQL_RES *query_result;
    MYSQL_ROW row;
       
    strcpy( folder, DATA->user ); 

    sprintf (buffer, "SELECT ID FROM %s_VEVENT WHERE UID = %lu", folder, uid);
    if ( mysqldrv_mysql_query(stream, buffer) ) {
        // the record exists in VEVENT
        query_result = mysql_store_result( DATA );
        if (query_result->row_count == 0) {
	    // no data
	    error = true;
        } else {
            row = mysql_fetch_row( query_result );
            id = strdup (row[0]);
	}
	mysql_free_result( query_result );
    
        sprintf (buffer, "UPDATE %s_X_PROP SET VALUE=0 WHERE NAME='X-ALARM' AND VALUE_KEY = %s", folder, id);
        if ( !mysqldrv_mysql_query(stream, buffer) ) {
            error = true;
        }
    } else {
        error = true;
    }
    return !error;
}

bool
mysqldrv_store( CALSTREAM *stream, CALEVENT *event) {
    bool    error;
    unsigned long id;
    CALADDR addr;
    
    error = !mysqldrv_remove ( stream, event->id );
    if (!error) {
        error = !mysqldrv_append ( stream, &addr, &id, event);
    }
    
    return !error;
}


/*
PSEUDOCODE for SEARCH_RANGE.  (this is has lots of errors in it though)

not supported:
 COUNT
 BYDAY (list of dates)
 BYSECOND
 BYMINUTE
 BYHOUR
 BYWEEKNO (week of year)
 BYSETPOS (wacky, wacky)
 WKST assumed to be sunday 


sql pseudocode    
    SELECT UID 
    FROM lauren_VEVENT --joined to lauren_RRULE
    WHERE DTSTART <= start_unix
          AND
          (
            ( DTEND >= end_unix )
            OR
            (
              ( UNTIL >= end_unix ) 
              AND
              (
                ( FREQ = "DAILY"
                  AND
                  INTERVAL_VAL - MOD((start_days-TO_DAYS(FROM_UNIXTIME(DTSTART))),INTERVAL_VAL) <= end_days-start_days
                )
                OR
                ( FREQ = "WEEKLY"
                  AND
generate based on days in start..end
                  ((BYDAY LIKE "%MO%")OR(BYDAY LIKE "....))
this code does not identify interval steps, may incorrectly identify
                )
                OR
                ( FREQ = "MONTHLY"
                  AND
                  IS NULL(BYDAY)
                  AND
                  INTERVAL_VAL- ((start_month-MONTH(FROM_UNIXTIME(DTSTART))+12*(start_year-YEAR(FROM_UNIXTIME(DTSTART)))mod INTERVAL_VAL <= (end_month - start_month)+12*(end_year-start_year)
                  AND
                  DAYOFMONTH(DTSTART) >= start_dayofmonth
                  AND
                  DAYOFMONTH(DTSTART) <= end_dayofmonth
                )
                OR
                ( FREQ = "MONTHLY"
                  AND
                  IS NOT NULL(BYDAY)
                  AND
not done                            
                )
                OR
                ( FREQ = "YEARLY"
                  AND
                  IS NULL(BYDAY)
                  AND
                  DAYOFYEAR(DTSTART)>=start_dayofyear
                  AND
                  DAYOFYEAR(DTSTART)<=end_dayofyear
                )
              )
            )
          )
          

original pseudocode
          
        switch (RRULE)
            case(NULL)
                DTSTART <=(timestamp)start
                and DTEND >=(timestamp)end
            else
                switch(FREQ)
                    case(DAILY) //INTERVAL_VAL
                        DTSTART <=(timestamp)start
                        and UNTIL >=(timestamp)end
                        and start+(INTERVAL_VAL- (start-DTSTART)mod INTERVAL_VAL) <= end
                    case(WEEKLY)//BYDAY,INTERVAL_VAL,
************

                    case(MONTHLY)  //monthly by day, date
                        switch(BYDAY)
                            case(NULL) //monthly by date
                                DTSTART <=(timestamp)start
                                and UNTIL >=(timestamp)end
                                and month(start)+(INTERVAL_VAL- (month(start)-month(DTSTART))mod INTERVAL_VAL) <= month(end)
                                and dayofmonth(DTSTART) >= start-firstdayofmonth
                                and dayofmonth(DTSTART) <= end-firstdayofmonth
                            else //monthly by day
                                DTSTART <=(timestamp)start
                                and UNTIL >=(timestamp)end
                                and month(start)+(INTERVAL_VAL- (month(start)-month(DTSTART))mod INTERVAL_VAL) <= month(end)
                                and dayofmonth(BYDAY rule) >= start-firstdayofmonth
                                and dayofmonth(BYDAY rule) <= end-firstdayofmonth
                                
                    case(YEARLY)
                        DTSTART <=(timestamp)start
                        and UNTIL >=(timestamp)end
                        and dayofyear(DTSTART)>=start-firstdayofyear
                        and dayofyear(DTSTART)<=end-firstdayofyear
*/



