/*
 *	#$Id: icalroutines.h,v 1.2 2000/05/11 19:43:23 inan Exp $
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

#ifndef	_ICALROUTINES_H
#define	_ICALROUTINES_H

#include <stdio.h>
#include "mcal.h"

typedef enum {
	ICALTOK_ID,
	ICALTOK_COLON,
	ICALTOK_PARAMETER,
	ICALTOK_VALUE,
	ICALTOK_LF,
	ICALTOK_JUNK,
	ICALTOK_EOF
} icaltoken_t;


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

/* ICAL formatting. */
#define BYDAY_INIT {0, {0,0,0,0,0,0,0}}
typedef struct {
	unsigned int 	weekdays;
	int			ordwk[7];
} byday_t;

void		ical_set_byday(char *output, const byday_t *input);
void		ical_get_byday(byday_t *output, const char *input);
void		ical_get_recur_freq( recur_t *output, const char *input, const char *byday);

#endif
