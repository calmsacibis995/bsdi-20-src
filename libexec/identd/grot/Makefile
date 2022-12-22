#
# Makefile				Makefile for the Pidentd daemon
#
# This program is in the public domain and may be used freely by anyone
# who wants to. 
#
# Last update: 21 Oct 1994
#
# Please send bug fixes/bug reports to: Peter Eriksson <pen@lysator.liu.se>
#

#
# Modify it below to suit your particular system, or specify it
# on the command line (like "make DESTROOT=/usr/wheel sunos4")
#
MAKE=make
DESTROOT=/usr/local
DESTDIR=$(DESTROOT)/sbin
MANROOT=$(DESTROOT)/man
MANDIR=$(MANROOT)/man8

# NEXTSTEP 3.x Multi-Architecture-Binary (FAT) compiles
# If you want compile only for one specific platform, specifiy
# 	"-arch m68k" for Motorola 600[34]0
# or
# 	"-arch i386" for Intel i486
# (The Multi-Architecture-Binary is about double as big as the
#  Single-Architecture-Binary)
NEXTARCHS=-arch m68k -arch i386

#
# Global compile-time and link-time options
#
# Please note the the INCLUDE_CRYPT option needs a DES library. You can
# find a free one on ftp.lysator.liu.se in pub/libraries or on
# ftp.funet.fi in pub/crypt/libdes.
#
# GDEFS=-DINCLUDE_EXTENSIONS -DINCLUDE_PROXY -DINCLUDE_CRYPT
GDEFS=-DINCLUDE_EXTENSIONS

# GLIBS=-lident -ldes
GLIBS=

CFLAGS=-O

all:
	@echo "Please specify the type of system you wish to build for."
	@echo "Valid systems are:"
	@echo ""
	@echo "	sunos3		SunOS 3.x (tested with 3.5)"
	@echo "	sunos4		Sun SunOS 4"
	@echo "	sunos5		Sun SunOS 5 (Solaris 2)"
	@echo "	bsdi		BSD/386 (The commercial one)"
	@echo "	386bsd		386BSD (The free one)"
	@echo "	netbsd		NetBSD"
	@echo " freebsd		FreeBSD 2.x"
	@echo "	4.3bsd		4.3BSD Reno"
	@echo "	4.3bsd-tahoe	4.3BSD Tahoe"
	@echo "	dynix3		Sequent Dynix 3"
	@echo "	riscos4		MIPS RISC/OS 4"
	@echo "	aix		IBM AIX 3.2"
	@echo "	irix4		Silicon Graphics IRIX 4"
	@echo "	irix5		Silicon Graphics IRIX 5"
	@echo "	hpux7		Hewlett-Packard HP-UX 7"
	@echo "	hpux8		Hewlett-Packard HP-UX 8"
	@echo "	hpux9		Hewlett-Packard HP-UX 9"
	@echo "	sco		SCO unix System V release 3.2 (v4.0 & 4.1)"
	@echo "	svr4		Generic System V Release 4 UNIX"
	@echo "	attsvr4		AT&T's own System V Release 4"
	@echo "	aux2		Apple A/UX 2"
	@echo "	aux3		Apple A/UX 3"
	@echo "	ultrix3		Digital Ultrix 3"
	@echo "	ultrix4		Digital Ultrix 4"
	@echo "	alpha		Digital Alpha AXP OSF"
	@echo "	alpha3		Digital Alpha AXP OSF; 3.0 or later"
	@echo "	linux		Linux 0.99.13q or later"
	@echo "	unicos6		Cray UNICOS 6"
	@echo "	convex		Convex ConvexOS"
	@echo "	next		NeXT/Mach (NeXTSTEP 2 or 3.0)"
	@echo "	nextmab		NeXT/Mach (NeXTSTEP 3.1 MAB)"
	@echo "	pyramid4	Pyramid's dualPort OSx 4"
	@echo "	other		Other BSD-based Unixes (who knows, it may even work :-)"
	@echo ""
	@echo "For example: make sunos4"
	@echo ""

