# safewrite package Makefile

MYNAME = safewrite
VERSION = 1.3
BLURB = "Safely write STDOUT"
DOC = GPL
TAR = $(MYNAME)-$(VERSION).tar.gz
COPYRIGHT = GPL

all: safewrite

safewrite: safewrite.c
	cc safewrite.c -o safewrite

clean:
	rm -f safewrite

rpm: $(TAR)
	rpm -ta $(TAR)

dist $(TAR) MANIFEST : Makefile
	makespec

install: installprefix
	install -m 0555 safewrite `cat installprefix`/bin/safewrite
	install -m 0444 safewrite.1 `cat installprefix`/man/man1/safewrite.1
