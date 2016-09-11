/*
 * Libmcal - Modular Calendar Access Library 
 * Copyright (C) 1999 Mark Musone and Andrew Skalski
 * 
 *	#$Id: icap.c,v 1.4 2000/07/07 15:16:18 markie Exp $
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

#include <string.h>
#include <stdlib.h>
#include "mcal.h"
#include "icaproutines.h"


#define ICAP_VAR "0.2"
/** ICAP Driver **/
#define	DATA_T	struct icap_data
#define	DATA	((DATA_T*) stream->data)


static void		icap_freestream(CALSTREAM *stream);

static bool		icap_valid(const CALADDR *addr);
static CALSTREAM*	icap_open(	CALSTREAM *stream,
					const CALADDR *addr, long options);
static CALSTREAM*	icap_close(CALSTREAM *stream, long options);
static bool		icap_ping(CALSTREAM *stream);
static bool		icap_create(CALSTREAM *stream, const char *calendar);
static bool		icap_search_range(	CALSTREAM *stream,
						const datetime_t *start,
						const datetime_t *end);
static bool		icap_search_alarm(	CALSTREAM *stream,
						const datetime_t *when);
static bool		icap_fetch(	CALSTREAM *stream,
					unsigned long id,
					CALEVENT **event);
static bool		icap_append(	CALSTREAM *stream,
					const CALADDR *addr,
					unsigned long *id,
					const CALEVENT *event);
static bool		icap_remove(	CALSTREAM *stream,
					unsigned long id);
static bool		icap_snooze(	CALSTREAM *stream,
					unsigned long id);

const CALDRIVER icap_driver =
{
	icap_valid,
	icap_open,
	icap_close,
	icap_ping,
	icap_create,
	icap_search_range,
	icap_search_alarm,
	icap_fetch,
	icap_append,
	icap_remove,
	icap_snooze,
};


DATA_T {
	ICAPNET		*net;
	char		*host;
	char		*folder;
	char		*login_user;
	char		*folder_user;
};


void
icap_freestream(CALSTREAM *stream)
{
	if (stream) {
		if (DATA) {
			free(DATA->host);
			free(DATA->folder);
			free(DATA->login_user);
			free(DATA->folder_user);
			icapnet_close(DATA->net);
			free(DATA);
		}
		free(stream);
	}
}


bool
icap_valid(const CALADDR *addr)
{
	if (!addr->proto || strcmp(addr->proto, "icap"))
		return false;
	if (addr->host == NULL)
		return false;
	return true;
}


CALSTREAM*
icap_open(CALSTREAM *stream, const CALADDR *addr, long options)
{
	const char	*username = NULL;
	const char	*password = NULL;
	bool		reopen = false;


	if (stream) {
		reopen = true;

		/* Reject if different host. */
		if (addr->host && strcasecmp(DATA->host, addr->host))
			return icap_close(stream, 0);

		free(DATA->folder);
		free(DATA->folder_user);
		DATA->folder_user = NULL;
	}
	else {
		options |= CAL_LOGIN;
	}

	if (options & CAL_LOGIN) {
		if (stream) {
			free(DATA->login_user);
			DATA->login_user = NULL;
		}

		cc_login(&username, &password);
		if (username == NULL)
			goto fail;
		if (password == NULL)
			goto fail;
	}

	if (!reopen) {
		if ((stream = calloc(1, sizeof(*stream))) == NULL)
			goto fail;
		if ((stream->data = calloc(1, sizeof(*DATA))) == NULL)
			goto fail;

		/* Copy host. */
		if ((DATA->host = strdup(addr->host)) == NULL)
			goto fail;
	}

	if (options & CAL_LOGIN)
		if ((DATA->login_user = strdup(username)) == NULL)
			goto fail;

	if ((DATA->folder = strdup(addr->folder)) == NULL)
		goto fail;

	if (addr->user) {
		if ((DATA->folder_user = strdup(addr->user)) == NULL)
			goto fail;
	}

	if (!reopen)
		if ((DATA->net = icapnet_open(DATA->host, 0)) == NULL)
			goto fail;

	if (options & CAL_LOGIN) {
		if (reopen) {
			if (!icap_begin(DATA->net, "USERLOGOUT"))
				goto fail;
			if (icap_end(DATA->net) != ICAP_OK)
				goto fail;
		}

		if (!icap_begin(DATA->net, "LOGIN"))
			goto fail;
		if (!icap_literal(DATA->net, username))
			goto fail;
		if (!icap_literal(DATA->net, password))
			goto fail;
		if (icap_end(DATA->net) != ICAP_OK)
			goto fail;
	}

	if (!icap_begin(DATA->net, "SELECT"))
		goto fail;
	if (!icap_literal(DATA->net, DATA->folder))
		goto fail;
	if (icap_end(DATA->net) != ICAP_OK)
		goto fail;

	return stream;

fail:
	icap_freestream(stream);
	return NULL;
}


