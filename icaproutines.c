/*
 *	#$Id: icaproutines.c,v 1.1 1999/12/02 08:02:27 zircote Exp $
 *
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

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include "icaproutines.h"


unsigned long		icaptok_n = 0;
const char*		icaptok_s = NULL;

CALEVENT		**icap_fetched_event = NULL;
unsigned long		icap_uid = 0;


ICAPNET*
icapnet_open(const char *host, unsigned short port)
{
	ICAPNET			*net = NULL;
	struct hostent		*he;
	struct sockaddr_in	addr;
	int			fd = -1;
	int			x;
	char			tag[ICAPMAXTAG];

	net = calloc(1, sizeof(*net));
	if (net == NULL)
		goto sysfail;

	he = gethostbyname(host);
	if (he == NULL)
		goto hostfail;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	memcpy(&addr.sin_addr.s_addr, he->h_addr_list[0], he->h_length);
	addr.sin_port = htons(port ? port : (ICAPPORT));

	fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd == -1)
		goto sysfail;

	x = connect(fd, (struct sockaddr*) &addr, sizeof(addr));
	if (x == -1)
		goto sysfail;

	net->in = fdopen(fd, "r+");
	if (net->in == NULL)
		goto sysfail;
	net->out = net->in;

	net->buffer = icap_makebuf(net->in);
	if (net->buffer == false)
		goto fail;

	if (icap_getresp(net, tag, ICAPMAXTAG) != ICAP_OK)
		goto fail;
	if (tag[0] != '*' || tag[1])
		goto fail;

	return net;


hostfail:
	herror("gethostbyname");
	goto fail;
sysfail:
	perror("icapnet_open");
fail:
	if (net) {
		if (net->buffer)
			icap_killbuf(net->buffer);
		if (net->in)
			fclose(net->in);
		if (net->out && net->in != net->out)
			fclose(net->out);
		else if (fd != -1)
			close(fd);
		free(net);
	}
	return NULL;
}


ICAPNET*
icapnet_close(ICAPNET *net)
{
	if (net) {
		if (net->buffer)
			icap_killbuf(net->buffer);
		if (net->in)
			fclose(net->in);
		if (net->out && net->in != net->out)
			fclose(net->out);
		free(net);
	}
	return NULL;
}


bool
icap_begin(ICAPNET *net, const char *cmd)
{
	fprintf(net->out, "%05lu %s", ++net->seq, cmd);
	return true;
}


bool
icap_flags(ICAPNET *net, unsigned long flags)
{
	fprintf(net->out, " ()");
	return true;
}


bool
icap_opaque(ICAPNET *net, const char *arg)
{
	fprintf(net->out, "%s", arg);
	return true;
}


bool
icap_literal(ICAPNET *net, const char *arg)
{
	icapresp_t	resp;
	char		tag[ICAPMAXTAG];

	fprintf(net->out, " {%u}\r\n", strlen(arg));

	do {
		resp = icap_getresp(net, tag, ICAPMAXTAG);
		if (resp == ICAP_INVALID)
			return false;
	} while (tag[0] == '*');
	if (resp != ICAP_GOAHEAD)
		return false;

	fprintf(net->out, "%s", arg);

	return true;
}


icapresp_t
icap_end(ICAPNET *net)
{
	icapresp_t	resp;
	char		tag[ICAPMAXTAG];

	fprintf(net->out, "\r\n");

	do {
		resp = icap_getresp(net, tag, ICAPMAXTAG);
		if (resp == ICAP_INVALID)
			return resp;
	} while (tag[0] == '*');

	return resp;
}


icapresp_t
icap_getresp(ICAPNET *net, char *tag, int size)
{
	icapresp_t	ret = ICAP_INVALID;
	int		token;


	if (!icap_tag(net, tag, size))
		return ICAP_INVALID;

	if (tag[0] == '+') {
		if (!icap_gobble(net))
			return ICAP_INVALID;
		return ICAP_GOAHEAD;
	}

	token = icap_token(net);
	switch (token) {
	case ICAPTOK_NUMBER:
		if ((token = icap_token(net)) == ICAPTOK_STRING) {
			if (	!strcasecmp(icaptok_s, "FETCH") &&
				(token = icap_token(net)) == ICAPTOK_STRING &&
				!strcasecmp(icaptok_s, "ICAL") &&
				(token = icap_token(net)) == ICAPTOK_SIZE &&
				(token = icap_token(net)) == ICAPTOK_CRLF)
			{
				char		*buf;
				size_t		size;

				size = icaptok_n;

				buf = calloc(1, size + 2);
				if (	buf == NULL ||
					!icap_readraw(buf, size) ||
					icap_token(net) != ICAPTOK_CRLF)
				{
					free(buf);
					break;
				}

				if (!icap_fetched_event || *icap_fetched_event)
				{	free(buf);
					ret = ICAP_MISC;
					break;
				}

				ical_preprocess(buf, &size);

				buf[size] = '\0';
				buf[size+1] = '\0';


				if (!ical_parse(icap_fetched_event, buf, size))
				{	free(buf);
					break;
				}

				free(buf);
				ret = ICAP_MISC;
			}
		}
		break;
	case ICAPTOK_STRING:
		if (!strcasecmp(icaptok_s, "OK"))
			ret = ICAP_OK;
		else if (!strcasecmp(icaptok_s, "NO"))
			ret = ICAP_NO;
		else if (!strcasecmp(icaptok_s, "BAD"))
			ret = ICAP_BAD;
		else if (!strcasecmp(icaptok_s, "BYE"))
			ret = ICAP_BYE;
		else if (!strcasecmp(icaptok_s, "SEARCH")) {
			token = icap_token(net);
			while (token == ICAPTOK_NUMBER) {
				cc_searched(icaptok_n);
				token = icap_token(net);
			}
			if (token != ICAPTOK_CRLF)
				return ICAP_INVALID;
			return ICAP_MISC;
		}
		else if (!strcasecmp(icaptok_s, "UID")) {
			if (	icap_token(net) == ICAPTOK_NUMBER &&
				icap_token(net) == ICAPTOK_CRLF)
			{
				icap_uid = icaptok_n;
				return ICAP_MISC;
			}
		}
		break;
	}

	if (!icap_gobble(net))
		return ICAP_INVALID;

	return ret;
}


icaptoken_t
icap_token(ICAPNET *net)
{
	int		token;


	fflush(net->out);
	icap_usebuf(net->buffer);
	token = icap_yylex();
	switch (token) {
	case ICAPTOK_SIZE:
		icap_yytext++;
	case ICAPTOK_NUMBER:
		errno = 0;
		icaptok_n = strtoul(icap_yytext, NULL, 10);
		if (errno)
			return ICAPTOK_JUNK;
		break;
	case ICAPTOK_STRING:
		icaptok_s = icap_yytext;
		break;
	}

	return token;
}


bool
icap_gobble(ICAPNET *net)
{
	fflush(net->out);
	icap_usebuf(net->buffer);
	return icap_readgobble();
}


bool
icap_getsearch(ICAPNET *net, ICAPSEARCH *search)
{
	fflush(net->out);
	icap_usebuf(net->buffer);
	return icap_readsearch(search);
}


bool
icap_tag(ICAPNET *net, char *tag, int size)
{
	fflush(net->out);
	icap_usebuf(net->buffer);
	return icap_readtag(tag, size);
}


void
ical_preprocess(char *buf, size_t *size)
{
	enum {	st_norm,
		st_cr,
		st_crlf
	}		state = st_norm;
	char		*in = buf;
	char		*out = buf;

	for (; (in-buf) < *size; in++)
		switch (state) {
		case st_cr:
			if (*in == '\n') {
				*(out-1) = *in;
				state = st_crlf;
			}
			else {
				state = st_norm;
				*out++ = *in;
			}
			break;
		case st_crlf:
			if (*in == ' ') {
				out--;
				state = st_norm;
				break;
			}
			state = st_norm;
		case st_norm:
			if (*in == '\r')
				state = st_cr;
			else if (*in == '\n')
				state = st_crlf;
			*out++ = *in;
		}

	*size = out - buf;
}


#define	NEXT_IS(t)	((token = ical_yylex()) == (t))
#define	VALUE_IS(v)	(!strcasecmp(ical_yytext, (v)))
bool
ical_parse(CALEVENT **event, const char *buf, size_t size)
{
	int		token;


	*event = NULL;
	ical_usebuf(buf, size);

	if (!(	NEXT_IS(ICALTOK_ID) && VALUE_IS("begin") &&
		NEXT_IS(ICALTOK_VALUE) && VALUE_IS("vcalendar") &&
		NEXT_IS(ICALTOK_LF)))
	{
		goto fail;
	}

	while (true) {
		if (!NEXT_IS(ICALTOK_ID))
			goto fail;
		if (VALUE_IS("end")) {
			if (!(	NEXT_IS(ICALTOK_VALUE) &&
				VALUE_IS("vcalendar")) &&
				NEXT_IS(ICALTOK_LF))
			{
				goto fail;
			}
			return true;
		}
		else if (VALUE_IS("begin")) {
			if (!NEXT_IS(ICALTOK_VALUE))
				goto fail;
			if (VALUE_IS("vevent") && NEXT_IS(ICALTOK_LF)) {
				if (*event)
					*event = calevent_free(*event);
				*event = calevent_new();
				if (*event == NULL)
					goto fail;
				if (!ical_parse_vevent(*event))
					goto fail;
			}
			else if (!NEXT_IS(ICALTOK_LF))
				goto fail;
		}
		else if (VALUE_IS("version")) {
			if (!(NEXT_IS(ICALTOK_VALUE) && VALUE_IS("2.0")))
				goto fail;
			if (!(NEXT_IS(ICALTOK_LF)))
				goto fail;
		}
		else if (VALUE_IS("prodid")) {
			if (!(NEXT_IS(ICALTOK_VALUE) && NEXT_IS(ICALTOK_LF)))
				goto fail;
		}
	}

fail:
	if (*event)
		*event = calevent_free(*event);
	return false;
}


bool
ical_parse_vevent(CALEVENT *event)
{
	char		*ptr;
	char		*value = NULL;
	size_t		size;
	int		token;
	enum {
		P_UNKNOWN,
		P_END,
		P_DESCRIPTION,
		P_CATEGORIES,
		P_SUMMARY,
		P_DTSTART,
		P_DTEND,
		P_UID,
		P_CLASS,
		P_XALARM,
		P_XRECURTYPE,
		P_XRECURINTERVAL,
		P_XRECURWEEKDAYS,
		P_XRECURENDDATE,
	}		property;
	enum {
		E_NONE,
		E_BASE64,
	}		encoding;


	while (true) {
		property = P_UNKNOWN;
		encoding = E_NONE;

		if (!NEXT_IS(ICALTOK_ID))
			return false;

		if (VALUE_IS("end"))
			property = P_END;
		else if (VALUE_IS("description"))
			property = P_DESCRIPTION;
		else if (VALUE_IS("categories"))
			property = P_CATEGORIES;
		else if (VALUE_IS("summary"))
			property = P_SUMMARY;
		else if (VALUE_IS("dtstart"))
			property = P_DTSTART;
		else if (VALUE_IS("dtend"))
			property = P_DTEND;
		else if (VALUE_IS("uid"))
			property = P_UID;
		else if (VALUE_IS("class"))
			property = P_CLASS;
		else if (VALUE_IS("x-alarm"))
			property = P_XALARM;
		else if (VALUE_IS("x-recur-type"))
			property = P_XRECURTYPE;
		else if (VALUE_IS("x-recur-interval"))
			property = P_XRECURINTERVAL;
		else if (VALUE_IS("x-recur-weekdays"))
			property = P_XRECURWEEKDAYS;
		else if (VALUE_IS("x-recur-enddate"))
			property = P_XRECURENDDATE;

		while (NEXT_IS(ICALTOK_PARAMETER)) {
			ptr = strchr(ical_yytext, '=');
			*ptr++ = '\0';
			if (!strcasecmp(ical_yytext, "encoding")) {
				/* Only one encoding may be specified. */
				if (encoding != E_NONE)
					return false;

				if (!strcasecmp(ptr, "base64"))
					encoding = E_BASE64;
				else /* Unknown encoding. */
					return false;
			}
		}

		if (token != ICALTOK_VALUE)
			return false;

		/* decode the value */
		if (encoding == E_NONE) {
			size = ical_yyleng;
			value = ical_yytext;
		}
		else if (encoding == E_BASE64) {
			size = ical_yyleng;
			value = icap_decode_base64(ical_yytext, &size);
			if (value == NULL)
				return false;
		}

		switch (property) {
		case P_END:
			if (strcasecmp(value, "vevent"))
				return false;
			if (!NEXT_IS(ICALTOK_LF))
				return false;

			/* XXX fields check */
			return true;
		case P_DESCRIPTION:
			if (event->description)
				return false;
			if ((event->description = strdup(value)) == NULL)
				return false;
			break;
		case P_CATEGORIES:
			if (event->category)
				return false;
			if ((event->category = strdup(value)) == NULL)
				return false;
			break;
		case P_SUMMARY:
			if (event->title)
				return false;
			if ((event->title = strdup(value)) == NULL)
				return false;
			break;
		case P_DTSTART:
			if (	dt_hasdate(&event->start) ||
				dt_hastime(&event->start))
			{
				return false;
			}
			if (!icap_decode_dt(&event->start, value))
				return false;
			if (!dt_hasdate(&event->start))
				return false;
			break;
		case P_DTEND:
			if (	dt_hasdate(&event->end) ||
				dt_hastime(&event->end))
			{
				return false;
			}
			if (!icap_decode_dt(&event->end, value))
				return false;
			if (!dt_hasdate(&event->end))
				return false;
			break;
		case P_UID:
			if (event->id)
				return false;
			errno = 0;
			event->id = strtoul(value, &ptr, 10);
			if (*ptr || errno || !event->id)
				return false;
			break;
		case P_CLASS:
			if (!strcasecmp(value, "public"))
				event->public = 1;
			else if (!strcasecmp(value, "private"))
				event->public = 0;
			else if (!strcasecmp(value, "confidential"))
				event->public = 0;
			else
				return false;
			break;
		case P_XALARM:
			if (event->alarm)
				return false;
			errno = 0;
			event->alarm = strtoul(value, &ptr, 10);
			if (*ptr || errno || !event->alarm)
				return false;
			break;
		case P_XRECURTYPE:
			if (event->recur_type)
				return false;
			errno = 0;
			event->recur_type = strtoul(value, &ptr, 10);
			if (*ptr || errno || !event->recur_type ||
				event->recur_type >= NUM_RECUR_TYPES)
			{
				return false;
			}
			break;
		case P_XRECURINTERVAL:
			if (event->recur_interval)
				return false;
			errno = 0;
			event->recur_interval = strtoul(value, &ptr, 10);
			if (*ptr || errno || !event->recur_interval)
				return false;
			break;
		case P_XRECURWEEKDAYS:
			if (event->recur_data.weekly_wday)
				return false;
			errno = 0;
			event->recur_data.weekly_wday =
				strtoul(value, &ptr, 10);
			if (*ptr || errno)
				return false;
			if (!(event->recur_data.weekly_wday & M_ALLDAYS))
				return false;
			if (event->recur_data.weekly_wday & ~M_ALLDAYS)
				return false;
			break;
		case P_XRECURENDDATE:
			if (dt_hasdate(&event->recur_enddate))
				return false;
			if (!icap_decode_dt(&event->recur_enddate, value))
				return false;
			if (!dt_hasdate(&event->recur_enddate))
				return false;
			if (dt_hastime(&event->recur_enddate))
				return false;
			break;
			break;
		default:
		}

		if (!NEXT_IS(ICALTOK_LF))
			return false;
	}
}
#undef	NEXT_IS
#undef	VALUE_IS


