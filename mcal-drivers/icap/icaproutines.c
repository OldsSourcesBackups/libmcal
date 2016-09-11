/*
 *	#$Id: icaproutines.c,v 1.4 2000/01/25 03:08:10 markie Exp $
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