CALSTREAM*
icap_close(CALSTREAM *stream, long options)
{
	if (stream == NULL)
		return NULL;

	(void) icap_begin(DATA->net, "LOGOUT");
	(void) icap_end(DATA->net);

	icap_freestream(stream);
	return NULL;
}


bool
icap_ping(CALSTREAM *stream)
{
	if (stream == NULL)
		return false;

	if (!icap_begin(DATA->net, "NOOP"))
		return false;
	if (icap_end(DATA->net) != ICAP_OK)
		return false;
	return true;
}


bool
icap_create(CALSTREAM *stream, const char *calendar)
{
	return false;
}


bool
icap_search_range(	CALSTREAM *stream,
			const datetime_t *start,
			const datetime_t *end)
{
	char		query[1024];
	char		*endq;

	if (stream == NULL)
		return false;

	if (!icap_begin(DATA->net, "UID SEARCH"))
		return false;

	endq = query;
	if (start && dt_hasdate(start)) {
		endq += sprintf(endq, " ICAL DTSTART > %04u%02u%02u",
			start->year, start->mon, start->mday-1);
	}
	if (end && dt_hasdate(end)) {
		endq += sprintf(endq, " ICAL DTSTART < %04u%02u%02u",
			end->year, end->mon, end->mday+1);
	}
	if (endq == query)
		strcpy(query, " ALL");

	if (!icap_opaque(DATA->net, query))
		return false;

	if (icap_end(DATA->net) != ICAP_OK)
		return false;

	return true;
}


bool
icap_search_alarm(CALSTREAM *stream, const datetime_t *when)
{
	char		query[1024];

	if (stream == NULL)
		return false;
	if (dt_empty(when))
		return false;

	sprintf(query,	"UID SEARCH COMPONENT VALARM ICAL DTSTART =  %04u%02u%02uT%02u%02u%02uZ",
			when->year, when->mon, when->mday,
			when->hour, when->min, when->sec);

	if (!icap_begin(DATA->net, query))
		return false;
	return (icap_end(DATA->net) == ICAP_OK);
}


bool
icap_fetch(CALSTREAM *stream, unsigned long id, CALEVENT **event)
{
	char		query[1024];

	if (!icap_begin(DATA->net, "UID FETCH "))
		return false;
	sprintf(query, "%lu", id);
	if (!icap_opaque(DATA->net, query))
		return false;
	if (!icap_opaque(DATA->net, " ICAL"))
		return false;

	*event = NULL;
	icap_fetched_event = event;
	if (icap_end(DATA->net) != ICAP_OK) {
		*event = calevent_free(*event);
		icap_fetched_event = NULL;
		return false;
	}
	icap_fetched_event = NULL;
	return true;
}


bool
icap_append(	CALSTREAM *stream, const CALADDR *addr,
		unsigned long *id, const CALEVENT *event)
{
	FILE		*tmp;
	char		*buf;
	char		*folder;

	folder = caladdr_out(addr);
	if (folder == NULL)
		return false;

	if (!icap_begin(DATA->net, "APPEND")) {
		free(folder);
		return false;
	}
	if (!icap_literal(DATA->net, folder)) {
		free(folder);
		return false;
	}
	free(folder);
	if (!icap_opaque(DATA->net, " ()")) /* future flags */
		return false;

	if ((tmp = icalout_begin()) == NULL)
		return false;
	if (!icalout_event(tmp, event))
		return false;
	if ((buf = icalout_end(tmp)) == NULL)
		return false;
	if (!icap_literal(DATA->net, buf)) {
		free(buf);
		return false;
	}
	free(buf);

	icap_uid = 0;
	if (icap_end(DATA->net) != ICAP_OK)
		return false;
	*id = icap_uid;

	return true;
}


bool
icap_remove(CALSTREAM *stream, unsigned long id)
{
	char		query[1024];

	if (!icap_begin(DATA->net, "UID STORE "))
		return false;
	sprintf(query, "%lu +FLAGS \\Deleted", id);
	if (!icap_opaque(DATA->net, query))
		return false;
	if (icap_end(DATA->net) != ICAP_OK)
		return false;
	return true;
}


bool
icap_snooze(CALSTREAM *stream, unsigned long id)
{
	char		query[1024];

	if (!icap_begin(DATA->net, "UID SNOOZE "))
		return false;
	sprintf(query, "%lu", id);
	if (!icap_opaque(DATA->net, query))
		return false;
	if (icap_end(DATA->net) != ICAP_OK)
		return false;
	return true;
}

#undef	DATA_T
#undef	DATA
/*****************/