unsigned char*
icap_decode_base64(unsigned char *buf, size_t *size)
{
	size_t		left;
	unsigned char	*in;
	unsigned char	*out;
	int		bsize;
	int		i;

	if (*size % 4)
		return NULL;

	left = *size;
	*size = 0;
	for (in=buf,out=buf; left>0; in+=4,left-=4) {
		if (in[0]=='=' || in[1]=='=')
			return NULL;
		if (in[2]=='=' && in[3]!='=')
			return NULL;
		if (in[3]=='=' && left>4)
			return NULL;

		bsize = 3;
		for (i=0; i<4; i++) {
			switch (in[i]) {
			case '=':
				in[i] = 0;
				bsize--;
				break;
			case '+':
				in[i] = 62; break;
			case '/':
				in[i] = 63; break;
			default:
				if (in[i]>='A' && in[i]<='Z')
					in[i] -= 'A';
				else if (in[i]>='a' && in[i]<='z')
					in[i] -= 'a' - 26;
				else if (in[i]>='0' && in[i]<='9')
					in[i] -= '0' - 52;
				else
					return NULL;
			}
		}

		out[0] = (0xfc&in[0]<<2) | (0x03&in[1]>>4);
		if (bsize>0)
			out[1] = (0xf0&in[1]<<4) | (0x0f&in[2]>>2);
		if (bsize>1)
			out[2] = (0xc0&in[2]<<6) | (0x3f&in[3]);

		out += bsize;
	}

	*out = '\0';
	*size = out - buf;

	return buf;
}


