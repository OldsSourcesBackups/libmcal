/*

	$Id: mcal.c,v 1.1 1999/12/02 08:02:04 zircote Exp $

    mcal - libmcal powered cal replacement
    Copyright (C) 1999 Mark Musone and Andrew Skalski

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


Contact Information:

Mark Musone
musone@chek.com

Andrew Skalski
skalski@chek.com

Mcal mailing list:
mcal@lists.chek.com

*/

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>

#include "mcal.h"

#define REVON "\033[7m"
#define REVOFF "\033[27m"
#define MCAL_VER "0.2"

CALSTREAM *stream;
int events[13][32];
unsigned long *eventlist=NULL;
int eventlistcapacity=0;
int eventlistsize=0;
int prompt_user=0;
int prompt_password=1;
char *domain=NULL;
int store_event=0;

int main(int argc, char **argv)
{
  char *inputdate=NULL;
  int year;
  int month;
  int day;
  int c;
 // int digit_optind = 0;
  char *foldername;
  datetime_t startdate,enddate;
  CALEVENT *event;
  const char *months[]={NULL,"January","February","March","April","May","June","July","August","September","October","November","December"};

  domain=getenv("ICAP_DOMAIN");
  bzero(events,31*13*sizeof(**events));

  while (1)
    {
      int option_index = 0;
      static struct option long_options[] =
      {
	{"folder", 1, 0, 'f'},
	{"user", 0, 0, 'u'},
	{"password", 0, 0, 'p'},
	{"store", 0, 0, 's'},
	{"version", 0, 0, 'v'},
	{"help", 0, 0, 'h'},
	{0, 0, 0, 0}
      };

      c = getopt_long (argc, argv, "f:upsvh",
		       long_options, &option_index);
      if (c == -1)
	break;

      switch (c)
       {
       case 'f':
	 foldername=strdup(optarg);
	 break;
       case 'u':
	 prompt_user=1;
	 break;
       case 'p':
	 prompt_password=0;
	 break;
       case 's':
	 store_event=1;
	 break;
	   case 'v':
	 printf("MCAL Version %s\n",MCAL_VER);
	 printf("\tLicensed Under GPL\n");
	 printf("\thttp://mcal.chek.com\n");
	 printf("\tMark Musone musone@edgeglobal.com\n\n");
 	 exit(0);
	 break;
       case 'h':
	 printf("Usage: %s [-f foldername] [-u] [-p] [-s] [-v] [-h] YYYY[MM[DD]]\n\n",argv[0]);
	 printf("\t-f foldername\n");
	 printf("\tuse calendar from specified folder or server.\n");
	 printf("\tfolder names can be standard calendar folders:\n");
	 printf("\t{servername/protocol}foldername\n\n");
	 printf("\t-u prompt for a username. uses current username if flag not set\n");
	 printf("\t-p dont prompt for a password. sends a blank password if flag set\n");
	 printf("\t-s store event in the given YYYYMMDD\n");
	 printf("\t-v print version information\n");
	 printf("Uses environment variable ICAP_DOMAIN and appends that with an @ to the username\n");
	 exit(0);
	 break;
       }
   }

  if (optind == argc)
    {
      dt_now(&startdate);
      inputdate="123456";
    }
  else if(optind+1 == argc)
    {
      inputdate=argv[optind];
    }
  else if(optind+2 == argc)
    {

    }
 if (strlen(inputdate) == 4 )
   {
     int i,days,dayofweek[13]={0,0,0,0,0,0,0,0,0,0,0,0,0},startday/*,x,y*/;
    // int column=0;
     year=atoi(inputdate);
     stream=cal_open(NULL,foldername,0);
     if(!stream)
       {
	 prompt_password=1;
	 stream=cal_open(NULL,foldername,0);
       }

     for(month=1;month<=12;month++)
       {

	 bzero(events[month],31*sizeof(**events));
	 dt_init(&startdate);
	 dt_init(&enddate);
	 dt_setdate(&startdate, year, month, 1);
	 dt_setdate(&enddate, year, month, daysinmonth(month,isleapyear(year)));
	 cal_search_range(stream,&startdate,&enddate);
	 for(i=0;i<eventlistsize;i++)
	   {
	     cal_fetch(stream,eventlist[i],&event);
	 if(event->recur_type!=RECUR_NONE)
	   {
	   	datetime_t clamp = startdate;
	     calevent_next_recurrence(event,&clamp,SUNDAY);
	     while(dt_hasdate(&clamp) && clamp.year == year && clamp.mon == month)
	       {
		 events[month][clamp.mday] = 1;
		 clamp.mday++;
		 if(clamp.mday > enddate.mday)
		   break;
		 calevent_next_recurrence(event,&clamp,SUNDAY);
	       }
	   }
	 else {
		 events[month][event->start.mday]=1;
	 }
	     calevent_free(event);
	   }
	 free(eventlist);
	 eventlistcapacity=0;
	 eventlistsize=0;
	 eventlist=NULL;
       }
     for(month=1;month<=12;month++)
       {
	 dt_setdate(&startdate, year, month, 1);
	 startday=dt_dayofweek(&startdate);
	 days=daysinmonth(month,isleapyear(year));
	 printf("     %s %d\t\n",months[month],year);
	 printf("Su Mo Tu We Th Fr Sa");
	 printf("\n");
	 for(i=0;i<startday;i++)
	   printf("   ");
	 dayofweek[0]=startday;
	 for(i=1;i<=days;i++)
	   {
	     if(dayofweek[0] % 7 ==0 && dayofweek[0] !=0)
	       printf("\n");
	     if(events[month][i]==1)
	       {
		 printf(REVON "%2d" REVOFF " ",i);
	       }
	     else
	       printf("%2d ",i);
	     dayofweek[0]++;
	   }
	 printf("\n");
       }
     /*
     month=1;
     for(y=1;y<=4;y++)
       {
	 for(x=1;x<=3;x++)
	   {
	 printf("\t\t%s",months[x*y]);
	   }
	 printf("\n");
	 for(x=1;x<3;x++)
	   {
	     printf("Su Mo Tu We Th Fr Sa\t");
	   }
	 printf("\n");
	 for(x=1;x<3;x++)
	   {
	     for(i=0;i<startday;i++)
	       printf("   ");
	     printf("\t");
	   }
	 printf("\n");
       }
     */
  }
 else if (strlen(inputdate) == 6)
   {
     int i,days,dayofweek=0,startday;

     stream=cal_open(NULL,foldername,0);
     if(!stream)
       {
	 prompt_password=1;
	 stream=cal_open(NULL,foldername,0);
       }
  if (optind == argc)
       {
	 dt_init(&startdate);
	 dt_now(&startdate);
	 month=startdate.mon;
	 year=startdate.year;
	 startdate.mday=1;
	 }
  else
    {
      month=atoi(&inputdate[4]);
      inputdate[4]=0x00;
      year=atoi(inputdate);
      dt_init(&startdate);
      dt_setdate(&startdate, year, month, 1);
    }
  dt_init(&enddate);
  dt_setdate(&enddate, year, month, daysinmonth(month,isleapyear(year)));
  startday=dt_dayofweek(&startdate);
     cal_search_range(stream,&startdate,&enddate);
     for(i=0;i<eventlistsize;i++)
       {
	 cal_fetch(stream,eventlist[i],&event);
	 printf("uid: %d recur_type: %d\n",eventlist[i],event->recur_type);
	 if(event->recur_type!=RECUR_NONE)
	   {
	   	datetime_t clamp = startdate;
	     calevent_next_recurrence(event,&clamp,SUNDAY);
	     while(dt_hasdate(&clamp) && clamp.year == year && clamp.mon == month)
	       {
		 events[month][clamp.mday] = 1;
		 clamp.mday++;
		 if(clamp.mday > enddate.mday)
		   break;
		 calevent_next_recurrence(event,&clamp,SUNDAY);
	       }
	   }
	 else {
		 events[month][event->start.mday]=1;
	 }
	 calevent_free(event);
       }
     free(eventlist);
     eventlistcapacity=0;
     eventlistsize=0;
     eventlist=NULL;

     days=daysinmonth(month,isleapyear(year));
     printf("     %s %d\t\n",months[month],year);
     printf("Su Mo Tu We Th Fr Sa");
     printf("\n");
     for(i=0;i<startday;i++)
       printf("   ");
     dayofweek=startday;
     for(i=1;i<=days;i++)
       {
	 if(dayofweek % 7 ==0 && dayofweek !=0)
	   printf("\n");
if(events[month][i]==1)
	 printf(REVON "%2d" REVOFF " ",i);
else
	 printf("%2d ",i);
	 dayofweek++;
       }
     printf("\n");
   }
 else if (strlen(inputdate) == 8 && store_event == 0)
   {
     int i;
     stream=cal_open(NULL,foldername,0);
     if(!stream)
       {
	 prompt_password=1;
	 stream=cal_open(NULL,foldername,0);
       }
     if(!stream) printf("Arghh!!! couldnt log in! \n");
     day=atoi(&inputdate[6]);
     inputdate[6]=0x00;
     month=atoi(&inputdate[4]);
     inputdate[4]=0x00;
     year=atoi(inputdate);
     printf("     %s %2d %d\n",months[month],day,year);
     dt_init(&startdate);
     dt_init(&enddate);


     dt_setdate(&startdate, year, month, day);
     dt_setdate(&enddate, year, month, day);

     cal_search_range(stream,&startdate,&enddate);
     for(i=0;i<eventlistsize;i++)
       {
	 printf("\n---------------------------------------------\n");
	 cal_fetch(stream,eventlist[i],&event);
	 printf("Title: %s\n",event->title);
	 printf("Category: %s\n",event->category);
	 printf("Description: %s\n",event->description);
	 printf("Public: %s\n",event->public ? "Yes" : "No" );
	 printf("Start: %s %d %d - %d:%02d:%02d\n",months[event->start.mon],event->start.mday,event->start.year,event->start.hour,event->start.min,event->start.sec);
	 calevent_free(event);
       }
     free(eventlist);
     eventlistcapacity=0;
     eventlistsize=0;
     eventlist=NULL;
 cal_close(stream,0);
   }
 else if (strlen(inputdate) == 8 && store_event == 1)
   {
     unsigned long uid;
     int i;
     char title[1000];
     char category[1000];
     FILE *myfile;
     char *ptr;
     char *buffer;
     store_event=0;

     stream=cal_open(NULL,foldername,0);
     event=calevent_new();
     if(!stream)
       {
	 prompt_password=1;
	 stream=cal_open(NULL,foldername,0);
       }
     if(!stream) printf("Arghh!!! couldnt log in! \n");
     day=atoi(&inputdate[6]);
     inputdate[6]=0x00;
     month=atoi(&inputdate[4]);
     inputdate[4]=0x00;
     year=atoi(inputdate);

     dt_setdate(&event->start, year, month, day);
     dt_setdate(&event->end, year, month, day);
     printf("Title:\n");
     fgets(title,999,stdin);
     ptr=strchr(title,'\n');
     if(ptr) *ptr=0x00;
     printf("Category:\n");
     fgets(category,999,stdin);
     ptr=strchr(category,'\n');
     if(ptr) *ptr=0x00;
     event->title=title;
     event->category=category;


     printf("Description: (hit ^d to end)\n");
     myfile=tmpfile();
     while((i=getc(stdin))!=EOF)
       {
	 putc(i,myfile);
       }
     i=ftell(myfile);
     buffer=malloc(i+1);
     rewind(myfile);
     fread(buffer,i,1,myfile);
     fclose(myfile);
     buffer[i]=0x00;
     event->description=buffer;
     cal_append(stream,"INBOX",&uid,event);
     event->title=NULL;
     event->category=NULL;
     calevent_free(event);
     cal_close(stream,0);

   }
}

void
cc_login(const char **username, const char **password)
{
  static char myuser[80];
  static char *mypass;
  char *ptr;
  struct passwd *myent;
  if(prompt_user==0)
    {
      myent=getpwuid(getuid());
      if(domain)
	{
	  sprintf(myuser,"%s@%s",myent->pw_name,domain);
	  *username=myuser;
	}
      else *username=myent->pw_name;

    }
  else
    {
        printf("Username: ");
	fgets(myuser,80,stdin);
	*username=myuser;
	ptr=strchr(myuser,'\n');
	if(ptr) *ptr=0x00;
	else printf("max of 80 chars!\n");
    }
if(prompt_password)
{
  mypass=getpass("Password:");
  *password=mypass;
}
else *password="";
}

void
cc_searched(unsigned long id)
{
  if(eventlistsize == eventlistcapacity)
    {
      if(eventlistcapacity==0)
	{
	  eventlistcapacity=8;
	}
      eventlistcapacity <<=1;
    }
  eventlist=realloc(eventlist,eventlistcapacity*sizeof(*eventlist));
  eventlist[eventlistsize++]=id;
}


void
cc_vlog(const char *fmt, va_list ap)
{

}


void
cc_vdlog(const char *fmt, va_list ap)
{

}
