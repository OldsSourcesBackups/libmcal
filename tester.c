/*
 *	$Id: tester.c,v 1.2 2000/05/11 19:43:23 inan Exp $
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
#include <stdlib.h>
#include <string.h>
#include "mcal.h"

#define	TEST(text)	{printf("Testing [%s]... ", (text)); fflush(stdout);}
#define	PASS		printf("PASS\n");
#define	PASSALL		return 1;
//#define	FAIL		printf("FAIL\n"); 
#define	FAIL		{printf("FAIL\n"); return 0;}
#define	IGN		printf("IGNORED\n");

static int		test_datetime(void);
static int		test_parse_addr(void);
static int		test_cal(void);
static int		test_icap(void);
static int		test_mysql(void);


static const char*	login_user = "askalski@chek.com";

#include <time.h>


int
main(void)
{
	if (!test_mysql()) {
		printf("FAIL: test_mysql()\n");
		return 1;
	}
/*	
	if (!test_datetime()) {
		printf("FAIL: test_datetime()\n");
		return 1;
	}

	if (!test_parse_addr()) {
		printf("FAIL: test_parse_addr()\n");
		return 1;
	}

	if (!test_cal()) {
		printf("FAIL: test_cal()\n");
		return 1;
	}

	if (!test_icap()) {
		printf("FAIL: test_icap()\n");
		return 1;
	}
*/
	printf("All tests PASS.\n");

	return 0;
}

int
test_mysql(void)
{
	CALSTREAM		*stream;
	CALEVENT		*event;
	unsigned long		id;
	bool			good;
	datetime_t		when;
	



	TEST("cal_open");
	stream = cal_open(NULL, "{localhost/mysql}", 0);
	if (stream == NULL) FAIL;
	PASS;

	TEST("cal_ping");
	id = cal_ping( stream );
	if (id == false) FAIL;
	PASS;

	TEST("cal_create");
	id = cal_create( stream, "http" );
	if (id == false) FAIL;
	PASS;

	TEST("cal_append");
	event = calevent_new();
	dt_setdate(&event->start, 2000, APRIL, 1);
	dt_setdate(&event->end, 2000, APRIL, 22);
	dt_settime(&event->end, 12, 11, 10);
	event->category = strdup("Dinner");
	event->title = strdup("chicken");
	event->description = strdup("Is this working?");
	event->id = 1420; 
	event->recur_type = RECUR_WEEKLY;
        event->recur_data.weekly_wday = M_MONDAY+M_FRIDAY;
        event->recur_interval = 2;
        dt_setdate(&event->recur_enddate, 2001, MAY, 3);
        event->alarm = 1000;
	good = cal_append(stream, "<>", &id, event);
	if (!good) FAIL;
	printf("{Appended %lu}", event->id);
	id = event->id;
	calevent_free(event);
	PASS;
	
	TEST("cal_store");
	event = calevent_new();
	dt_setdate(&event->start, 2000, APRIL, 21);
	dt_setdate(&event->end, 2000, APRIL, 22);
	dt_settime(&event->end, 12, 11, 10);
	event->category = strdup("Dinner");
	event->title = strdup("chicken");
	event->description = strdup("Sure is!");
        event->id = id;
        event->recur_type = RECUR_DAILY;
        event->recur_interval = 1;
        dt_setdate(&event->recur_enddate, 2000, MAY, 3);
        event->alarm = 1000;
	good = cal_store(stream, event);
	if (!good) FAIL;
	printf("{Stored %lu}", event->id);
	id = event->id;
	calevent_free(event);
	PASS;

/*	
	TEST("cal_fetch");
	event = calevent_new();
	if (!cal_fetch(stream, 40, &event)) FAIL;
	printf("Event:%s", event->description);
	calevent_free(event);
	PASS;
*/
	TEST("cal_fetch");
	event = calevent_new();
	if (!cal_fetch(stream, id, &event)) FAIL;
	printf("Event:%s", event->description);
	calevent_free(event);
	PASS;


	TEST("cal_search_range");
	event = calevent_new();
	dt_setdate(&event->start, 2000, APRIL,21);
	dt_setdate(&event->end, 2000, APRIL, 21);
	if (!cal_search_range(stream, &(event->start), &(event->end))) FAIL;
	calevent_free(event);
	PASS;

	TEST("cal_snooze");
	event = calevent_new();
	if (!cal_fetch(stream, id, &event)) FAIL;
	printf("Event:%lu", event->alarm);
	if (!cal_snooze(stream, id)) FAIL;
	if (!cal_fetch(stream, id, &event)) FAIL;
	printf("Event:%lu", event->alarm);
	calevent_free(event);
	PASS;

	TEST("cal_search_alarm");
	dt_setdate(&when, 2000, APRIL, 20);
	dt_settime(&when, 0, 0, 0);
	if (!cal_search_alarm(stream, &when)) FAIL;
	PASS;

	TEST("cal_remove");
	if (!cal_remove(stream, id)) FAIL;
	PASS;

	TEST("cal_close");
	stream = cal_close(stream, 0);
	if (stream != NULL) FAIL;
	PASS;


	PASSALL;
}


