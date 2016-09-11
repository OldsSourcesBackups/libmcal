#$Id: Makefile,v 1.3 2001/04/16 18:08:53 rufustfirefly Exp $

CC=gcc
FLEX=flex
INCLUDE=-I.. -I../libmcal
CFLAGS=-O0 -fPIC -Wall -g $(INCLUDE)
ALLOBJS=icap.o icaproutines.o lex.icap_yy.o
TARGET=icap_driver.o

all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $<

$(TARGET): $(ALLOBJS)
	ld -r -o $(TARGET) $(ALLOBJS)
	touch bootstrap.in

lex.icap_yy.c: icapscanner.lex
	$(FLEX) $<

clean:
	rm -f $(ALLOBJS) $(TARGET) lex.ical_yy.c lex.icap_yy.c bootstrap.in

dep: depend

depend:
	makedepend $(INCLUDE) -- $(ALLOBJS:%.o=%.c) >& /dev/null

