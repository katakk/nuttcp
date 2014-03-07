APP = nuttcp-v5.1.11
EXTRAVERSION=
CC = gcc
#OPT = -g -O0
OPT = -O3
CFLAGS = $(OPT) -I. -Imissing
LIBS = 
APPEXT =

CFLAGS.MISSING = $(CFLAGS) -DHAVE_CONFIG_H
OBJS.GETADD = getaddrinfo.o
OBJS.INETFUN = inet_ntop.o inet_pton.o inet_aton.o 

TAR = gtar

all: $(APP)$(APPEXT)

sgicc:
	$(MAKE) CC=cc OPT="-O -OPT:Olimit=0"

sol28:
	$(MAKE) LIBS="-lsocket -lnsl"

sol28cc:
	$(MAKE) CC=cc OPT=-O LIBS="-lsocket -lnsl"

sol26:
	$(MAKE) LIBS="$(OBJS.GETADD) $(OBJS.INETFUN) -lsocket -lnsl"

sol26cc:
	$(MAKE) CC=cc OPT=-O LIBS="$(OBJS.GETADD) $(OBJS.INETFUN) -lsocket -lnsl"

cyg:
	$(MAKE) APPEXT=".exe"

oldcyg:
	$(MAKE) LIBS="$(OBJS.GETADD) $(OBJS.INETFUN)" APPEXT=".exe"

win32:
	$(MAKE) LIBS="$(OBJS.INETFUN) -Lwin32 -llibc.a -Bstatic" APPEXT=".exe"

icc:
	$(MAKE) CC=icc OPT=-O2
#	$(MAKE) CC=icc OPT="-w -O3 -parallel -unroll -align -xM -vec_report -par_report2"

$(APP)$(APPEXT): $(APP)$(EXTRAVERSION).c $(LIBS)
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)
inet_ntop.o: missing/inet_ntop.c missing/config.h
	$(CC) $(CFLAGS.MISSING) -o $@ -c $<
inet_pton.o: missing/inet_pton.c missing/config.h
	$(CC) $(CFLAGS.MISSING) -o $@ -c $<
inet_aton.o: missing/inet_aton.c missing/config.h
	$(CC) $(CFLAGS.MISSING) -o $@ -c $<
getaddrinfo.o: missing/getaddrinfo.c missing/config.h
	$(CC) $(CFLAGS.MISSING) -o $@ -c $<

clean:
	rm  -f $(APP)$(APPEXT) $(APP).o $(OBJS.GETADD) $(OBJS.INETFUN)

tar:
	(cd ..; $(TAR) cfz $(APP)$(EXTRAVERSION).tar.gz --exclude old --exclude bin $(APP))
