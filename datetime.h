/*
 *	$Id: datetime.h,v 1.3 2000/05/11 19:43:23 inan Exp $
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

#ifndef	_DATETIME_H
#define	_DATETIME_H

#include <time.h>
#include "bool.h"

/**
 * Represents the day of the week.
 */
typedef enum {
	SUNDAY,
	MONDAY,
	TUESDAY,
	WEDNESDAY,
	THURSDAY,
	FRIDAY,
	SATURDAY
} weekday_t;

/**
 * Represents the month of the year.
 */
typedef enum {
	JANUARY = 1,
	FEBRUARY,
	MARCH,
	APRIL,
	MAY,
	JUNE,
	JULY,
	AUGUST,
	SEPTEMBER,
	OCTOBER,
	NOVEMBER,
	DECEMBER
} month_t;

typedef enum {
	DT_FORWARD,
	DT_BACKWARD
} direction_t;

/**
 * struct datetime
 *
 * Used to represent and manipulate dates in the proleptic
 * Gregorian calendar.
 *
 */
typedef struct datetime
{
	bool		has_date;	/* has a date value */
	int		year;		/* year */
	int		mon;		/* month */
	int		mday;		/* day of month */

	bool		has_time;	/* has a time value */
	int		hour;		/* hour */
	int		min;		/* minute */
	int		sec;		/* second */
} datetime_t;


#define	DOE_MIN		1		/* January 1, 1 */
#define	DOE_MAX		3652059		/* December 31, 9999 */
#define	YEAR_MIN	1
#define	YEAR_MAX	9999

#define	DT_INIT		{ false, 0, 0, 0, false, 0, 0, 0 }



/** general date/time functions **/

/* returns true if <year> is a leap year */
bool	isleapyear(int year);

/* returns days in <month>; if <leap> is true, gives result for leap year */
int	daysinmonth(int month, bool leap);

/* returns true if the date is valid */
bool	datevalid(int year, int mon, int mday);

/* returns true if the time is valid */
bool	timevalid(int hour, int min, int sec);

/*********************************/

/** datetime_t related functions **/

/* initializes <dt> with no date or time */
void	dt_init(datetime_t *dt);

/* fills the current date/time into <dt> */
bool	dt_now(datetime_t *dt);

/* clears the date portion of <dt> */
void	dt_cleardate(datetime_t *dt);

/* clears the time portion of <dt> */
void	dt_cleartime(datetime_t *dt);

/* returns true if <dt> has a date */
bool	dt_hasdate(const datetime_t *dt);

/* returns true if <dt> has a time */
bool	dt_hastime(const datetime_t *dt);

/* returns true if <dt> has no value */
bool	dt_empty(const datetime_t *dt);

/* sets <dt> from the fields of <tm> */
bool	dt_settm(datetime_t *dt, const struct tm *tm);

/* sets the date portion of <dt> */
bool	dt_setdate(datetime_t *dt, int year, int mon, int mday);

/* sets the time portion of <dt> */
bool	dt_settime(datetime_t *dt, int hour, int min, int sec);

/* returns <0, 0, >0 if a<b, a==b, a>b respectively */
int	dt_compare(const datetime_t *a, const datetime_t *b);

/* returns the day-of-year of <dt> */
int	dt_dayofyear(const datetime_t *dt);

/* returns the day-of-week of <dt> */
int	dt_dayofweek(const datetime_t *dt);

/* returns the day-of-epoch (1 is January 1, 1).  this format has the
 * property that DOW==DOE%7
 */
int	dt_dayofepoch(const datetime_t *dt);

/* sets the datetime from day-of-epoch */
bool	dt_setdoe(datetime_t *dt, int doe);

/* rolls the time by <hour> <min> and <sec>, spilling over into the
 * date if it exists
 */
bool	dt_roll_time(datetime_t *dt, int hour, int min, int sec);

/* sets <dt> to be the <wday> in the same week as <ref>, if <weekstart>
 * is the first day of the week.
 */
bool	dt_setweekof(	datetime_t *dt, const datetime_t *ref,
			weekday_t weekstart, weekday_t wday);

/* sets the datetime from year, month, week and day */
bool	dt_setnthwday(datetime_t *dt, int year, int mon, int nth, int wday);

/* Returns the week number for d=day, m=month, y=year */
int 	dt_weekofyear(int d, int m, int y);

/* Returns the week number in the given month counting in the given direction */
int 	dt_orderofmonth( const datetime_t *dt, const direction_t direction);

/* convenience macros to get the first/last days of a week */
#define	dt_startofweek(dt, ref, weekstart) \
		dt_setweekof((dt), (ref), (weekstart), (weekstart))
#define	dt_endofweek(dt, ref, weekstart) \
		dt_setweekof((dt), (ref), (weekstart), (7+(weekstart)-1)%7)
#define	dt_dayofweekstr(dayno)        \
		(dayno) >= 4 ?        \
			(dayno) = 6 ?     \
				"SA" :        \
				(dayno) = 5 ? \
					"FR" :    \
					"TH"      \
				;             \
			;:                \
			(dayno) >= 2 ?    \
				(dayno) = 3 ? \
					"WE" :    \
					"TU"      \
				;:             \
				(dayno) = 1 ? \
					"MO" :    \
					"SU"      \
				;             \
			;                 \
				

/**********************************/

#endif
