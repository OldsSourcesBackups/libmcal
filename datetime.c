/*
 *	$Id: datetime.c,v 1.2 2000/03/11 02:14:42 chuck Exp $
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

#include <time.h>
#include <string.h>
#include "datetime.h"


static const int doylookup[2][13] = {
	{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
	{ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }};


bool
isleapyear(int year)
{
	/* Leap year every 4th year, except every 100th year,
	 * not including every 400th year.
	 */
	return !(year % 4) && ((year % 100) || !(year % 400));
}


int
daysinmonth(int month, bool leap)
{
	/* Validate the month. */
	if (month < JANUARY || month > DECEMBER)
		return -1;

	/* Return 28, 29, 30, or 31 based on month/leap. */
	switch (month) {
	case FEBRUARY:
		return leap ? 29 : 28;
	case APRIL:
	case JUNE:
	case SEPTEMBER:
	case NOVEMBER:
		return 30;
	default:
		return 31;
	}
}


bool
datevalid(int year, int mon, int mday)
{
	/* Years may be between YEAR_MIN-YEAR_MAX; months JANUARY-DECEMBER,
	 * and days must be validated based on year and month.
	 */
	if (year < YEAR_MIN || year > YEAR_MAX)
		return false;
	if (mon < JANUARY || mon > DECEMBER)
		return false;
	if (mday < 1 || mday > daysinmonth(mon, isleapyear(year)))
		return false;
	return true;
}


bool
timevalid(int hour, int min, int sec)
{
	/* Hours must be 0-23, minutes 0-59, seconds 0-59. */
	if (hour < 0 || hour > 23)
		return false;
	if (min < 0 || min > 59)
		return false;
	if (sec < 0 || sec > 59)
		return false;
	return true;
}


void
dt_init(datetime_t *dt)
{
	/* Clear out the date and time fields, zero everything else. */
	memset(dt, 0, sizeof(*dt));
	dt->has_date = false;
	dt->has_time = false;
}


#define	LT		-1
#define	EQ		0
#define	GT		1
#define	CMP(field)	if (a->field < b->field) return LT; \
			if (a->field > b->field) return GT;

int
dt_compare(const datetime_t *a, const datetime_t *b)
{
	/* Fields have the precedence:
	 * year > month > mday > hour > minute > second
	 * Missing field < present field
	 * missing field == missing field
	 * Returns (-1 if a<b) (0 if a==b) (1 if a>b)
	 */
	if (a->has_date) {
		if (!b->has_date)
			return GT;
		CMP(year);
		CMP(mon);
		CMP(mday);
	}
	else if (b->has_date)
		return LT;

	if (a->has_time) {
		if (!b->has_time)
			return GT;
		CMP(hour);
		CMP(min);
		CMP(sec);
	}
	else if (b->has_time)
		return LT;

	return EQ;
}

#undef	LT
#undef	EQ
#undef	GT
#undef	CMP


bool
dt_now(datetime_t *dt)
{
	struct tm	*tm;
	time_t		t;

	/* Fetch the system time and break it down into GMT fields. */
	if (	(t = time(NULL)) == -1 ||
		(tm = gmtime(&t)) == NULL)
	{
		return false;
	}

	return dt_settm(dt, tm);
}


bool
dt_settm(datetime_t *dt, const struct tm *tm)
{
	/* Validate the date and time. */
	if (!datevalid(tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday))
		return false;
	if (!timevalid(tm->tm_hour, tm->tm_min, tm->tm_sec))
		return false;

	/* Set the fields. */
	dt->has_date = true;
	dt->year = tm->tm_year + 1900;
	dt->mon = tm->tm_mon + 1;
	dt->mday = tm->tm_mday;
	dt->has_time = true;
	dt->hour = tm->tm_hour;
	dt->min = tm->tm_min;
	dt->sec = tm->tm_sec;

	return true;
}


bool
dt_hasdate(const datetime_t *dt)
{
	return dt->has_date;
}


void
dt_cleardate(datetime_t *dt)
{
	dt->has_date = false;
}


bool
dt_setdate(datetime_t *dt, int year, int mon, int mday)
{
	/* Validate the fields. */
	if (!datevalid(year, mon, mday))
		return false;

	/* Set the date fields. */
	dt->has_date = true;
	dt->year = year;
	dt->mon = mon;
	dt->mday = mday;

	return true;
}


bool
dt_hastime(const datetime_t *dt)
{
	return dt->has_time;
}


void
dt_cleartime(datetime_t *dt)
{
	dt->has_time = false;
}


bool
dt_settime(datetime_t *dt, int hour, int min, int sec)
{
	/* Validate the fields. */
	if (!timevalid(hour, min, sec))
		return false;

	/* Set the time fields. */
	dt->has_time = true;
	dt->hour = hour;
	dt->min = min;
	dt->sec = sec;

	return true;
}


bool
dt_empty(const datetime_t *dt)
{
	return !dt->has_date && !dt->has_time;
}