int
test_datetime(void)
{
	datetime_t	a, b, c;


	TEST("isleapyear");
	if (isleapyear(1999)) FAIL;
	if (isleapyear(1900)) FAIL;
	if (!isleapyear(2000)) FAIL;
	if (!isleapyear(1996)) FAIL;
	PASS;

	TEST("daysinmonth");
	IGN;

	TEST("datevalid");
	IGN;

	TEST("timevalid");
	IGN;

	dt_init(&a);
	dt_init(&b);
	dt_init(&c);

	TEST("dt_now");
	if (!dt_now(&a)) FAIL;
	PASS;

	TEST("dt_hasdate");
	if (!dt_hasdate(&a)) FAIL;
	PASS;

	TEST("dt_cleardate");
	dt_cleardate(&a);
	if (dt_hasdate(&a)) FAIL;
	PASS;

	TEST("dt_hastime");
	if (!dt_hastime(&a)) FAIL;
	PASS;

	TEST("dt_cleartime");
	dt_cleartime(&a);
	if (dt_hastime(&a)) FAIL;
	PASS;

	TEST("dt_settm");
	IGN;

	TEST("dt_setdate");
	dt_setdate(&a, 1999, AUGUST, 11);
	if (a.year!=1999 || a.mon!=AUGUST || a.mday!=11) FAIL;
	PASS;

	TEST("dt_settime");
	dt_settime(&a, 18, 36, 7);
	if (a.hour!=18 || a.min!=36 || a.sec!=7) FAIL;
	PASS;

	TEST("dt_compare");
	if (dt_compare(&a, &b) <= 0) FAIL;
	dt_setdate(&b, 1999, SEPTEMBER, 25);
	if (dt_compare(&a, &b) >= 0) FAIL;
	dt_setdate(&b, a.year, a.mon, a.mday);
	if (dt_compare(&a, &b) <= 0) FAIL;
	dt_settime(&b, a.hour, a.min, a.sec);
	if (dt_compare(&a, &b) != 0) FAIL;
	dt_settime(&b, 23, 59, 59);
	if (dt_compare(&a, &b) >= 0) FAIL;
	PASS;

	TEST("dt_dayofyear");
	IGN;

	TEST("dt_dayofweek");
	if (dt_dayofweek(&a) != WEDNESDAY) FAIL;
	PASS;

	PASSALL;
}


int
test_parse_addr(void)
{
	CALADDR	*addr;

	TEST("caladdr_parse");
	addr = caladdr_parse(
		"{icap.chek.com/icap}<askalski!chek.com>INBOX");
	if (	!addr ||
		!addr->host || !addr->proto || !addr->user || !addr->folder ||
		strcmp(addr->host, "icap.chek.com") ||
		strcmp(addr->proto, "icap") ||
		strcmp(addr->user, "askalski!chek.com") ||
		strcmp(addr->folder, "INBOX"))
	{
		addr = caladdr_free(addr);
		FAIL;
	}
	addr = caladdr_free(addr);
	PASS;

	PASSALL;
}