#
# Use this if Linux 0.99.13q or later
#
linux:
	@echo "Building for Linux 0.99.13q or later ..."
	@(cd src ; $(MAKE) CC="$(CC)" LDFLAGS="$(LDFLAGS)" LIBS="$(GLIBS)" CFLAGS="$(CFLAGS) -DLINUX -DNO_KVM $(GDEFS)" KSRC=linux in.identd)


#
# Use this if Sun SunOS 4.*
#
sunos sunos4:
	@echo "Building for Sun SunOS 4 ..."
	@(cd src ; $(MAKE) CC="$(CC)" LDFLAGS="$(LDFLAGS)" LIBS="-lkvm $(GLIBS)" CFLAGS="$(CFLAGS) -DSUN $(GDEFS)" KSRC=sunos4 in.identd)


#
# Use this if Sun SunOS 5.* (Solaris 2.*)
#
solaris solaris2 sunos5:
	@echo "Building for Sun SunOS 5 (Solaris 2) ..."
	@(cd src ; $(MAKE) CC="$(CC)" LDFLAGS="$(LDFLAGS)" LIBS="-lkvm -lelf -lsocket -lnsl $(GLIBS)" CFLAGS="$(CFLAGS) -DHAVE_KVM -DSOLARIS -DSVR4 $(GDEFS)" KSRC=sunos5 in.identd)

#
# Use this if 386BSD
#
386bsd:
	@echo "Building for 386BSD (The free one) ..."
	@(cd src ; $(MAKE) CC="$(CC)" LDFLAGS="$(LDFLAGS)" LIBS="-lutil $(GLIBS)" CFLAGS="$(CFLAGS) -DBSD386 $(GDEFS)" KSRC=386bsd in.identd)

#
# Use this if NetBSD
#
netbsd:
	@echo "Building for NetBSD ..."
	@(cd src ; $(MAKE) CC="$(CC)" LDFLAGS="$(LDFLAGS)" \
		LIBS="-lutil $(GLIBS)" CFLAGS="$(CFLAGS) -DBSD386 \
		$(GDEFS) -DHAVE_KVM" KSRC=386bsd in.identd)

#
# Use this if FreeBSD 2.x
#
freebsd:
	@echo "Building for FreeBSD 2.x ..."
	@(cd src ; $(MAKE) CC="$(CC)" LDFLAGS="$(LDFLAGS)" \
		LIBS="-lkvm $(GLIBS)" CFLAGS="$(CFLAGS) -DFREEBSD \
		$(GDEFS) -DHAVE_KVM" KSRC=freebsd in.identd)

#
# Use this if BSDI's BSD/386 (the commercial BSD)
#
bsdi bsd386:
	@echo "Building for BSDI's BSD/386 ..."
	@(cd src ; $(MAKE) CC="$(CC)" LDFLAGS="$(LDFLAGS)" LIBS="-lutil -lkvm $(GLIBS)" CFLAGS="$(CFLAGS) -DBSDI -DHAVE_KVM $(GDEFS)" KSRC=bsdi in.identd)

#
# Use this if 4.3BSD
#
bsd 4.3bsd 4.3bsd-reno:
	@echo "Building for 4.3BSD-Reno ..."
	@(cd src ; $(MAKE) CC="$(CC)" LDFLAGS="$(LDFLAGS)" LIBS="$(GLIBS)" CFLAGS="$(CFLAGS) -DBSD43 $(GDEFS)" MAKE=$(MAKE) KSRC=4.3bsd in.identd)

4.3bsd-tahoe:
	@echo "Building for 4.3BSD-Tahoe ..."
	@(cd src ; $(MAKE) CC="$(CC)" LDFLAGS="$(LDFLAGS)" LIBS="$(GLIBS)" CFLAGS="$(CFLAGS) -DBSD43 -DTAHOE $(GDEFS)" MAKE=$(MAKE) KSRC=4.3bsd in.identd)

