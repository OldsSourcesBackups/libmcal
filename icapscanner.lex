%{
/*
 *	#$Id: icapscanner.lex,v 1.1 1999/12/02 08:02:27 zircote Exp $
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

#include <string.h>
#include "icaproutines.h"
%}

%option nounput
%option noyywrap
%option always-interactive
%option case-insensitive
%option prefix="icap_yy"

CHAR		[A-Za-z0-9!@<>\./{}]
DIGIT		[0-9]
CRLF		\r?\n

DATE		{DIGIT}{8}
TIME		T{DIGIT}{6}(Z?)

%x tag gobble icapsearch

%%

%{
	yy_set_bol(0);
%}

<icapsearch>{
"ALL"			|
"ICAL"			|
"DTSTART"		|
"ALARMING"		return ICAPTOK_STRING;
{DATE}			|
{TIME}			|
{DATE}{TIME}		return ICAPTOK_DATETIME;
"<="			|
">="			return ICAPTOK_OPER;
{CRLF}			yy_set_bol(1); return ICAPTOK_CRLF;
" "+			/* ignore */
.			return ICAPTOK_JUNK;
<<EOF>>			return ICAPTOK_EOF;
}

<gobble>{
[^\r\n]+		return ICAPTOK_JUNK;
{CRLF}			yy_set_bol(1); return ICAPTOK_CRLF;
<<EOF>>			return ICAPTOK_EOF;
}

<tag>{
"+"			|
"*"			|
[[:alnum:]]+		return ICAPTOK_TAG;
[.\n]			yyless(0); return ICAPTOK_JUNK;
}


"{"{DIGIT}+"}"		return ICAPTOK_SIZE;
{DIGIT}+		return ICAPTOK_NUMBER;
{CRLF}			yy_set_bol(1); return ICAPTOK_CRLF;
{CHAR}+			return ICAPTOK_STRING;
"()"			return ICAPTOK_FLAGS;
"\"\""			{	icap_yyleng = 0;
				*icap_yytext = '\0';
				return ICAPTOK_STRING; }
" "+			/* ignore */
.			return ICAPTOK_JUNK;
<<EOF>>			return ICAPTOK_EOF;

%%

void*
icap_makebuf(FILE *fp)
{
	return yy_create_buffer(fp, YY_BUF_SIZE);
}


void
icap_killbuf(void *buffer)
{
	yy_delete_buffer((YY_BUFFER_STATE) buffer);
}


void
icap_usebuf(void *buffer)
{
	yy_switch_to_buffer((YY_BUFFER_STATE) buffer);
}


bool
icap_readraw(char *buf, size_t size)
{
	int		ch;

	while (size-- > 0) {
		if ((ch = input()) == EOF)
			return false;
		*buf++ = (char) ch;
	}

	return true;
}


bool
icap_readtag(char *buf, size_t size)
{
	int		token;

	BEGIN(tag);
	token = icap_yylex();
	BEGIN(INITIAL);

	if (token != ICAPTOK_TAG)
		return false;
	if (yyleng >= size)
		return false;

	strcpy(buf, yytext);
	return true;
}


bool
icap_readgobble(void)
{
	int		token;

	if (YY_AT_BOL())
		return true;

	BEGIN(gobble);
	while ((token = icap_yylex()) != ICAPTOK_CRLF) {
		if (token == ICAPTOK_EOF) {
			BEGIN(INITIAL);
			return false;
		}
	}

	return true;
}


bool
icap_readsearch(ICAPSEARCH *search)
{
	int		token;
	bool		good = true;
	enum {
		OP_LE,
		OP_GE,
	}		oper;

	search->style = ICAPSEARCH_INVALID;

	BEGIN(icapsearch);
	token = icap_yylex();
	if (token != ICAPTOK_STRING)
		goto out;

	if (!strcasecmp(icap_yytext, "ALL")) {
		search->style = ICAPSEARCH_RANGE;
		dt_init(&search->range_start);
		dt_init(&search->range_end);
	}
	else if (!strcasecmp(icap_yytext, "ALARMING")) {
		token = icap_yylex();
		if (	token != ICAPTOK_DATETIME ||
			!icap_decode_dt(&search->alarm_when, icap_yytext))
		{
			goto out;
		}
		search->style = ICAPSEARCH_ALARM;
	}
	else if (!strcasecmp(icap_yytext, "ICAL")) {
		dt_init(&search->range_start);
		dt_init(&search->range_end);

		do {
			token = icap_yylex();
			if (token != ICAPTOK_STRING)
				goto out;
			if (strcasecmp(icap_yytext, "DTSTART"))
				goto out;

			token = icap_yylex();
			if (token != ICAPTOK_OPER)
				goto out;
			if (!strcmp(icap_yytext, "<="))
				oper = OP_LE;
			else if (!strcmp(icap_yytext, ">="))
				oper = OP_GE;
			else
				goto out;

			token = icap_yylex();
			if (token != ICAPTOK_DATETIME)
				goto out;
			switch (oper) {
			case OP_GE:
				if (!dt_empty(&search->range_start))
					goto out;
				good = icap_decode_dt(	&search->range_start,
							icap_yytext);
				good &= dt_hasdate(&search->range_start);
				break;
			case OP_LE:
				if (!dt_empty(&search->range_end))
					goto out;
				good = icap_decode_dt(	&search->range_end,
							icap_yytext);
				good &= dt_hasdate(&search->range_end);
				break;
			}

			if (!good)
				goto out;

			token = icap_yylex();
		} while (	token == ICAPTOK_STRING &&
				!strcasecmp(icap_yytext, "ICAL"));
		search->style = ICAPSEARCH_RANGE;
	}
	BEGIN(INITIAL);

	return (YY_AT_BOL() || icap_yylex() == ICAPTOK_CRLF);
out:
	BEGIN(INITIAL);
	return false;
}
