# safewrite package Makefile

MYNAME = safewrite
VERSION = 1.1
BLURB = "Safely write STDOUT"
GROUP = Applications/Networking
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

install:
	install -m 0555 safewrite $(RPM_BUILD_ROOT)/usr/bin/safewrite
	install -m 0444 safewrite.1 $(RPM_BUILD_ROOT)/usr/man/man1/safewrite.1
