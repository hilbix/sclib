# $Header: /CVSROOT/scylla-charybdis/Makefile,v 1.9 2003/11/06 12:10:58 tino Exp $
#
# $Log: Makefile,v $
# Revision 1.9  2003/11/06 12:10:58  tino
# Make in subdir example
#
# Revision 1.8  2003/11/06 07:03:16  tino
# distclean now clean subdirs, too.
#
# Revision 1.7  2003/11/06 06:43:31  tino
# -
#
# Revision 1.6  2003/11/06 05:27:24  tino
# tino-deploy now in CVS
#
# Revision 1.5  2003/11/02 22:33:07  tino
# version.h now distcleaned
#
# Revision 1.4  2003/11/02 22:22:26  tino
# version.h added
#
# Revision 1.3  2003/10/21 19:29:36  tino
# Very little improvements ;)
#
# Revision 1.2  2003/10/07 19:48:33  tino
# make distclean was a little funny

CFLAGS=-Wall -I/usr/ssl/include -I/usr/include/ssl -I/usr/include/openssl #-Itinolib #-DNO_SSL
LDFLAGS=-L/usr/ssl/lib #-Ltinolib
LDLIBS=-lssl -lcrypto -lz #-ltino

APPS=scylla charybdis odysseus

all:	$(APPS)
	cd example && make all

# Poor man's install
install:
	cp $(APPS) /usr/local/bin

charybdis:	charybdis.o
scylla:		scylla.o
odysseus:	odysseus.o

charybdis.o:	version.h
scylla.o:	version.h
odysseus.o:	version.h

version.h:	VERSION Makefile
	( \
	echo "#define	VERSION	\"`head -1 VERSION`\""; \
	) > version.h

clean:
	cd example && make clean
	$(RM) $(APPS) *.o

distclean:	clean
	cd example && make distclean
	$(RM) version.h *~ */*~

dist:	distclean
	../tino-deploy/make-tar-version.sh .