int
dt_dayofyear(const datetime_t *dt)
{
	/* Check for invalidated year field. */
	if (!dt->has_date)
		return -1;
	/* Return day of year. */
	return dt->mday + doylookup[isleapyear(dt->year) ? 1 : 0][dt->mon - 1];
}


int
dt_dayofweek(const datetime_t *dt)
{
	return dt_dayofepoch(dt) % 7;
}


int
dt_dayofepoch(const datetime_t *dt)
{
	int		doe;
	int		era, cent, quad, rest;

	if (!dt->has_date)
		return -1;

	/* break down the year into 400, 100, 4, and 1 year multiples */
	rest = dt->year - 1;
	quad = rest / 4;	rest %= 4;
	cent = quad / 25;	quad %= 25;
	era = cent / 4;		cent %= 4;

	/* set up doe */
	doe = dt_dayofyear(dt);
	doe += era * (400 * 365 + 97);
	doe += cent * (100 * 365 + 24);
	doe += quad * (4 * 365 + 1);
	doe += rest * 365;

	return doe;
}


bool
dt_setdoe(datetime_t *dt, int doe)
{
	int		leap;
	int		year;
	int		mon;
	int		mday;
	bool		dec31;

	/* bounds check */
	if (doe < DOE_MIN || doe > DOE_MAX)
		return false;

	/* make sure day 366 of leap year does not roll over */
	dec31 = (dt->mon == DECEMBER && dt->mday == 31);


	/* be careful with this code.. any small modification may
	 * introduce problems.  it is highly recommended to test
	 * for every single date from 1-1-1 to 9999-12-31
	 */
	doe--;
	if (dec31) doe--;
	year = 400 * (doe / (400 * 365 + 97));	doe %= (400 * 365 + 97);
	year += 100 * (doe / (100 * 365 + 24));	doe %= (100 * 365 + 24);
	year += 4 * (doe / (4 * 365 + 1));	doe %= (4 * 365 + 1);
	year += (doe / 365) + 1;		doe %= 365;
	if (dec31) doe++;

	leap = isleapyear(year) ? 1 : 0;

	mon = doe / 31;
	if (doylookup[leap][mon+1] <= doe)
		mon++;
	mday = doe - doylookup[leap][mon] + 1;

	return dt_setdate(dt, year, mon+1, mday);
}


bool
dt_roll_time(datetime_t *dt, int hour, int min, int sec)
{
	int		spillage;
	int		doe;

	if (!dt->has_time)
		return false;

	sec += dt->sec;
	min += dt->min + (sec - (sec < 0 ? 59 : 0)) / 60;
	hour += dt->hour + (min - (min < 0 ? 59 : 0)) / 60;
	spillage = (hour - (hour < 0 ? 23 : 0)) / 24;

	if ((sec %= 60) < 0) sec += 60;
	if ((min %= 60) < 0) min += 60;
	if ((hour %= 24) < 0) hour += 24;

	if (dt->has_date) {
		doe = dt_dayofepoch(dt);
		spillage += doe;
		if (spillage < DOE_MIN || spillage > DOE_MAX)
			return false;
		if (!dt_setdoe(dt, spillage))
			return false;
	}

	if (!dt_settime(dt, hour, min, sec)) {
		if (dt->has_date)
			dt_setdoe(dt, doe);
		return false;
	}

	return true;
}


bool
dt_setweekof(	datetime_t *dt, const datetime_t *ref,
		weekday_t weekstart, weekday_t wday)
{
	int		doe;
	int		n_base;
	int		n_want;

	doe = dt_dayofepoch(ref);
	if (doe == -1)
		return false;

	/* monkey with the days so they are in proper week-order
	 * based on <weekstart> as the first day in the week
	 */
	if ((n_base = doe % 7) < weekstart)
		n_base += 7;
	if ((n_want = wday) < weekstart)
		n_want += 7;

	/* adjust doe by the offset */
	doe += n_want - n_base;

	/* bounds check */
	if (doe < DOE_MIN || doe > DOE_MAX)
		return false;

	return dt_setdoe(dt, doe);
}


bool
dt_setnthwday(datetime_t *dt, int year, int mon, int nth, int wday)
{
	weekday_t	first;
	datetime_t	tmp = DT_INIT;


	if (wday < SUNDAY || wday > SATURDAY)
		return false;

	if (!dt_setdate(&tmp, year, mon, 1))
		return false;
	first = dt_dayofweek(&tmp);

	if (wday < first)
		tmp.mday = 7 + wday - first;
	else
		tmp.mday = wday - first;
	tmp.mday += 7 * nth - 6;

	return dt_setdate(dt, year, mon, tmp.mday);
}

int
julian(int d, int m, int y)
{
	int n1, n2;
	n1 = 12 * y + m - 3;
	n2 = n1/12;
	return (734 * n1 + 15)/24 - 2 * n2 + n2/4 - n2/100 + n2/400 + d + 1721119;
}

int 
dt_weekofyear(int d, int m, int y)
{
	int n1, n2, w;
	n1 = julian(d, m, y);
	n2 = 7 * (n1/7) + 10;
	y++;
	while ((w = (n2 - julian(1, 1, y))/7) <= 0) {
		y--;
	}
	return w;
}
