# $Id: Makefile,v 1.1 1999/12/02 08:02:57 zircote Exp $

CC=gcc
FLEX=flex
INCLUDE=-I..
CFLAGS=-O0 -Wall -g $(INCLUDE)
ALLOBJS=mstore.o
TARGET=mstore_driver.o

all: $(TARGET)


$(TARGET): $(ALLOBJS)
	ld -r -o $(TARGET) $(ALLOBJS)
	touch bootstrap.in

clean:
	rm -f $(ALLOBJS) $(TARGET) bootstrap.in

dep: depend

depend:
	makedepend $(INCLUDE) -- $(ALLOBJS:%.o=%.c) >& /dev/null
