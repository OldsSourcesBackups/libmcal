/*
 * cc -g -o client test.o -L/usr/local/lib/mysql -lmysqlclient -lsocket -lnsl
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
  ATTACH                  TINYINT,
  ATTENDEE                TINYINT,
  CATEGORIES              TINYINT,
  CLASS                   ENUM(\"PUBLIC\", \"PRIVATE\", \"CONFIDENTIAL\", \"<other>\") NOT NULL,  /* Max 1 per VEVENT*/
  CLASS_OTHER_VALUE       TEXT,
  CLASS_XPARAM            INTEGER,                /* VALUE_KEY */ 
  COMMENT                 TINYINT,
  CONTACT                 TINYINT,
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
  EXDATE                  TINYINT,
  EXRULE                  TINYINT,
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
  METHOD                  TINYINT,
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
  RDATE_KEY               TINYINT,
  RELATED_TO              TINYINT,
  RESOURCES               TINYINT,
  RRULE                   TINYINT,                /* VALUE_KEY */
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
  X_PROP_KEY              TINYINT,               
  ALARM_KEY               TINYINT,
  UNIQUE KEY UID(UID)
  )"
/* would also like to index on: but they can be null... hmmm...
 * KEY DTSTART(DTSTART),
 * KEY DTEND(DTEND)
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
           NAME                     TEXT NOT NULL,
           PARAMS                   INTEGER,             /* VALUE_KEY (XPARAM)*/
           KEY  VALUE_KEY(VALUE_KEY)
   )"
#define XPARAM_TABLE "_XPARAM (
           ID                       INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY,
           VALUE_KEY                INTEGER NOT NULL,
           VALUE                    TEXT NOT NULL,
           NAME                     TEXT NOT NULL,
           KEY  VALUE_KEY(VALUE_KEY)
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
//static bool		mysqldrv_validuser(const char *username,const char *password);
//static bool		mysqldrv_userexists(const char *username);
static void 		mysqldrvout_datetime (char *buffer, const datetime_t *input);
static void 		mysqldrvout_ulong (char *buffer, const unsigned long input);

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

/* returns unique index, 0 or null
 */
