# $Id: Makefile,v 1.2 2000/10/05 17:38:18 rufustfirefly Exp $

CC=gcc
FLEX=flex
INCLUDE=-I..
CFLAGS=-O0 -Wall -g $(INCLUDE) -DDEBUG
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
