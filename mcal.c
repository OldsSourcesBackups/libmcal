/*
 *	$Id: mcal.c,v 1.9 2001/01/09 03:26:48 markie Exp $
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

#include <stdlib.h>
#include <string.h>
#include "mcal.h"

#include "drivers.h"
#include "drivers.c"


void
cc_log(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	cc_vlog(fmt, ap);
	va_end(ap);
}


void
cc_dlog(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	cc_vdlog(fmt, ap);
	va_end(ap);
}


const CALDRIVER*
cal_getdriver(const CALADDR *addr)
{
	const CALDRIVER **driver;

	/* Search for the first driver where valid() returns true. */
	for (driver=driver_registry; *driver; driver++) {
		if ((*driver)->valid(addr))
			return *driver;
	}

	return NULL;
}


CALADDR*
caladdr_parse(const char *address)
{
	CALADDR		*ret;
	char		*ptr;


	/* allocate the parsed-addr structure */
	if ((ret = calloc(1, sizeof(*ret))) == NULL)
		return NULL;

	/* default to INBOX if <address> is NULL */
	if (address == NULL) {
		ret->folder = DEFAULT_FOLDER;
		return ret;
	}

	/* use a copy of <address> as the dynamic buffer */
	if ((ret->buf = strdup(address)) == NULL) {
		free(ret);
		return NULL;
	}
	ret->bufsize = strlen(address) + 1;


	/* parse out the dynamic buffer */
	ptr = ret->buf;

	/* parse: "{" host [/proto] "}" */
	if (*ptr == '{') {
		ret->host = ++ptr;
		while (*ptr && *ptr != '/' && *ptr != '}')
			ptr++;

		if (!*ptr) {
			caladdr_free(ret);
			return NULL;
		}

		/* parse: [/proto] */
		if (*ptr == '/') {
			*ptr++ = 0;
			ret->proto = ptr;
			while (*ptr && *ptr != '}')
				ptr++;

			if (!*ptr) {
				caladdr_free(ret);
				return NULL;
			}
		}

		*ptr++ = 0;
	}

	/* parse: "<" username ">" */
	if (*ptr == '<') {
		ret->user = ++ptr;
		while (*ptr && *ptr != '>')
			ptr++;

		if (!*ptr) {
			caladdr_free(ret);
			return NULL;
		}

		*ptr++ = 0;
	}

	/* parse: folder */
	ret->folder = ptr;

	/* NULL out any missing fields. */
	if (ret->host && !*ret->host) ret->host = NULL;
	if (ret->proto && !*ret->proto) ret->proto = NULL;
	if (ret->user && !*ret->user) ret->user = NULL;
	if (ret->folder && !*ret->folder) ret->folder = NULL;

	/* substitute default folder if null */
	if (ret->folder == NULL)
		ret->folder = DEFAULT_FOLDER;

	/* substitute default protocol if null */
	if (ret->host && !ret->proto)
		ret->proto = DEFAULT_PROTO;

	return ret;
}


char*
caladdr_out(const CALADDR *addr)
{
	size_t		size = 0;
	char		*buf;


	/* size-count and sanity-check all fields */
	if (addr->host) {
		/* sanity: host contains neither '/' nor '}' */
		if (strpbrk(addr->host, "}/"))
			return NULL;
		size += strlen(addr->host) + 2;

		/* output proto */
		if (addr->proto) {
			/* sanity: proto does not contain '}' */
			if (strchr(addr->proto, '}'))
				return NULL;
			size += strlen(addr->proto) + 1;
		}
	}
	if (addr->user) {
		/* sanity: user does not contain '>' */
		if (strchr(addr->user, '>'))
			return NULL;
		size += strlen(addr->user) + 2;
	}
	if (addr->folder)
		size += strlen(addr->folder);


	/* allocate the return buffer */
	buf = calloc(1, size + 1);
	if (buf == NULL)
		return NULL;


	/* write out the address to string */
	if (addr->host) {
		strcat(buf, "{");
		strcat(buf, addr->host);
		if (addr->proto) {
			strcat(buf, "/");
			strcat(buf, addr->proto);
		}
		strcat(buf, "}");
	}
	if (addr->user) {
		strcat(buf, "<");
		strcat(buf, addr->user);
		strcat(buf, ">");
	}
	if (addr->folder)
		strcat(buf, addr->folder);

	return buf;
}


