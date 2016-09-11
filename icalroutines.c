/*
 *	#$Id: icalroutines.c,v 1.3 2001/12/10 03:16:41 chuck Exp $
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
#include "icalroutines.h"

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
	char		*name = NULL;
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
		else if ((name = strdup(ical_yytext)) == NULL)
			return false;

		while (NEXT_IS(ICALTOK_PARAMETER)) {
			ptr = strchr(ical_yytext, '=');
			*ptr++ = '\0';
			if (!strcasecmp(ical_yytext, "encoding")) {
				/* Only one encoding may be specified. */
				if (encoding != E_NONE) {
					free(name);
					return false;
				}

				if (!strcasecmp(ptr, "base64"))
					encoding = E_BASE64;
				else {
					/* Unknown encoding. */
					free(name);
					return false;
				}
			}
		}

		if (token != ICALTOK_VALUE) {
			free(name);
			return false;
		}

		/* decode the value */
		if (encoding == E_NONE) {
			size = ical_yyleng;
			value = ical_yytext;
		}
		else if (encoding == E_BASE64) {
			size = ical_yyleng;
			value = (char *)cal_decode_base64((unsigned char *)ical_yytext, &size);
			if (value == NULL) {
				free(name);
				return false;
			}
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
			if (!cal_decode_dt(&event->start, value))
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
			if (!cal_decode_dt(&event->end, value))
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
			if (!cal_decode_dt(&event->recur_enddate, value))
				return false;
			if (!dt_hasdate(&event->recur_enddate))
				return false;
			if (dt_hastime(&event->recur_enddate))
				return false;
			break;
		case P_UNKNOWN:
			if (	calevent_getattr(event, name) ||
				!calevent_setattr(event, name, value))
			{
				free(name);
				return false;
			}
			free(name);
			break;
		}

		if (!NEXT_IS(ICALTOK_LF))
			return false;
	}
}
#undef	NEXT_IS
#undef	VALUE_IS

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
	CALATTR *attr;

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
	if(event->attrlist)
	  {
	    for (attr = event->attrlist; attr; attr = attr->next) {
	      icalout_label(tmp,attr->name);
	      icalout_string(tmp,attr->value);
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
	ical_encode_base64(out, (const unsigned char *)value, strlen(value));
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
        int             ch;

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

#define MAX(a,b) (a)>(b) ? (a) :(b) 
void
ical_set_byday ( char *output, const byday_t *input ) {
	int	i;
	char	temp_string[50];
	
	strcpy (output, "");
	
	for (i=0; i<=6; i++) {
		/* wdays is a bit field corresponding to days of the week */
		if ((input->weekdays) & (0x1 <<i) ) {
			if (input->ordwk[i] != 0) {
				sprintf( temp_string, "%d", input->ordwk[i] );
				strcat ( output, temp_string );
			}
			switch (i) {
				case 0:
					strcat ( output, "SU, ");
					break;
				case 1:
					strcat ( output, "MO, ");
					break;	
				case 2:
					strcat ( output, "TU, ");
					break;
				case 3:
					strcat ( output, "WE, ");
					break;
				case 4:
					strcat ( output, "TH, ");
					break;
				case 5:
					strcat ( output, "FR, ");
					break;
				case 6:
					strcat ( output, "SA, ");
					break;
			}
		}
	}
        output[MAX(strlen(output)-2,0)] = '\0';

}
#undef MAX

void
ical_get_byday ( byday_t *output, const char *input ) {
    char *temp_string;  
    char *token;
    char *day_start;
    int  interval;
    int  day_index;
    temp_string = strdup(input);

    token = strtok (temp_string, ",");
    while ( token != NULL ) {
        day_start = token + strlen(token) - 2;
	if (strcasecmp (day_start, "SU")==0) {
	    output->weekdays |= M_SUNDAY;
	    day_index = 0;
	} else if (strcasecmp (day_start, "MO")==0) {
	    output->weekdays |= M_MONDAY;
	    day_index = 1;
	} else if (strcasecmp (day_start, "TU")==0) {
	    output->weekdays |= M_TUESDAY;
	    day_index = 2;
	} else if (strcasecmp (day_start, "WE")==0) {
	    output->weekdays |= M_WEDNESDAY;
	    day_index = 3;
	} else if (strcasecmp (day_start, "TH")==0) {
	    output->weekdays |= M_THURSDAY;
	    day_index = 4;
	} else if (strcasecmp (day_start, "FR")==0) {
	    output->weekdays |= M_FRIDAY;
	    day_index = 5;
	} else if (strcasecmp (day_start, "SA")==0) {
	    output->weekdays |= M_SATURDAY;
	    day_index = 6;
        }
        *day_start = '\0';
	interval = atoi (token);
	output->ordwk[day_index] = interval;
    }
    
    free( token );
    free( temp_string );
}

void
ical_get_recur_freq( recur_t *output, const char *input, const char *byday) {
    if (strncmp (input, "D",1)==0) {
        *output = RECUR_DAILY;
    } else if (strncmp (input, "W",1)==0) {
        *output = RECUR_WEEKLY;
    } else if (strncmp (input, "M",1)==0) {
        if (strlen(byday) == 0) {
	    *output = RECUR_MONTHLY_MDAY;
	} else {
            *output = RECUR_MONTHLY_WDAY;
	}
    } else if (strncmp (input, "Y",1)==0) {
        *output = RECUR_YEARLY;
    } else {
        *output = RECUR_NONE;
    }
}