bool
icap_decode_dt(datetime_t *dt, const char *s)
{
	unsigned long	n;
	char		*endp;

	dt_init(dt);

	if (*s != 'T' && *s != 't') {
		n = strtoul(s, &endp, 10);
		if (endp - s != 8)
			return false;
		if (!dt_setdate(dt, n/10000, (n/100)%100, n%100))
			return false;
		if (*endp == '\0')
			return true;
		s = endp;
	}

	if (*s == 'T' || *s == 't') {
		n = strtoul(++s, &endp, 10);
		if (endp - s != 6)
			return false;
		if (!dt_settime(dt, n/10000, (n/100)%100, n%100))
			return false;
		if (*endp && *endp != 'Z' && *endp != 'z')
			return false;
	}
	else {
		return false;
	}

	return true;
}


/* ICAL output. */
FILE*
icalout_begin(void)
{
	FILE		*tmp;

	if ((tmp = tmpfile()) == NULL)
		return NULL;

	fputs(	"BEGIN:VCALENDAR\r\n"
		"VERSION:2.0\r\n"
		"PRODID:-//Chek Inc//NONSGML Chek Calendar//EN\r\n",
		tmp);

	if (feof(tmp) || ferror(tmp)) {
		fclose(tmp);
		tmp = NULL;
	}

	return tmp;
}