CALADDR*
caladdr_free(CALADDR *addr)
{
	/* free the address and dynamic buffer */
	if (addr) {
		free(addr->buf);
		free(addr);
	}
	return NULL;
}


CALADDR*
caladdr_dup(const CALADDR *addr)
{
	CALADDR		*ret;
	long		offset;

	/* allocate the CALADDR structure */
	ret = calloc(1, sizeof(*addr));
	if (ret == NULL)
		return NULL;

	/* allocate the dynamic buffer */
	ret->buf = calloc(1, addr->bufsize);
	if (ret->buf == NULL) {
		free(ret);
		return NULL;
	}

	/* copy the dynamic buffer */
	memcpy(ret->buf, addr->buf, addr->bufsize);


	/* MIMIC: if field is within bounds of dynamic buffer, translate
	 *        the offset relative to the new buffer.  otherwise, copy
	 *        it explicitly.
	 */
#define	MIMIC(field)	if (addr->field) { \
				offset = addr->field - addr->buf; \
				if (offset >= 0 && offset < addr->bufsize) \
					ret->field = ret->buf + offset; \
				else \
					ret->field = addr->field; }

	MIMIC(host);
	MIMIC(proto);
	MIMIC(user);
	MIMIC(folder);

#undef	MIMIC

	return ret;
}


CALEVENT*
calevent_new(void)
{
	CALEVENT		*event;

	if ((event = calloc(1, sizeof(CALEVENT))) == NULL)
		return NULL;
	/* set all pointers to NULL */
	event->category = NULL;
	event->title = NULL;
	event->description = NULL;
	event->attrlist = NULL;

	return event;
}


CALEVENT*
calevent_free(CALEVENT *event)
{
	CALATTR		*attr, *next;

	if (event) {
		free(event->category);
		free(event->title);
		free(event->description);
		for (attr = event->attrlist; attr; attr = next) {
			next = attr->next;
			free(attr->name);
			free(attr->value);
			free(attr);
		}
		free(event);
	}
	return NULL;
}


bool
calevent_valid(const CALEVENT *event)
{
	int n = 0;

	/* both must have date field set */
	if (!dt_hasdate(&event->start) || !dt_hasdate(&event->end))
		return false;

	/* either none or both may have time field set */
	if (dt_hastime(&event->start)) n++;
	if (dt_hastime(&event->end)) n++;
	if (n == 1)
		return false;

	/* start must precede end */
	if (dt_compare(&event->start, &event->end) > 0)
		return false;

	return true;
}


const char*
calevent_getattr(const CALEVENT *event, const char *name)
{
	const CALATTR	*attr;

	for (attr = event->attrlist; attr; attr = attr->next)
		if (!strcasecmp(attr->name, name))
			return attr->value;

	return NULL;
}


bool
calevent_setattr(CALEVENT *event, const char *name, const char *value)
{
	CALATTR		*attr;
	char		*tmp;

	if (value && (tmp = strdup(value)) == NULL)
		return false;

	for (attr = event->attrlist; attr; attr = attr->next)
		if (!strcasecmp(attr->name, name))
			break;

	if (value) {
		if (attr) {
			free(attr->value);
		}
		else {
			if (	(attr = malloc(sizeof(CALATTR))) == NULL ||
				(attr->name = strdup(name)) == NULL)
			{
				if (attr) {
					free(attr->name);
					free(attr);
				}
				free(tmp);
				return false;
			}

			attr->prev = &event->attrlist;
			if ((attr->next = event->attrlist))
				event->attrlist->prev = &attr->next;
			event->attrlist = attr;
		}

		attr->value = tmp;
	}
	else if (attr) {
		if ((*attr->prev = attr->next))
			attr->next->prev = attr->prev;
		free(attr->name);
		free(attr->value);
		free(attr);
	}

	return true;
}


bool
calevent_recur_none(CALEVENT *event)
{
	event->recur_type = RECUR_NONE;
	return true;
}


