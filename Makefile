# Makefile for 'upr' (microprinter)

CC=gcc
CFLAGS=-Wall -c

LD=gcc
LDFLAGS=

all: upr
.PHONY: all

upr: upr.o
	$(LD) $(LDFLAGS) -o upr upr.o

upr.o: upr.c
	$(CC) $(CFLAGS) -o upr.o upr.c