bool
icalout_event(FILE *tmp, const CALEVENT *event)
{
	datetime_t	dt;

	fputs("BEGIN:VEVENT\r\n", tmp);
	if (event->id) {
		icalout_label(tmp, "UID");
		icalout_number(tmp, event->id);
	}

	fprintf(tmp, "CLASS:%s\r\n", (event->public) ? "PUBLIC" : "PRIVATE");

	if (!dt_empty(&event->start)) {
		icalout_label(tmp, "DTSTART");
		icalout_datetime(tmp, &event->start);
	}
	if (!dt_empty(&event->end)) {
		dt = event->end;

		if (	!dt_hasdate(&event->end) &&
			dt_hasdate(&event->start))
		{
			dt_setdate(&dt,	event->start.year,
					event->start.mon,
					event->start.mday);
		}
		icalout_label(tmp, "DTEND");
		icalout_datetime(tmp, &dt);
	}
	if (event->category) {
		icalout_label(tmp, "CATEGORIES");
		icalout_string(tmp, event->category);
	}
	if (event->title) {
		icalout_label(tmp, "SUMMARY");
		icalout_string(tmp, event->title);
	}
	if (event->description) {
		icalout_label(tmp, "DESCRIPTION");
		icalout_string(tmp, event->description);
	}

	if (event->alarm) {
		icalout_label(tmp, "X-ALARM");
		icalout_number(tmp, event->alarm);
	}

	if (event->recur_type != RECUR_NONE) {
		icalout_label(tmp, "X-RECUR-TYPE");
		icalout_number(tmp, event->recur_type);
		icalout_label(tmp, "X-RECUR-INTERVAL");
		icalout_number(tmp, event->recur_interval);
		if (dt_hasdate(&event->recur_enddate)) {
			icalout_label(tmp, "X-RECUR-ENDDATE");
			icalout_datetime(tmp, &event->recur_enddate);
		}
		if (event->recur_type == RECUR_WEEKLY) {
			icalout_label(tmp, "X-RECUR-WEEKDAYS");
			icalout_number(tmp, event->recur_data.weekly_wday);
		}
	}

	fputs("END:VEVENT\r\n", tmp);

	if (feof(tmp) || ferror(tmp)) {
		fclose(tmp);
		return false;
	}

	return true;
}