bool
calevent_recur_daily(CALEVENT *event, datetime_t *enddate, long interval)
{
	if (!dt_hasdate(enddate))
		return false;
	if (interval < 1)
		return false;

	event->recur_type = RECUR_DAILY;
	event->recur_enddate = *enddate;
	event->recur_interval = interval;

	return true;
}


bool
calevent_recur_weekly(	CALEVENT *event, datetime_t *enddate,
			long interval, long weekdays)
{
	if (!dt_hasdate(enddate))
		return false;
	if (interval < 1)
		return false;
	if ((weekdays & M_ALLDAYS) == 0)
		return false;
	if (weekdays & ~(long)M_ALLDAYS)
		return false;

	event->recur_type = RECUR_WEEKLY;
	event->recur_enddate = *enddate;
	event->recur_interval = interval;
	event->recur_data.weekly_wday = weekdays;

	return true;
}


bool
calevent_recur_monthly_mday(CALEVENT *event, datetime_t *enddate, long interval)
{
	if (!dt_hasdate(enddate))
		return false;
	if (interval < 1)
		return false;

	event->recur_type = RECUR_MONTHLY_MDAY;
	event->recur_enddate = *enddate;
	event->recur_interval = interval;

	return true;
}


bool
calevent_recur_monthly_wday(CALEVENT *event, datetime_t *enddate, long interval)
{
	if (!dt_hasdate(enddate))
		return false;
	if (interval < 1)
		return false;

	event->recur_type = RECUR_MONTHLY_WDAY;
	event->recur_enddate = *enddate;
	event->recur_interval = interval;

	return true;
}


bool
calevent_recur_yearly(CALEVENT *event, datetime_t *enddate, long interval)
{
	if (!dt_hasdate(enddate))
		return false;
	if (interval < 1)
		return false;

	event->recur_type = RECUR_YEARLY;
	event->recur_enddate = *enddate;
	event->recur_interval = interval;

	return true;
}


