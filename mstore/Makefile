# $Id: Makefile,v 1.4 2001/05/07 17:36:29 chuck Exp $

CC=gcc
FLEX=flex
INCLUDE=-I.. -I../libmcal
# CFLAGS=-O0 -Wall -g $(INCLUDE) -DDEBUG -DUSE_PAM
CFLAGS=-O0 -fPIC -Wall -g $(INCLUDE) -DDEBUG
ALLOBJS=mstore.o
TARGET=mstore_driver.o

all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $<

$(TARGET): $(ALLOBJS)
	ld -r -o $(TARGET) $(ALLOBJS)
	touch bootstrap.in

clean:
	rm -f $(ALLOBJS) $(TARGET) bootstrap.in

dep: depend

depend:
	makedepend $(INCLUDE) -- $(ALLOBJS:%.o=%.c) >& /dev/null
