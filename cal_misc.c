#include <stdlib.h>
#include "cal_misc.h"

bool
cal_decode_dt(datetime_t *dt, const char *s)
{
        unsigned long   n;
        char            *endp;

        dt_init(dt);

        if (*s != 'T' && *s != 't') {
                n = strtoul(s, &endp, 10);
                if (endp - s != 8)
                        return false;
                if (!dt_setdate(dt, n/10000, (n/100)%100, n%100))
                        return false;
                if (*endp == '\0')
                        return true;
                s = endp;
        }

        if (*s == 'T' || *s == 't') {
                n = strtoul(++s, &endp, 10);
                if (endp - s != 6)
                        return false;
                if (!dt_settime(dt, n/10000, (n/100)%100, n%100))
                        return false;
                if (*endp && *endp != 'Z' && *endp != 'z')
                        return false;
        }
        else {
                return false;
        }

        return true;
}

unsigned char*
cal_decode_base64(unsigned char *buf, size_t *size)
{
        size_t          left;
        unsigned char   *in;
        unsigned char   *out;
        int             bsize;
        int             i;

        if (*size % 4)
                return NULL;

        left = *size;
        *size = 0;
        for (in=buf,out=buf; left>0; in+=4,left-=4) {
                if (in[0]=='=' || in[1]=='=')
                        return NULL;
                if (in[2]=='=' && in[3]!='=')
                        return NULL;
                if (in[3]=='=' && left>4)
                        return NULL;

                bsize = 3;
                for (i=0; i<4; i++) {
		  switch (in[i]) {
		  case '=':
                                in[i] = 0;
                                bsize--;
                                break;
		  case '+':
                                in[i] = 62; break;
		  case '/':
                                in[i] = 63; break;
		  default:
                                if (in[i]>='A' && in[i]<='Z')
                                        in[i] -= 'A';
                                else if (in[i]>='a' && in[i]<='z')
                                        in[i] -= 'a' - 26;
                                else if (in[i]>='0' && in[i]<='9')
                                        in[i] -= '0' - 52;
                                else
                                        return NULL;
		  }
                }

                out[0] = (0xfc&in[0]<<2) | (0x03&in[1]>>4);
                if (bsize>0)
                        out[1] = (0xf0&in[1]<<4) | (0x0f&in[2]>>2);
                if (bsize>1)
                        out[2] = (0xc0&in[2]<<6) | (0x3f&in[3]);

                out += bsize;
        }

        *out = '\0';
        *size = out - buf;

        return buf;
}
