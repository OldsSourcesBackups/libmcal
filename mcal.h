/*
 *	$Id: mcal.h,v 1.1 1999/12/02 08:01:40 zircote Exp $
 * Libmcal - Modular Calendar Access Library
 * Copyright (C) 1999 Mark Musone and Andrew Skalski
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
 * Mark Musone
 * musone@chek.com
 *
 * Andrew Skalski
 * askalski@chek.com
 *
 * mcal@lists.chek.com
 */

#ifndef	_MCAL_H
#define	_MCAL_H

#include <stdarg.h>
#include "bool.h"
#include "datetime.h"

/* default folder, if none is specified */
#define	DEFAULT_FOLDER	"INBOX"

/* default protocol if none is specified */
#define	DEFAULT_PROTO	"icap"

/* calendar library version */
#define CALVER		"0.5"

/* structs */
#define	CALDRIVER	struct cal_driver
#define	CALSTREAM	struct cal_stream
#define	CALADDR		struct cal_addr
#define	CALEVENT	struct cal_event


/* calendar options */
enum {
	CAL_LOGIN	= 1,		/* cal_open() - relogin */
};


/* recurrence options */
typedef enum {
	RECUR_NONE,		/* event does not recur */
	RECUR_DAILY,		/* daily */
	RECUR_WEEKLY,		/* weekly on a set of weekdays */
	RECUR_MONTHLY_MDAY,	/* monthly on a specific date */
	RECUR_MONTHLY_WDAY,	/* monthly on the nth fooday */
	RECUR_YEARLY,		/* yearly */
	/* new recur types go here */
	NUM_RECUR_TYPES
} recur_t;

/* weekday mask type */
enum {
	M_SUNDAY	= 0x1,
	M_MONDAY	= 0x2,
	M_TUESDAY	= 0x4,
	M_WEDNESDAY	= 0x8,
	M_THURSDAY	= 0x10,
	M_FRIDAY	= 0x20,
	M_SATURDAY	= 0x40,

	M_WEEKDAYS	= 0x3e,
	M_WEEKEND	= 0x41,
	M_ALLDAYS	= 0x7f,
};


/* recurrence data union */
typedef union recurdata
{
	/* only recur type that is not fully specified using only the
	 * start-date field is weekly, since a set of weekdays must be
	 * supplied.
	 */
	long		weekly_wday;
} recurdata_t;


/* event structure */
CALEVENT {
	unsigned long		id;		/* ID number */
	bool			public;		/* public flag */
	datetime_t		start;		/* start of event */
	datetime_t		end;		/* end of event */
	char			*category;	/* event category */
	char			*title;		/* event title */
	char			*description;	/* event description */

	/* minutes before event that alarm is to sound (0 means no alarm) */
	long			alarm;

	/* recurrence info */
	recur_t			recur_type;	/* recurrence type */
	long			recur_interval;	/* recurrence interval */
	datetime_t		recur_enddate;	/* last recurrence */
	recurdata_t		recur_data;	/* type-specific data */
};


/* calendar address structure */
CALADDR {
	const char		*host;		/* network host */
	const char		*proto;		/* network protocol */
	const char		*user;		/* folder owner */
	const char		*folder;	/* folder name */

	char			*buf;		/* dynamic buffer */
	size_t			bufsize;	/* buffer size */
};


/* calendar driver structure */
CALDRIVER {
	/* return true if <addr> is valid for this driver */
	bool		(*valid)(	const CALADDR *addr);

	/* open a calendar stream to the given <addr>.
	 * if <stream> is non-NULL, attempt to re-use it;
	 * otherwise allocate a new stream and open it.
	 * do not allocate if stream re-use is not possible.
	 * return NULL on error.
	 */
	CALSTREAM*	(*open)(	CALSTREAM *stream,
					const CALADDR *addr,
					long options);

	/* close <stream>, release its resources, and return NULL.
	 * in the future, it may be possible to keep the stream
	 * open (but the folder is closed and user is logged out)
	 * for persistent connections.  this must be specified
	 * as an option flag.
	 */
	CALSTREAM*	(*close)(	CALSTREAM *stream,
					long options);

	/* return true if the stream is still alive */
	bool		(*ping)(	CALSTREAM *stream);

	/* search the current folder for events between <start> and
	 * <end> (inclusive.)  if either lacks a date or is NULL, that
	 * bound will not be checked.  if both lack a date or are NULL,
	 * then all events in the folder will be returned.  event id's
	 * are returned by the cc_searched() callback.  returns false
	 * on error.
	 */
	bool		(*search_range)(	CALSTREAM *stream,
						const datetime_t *start,
						const datetime_t *end);

	/* search the current folder for active alarms.  the alarms
	 * are checked against the specified <when> date/time.
	 * alarms are considered active if <when> is after {alarm}
	 * minutes before event start, and before the event end,
	 * inclusively.  returns false on error.
	 */
	bool		(*search_alarm)(	CALSTREAM *stream,
						const datetime_t *when);

	/* fetch the event matching <id>.  if the event is found, it
 	 * is allocated and a pointer to it is stored in (*event).
	 * otherwise NULL is stored in (*event).  returns false on error.
	 */
	bool		(*fetch)(	CALSTREAM *stream,
					unsigned long id,
					CALEVENT **event);


	/* appends <event> to the folder specified by <addr>.  if
	 * successful, the ID of the new event is stored in (*id).
	 * returns true on success.
	 */
	bool		(*append)(	CALSTREAM *stream,
					const CALADDR *addr,
					unsigned long *id,
					const CALEVENT *event);

	/* removes the event specified by <id> from the folder.  returns
	 * false on error.
	 */
	bool		(*remove)(	CALSTREAM *stream,
					unsigned long id);

	/* cancels the alarm for the event specified by <id>.  returns
	 * false on error.
	 */
	bool		(*snooze)(	CALSTREAM *stream,
					unsigned long id);
};


