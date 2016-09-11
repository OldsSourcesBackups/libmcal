%{
/*
 *	#$Id: icalscanner.lex,v 1.1 2000/01/25 03:08:10 markie Exp $
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

#include "icalroutines.h"

void ical_usebuf(const char *buf, size_t size)
{
	BEGIN(0);
	yy_scan_bytes(buf, size);
}
%}


LF		\n
DIGIT		[0-9]
SAFECHAR	[^\x00-\x1F":;,]
IDCHAR		[a-zA-Z0-9]|"-"


%option case-insensitive
%option nomain
%option noyywrap
%option nounput
%option prefix="ical_yy"

%s param

%%

<INITIAL>{IDCHAR}+		BEGIN(param); return ICALTOK_ID;
<param>";"{IDCHAR}+"="{IDCHAR}+	{	ical_yyleng--;
					ical_yytext++;
					return ICALTOK_PARAMETER; }
<param>":"[^\n]*		{	ical_yyleng--;
					ical_yytext++;
					return ICALTOK_VALUE; }
{LF}				BEGIN(INITIAL); return ICALTOK_LF;
.				return ICALTOK_JUNK;
<<EOF>>				return ICALTOK_EOF;

