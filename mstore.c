/* $Id: mstore.c,v 1.8 2000/01/28 01:35:50 markie Exp $ */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pwd.h>
#include <unistd.h>
#include <crypt.h>
#include "mcal.h"
#include "mstore.h"
#include "icap/icaproutines.h"


#define MSTORE_VER "0.4"
#define	DATA_T	struct mstore_data
#define	DATA	((DATA_T*) stream->data)



static void		mstore_freestream(CALSTREAM *stream);
static bool		mstore_validuser(const char *username,const char *password);
static bool		mstore_userexists(const char *username);

static bool		mstore_valid(const CALADDR *addr);
static CALSTREAM*	mstore_open(	CALSTREAM *stream,
					const CALADDR *addr, long options);
static CALSTREAM*	mstore_close(CALSTREAM *stream, long options);
static bool		mstore_ping(CALSTREAM *stream);
static bool		mstore_search_range(	CALSTREAM *stream,
						const datetime_t *start,
						const datetime_t *end);
static bool		mstore_search_alarm(	CALSTREAM *stream,
						const datetime_t *when);
static bool		mstore_fetch(	CALSTREAM *stream,
						unsigned long id,
						CALEVENT **event);
static bool		mstore_append(	CALSTREAM *stream,
						const CALADDR *addr,
						unsigned long *id,
						const CALEVENT *event);
static bool		mstore_remove(	CALSTREAM *stream,
						unsigned long id);
static bool		mstore_snooze(	CALSTREAM *stream,
					unsigned long id);
static bool             mstore_store(   CALSTREAM *stream, 
					const CALEVENT *modified_event);
CALDRIVER mstore_driver =
{
	mstore_valid,
	mstore_open,
	mstore_close,
	mstore_ping,
	mstore_search_range,
	mstore_search_alarm,
	mstore_fetch,
	mstore_append,
	mstore_remove,
	mstore_snooze,
	mstore_store,
};



DATA_T {
	char		*host;
	char		*login_userbuf;
	const char	*login_user;
	char		*folder_userbuf;
	const char	*folder_user;
	char		*folder;
	char *base_path;
};


void
mstore_freestream(CALSTREAM *stream)
{
	if (stream) {
		if (DATA) {
			free(DATA->login_userbuf);
			free(DATA->folder_userbuf);
			free(DATA->folder);
			free(DATA->host);
			free(DATA);
		}
		free(stream);
	}
}


bool
mstore_validuser(const char *username,const char *password)
{
  FILE *mpasswd;
  char line[1000];
  char *musername,*mpassword;
  mpasswd=fopen("/etc/mpasswd","r");
  if(!mpasswd)
    {
      printf("Error! couldn't open mpasswd file!\n");
      exit(1);
    }
  while(fgets(line,900,mpasswd))
    {
      if(line[strlen(line)-1]=='\n') line[strlen(line)-1]=0x00;
      musername=line;
      mpassword=strchr(line,':');
      *mpassword=0x00;
      mpassword++;
      if(!strcmp(username,musername))
	{
	  if(!strcmp(crypt(password,mpassword),mpassword))
	{
	  fclose(mpasswd);
	  return true;
	}
	  else
	    {
	      fclose(mpasswd);
	      return false;
	    }
	}
    }
	fclose(mpasswd);
      return false;
}


bool
mstore_userexists(const char *username)
{
  FILE *mpasswd;
  char line[1000];
  char *musername,*mpassword;
  mpasswd=fopen("/etc/mpasswd","r");
  if(!mpasswd)
    {
      printf("Error! couldn't open mpasswd file!\n");
      exit(1);
    }
  while(fgets(line,900,mpasswd))
    {
      if(line[strlen(line)-1]=='\n') line[strlen(line)-1]=0x00;
      musername=line;
      mpassword=strchr(line,':');
      *mpassword=0x00;
      mpassword++;
      if(!strcmp(username,musername))
	{
	      fclose(mpasswd);
	      return true;
	}
    }
	fclose(mpasswd);
      return false;
}


bool
mstore_valid(const CALADDR *addr)
{
	if (!addr->proto || strcasecmp(addr->proto, "mstore"))
		return false;
	if (addr->user && !mstore_userexists(addr->user))
		return false;
	return true;
}