/* calendar stream struct */
CALSTREAM {
	const CALDRIVER		*driver;	/* stream driver */
	CALADDR			*addr;		/* folder address */
	bool			dead;		/* dead stream? */
	weekday_t		startofweek;	/* first day of week */
	void			*data;		/* driver-specific data */
};


/** calendar client callbacks **/

/* Called when a stream driver requires a username/password.  It is
 * only called during cal_open().
 */
void		cc_login(const char **username, const char **password);

/* Called whenever an ID is returned by the cal_search family of
 * routines.
 */
void		cc_searched(unsigned long id);

/* normal system logging */
void		cc_vlog(const char *fmt, va_list ap);

/* debug logging */
void		cc_vdlog(const char *fmt, va_list ap);



/** logging wrapper functions **/
void		cc_log(const char *fmt, ...);	/* normal system logging */
void		cc_dlog(const char *fmt, ...);	/* debug logging */


/** calendar client functions **/

/* Parses a calendar address and returns a CALADDR structure, which must
 * be disposed of with caladdr_free().  returns NULL on error.
 */
CALADDR*	caladdr_parse(const char *address);

/* Duplicates the CALADDR struct (must later be disposed
 * of with caladdr_free())
 */
CALADDR*	caladdr_dup(const CALADDR *addr);

/* Constructs a string representing a CALADDR (must be freed())
 * returns NULL on error
 */
char*		caladdr_out(const CALADDR *addr);

/* Disposes of a CALADDR structure, returns NULL for convenience. */
CALADDR*	caladdr_free(CALADDR *addr);

/* Allocates a new CALEVENT. */
CALEVENT*	calevent_new(void);

/* Disposes of a CALEVENT, returns NULL for convenience. */
CALEVENT*	calevent_free(CALEVENT *event);

/* Routines to alter event recurrence. */
bool		calevent_recur_none(CALEVENT *event);
bool		calevent_recur_daily(CALEVENT *event, datetime_t *enddate,
					long interval);
bool		calevent_recur_weekly(CALEVENT *event, datetime_t *enddate,
					long interval, long weekdays);
bool		calevent_recur_monthly_mday(CALEVENT *event,
					datetime_t *enddate, long interval);
bool		calevent_recur_monthly_wday(CALEVENT *event,
					datetime_t *enddate, long interval);
bool		calevent_recur_yearly(CALEVENT *event,
					datetime_t *enddate, long interval);

/* Routines to evaluate event recurrence. */

/* fills in <next> with the next date the event occurs.  if <next> has
 * a date value, it is filled with the next date the event occurs, on or
 * after the supplied date.  Returns empty date field if event does not
 * occur or something is invalid.
 */
void		calevent_next_recurrence(	const CALEVENT *event,
						datetime_t *next,
						weekday_t startofweek);

/* moves <clamp> ahead to the first day in <mask> after <clamp>, using
 * <weekstart> as the first day of the week.  returns false if none
 * was found.
 */
bool		first_day_not_before(	int mask, weekday_t *clamp,
					weekday_t weekstart);

/* Returns true if the address is valid for any of the calendar drivers */
bool		cal_valid(const char *address);

/* Opens a stream (recycles <stream> if not NULL) and returns it.  returns
 * NULL on error.
 */
CALSTREAM*	cal_open(CALSTREAM *stream, const char *address, long options);

/* Same as cal_open() but takes a parsed CALADDR structure instead of a
 * string folder name
 */
CALSTREAM*	cal_open_addr(	CALSTREAM *stream, const CALADDR *addr,
				long options);

/* Closes the stream and returns a new handle.  This will be normally
 * be NULL, but certain options may allow for persistent connections,
 * in which case the folder will be closed and user logged out, but the
 * stream left open for further use.
 */
CALSTREAM*	cal_close(CALSTREAM *stream, long options);

/* Returns true if the stream is still alive. */
bool		cal_ping(CALSTREAM *stream);

/* Searches the current folder for events between <start> and <end>,
 * inclusively.  If either <start> or <end> lacks a date or is NULL,
 * it will be ignored.
 */
bool		cal_search_range(	CALSTREAM *stream,
					const datetime_t *start,
					const datetime_t *end);

/* Searches the current folder for active alarms, using <when> as the
 * reference time.  <when> must contain both date and time components.
 */
bool		cal_search_alarm(CALSTREAM *stream, const datetime_t *when);

/* Fetches the event with id of <id> from the folder.  Returns false on
 * error.  (*event) points to the event (which must be freed) or to NULL
 * if it was not found.
 */
bool		cal_fetch(	CALSTREAM *stream,
				unsigned long id,
				CALEVENT **event);

/* Appends an event to <folder>.  Returns false on error.  If successful,
 * the ID of the new event is stored in (*id)
 */
bool		cal_append(	CALSTREAM *stream,
				const char *folder,
				unsigned long *id,
				const CALEVENT *event);

/* Same as cal_append(), but uses a parsed CALADDR instead of a folder
 * string.
 */
bool		cal_append_addr(CALSTREAM *stream,
				const CALADDR *addr,
				unsigned long *id,
				const CALEVENT *event);

/* Removes the event with id of <id> from the folder.  Returns false on
 * error.
 */
bool		cal_remove(	CALSTREAM *stream,
				unsigned long id);

/* Cancels the alarm for event with id of <id>.  Returns false on error. */
bool		cal_snooze(	CALSTREAM *stream,
				unsigned long id);


/* private functions */

/* Searches for a driver for the given calendar address.  Returns NULL
 * if none is found.
 */
const CALDRIVER*	cal_getdriver(const CALADDR *addr);

#endif