#
# Use this for AIX 3.2
#
aix:
	@echo "Building for AIX 3.2 ..."
	@(cd src ; $(MAKE) CC="$(CC)" LDFLAGS="$(LDFLAGS)" LIBS="$(GLIBS)" CFLAGS="-D_AIX32 $(CFLAGS) $(GDEFS)" MAKE=$(MAKE) KSRC=aix in.identd)

#
# Use this if Sequent Dynix 3.*
#
dynix dynix3:
	@echo "Building for Sequent Dynix 3 ..."
	@(cd src ; $(MAKE) CC="$(CC)" LDFLAGS="$(LDFLAGS)" LIBS="$(GLIBS)" CFLAGS="$(CFLAGS) -DSEQUENT $(GDEFS)" MAKE=$(MAKE) KSRC=other in.identd)

#
# Use this if Mips RISC/OS 4.*
#
mips riscos riscos4:
	@echo "Building for MIPS RISC/OS 4 ..."
	@(cd src ; $(MAKE) CC="$(CC) -systype bsd43" LDFLAGS="$(LDFLAGS)" LIBS="-lmld $(GLIBS)" CFLAGS="$(CFLAGS) -I/usr/include/bsd43 -I/usr/include/bsd43/sys -I/usr/include -DMIPS $(GDEFS)" MAKE=$(MAKE) KSRC=other in.identd)

#
# Use this if SGI IRIX 4.*
#
irix4:
	@echo "Building for Silicon Graphics IRIX 4 ..."
	@(cd src ; $(MAKE) CC="$(CC)" LDFLAGS="$(LDFLAGS)" LIBS="-lmld -lsun -lc_s $(GLIBS)" CFLAGS="$(CFLAGS) -DIRIX $(GDEFS)" MAKE=$(MAKE) KSRC=irix4 in.identd)

#
# Use this if SGI IRIX 5.*
#
sgi irix irix5:
	@echo "Building for Silicon Graphics IRIX 5 ..."
	@(cd src ; $(MAKE) CC="$(CC)" LDFLAGS="$(LDFLAGS)" LIBS="-lmld -lsun -lc_s $(GLIBS)" CFLAGS="$(CFLAGS) -DIRIX $(GDEFS)" MAKE=$(MAKE) KSRC=irix5 in.identd)


#
# Use this if SCO SVR3.2v4.0 and v4.1
#
sco sco324 sco32v4 sco3.2v4:
	@echo "Building for SCO 3.2v4.0 and 3.2v4.1..."
	@(cd src ; $(MAKE) CC="$(CC)" LDFLAGS="$(LDFLAGS)" LIBS="$(GLIBS) -lsocket -lc_s" CFLAGS="$(CFLAGS) -Dsco $(GDEFS)" MAKE=$(MAKE) KSRC=sco324 in.identd)


#
# Use this if Hewlett-Packard HP-UX 7.*
#
hpux7:
	@echo "Building for Hewlett-Packard HP-UX 7 ..."
	@(cd src ; $(MAKE) CC="$(CC)" LDFLAGS="$(LDFLAGS)" LIBS="$(GLIBS)" CFLAGS="$(CFLAGS) -DHPUX7 $(GDEFS)" MAKE=$(MAKE) KSRC=hpux in.identd)

#
# Use this if Hewlett-Packard HP-UX 8.*
#
hpux hpux8 hpux9:
	@echo "Building for Hewlett-Packard HP-UX 8 or 9..."
	@(cd src ; $(MAKE) CC="$(CC)" LDFLAGS="$(LDFLAGS)" LIBS="$(GLIBS)" CFLAGS="-D_HPUX_SOURCE $(CFLAGS) $(GDEFS)" MAKE=$(MAKE) KSRC=hpux in.identd)