void
calevent_next_recurrence(	const CALEVENT *event, datetime_t *first,
				weekday_t startofweek)
{
	datetime_t	clamp;
	datetime_t	estart;
	int		base;
	int		offset;
	int		hop;

	dt_cleartime(first);
	clamp = *first;
	dt_cleardate(first);

	/* error condition */
	estart = event->start;
	dt_cleartime(&estart);
	if (!dt_hasdate(&estart))
		return;

	/* weed out impossible conditions */
	if (event->recur_type != RECUR_NONE) {
		if (dt_compare(&event->recur_enddate, &estart) < 0)
			return;
		if (dt_compare(&event->recur_enddate, &clamp) < 0)
			return;
	}

	if (dt_compare(&clamp, &estart) < 0)
		clamp = estart;

	hop = event->recur_interval;

	switch (event->recur_type) {
	case NUM_RECUR_TYPES:
		return;
	case RECUR_NONE:
		if (dt_hasdate(&clamp) && dt_compare(&estart, &clamp) < 0)
			return;
		*first = estart;
		return;
	case RECUR_YEARLY:
		/* XXX this code sucks */

		/* special case for feb29 */
		if (estart.mon == FEBRUARY && estart.mday == 29) {
			/* advance clamp to next leap year */
			if (clamp.mon > estart.mon)
				while (!isleapyear(++clamp.year)) {
					if (clamp.year > YEAR_MAX)
						return;
				}
			clamp.mon = estart.mon;
			clamp.mday = estart.mday;

			offset = clamp.year - estart.year;
			while (	offset%hop || !isleapyear(clamp.year)) {
				offset += 4;
				clamp.year += 4;
				if (clamp.year > YEAR_MAX)
					return;
			}

			if (dt_compare(&event->recur_enddate, &clamp) < 0)
				return;

			*first = clamp;
			return;
		}


		/* general case (non-feb29) */

		if (	clamp.mon > estart.mon ||
			(clamp.mon == estart.mon && clamp.mday > estart.mday))
		{
			clamp.year++;
			clamp.mon = estart.mon;
			clamp.mday = estart.mday;
		}

		/* adjust estart to be the first candidate */
		offset = clamp.year - estart.year;
		if (offset > 0) {
			offset = ((offset + hop - 1) / hop) * hop;
			estart.year += offset;
		}

		/* bail if we hopped past the event end */
		if (dt_compare(&event->recur_enddate, &estart) < 0)
			return;

		*first = estart;
		return;
	case RECUR_MONTHLY_MDAY:
		if (clamp.mday > estart.mday) {
			if (++clamp.mon > DECEMBER) {
				clamp.mon = JANUARY;
				clamp.year++;
			}
			clamp.mday = estart.mday;
		}

		/* adjust estart to be the first match */
		offset = (clamp.mon - estart.mon) +
			 (clamp.year - estart.year) * 12;
		offset = ((offset + hop - 1) / hop) * hop;

		estart.mon += offset;
		estart.year += (estart.mon - 1) / 12;
		estart.mon = (estart.mon - 1) % 12 + 1;

		while (!datevalid(estart.year, estart.mon, estart.mday)) {
			estart.mon += hop;
			estart.year += (estart.mon - 1) / 12;
			estart.mon = (estart.mon - 1) % 12 + 1;

			if (estart.year > event->recur_enddate.year)
				return;
		}

		/* bail if we hopped past the event end */
		if (dt_compare(&event->recur_enddate, &estart) < 0)
			return;

		*first = estart;
		return;
	case RECUR_DAILY:
		base = dt_dayofepoch(&estart);
		offset = dt_dayofepoch(&clamp) - base;
		offset = ((offset + hop - 1) / hop) * hop;
		dt_setdoe(&estart, base + offset);

		/* bail if we hopped past the event end */
		if (dt_compare(&event->recur_enddate, &estart) < 0)
			return;

		*first = estart;
		return;
	case RECUR_WEEKLY: {
		/* XXX i have no idea how this code works */

		datetime_t	weekend;
		datetime_t	clampweekend;
		datetime_t	candidate;
		weekday_t	wday;

		dt_init(&weekend);
		dt_init(&clampweekend);
		dt_init(&candidate);

		/* convert interval from weeks to days */
		hop *= 7;

		/* bail if the event does not occur.. ever */
		if (!(event->recur_data.weekly_wday & M_ALLDAYS))
			return;

		/* find the last day of the week of estart.  we use that
		 * to find the first candidate week after clamp
		 */
		if (!dt_endofweek(&weekend, &estart, startofweek))
			return;

		/* find last day of week of clamp */
		if (!dt_endofweek(&clampweekend, &clamp, startofweek))
			return;


		base = dt_dayofepoch(&weekend);
		offset = dt_dayofepoch(&clamp) - base;
		offset = ((offset + hop - 1) / hop) * hop;
		dt_setdoe(&weekend, base + offset);


		/* if candidate week same as clamp week... */
		if (dt_compare(&weekend, &clampweekend) == 0) {
			wday = dt_dayofweek(&clamp);
			if (first_day_not_before(
				event->recur_data.weekly_wday,
				&wday, startofweek))
			{
				if (!dt_setweekof(&candidate, &weekend,
					startofweek, wday))
				{
					return;
				}
			}
			else {
				base = dt_dayofepoch(&weekend);
				dt_setdoe(&weekend, base + hop);
			}
		}
		if (!dt_hasdate(&candidate)) {
			wday = startofweek;
			first_day_not_before(	event->recur_data.weekly_wday,
						&wday, startofweek);
			if (!dt_setweekof(&candidate, &weekend,
					startofweek, wday))
			{
				return;
			}
		}

		if (!dt_hasdate(&candidate))
			return;

		/* bail if we hopped past the event end */
		if (dt_compare(&event->recur_enddate, &candidate) < 0)
			return;

		*first = candidate;
		return;
	}
	case RECUR_MONTHLY_WDAY: {
		datetime_t	trial = DT_INIT;
		int		nth;
		int		wday;


		nth = (estart.mday - 1) / 7 + 1;
		wday = dt_dayofweek(&estart);

		/* adjust estart to be the first candidate */
		offset = (clamp.mon - estart.mon) +
			 (clamp.year - estart.year) * 12;
		offset = ((offset + hop - 1) / hop) * hop;

		estart.mon += offset - hop;
		do {
			estart.mon += hop;
			estart.year += (estart.mon - 1) / 12;
			estart.mon = (estart.mon - 1) % 12 + 1;

			if (!dt_setnthwday(&trial,
				estart.year, estart.mon, nth, wday))
			{
				continue;
			}

			if (dt_compare(&trial, &clamp) < 0)
				continue;
			if (dt_compare(&trial, &event->recur_enddate) > 0)
				return;

			break;
		} while (true);


		*first = trial;
		return;
	}
	}
}


