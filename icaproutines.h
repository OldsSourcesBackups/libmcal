/*
 *	#$Id: icaproutines.h,v 1.1 1999/12/02 08:02:27 zircote Exp $
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

#ifndef	_ICAPROUTINES_H
#define	_ICAPROUTINES_H

#include <stdio.h>
#include "mcal.h"

#define	ICAPNET		struct icap_net
#define	ICAPSEARCH	struct icap_search
#define	ICAPPORT	7668
#define	ICAPMAXTAG	16


typedef enum {
	ICAP_INVALID,
	ICAP_OK,
	ICAP_NO,
	ICAP_BAD,
	ICAP_BYE,
	ICAP_GOAHEAD,
	ICAP_MISC
} icapresp_t;

typedef enum {
	ICAPTOK_NUMBER,
	ICAPTOK_STRING,
	ICAPTOK_CRLF,
	ICAPTOK_SIZE,
	ICAPTOK_TAG,
	ICAPTOK_FLAGS,
	ICAPTOK_DATETIME,
	ICAPTOK_OPER,
	ICAPTOK_JUNK,
	ICAPTOK_EOF
} icaptoken_t;

typedef enum {
	ICALTOK_ID,
	ICALTOK_COLON,
	ICALTOK_PARAMETER,
	ICALTOK_VALUE,
	ICALTOK_LF,
	ICALTOK_JUNK,
	ICALTOK_EOF
} icaltoken_t;

ICAPNET {
	FILE		*in;
	FILE		*out;
	void		*buffer;
	unsigned long	seq;
};

typedef enum {
	ICAPSEARCH_INVALID,
	ICAPSEARCH_RANGE,
	ICAPSEARCH_ALARM,
} icapsearch_t;

ICAPSEARCH {
	icapsearch_t	style;
	union {
		struct {
			datetime_t	start;
			datetime_t	end;
		} range;
		struct {
			datetime_t	when;
		} alarm;
	}		data;
};
#define	range_start	data.range.start
#define	range_end	data.range.end
#define	alarm_when	data.alarm.when


extern const char	*icaptok_s;
extern unsigned long	icaptok_n;


ICAPNET*	icapnet_open(const char *host, unsigned short port);
ICAPNET*	icapnet_close(ICAPNET *net);

bool		icap_begin(ICAPNET *net, const char *cmd);
bool		icap_flags(ICAPNET *net, unsigned long flags);
bool		icap_opaque(ICAPNET *net, const char *arg);
bool		icap_literal(ICAPNET *net, const char *arg);
icapresp_t	icap_end(ICAPNET *net);

icapresp_t	icap_getresp(ICAPNET *net, char *tag, int size);
icaptoken_t	icap_token(ICAPNET *net);
bool		icap_gobble(ICAPNET *net);
bool		icap_tag(ICAPNET *net, char *tag, int size);
bool		icap_getsearch(ICAPNET *net, ICAPSEARCH *search);


/* ICAP parser. */
extern CALEVENT		**icap_fetched_event;
extern unsigned long	icap_uid;
extern char		*icap_yytext;
extern bool		icap_endl;
int			icap_yylex(void);
void*			icap_makebuf(FILE *fp);
void			icap_killbuf(void *buffer);
void			icap_usebuf(void *buffer);
bool			icap_readraw(char *buf, size_t size);
bool			icap_readtag(char *buf, size_t size);
bool			icap_readgobble(void);
bool			icap_readsearch(ICAPSEARCH *search);

/* ICAL parser. */
extern char	*ical_yytext;
extern int	ical_yyleng;
int		ical_yylex(void);
void		ical_usebuf(const char *buf, size_t size);
void		ical_preprocess(char *buf, size_t *size);
bool		ical_parse(CALEVENT **event, const char *buf, size_t size);
bool		ical_parse_vevent(CALEVENT *event);

/* ICAL output. */
FILE*		icalout_begin(void);
bool		icalout_event(FILE *tmp, const CALEVENT *event);
char*		icalout_end(FILE *tmp);

void		icalout_label(FILE *out, const char *label);
void		icalout_number(FILE *out, unsigned long value);
void		icalout_string(FILE *out, const char *value);
void		icalout_datetime(FILE *out, const datetime_t *value);

void		ical_encode_base64(	FILE *out,
					const unsigned char *buf,
					size_t size);


/* Parse routines. */
unsigned char*	icap_decode_base64(unsigned char *buf, size_t *size);
bool		icap_decode_dt(datetime_t *dt, const char *s);

#endif
