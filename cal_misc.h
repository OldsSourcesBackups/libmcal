#ifndef _CALMISC_H
#define _CALMISC_H

#include "mcal.h"

/* Parse routines. */
unsigned char*  cal_decode_base64(unsigned char *buf, size_t *size);
bool            cal_decode_dt(datetime_t *dt, const char *s);

#endif