unsigned long
mysqldrv_mysql_query(CALSTREAM *stream, const char *query) {
    unsigned long output;

    output = mysql_query(DATA,query);
    cc_vlog (query,"");
    cc_vlog (mysql_error(DATA),"");
    if (output == 0) {
        output = mysql_insert_id(DATA);
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

bool
mysqldrv_search_range(	CALSTREAM *stream, const datetime_t *start, const datetime_t *end) {
/*
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
    return false;
}

bool
mysqldrv_search_alarm(	CALSTREAM *stream, const datetime_t *when) {
    return false;
}

bool
mysqldrv_fetch(	CALSTREAM *stream, unsigned long id, CALEVENT **event){
    return false;
}

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
        sprintf ( temp_string, "%li", var);  \
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
        /* specify the value type if desired */                                                \
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
#define MAX(val1,val2) val1 > val2 ? val1 : val2
bool
mysqldrv_append( CALSTREAM *stream, const CALADDR *addr, unsigned long *id, CALEVENT *event) {
    /* I'm using buffers in this function both for simplicity and to cut down the number of mallocs
     * this function is in danger of memory overruns if the query size grows alot without changing the 
     * buffer sizes, strcat doesn't do any memory checking.
     */
    char buffer [10000];
    char fieldbuffer [1000];
    char valbuffer [9000];
    bool error;
    char error_text [100];
    char temp_string[100];
    bool append;
    char folder[40];
    unsigned long auto_id;
    CALATTR *attr;
    
    strcpy (folder, DATA->user);

    append = true;
    // set the UID, this isn't the best way to do it though, not overly unique.
    event->id = time(NULL);

    strcpy ( fieldbuffer, "(" );
    strcpy ( valbuffer , "(" );
    // insert any data integrety checks here as each field is added to a buffer.
    // required fields:    
    STORE_LONG ( event->id, "UID" );
    
    if (event->public) {
        strcpy ( temp_string, "PUBLIC" );
    } else {
        strcpy ( temp_string, "PRIVATE" );
    }
    STORE_STRING (temp_string, "CLASS" );
    
    if (!dt_hasdate(&event->start)) {
        error = true;
        strcat (error_text,"6.x DTSTART required\n");
    }
    STORE_DATE ( event->start, "DTSTART", true);

    /*
     * optional fields:
     */
     
    /* set end date if time but not date present */ 
    if (dt_hastime(&event->end) && !dt_hasdate(&event->end)) {
        if (dt_hasdate(&event->start)) {
            dt_setdate(&event->end,	event->start.year, event->start.mon, event->start.mday);
        } else {
            error = true;
            strcat (error_text,"6x DTEND date required\n");
        }
    } 
    STORE_DATE ( event->end, "DTEND", true);

    STORE_STRING ( event->title, "SUMMARY" );
    STORE_STRING ( event->description, "DESCRIPTION" );
    
    /*
    STORE_INTEGER ( vevent_categories, "CATEGORIES" );
    STORE_INTEGER ( vevent_rrule, "RRULE" );
    STORE_INTEGER ( vevent_x_prop_list, "X_PROP_LIST" );
    */
    
    /* cut ', ' off the ends of buffers.  Assumes lengths to be >=2 */
    valbuffer[MAX( strlen(valbuffer)-2, 0)] = '\0';
    fieldbuffer[MAX( strlen(fieldbuffer)-2, 0)] = '\0';    
    strcat (fieldbuffer, ")");
    strcat (valbuffer, ")");

    sprintf ( buffer, "INSERT INTO %s_VEVENT %s VALUES %s", folder, fieldbuffer, valbuffer );
    if (!(auto_id = mysqldrv_mysql_query(stream, buffer))){
        error = true;
        strcat (error_text, "query error\n");
    }

    if (event->category) {
        // this line will have to be expanded to pass through a token string once we support more than one category
        sprintf ( buffer, "INSERT INTO %s_CATEGORIES (VALUE_KEY, VALUE) VALUES (%lu, '%s')", folder, auto_id, event->category);
        mysqldrv_mysql_query( stream, buffer );
    }

    if (event->alarm) {
        sprintf ( buffer, "INSERT INTO %s_X_PROP (VALUE_KEY, NAME, VALUE) VALUES (%lu, '%s', %ld)", folder, auto_id, "X-ALARM", event->alarm);
        mysqldrv_mysql_query( stream, buffer );
    }
    
    if (event->attrlist) {
        sprintf( buffer, "INSERT INTO %s_X_PROP (VALUE_KEY, NAME, VALUE) VALUES", folder);
        for (attr = event->attrlist; attr; attr = attr->next) {
            strcpy ( temp_string, buffer );
	    sprintf ( buffer, "%s (%lu, '%s', '%s')", temp_string, auto_id, attr->name, attr->value);
	}
        mysqldrv_mysql_query( stream, buffer );
    }
    
    if ((event->recur_type) && (event->recur_type != RECUR_NONE)) {
        int     start_day;
        byday_t byday = BYDAY_INIT;
        
        sprintf( buffer, "INSERT INTO %s_RRULE", folder);
        strcpy ( fieldbuffer, "(" );
        strcpy ( valbuffer, "(" );

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

        /* cut ', ' off the ends of buffers.  Assumes lengths to be >=2 */
        valbuffer[MAX( strlen(valbuffer)-2, 0)] = '\0';
        fieldbuffer[MAX( strlen(fieldbuffer)-2, 0)] = '\0';    
        strcat (fieldbuffer, ")");
        strcat (valbuffer, ")");

        sprintf ( buffer, "INSERT INTO %s_RRULE %s VALUES %s", folder, fieldbuffer, valbuffer );
        if (!(auto_id = mysqldrv_mysql_query(stream, buffer))){
            error = true;
            strcat (error_text, "query error\n");
        }
    }

    return !error;    
}
#undef MAX
#undef STORE_STRING
#undef STORE_LONG
#undef STORE_DATE
#undef STORE_INTEGER


bool		mysqldrv_remove(	CALSTREAM *stream, unsigned long id){
    return false;
}
bool		mysqldrv_snooze(	CALSTREAM *stream, unsigned long id){
    return false;
}


void
mysqldrvout_datetime (char *buffer, const datetime_t *input) {
    // this buffer size assumption follows from mysqldrv_store
    char buffer2[9000];
    strcpy (buffer2, buffer);
    strcat (buffer2, "\'");
        cc_vlog (buffer2,"");
    if ( dt_hasdate(input) ) {
        snprintf (buffer, 9000, "%s%04u-%02u-%02u", buffer2, input->year, input->mon, input->mday );        
        if ( dt_hastime(input) ) {
            snprintf (buffer, 9000, " %02u:%02u:%02u", input->hour, input->min, input->sec );
        }
    strcat (buffer,"\'");
    }
    cc_vlog (buffer,"");
}

void
mysqldrvout_ulong ( char *buffer, const unsigned long input) {
    // this buffer size assumption follows from mysqldrv_store
    char buffer2[9000];
    strcpy (buffer2, buffer);
    snprintf ( buffer, 9000, "%s%li", buffer2, input);
}

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
        sprintf ( temp_string, "%li", var);  \
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
        /* specify the value type if desired */                                                \
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
mysqldrv_store(CALSTREAM *stream, CALEVENT *event) {
    /* I'm using buffers in this function both for simplicity and to cut down the number of mallocs
     * this function is in danger of memory overruns if the query size grows alot without changing the 
     * buffer sizes, strcat doesn't do any memory checking.
     */
    char buffer [10000];
    char fieldbuffer [1000];
    char valbuffer [9000];
    bool error;
    char error_text [100];
    char temp_string[100];
    int  temp_int;
    bool append;
    MYSQL_RES *query_result;
    MYSQL_ROW temp_row;
    int vevent_categories;
    int vevent_rrule;
    int vevent_x_prop_key;
    char auto_id[20];
    char folder[40];
    unsigned long a_id;
    
    strcpy (folder, DATA->user);

/*
cat
alarm
rec
unknown
*/    

    if (!event->id) {
        append = true;
        // set the UID, this isn't the best way to do it though, not overly unique.
        event->id = time(NULL);
        strcpy (auto_id, "LAST_INSERT_ID()");
    } else {
        append = false;
        // get the record id
/*
        strcpy ( buffer, "SELECT ID FROM ");
        strcat ( buffer, folder);
        strcat ( buffer, "_VEVENT WHERE UID = " );
        sprintf ( temp_string, "%li", event->id );
        strcat ( buffer, temp_string );
        error = error || mysqldrv_mysql_query(stream, buffer);
        query_result = mysql_store_result( DATA );
        temp_row = mysql_fetch_row( query_result );
        sprintf ( auto_id, "%ul", temp_row[0] );
        mysql_free_result( query_result );
*/
    }

    strcpy ( fieldbuffer, "(" );
    strcpy ( valbuffer , "(" );
    // insert any data integrety checks here as each field is added to a buffer.
    // required fields:    
    STORE_LONG ( event->id, "UID" );
    
    if (event->public) {
        strcpy ( temp_string, "PUBLIC" );
    } else {
        strcpy ( temp_string, "PRIVATE" );
    }
    STORE_STRING (temp_string, "CLASS" );
    
    if (!dt_hasdate(&event->start)) {
        error = true;
        strcat (error_text,"6.x DTSTART required\n");
    }
    STORE_DATE ( event->start, "DTSTART", true);

    /*
     * optional fields:
     */
     
    /* set end date if time but not date present */ 
    if (dt_hastime(&event->end) && !dt_hasdate(&event->end)) {
        if (dt_hasdate(&event->start)) {
            dt_setdate(&event->end,	event->start.year, event->start.mon, event->start.mday);
        } else {
            error = true;
            strcat (error_text,"6x DTEND date required\n");
        }
    } 
    STORE_DATE ( event->end, "DTEND", true);

    STORE_STRING ( event->title, "SUMMARY" );
    STORE_STRING ( event->description, "DESCRIPTION" );
    
    /*
    STORE_INTEGER ( vevent_categories, "CATEGORIES" );
    STORE_INTEGER ( vevent_rrule, "RRULE" );
    STORE_INTEGER ( vevent_x_prop_list, "X_PROP_LIST" );
    */
    
    /* cut ', ' off the ends of buffers.  Assumes lengths to be >=2 */
    valbuffer[strlen(valbuffer)-2] = '\0';
    fieldbuffer[strlen(fieldbuffer)-2] = '\0';    
    strcat (fieldbuffer, ")");
    strcat (valbuffer, ")");

    strcpy ( buffer, "REPLACE INTO " );    
    strcat ( buffer, folder );
    strcat ( buffer, "_VEVENT" );
    strcat ( buffer, fieldbuffer );
    strcat ( buffer, " VALUES " );
    strcat ( buffer, valbuffer );
//    strcpy ( buffer, "UPDATE http_VEVENT SET SUMMARY = 'wow, hello' WHERE UID=17");l
    cc_vlog (buffer,"");    
    if (!(a_id = mysqldrv_mysql_query(stream, buffer))){
        error = true;
        strcat (error_text, "query error\n");
    }

    if (event->category) {
        sprintf ( buffer, "DELETE FROM %s_CATEGORIES WHERE VALUE_KEY = %lu", folder, a_id );
        mysqldrv_mysql_query( stream, buffer );
        // this line will have to be expanded to pass through a token string once we support more than one category
        sprintf ( buffer, "INSERT INTO %s_CATEGORIES (VALUE_KEY, VALUE) VALUES (%lu, '%s')", folder, a_id, event->category);
        mysqldrv_mysql_query( stream, buffer );
    }
    

/*    
    what with CATEGORIES
    RRULE T/F
     
    INSERT INTO ****_VEVENT (UID, CLASS, DTSTART, DTEND, CATEGORIES, SUMMARY, DESCRIPTION)
    
    X-PROP
    X-ALARM
*/

return !error;    
}
#undef STORE_STRING
#undef STORE_LONG
#undef STORE_DATE