bool
first_day_not_before(int mask, weekday_t *clamp, weekday_t weekstart)
{
	weekday_t	wday;

	if (*clamp < SUNDAY || *clamp > SATURDAY)
		return false;

	wday = *clamp;
	do {
		if (mask & (1 << wday)) {
			*clamp = wday;
			return true;
		}
		wday = (wday + 1) % 7;
	} while (wday != weekstart);

	return false;
}

bool
cal_create(CALSTREAM *stream,const char *calendar) {
	bool output;
	
	if (stream == NULL) {
		output = false;
	} else {
		output = stream->driver->create(stream, calendar);
	}
	
	return output;
}

bool
cal_valid(const char *address)
{
	CALADDR		*addr;
	bool		found;

	/* parse out the address and try to find a driver for it */
	addr = caladdr_parse(address);
	if (addr == NULL)
		return false;

	found = (cal_getdriver(addr) != NULL);

	caladdr_free(addr);

	return found;
}


CALSTREAM*
cal_open_addr(CALSTREAM *stream, const CALADDR *addr, long options)
{
	const CALDRIVER		*driver;

	if (stream && stream->dead)
		stream = stream->driver->close(stream, 0);

	/* try to reuse an existing stream */
	if (stream)
		stream = stream->driver->open(stream, addr, options);

	/* make a new stream */
	if (stream == NULL) {
		driver = cal_getdriver(addr);
		if (driver)
			stream = driver->open(NULL, addr, options);
		if (stream)
			stream->driver = driver;
	}

	/* update the address field */
	if (stream) {
		caladdr_free(stream->addr);
		stream->addr = caladdr_dup(addr);
	}

	return stream;
}


CALSTREAM*
cal_open(CALSTREAM *stream, const char *address, long options)
{
	CALADDR		*addr;

	/* parse out the address and pass it to cal_open_addr() */
	addr = caladdr_parse(address);
	if (addr == NULL) {
		if (stream)
			cal_close(stream, 0);
		return NULL;
	}

	stream = cal_open_addr(stream, addr, options);

	caladdr_free(addr);

	return stream;
}


CALSTREAM*
cal_close(CALSTREAM *stream, long options)
{
	if (stream == NULL)
		return NULL;

	/* Free our part and let the driver free the rest. */
	caladdr_free(stream->addr);
	stream = stream->driver->close(stream, options);

	return stream;
}


bool
cal_ping(CALSTREAM *stream)
{
	if (stream == NULL || stream->dead)
		return false;
	return stream->driver->ping(stream);
}


bool
cal_search_range(	CALSTREAM *stream,
			const datetime_t *start,
			const datetime_t *end)
{
	if (stream == NULL || stream->dead)
		return false;
	return stream->driver->search_range(stream, start, end);
}


bool
cal_search_alarm(CALSTREAM *stream, const datetime_t *when)
{
	if (stream == NULL || stream->dead)
		return false;
	return stream->driver->search_alarm(stream, when);
}


bool
cal_fetch(CALSTREAM *stream, unsigned long id, CALEVENT **event)
{
	if (stream == NULL || stream->dead)
		return false;
	return stream->driver->fetch(stream, id, event);
}


bool
cal_append_addr(CALSTREAM *stream, const CALADDR *addr,
		unsigned long *id, const CALEVENT *event)
{
	if (stream == NULL || stream->dead)
		return false;
	if (!calevent_valid(event))
		return false;
	return stream->driver->append(stream, addr, id, event);
}


bool
cal_append(	CALSTREAM *stream, const char *folder,
		unsigned long *id, const CALEVENT *event)
{
	CALADDR		*addr;
	bool		good;

	if (stream == NULL || stream->dead)
		return false;

	addr = caladdr_parse(folder);
	if (addr == NULL)
		return false;

	good = cal_append_addr(stream, addr, id, event);

	caladdr_free(addr);

	return good;
}


