#$Id: Makefile,v 1.2 2000/01/25 03:08:10 markie Exp $

CC=gcc
FLEX=flex
INCLUDE=-I..
CFLAGS=-O0 -Wall -g $(INCLUDE)
ALLOBJS=icap.o icaproutines.o lex.icap_yy.o
TARGET=icap_driver.o

all: $(TARGET)


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