#
# Use this if SysV R4
#
svr4:
	@echo "Building for UNIX System V Release 4 ..."
	@(cd src ; $(MAKE) CC="$(CC)" LDFLAGS="$(LDFLAGS)" LIBS="-lelf -lresolv -lsocket -lnsl $(GLIBS)" CFLAGS="$(CFLAGS) -DSVR4 $(GDEFS)" MAKE=$(MAKE) KSRC=svr4 in.identd)

#
# Use this if AT&T's own SysV R4
#
attsvr4:
	@echo "Building for AT&T's own System V Release 4 ..."
	@(cd src ; $(MAKE) CC="$(CC)" LDFLAGS="$(LDFLAGS)" LIBS="-lelf -lresolv -lsocket -lnsl $(GLIBS)" CFLAGS="$(CFLAGS) -DATTSVR4 -DSVR4 $(GDEFS)" MAKE=$(MAKE) KSRC=svr4 in.identd)

#
# Use this if A/UX 2
#
# MANROOT=/usr/catman/u_man
# MANDIR=$(MANROOT)/man8
aux2:
	@echo "Building for A/UX 2..."
	@(cd src ; $(MAKE) CC="$(CC)" LDFLAGS="$(LDFLAGS)" LIBS="$(GLIBS)" CFLAGS="$(CFLAGS) -D_AUX_SOURCE -ZB $(GDEFS)" MAKE=$(MAKE) KSRC=aux in.identd)
	@echo "Build done. When/if you do 'make install', don't forget to"
	@echo "specify MANDIR=/usr/catman/u_man/man8."

#
# Use this is A/UX 3
#
aux aux3:
	@echo "Building for A/UX 3..."
	@(cd src ; $(MAKE) CC="$(CC)" LDFLAGS="$(LDFLAGS)" LIBS="$(GLIBS)" CFLAGS="$(CFLAGS) -D_AUX_SOURCE -D'ushort=unsigned short' -D'uint=unsigned int' -ZB $(GDEFS)" MAKE=$(MAKE) KSRC=aux in.identd)
	@echo "Build done. When/if you do 'make install', don't forget to"
	@echo "specify MANDIR=/usr/catman/u_man/man8."

#
# Use this if DEC Ultrix 3 or 4
#
ultrix ultrix3 ultrix4:
	@echo "Building for Digital Ultrix 3 or 4 ..."
	@(cd src ; $(MAKE) CC="$(CC)" LDFLAGS="$(LDFLAGS)" LIBS="$(GLIBS)" CFLAGS="$(CFLAGS) $(GDEFS)" MAKE=$(MAKE) KSRC=other in.identd)


#
# Alpha AXP OSF/1, before version 3.0
#
alpha AlphaOSF:
	@echo "Building for Digital Alpha AXP OSF/1..."
	@(cd src ; $(MAKE) CC="$(CC)" LDFLAGS="$(LDFLAGS)" LIBS="$(GLIBS)" CFLAGS="-I/usr/sys/include -I/usr/sys/BINARY $(CFLAGS) $(GDEFS)" MAKE=$(MAKE) KSRC=other in.identd)

#
# Alpha AXP OSF/1, version 3.0 and later.
#
alpha3 alpha_3.0:
	@echo "Building for Digital Alpha AXP OSF/1 version 3.0 or later..."
	@(cd src ; $(MAKE) CC="$(CC)" LDFLAGS="$(LDFLAGS)" LIBS="$(GLIBS)" CFLAGS="$(CFLAGS) $(GDEFS)" MAKE=$(MAKE) KSRC=alpha3 in.identd)


#
# Use this if Cray UNICOS 6.*
#
cray unicos unicos6:
	@echo "Building for Cray UNICOS 6 ..."
	@(cd src ; $(MAKE) CC="$(CC)" LDFLAGS="$(LDFLAGS)" LIBS="$(GLIBS)" CFLAGS="$(CFLAGS) $(GDEFS)" MAKE=$(MAKE) KSRC=unicos6 in.identd)