char*
icalout_end(FILE *tmp)
{
	char		*buf = NULL;
	size_t		size;

	fputs("END:VCALENDAR\r\n", tmp);
	if (feof(tmp) || ferror(tmp))
		goto fail;

	size = ftell(tmp);
	buf = calloc(1, size + 1);
	if (buf == NULL)
		goto fail;

	rewind(tmp);
	fread(buf, size, 1, tmp);
	if (feof(tmp) || ferror(tmp))
		goto fail;
	buf[size] = '\0';

	fclose(tmp);
	return buf;

fail:
	free(buf);
	fclose(tmp);
	return NULL;
}


void
icalout_label(FILE *out, const char *label)
{
	while (*label)
		putc(*label++, out);
}


void
icalout_number(FILE *out, unsigned long value)
{
	fprintf(out, ":%lu\r\n", value);
}


void
icalout_string(FILE *out, const char *value)
{
	fputs(";ENCODING=BASE64:", out);
	ical_encode_base64(out, value, strlen(value));
	putc('\r', out);
	putc('\n', out);
}


void
icalout_datetime(FILE *out, const datetime_t *value)
{
	putc(':', out);

	if (dt_hasdate(value))
		fprintf(out,	"%04u%02u%02u",
				value->year, value->mon, value->mday);
	if (dt_hastime(value))
		fprintf(out,	"T%02u%02u%02uZ",
				value->hour, value->min, value->sec);
	putc('\r', out);
	putc('\n', out);
}


void
ical_encode_base64(FILE *out, const unsigned char *buf, size_t size)
{
	static const char pad = '=';
	static const char base64[64] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/";
	int		ch;

	for (; size>=3; buf+=3,size-=3) {
		putc(base64[0x3f&buf[0]>>2], out);
		putc(base64[(0x30&buf[0]<<4) | (0x0f&buf[1]>>4)], out);
		putc(base64[(0x3c&buf[1]<<2) | (0x03&buf[2]>>6)], out);
		putc(base64[0x3f&buf[2]], out);
	}

	if (size > 0) {
		putc(base64[0x3f&buf[0]>>2], out);
		ch = 0x30&buf[0]<<4;
		if (size > 1) {
			putc(base64[ch | (0x0f&buf[1]>>4)], out);
			putc(base64[0x3c&buf[1]<<2], out);
		}
		else {
			putc(base64[ch], out);
			putc(pad, out);
		}
		putc(pad, out);
	}
}