/* ASSERT: this function may not be given an invalid <addr> */
CALSTREAM*
mstore_open(CALSTREAM *stream, const CALADDR *addr, long options)
{
	const char	*username = NULL;
	const char	*password = NULL;
	bool		reopen = false;


	/* Try to reuse old stream. */
	if (stream) {
		reopen = true;

		free(DATA->folder);
		DATA->folder = NULL;
		free(DATA->folder_userbuf);
		DATA->folder_userbuf = NULL;
		DATA->folder_user = NULL;
	}
	else {
		options |= CAL_LOGIN;
	}

	if (options & CAL_LOGIN) {
		if (stream) {
			free(DATA->login_userbuf);
			DATA->login_user = NULL;
	}

		cc_login(&username, &password);
		if (username == NULL)
			goto fail;
		if (!mstore_validuser(username,password))
			goto fail;
	}

	if (!reopen) {
		if ((stream = calloc(1, sizeof(*stream))) == NULL)
			goto fail;
		if ((DATA = calloc(1, sizeof(*DATA))) == NULL)
			goto fail;
	}

	if (options & CAL_LOGIN) {
		/* Copy login_userbuf, folder. */
		if ((DATA->login_userbuf = strdup(username)) == NULL)
			goto fail;

	}

	if ((DATA->folder = strdup(addr->folder)) == NULL)
		goto fail;

	/* Set up folder_user */
	if(addr->host)
	  DATA->base_path=strdup(addr->host);
	else
	  DATA->base_path=strdup("/var/calendar");

		DATA->login_user=DATA->login_userbuf;
		if (addr->user) {
		  /* Copy and split folder_userbuf */
		  if ((DATA->folder_userbuf = strdup(addr->user)) == NULL)
		    goto fail;
		  /* Check for identical folder/login users. */
		  DATA->folder_user=DATA->folder_userbuf;
		  if (	!strcmp(DATA->login_user, DATA->folder_user))

		    {
		      free(DATA->folder_userbuf);
		      DATA->folder_userbuf = NULL;
		    }
		}

	/* Share if the login and folder users are the same. */
	if (DATA->folder_userbuf == NULL) {
		DATA->folder_user = DATA->login_user;
	}


	return stream;
fail:
	mstore_freestream(stream);
	return NULL;
}


CALSTREAM*
mstore_close(CALSTREAM *stream, long options)
{
	if (stream && DATA)
	mstore_freestream(stream);
	return NULL;
}


bool
mstore_ping(CALSTREAM *stream)
{
	return true;
}


CALEVENT *read_event(FILE *calfile)
{
	char		line[100];
	char		*buf;
	int		size;
	CALEVENT	*event;

	fgets(line, sizeof(line), calfile);
	if (sscanf(line, "%d", &size) != 1)
		return NULL;
	buf = malloc(size + 2);
	fread(buf, size, 1, calfile);
	ical_preprocess(buf, &size);
	buf[size] = 0;
	buf[size+1] = 0;
	ical_parse(&event, buf, size);
	free(buf);

	return event;
}


bool
write_event(FILE *calfile, const CALEVENT *event)
{
	FILE		*tmp;
	char		*buf;

	tmp = icalout_begin();
	if(!icalout_event(tmp, event))
	  {
	    printf("Error writing to tmp file!");
	    perror("write_event");
	    return false;
	  }
	buf = icalout_end(tmp);

	if (buf == NULL)
		return false;

	fprintf(calfile, "%u\r\n", strlen(buf));
	fputs(buf, calfile);
	free(buf);

	return !ferror(tmp);
}


bool
mstore_search_range(	CALSTREAM *stream,
			const datetime_t *start,
			const datetime_t *end)
{
	CALEVENT	*event;
	datetime_t	_start = DT_INIT;
	datetime_t	_end = DT_INIT;
	FILE *calfile;
	char userpath[1000];
	snprintf(userpath,900,"%s/%s",DATA->base_path,DATA->folder_user);
	calfile=fopen (userpath,"a+");
	if(!calfile)
	  {
	    printf("Error! couldn't open calendar file!\n");
	    exit(1);
	  }
	rewind(calfile);

	if (start) {
		if (!dt_hasdate(start))
			start = NULL;
		else {
			dt_setdate(&_start,
				start->year, start->mon, start->mday);
		}
	}
	if (end) {
		if (!dt_hasdate(end))
			end = NULL;
		else
			dt_setdate(&_end, end->year, end->mon, end->mday);
	}

	while((event=read_event(calfile))) {
		datetime_t	clamp = DT_INIT;

		if (!start)
			dt_setdate(&clamp, 1, JANUARY, 1);
		else {
			dt_setdate(&clamp,
				_start.year, _start.mon, _start.mday);
		}

		calevent_next_recurrence(event, &clamp, stream->startofweek);
		if (	dt_hasdate(&clamp) &&
			!(end && dt_compare(&clamp, &_end) > 0))
		{
			cc_searched(event->id);
		}

		calevent_free(event);
	}
	fclose(calfile);
	return true;
}


bool
mstore_search_alarm(CALSTREAM *stream, const datetime_t *when)
{
	datetime_t	start;
	datetime_t	end;
	long		alarm = 0;
	unsigned long	id = 0;
	CALEVENT	*event;
	FILE		*calfile;
	char		userpath[1000];

	snprintf(userpath,900,"%s/%s",DATA->base_path,DATA->folder_user);
	calfile=fopen (userpath,"a+");
	if(!calfile)
	  {
	    printf("Error! couldn't open calendar file!\n");
	    exit(1);
	  }
	rewind(calfile);
	while((event=read_event(calfile))) {
		if (dt_roll_time(&(event->start), 0, -alarm, 0) &&
			dt_compare(&(event->start),&start)<=0 &&
			dt_compare(&end,&(event->end)) <=0)
		{
			cc_searched(id);
		}
		calevent_free(event);
	}
	fclose(calfile);
	return true;
}