bool
cal_remove(CALSTREAM *stream, unsigned long id)
{
	if (stream == NULL || stream->dead)
		return false;
	return stream->driver->remove(stream, id);
}


bool
cal_snooze(CALSTREAM *stream, unsigned long id)
{
	if (stream == NULL || stream->dead)
		return false;
	return stream->driver->snooze(stream, id);
}


bool
cal_store(CALSTREAM *stream, CALEVENT *event)
{
	char		*folder;
	bool		good;
	folder = "INBOX";
	
        if (stream == NULL || stream->dead)
                return false;

	if (event->id == 0) {
		good = cal_append (stream, folder, &event->id, event);
	} 
	else {
		good = stream->driver->store(stream, event);
	}
		
        return good;
}


bool
cal_delete(CALSTREAM *stream, char *calendar)
{
	if (stream == NULL || stream->dead)
		return false;
	return stream->driver->delete(stream, calendar);
}

bool
cal_rename(CALSTREAM *stream, char *src,char *dest)
{
	if (stream == NULL || stream->dead)
		return false;
	return stream->driver->rename(stream, src,dest);
}


/** Dummy Driver **/
static bool		dummy_valid(const CALADDR *addr);
static CALSTREAM*	dummy_open(	CALSTREAM *stream,
					const CALADDR *addr, long options);
static CALSTREAM*	dummy_close(CALSTREAM *stream, long options);
static bool		dummy_ping(CALSTREAM *stream);
static bool		dummy_create(CALSTREAM *stream, const char *calendar);
static bool		dummy_search_range(	CALSTREAM *stream,
						const datetime_t *start,
						const datetime_t *end);
static bool		dummy_search_alarm(	CALSTREAM *stream,
						const datetime_t *when);
static bool		dummy_fetch(	CALSTREAM *stream,
					unsigned long id,
					CALEVENT **event);
static bool		dummy_append(	CALSTREAM *stream,
					const CALADDR *addr,
					unsigned long *id,
					const CALEVENT *event);
static bool		dummy_remove(	CALSTREAM *stream,
					unsigned long id);
static bool		dummy_snooze(	CALSTREAM *stream,
					unsigned long id);

static bool		dummy_delete(	CALSTREAM *stream,
					char *calendar);

static bool		dummy_rename(	CALSTREAM *stream,
					char *src,char *dest);

const CALDRIVER dummy_driver =
{
	dummy_valid,
	dummy_open,
	dummy_close,
	dummy_ping,
	dummy_create,
	dummy_search_range,
	dummy_search_alarm,
	dummy_fetch,
	dummy_append,
	dummy_remove,
	dummy_snooze,
	dummy_delete,
	dummy_rename,
	
};


bool
dummy_valid(const CALADDR *addr)
{
	return false;
}


CALSTREAM*
dummy_open(CALSTREAM *stream, const CALADDR *addr, long options)
{
	free(stream);
	return NULL;
}


CALSTREAM*
dummy_close(CALSTREAM *stream, long options)
{
	free(stream);
	return NULL;
}


bool
dummy_ping(CALSTREAM *stream)
{
	return false;
}

bool
dummy_create(CALSTREAM *stream, const char *calendar)
{
	return false;
}


bool
dummy_search_range(	CALSTREAM *stream,
			const datetime_t *start,
			const datetime_t *end)
{
	return false;
}


bool
dummy_search_alarm(CALSTREAM *stream, const datetime_t *when)
{
	return false;
}


bool
dummy_fetch(CALSTREAM *stream, unsigned long id, CALEVENT **event)
{
	return false;
}


bool
dummy_append(	CALSTREAM *stream, const CALADDR *addr,
		unsigned long *id, const CALEVENT *event)
{
	return false;
}


bool
dummy_remove(CALSTREAM *stream, unsigned long id)
{
	return false;
}


bool
dummy_snooze(CALSTREAM *stream, unsigned long id)
{
	return false;
}

bool
dummy_delete(CALSTREAM *stream, char *calendar)
{
	return false;
}

bool
dummy_rename(CALSTREAM *stream, char *src,char *dest)
{
	return false;
}
/******************/