int
test_cal(void)
{
	CALSTREAM		*stream;
	CALEVENT		*event;
	unsigned long		id;
	datetime_t		when;

	TEST("cal_valid");
	if (!cal_valid("{icap.chek.com/icap}")) FAIL;
	if (!cal_valid("{icap.chek.com}")) FAIL;
	PASS;

	TEST("cal_open");
	stream = cal_open(NULL, "{icap.chek.com/icap}", 0);
	if (stream == NULL) FAIL;
	PASS;

	TEST("cal_search_range");
	if (!cal_search_range(stream, NULL, NULL)) FAIL;
	PASS;

	TEST("cal_search_alarm");
	if (stream == NULL) FAIL;
	dt_setdate(&when, 1999, AUGUST, 5);
	dt_settime(&when, 14, 45, 0);
	if (!cal_search_alarm(stream, &when)) FAIL;
	PASS;


	TEST("cal_fetch");
	if (!cal_fetch(stream, 36, &event)) FAIL;
	event = calevent_free(event);
	PASS;

	TEST("cal_append");
	event = calevent_new();
	dt_setdate(&event->start, 1999, AUGUST, 16);
	dt_settime(&event->start, 12, 24, 0);
	dt_settime(&event->end, 13, 24, 0);
	event->category = strdup("FOOD");
	event->title = strdup("Lunch Hour");
	event->description = strdup("I am going Jim's.\n");
	event->alarm = 5;
	event->public = true;
	if (!cal_append(stream, "<>", &id, event)) {
		calevent_free(event);
		FAIL;
	}
	printf("{Appended %lu}", id);
	calevent_free(event);
	PASS;

	TEST("cal_remove");
	if (!cal_remove(stream, id)) FAIL;
	PASS;

	TEST("cal_open (other user)");
	if (!cal_open(stream, "<rwinston@chekinc.com>", 0)) FAIL;
	cal_search_range(stream, NULL, NULL);
	PASS;

	TEST("cal_close");
	stream = cal_close(stream, 0);
	if (stream != NULL) FAIL;
	PASS;

	PASSALL;
}


int
test_icap(void)
{
	CALSTREAM	*stream;
	CALEVENT	*event;
	unsigned long	id;
	bool		good;
	datetime_t	when;

	TEST("cal_open");
	stream = cal_open(NULL, "{icap.chek.com/icap}", 0);
	if (stream == NULL) FAIL;
	PASS;

	TEST("cal_open (recycle)");
	stream = cal_open(stream, "<rwinston@chekinc.com>", 0);
	if (stream == NULL) FAIL;
	PASS;

	TEST("cal_open (relogin)");
	login_user = "rwinston!chekinc.com";
	stream = cal_open(stream, "<>", CAL_LOGIN);
	if (stream == NULL) FAIL;
	PASS;

	login_user = "musone!chek.com";
	stream = cal_open(stream, "<>", CAL_LOGIN);
	if (stream == NULL) FAIL;

	TEST("cal_search_range");
	if (!cal_search_range(stream, NULL, NULL)) FAIL;
	PASS;

	TEST("cal_search_alarm");
	login_user = "rwinston@chekinc.com";
	stream = cal_open(stream, "<>", CAL_LOGIN);
	if (stream == NULL) FAIL;
	dt_setdate(&when, 1999, AUGUST, 12);
	dt_settime(&when, 6, 55, 0);
	if (!cal_search_alarm(stream, &when)) FAIL;
	PASS;

	TEST("cal_fetch");
	if (!cal_fetch(stream, 22, &event)) FAIL;
	event = calevent_free(event);
	PASS;

	TEST("cal_append");
	login_user = "askalski!chek.com";
	stream = cal_open(stream, "<>", CAL_LOGIN);
	if (stream == NULL) FAIL;
	event = calevent_new();
	dt_setdate(&event->start, 1999, AUGUST, 21);
	event->category = strdup("Dinner");
	event->title = strdup("hot steamy potatoes");
	event->description = strdup("the title says it all!");

	good = cal_append(stream, "INBOX", &id, event);
	calevent_free(event);
	if (!good) FAIL;
	printf("{Appended %lu}", id);
	PASS;

	TEST("cal_remove");
	if (!cal_remove(stream, id)) FAIL;
	PASS;

	TEST("cal_close");
	stream = cal_close(stream, 0);
	PASS;

	PASSALL;
}


void
cc_login(const char **username, const char **password)
{
	printf("{Login}");
	*username = "http";
	*password = "spj904";
}


void
cc_searched(unsigned long id)
{
	fprintf(stderr, "{Searched %lu}", id);
}


void
cc_vlog(const char *fmt, va_list ap)
{
	fprintf(stderr, "log: ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
}


void
cc_vdlog(const char *fmt, va_list ap)
{
	fprintf(stderr, "debug: ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
}
