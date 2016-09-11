# $Id: Makefile,v 1.1 1999/12/02 08:02:04 zircote Exp $
CC=gcc
FLEX=flex
INCLUDE=-I../libmcal
CFLAGS=-O0 -Wall -g $(INCLUDE)

LIBCAL=mcal
LIBDIR=../libmcal


all: mcal 

mcal: mcal.o 
	gcc -Wall -g -o mcal mcal.o -L$(LIBDIR) -l$(LIBCAL) -lcrypt

mcal.o: mcal.c
	gcc -Wall -c $(INCLUDE) -g mcal.c 

clean:
	rm -f *.o mcal 