#
# Use this if Convex ConvexOS
#
convex:
	@echo "Building for Convex ConvexOS ..."
	@(cd src ; $(MAKE) CC="$(CC)" LDFLAGS="$(LDFLAGS)" LIBS="$(GLIBS)" CFLAGS="$(CFLAGS) $(GDEFS)" MAKE=$(MAKE) KSRC=other in.identd)

#
# Use this if NeXTSTEP 3.1 or up - Multi-Architecture Binary or 
#				   Single-Architecture Binary depending on
#				   $(NEXTARCHS)
#
nextmab next3.1 nextstep3.1 nextintel:
	@echo "Building for NeXT/Mach 3.1 Multi-Architecture  (m68k+i386)"
	@(cd src ; $(MAKE) CC="$(CC)" \
		LDFLAGS="-s -object $(LDFLAGS) $(NEXTARCHS)" \
		LIBS="$(GLIBS)" \
		CFLAGS="-DNeXT31 -O2 -Wall -pipe $(GDEFS) $(NEXTARCHS)" \
		KSRC=next_mach in.identd)
	@size in.identd ; file in.identd ; lipo -info in.identd

#
# Use this if NeXTSTEP 2 or 3.0
#
next next3.0 nextstep3.0 next2 nextstep2:
	@echo "Building for NeXT/Mach 2 or 3.0 ..."
	@(cd src ; $(MAKE) CC="$(CC)" LDFLAGS="$(LDFLAGS)" LIBS="$(GLIBS)" CFLAGS="$(CFLAGS) $(GDEFS)" MAKE=$(MAKE) KSRC=next_mach in.identd)


#
# Use this if SunOS 3.x (tested on SunOS 3.5 using GCC)
#
sunos3:
	@echo "Building for SunOS 3..."
	@if [ ! -h h ]; then echo "You need a symbolic link put in here - ln -s /usr/include/sys h"; fi
	@(cd src ; $(MAKE) CC="$(CC)" LDFLAGS="$(LDFLAGS)" LIBS="$(GLIBS)" CFLAGS="$(CFLAGS) -DSUNOS35 $(GDEFS)" MAKE=$(MAKE) KSRC=other in.identd)

#
# Use this for Pyramid's dualPort OSx 4.*
#
pyramid4:
	@echo "Building for Pyramid's dualPort OSx 4..."
	@(cd src ; $(MAKE) CC="$(CC)" LDFLAGS="$(LDFLAGS)" LIBS="$(GLIBS)" CFLAGS="$(CFLAGS) $(GDEFS)" MAKE=$(MAKE) KSRC=other in.identd)

#
# And all others can try this - but don't put to much hope into it...
#
other:
	@echo "Building for other unspecified Unix (BSD-like) ..."
	@(cd src ; $(MAKE) CC="$(CC)" LDFLAGS="$(LDFLAGS)" LIBS="$(GLIBS)" CFLAGS="$(CFLAGS) $(GDEFS)" MAKE=$(MAKE) KSRC=other in.identd)


#
# End of system-type definitions --------------------------------------
#
in.identd: 
	@echo "Sorry, you have to manually specify what system you wish to"
	@echo "build for. Type 'make' without any arguments to see what"
	@echo "systems are supported."
	@exit 1

install: in.identd
	cp identd.8  $(MANDIR)/identd.8
	cp in.identd $(DESTDIR)/in.identd

tests:
	(cd testdir ; make)

clean:
	-rm -f in.identd idecrypt itest *~ core *.o \#*
	(cd src ; make clean)
	(cd testdir ; make clean)

version:
	(PACKNAME=`basename \`pwd\`` ; echo 'char version[] = "'`echo $$PACKNAME | cut -d- -f2`'";' >src/version.c)

distZ:		clean version 
	(PACKNAME=`basename \`pwd\`` ; cd .. ; /usr/gnu/bin/tar Zcf $$PACKNAME.tar.Z $$PACKNAME)

dist distz:	clean version
	(PACKNAME=`basename \`pwd\`` ; cd .. ; /usr/gnu/bin/tar cf - $$PACKNAME | gzip -9 >$$PACKNAME.tar.gz)


