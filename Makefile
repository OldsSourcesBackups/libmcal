# $Id: Makefile,v 1.2 2001/04/16 18:07:15 rufustfirefly Exp $

CC=gcc
FLEX=flex
INCLUDE=-I.. -I../libmcal
CFLAGS=-O0 -Wall -g $(INCLUDE)
ALLOBJS=mysqldrv.o
TARGET=mysqldrv_driver.o

all: $(TARGET)


$(TARGET): $(ALLOBJS)
	ld -r -o $(TARGET) $(ALLOBJS) -L/usr/lib/mysql -lmysqlclient -lnsl
	touch bootstrap.in

clean:
	rm -f $(ALLOBJS) $(TARGET) bootstrap.in

dep: depend

depend:
	makedepend $(INCLUDE) -- $(ALLOBJS:%.o=%.c) >& /dev/null
