# $Id: Makefile,v 1.3 2001/03/20 16:40:52 rufustfirefly Exp $

CC=gcc
FLEX=flex
INCLUDE=-I.. -I../libmcal
CFLAGS=-O0 -Wall -g $(INCLUDE) -DDEBUG -DUSE_PAM
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
