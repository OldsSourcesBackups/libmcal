# $Id: Makefile,v 1.1 2000/04/01 03:40:05 inan Exp $

CC=gcc
FLEX=flex
INCLUDE=-I..
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
