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
           ATTACH                  INTEGER,
           ATTENDEE                INTEGER,
           CATEGORIES              INTEGER,
           CLASS                   INTEGER,                /* Max 1 per VEVENT*/ 
           COMMENT                 INTEGER,
           CONTACT                 INTEGER,
           CREATED                 INTEGER,                /* Max 1 per VEVENT*/ 
           DESCRIPTION             TEXT,                   /* Max 1 per VEVENT*/ 
           DESCRIPTION_ALTREP      VARCHAR(255),                
           DESCRIPTION_LANGUAGE    VARCHAR(255),                
           DESCRIPTION_XPARAM      INTEGER,                /* VALUE_KEY */
           DTEND                   TIMESTAMP(14) NOT NULL, /* Max 1 per VEVENT no DURATION*/
           DTEND_VALUETYPE         ENUM(\"DATE-TIME\",\"DATE\") DEFAULT \"DATE-TIME\",
           DTEND_TZID              TEXT,
           DTEND_XPARAM            INTEGER,                /* VALUE_KEY */   
           DTSTAMP                 INTEGER,                /* Max 1 per VEVENT*/ 
           DTSTART                 TIMESTAMP(14) NOT NULL, /* Max 1 per VEVENT*/
           DTSTART_VALUETYPE       INTEGER NOT NULL,       /* VALUE_KEY */
           DTSTART_TZID            INTEGER,                /* VALUE_KEY */
           DTSTART_XPARAM          INTEGER,                /* VALUE_KEY */
           DURATION                INTEGER,                /* Max 1 per VEVENT no DTEND*/ 
           EXDATE                  INTEGER,
           EXRULE                  INTEGER,
           GEO                     INTEGER,                /* Max 1 per VEVENT*/ 
           LAST_MODIFIED           INTEGER,                /* Max 1 per VEVENT*/ 
           LOCATION                INTEGER,                /* Max 1 per VEVENT*/ 
           METHOD                  INTEGER,
           ORGANIZER               INTEGER,                /* Max 1 per VEVENT*/ 
           PRIORITY                INTEGER,                /* Max 1 per VEVENT*/ 
           RECURRENCE_ID           INTEGER,                /* Max 1 per VEVENT*/ 
           RDATE_KEY               INTEGER,
           RELATED_TO              INTEGER,
           RESOURCES               INTEGER,
           RRULE                   INTEGER,                /* VALUE_KEY */
           SUMMARY                 INTEGER,                /* Max 1 per VEVENT*/ 
           SEQUENCE                INTEGER,                /* Max 1 per VEVENT*/ 
           STATUS                  INTEGER,                /* Max 1 per VEVENT*/ 
           TRANSP                  INTEGER,                /* Max 1 per VEVENT*/                
           UID                     VARCHAR(255),                /* Max 1 per VEVENT*/
           UID_XPARAM              INTEGER,                /* VALUE_KEY */
           URL                     INTEGER,                /* Max 1 per VEVENT*/                 
           X_PROP_KEY              INTEGER,               
           VALARM_KEY              INTEGER                 
   )"
   
#define X_PROP_TABLE "_X_PROP (
           VALUE_KEY                INTEGER NOT NULL PRIMARY KEY,
           VALUE                    TEXT NOT NULL,
           NAME                     TEXT NOT NULL,
           PARAMS                   INTEGER             /* VALUE_KEY (XPARAM)*/
   )"
#define XPARAM_TABLE "_XPARAM (
           VALUE_KEY                INTEGER NOT NULL PRIMARY KEY,
           VALUE                    TEXT NOT NULL,
           NAME                     TEXT NOT NULL
   )"
#define RRULE_TABLE "_RRULE (
           VALUE_KEY             INTEGER NOT NULL PRIMARY KEY,
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
           XPARAM                INTEGER                 /* VALUE_KEY */
   )"

static void		mysqldrv_freestream(CALSTREAM *stream);
static bool		mysqldrv_mysql_query(CALSTREAM *stream, const char *query);
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
//static bool		mysqldrv_search_alarm(	CALSTREAM *stream,
//						const datetime_t *when);
//static bool		mysqldrv_fetch(	CALSTREAM *stream,
//						unsigned long id,
//						CALEVENT **event);
//static bool		mysqldrv_append(	CALSTREAM *stream,
//						const CALADDR *addr,
//						unsigned long *id,
//						const CALEVENT *event);
//static bool		mysqldrv_remove(	CALSTREAM *stream,
//						unsigned long id);
//static bool		mysqldrv_snooze(	CALSTREAM *stream,
//					unsigned long id);
//static bool		mysqldrv_store(   CALSTREAM *stream, 
//					const CALEVENT *modified_event);

CALDRIVER mysqldrv_driver =
{
	mysqldrv_valid,
	mysqldrv_open,
	mysqldrv_close,
	mysqldrv_ping,
	mysqldrv_create,
	mysqldrv_search_range,
//	mysqldrv_search_alarm,
//	mysqldrv_fetch,
//	mysqldrv_append,
//	mysqldrv_remove,
//	mysqldrv_snooze,
//	mysqldrv_store,
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

bool
mysqldrv_mysql_query(CALSTREAM *stream, const char *query) {
    bool output;

    output = mysql_query(DATA,query);
    cc_vlog (mysql_error(DATA),"");
    
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


bool
mysqldrv_create( CALSTREAM *stream, const char *calendar ) {
    bool output;
    char buffer[6000];

    strcpy (buffer, "CREATE TABLE ");
    strcat (buffer, calendar);
    strcat (buffer, VEVENT_TABLE);
    mysqldrv_mysql_query(stream, buffer);

    strcpy (buffer, "CREATE TABLE ");
    strcat (buffer, calendar);
    strcat (buffer, X_PROP_TABLE);
    mysqldrv_mysql_query(stream, buffer);

    strcpy (buffer, "CREATE TABLE ");
    strcat (buffer, calendar);
    strcat (buffer, XPARAM_TABLE);
    mysqldrv_mysql_query(stream, buffer);

    strcpy (buffer, "CREATE TABLE ");
    strcat (buffer, calendar);
    strcat (buffer, RRULE_TABLE);
    mysqldrv_mysql_query(stream, buffer);
 
    return output;
}


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
}