bool
mstore_fetch(CALSTREAM *stream, unsigned long id, CALEVENT **inevent)
{
	CALEVENT	*event;
	FILE		*calfile;
	char		userpath[1000];

	snprintf(userpath,900,"%s/%s",DATA->base_path,DATA->folder_user);
	calfile=fopen (userpath,"a+");
	if(!calfile)
	  {
	    printf("Error! couldn't open calendar file!\n");
	    exit(1);
	  }
	rewind(calfile);
	while((event=read_event(calfile))) {
		if(event->id==id) {
			*inevent=event;
			return true;
		}
		calevent_free(event);
	}
	fclose(calfile);

	return false;
}


bool
mstore_append(	CALSTREAM *stream, const CALADDR *addr,
			unsigned long *id, const CALEVENT *event)
{
	CALEVENT myevent;
	FILE *calfile;
	char userpath[1000];

	if (addr->host)
		return false;
	if (addr->user)
		return false;
	if (strcasecmp(addr->folder, "INBOX"))
		return false;

	if (DATA->folder_userbuf)
		return false;
	if (!dt_hasdate(&event->start))
		return false;

	snprintf(userpath,900,"%s/%s",DATA->base_path,DATA->folder_user);
	calfile=fopen (userpath,"a");
        if(!calfile)
          {
            printf("Error! couldn't open calendar file %s\n",userpath);
	    perror("mstore_append");
            return false;
          }

	myevent = *event;
	myevent.id = time(NULL);
	write_event(calfile, &myevent);

	fclose(calfile);

	*id = myevent.id;

	return true;
}


bool
mstore_snooze(CALSTREAM *stream, unsigned long id)
{
	CALEVENT	*event;
	FILE		*calfile;
	FILE		*tmpfile;
	char		calpath[1000];
	char		tmppath[1000];

	snprintf(calpath,900,"%s/%s",DATA->base_path,DATA->folder_user);
	snprintf(tmppath,900,"%s/%s.tmp",DATA->base_path,DATA->folder_user);
	calfile = fopen(calpath,"a+");
	if(!calfile)
	  {
	    printf("Error! couldn't open calendar file!\n");
	    exit(1);
	  }
	rewind(calfile);
	tmpfile = fopen(tmppath, "w");
	if(!tmpfile)
	  {
	    printf("Error! couldn't open temp calendar file!\n");
	    exit(1);
	  }
	rewind(calfile);
	while((event=read_event(calfile))) {
		if (event->id == id)
			event->alarm = 0;
		write_event(tmpfile, event);
		calevent_free(event);
	}

	fclose(calfile);
	fclose(tmpfile);
	rename(tmppath, calpath);
	
	return true;
}


bool
mstore_remove(CALSTREAM *stream, unsigned long id)
{
	CALEVENT	*event;
	FILE		*calfile;
	FILE		*tmpfile;
	char		calpath[1000];
	char		tmppath[1000];

	snprintf(calpath,900,"%s/%s",DATA->base_path,DATA->folder_user);
	snprintf(tmppath,900,"%s/%s.tmp",DATA->base_path,DATA->folder_user);
	calfile = fopen(calpath,"a+");
	if(!calfile)
	  {
	    printf("Error! couldn't open calendar file!\n");
	    exit(1);
	  }
	rewind(calfile);
	tmpfile = fopen(tmppath, "w");
	if(!tmpfile)
	  {
	    printf("Error! couldn't open temp calendar file!\n");
	    exit(1);
	  }
	rewind(calfile);
	while((event=read_event(calfile))) {
		if(event->id != id)
			write_event(tmpfile, event);
		calevent_free(event);
	}
	fclose(calfile);
	fclose(tmpfile);
	rename(tmppath, calpath);

	return true;
}

bool
mstore_store(CALSTREAM *stream, const CALEVENT *modified_event)
{
  CALEVENT        *event;
  FILE            *calfile;
  FILE            *tmpfile;
  char            calpath[1000];
  char            tmppath[1000];
  
if(!modified_event->id)
  return false;

 snprintf(calpath,900,"%s/%s",DATA->base_path,DATA->folder_user);
 snprintf(tmppath,900,"%s/%s.tmp",DATA->base_path,DATA->folder_user);
  calfile = fopen(calpath,"a+");
  if(!calfile)
    {
      printf("Error! couldn't open calendar file!\n");
      exit(1);
    }
  rewind(calfile);
  tmpfile = fopen(tmppath, "w");
  if(!tmpfile)
    {
      printf("Error! couldn't open temp calendar file!\n");
      exit(1);
    }
  
        while((event=read_event(calfile))) {
	  if (event->id == modified_event->id)
	    {
	    (const CALEVENT*)event = modified_event;         
	  /*is more required here to assign objects, a loop through all the properties*/
	    /*    We actually only want to modify any individual property, not the whole thing..
		  TODO */
	    }
	  write_event(tmpfile, event);
	  calevent_free(event);
        }
	
        fclose(calfile);
        fclose(tmpfile);
        rename(tmppath, calpath);
	
        return true;
}

