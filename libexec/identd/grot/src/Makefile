#
# Makefile
#
# Makefile for the Pidentd daemon
#
# Author: Peter Eriksson <pen@lysator.liu.se>
#

KSRC=kernel.c
COBJS= parse.o kernel.o kvm.o version.o proxy.o config.o crypto.o

foo:
	@echo 'Nono! Don't type "make" in this directory, use the one above!'
	
in.identd:	identd.o $(COBJS)   idecrypt itest
	$(CC) $(LDFLAGS) -o in.identd identd.o $(COBJS) $(LIBS)
	@mv in.identd ..

idecrypt:	idecrypt.o crypto.o
	$(CC) $(LDFLAGS) -o idecrypt idecrypt.o crypto.o $(LIBS)
	@mv idecrypt ..

identd.o:	 identd.c identd.h error.h crypto.h Makefile
parse.o:	 parse.c identd.h error.h crypto.h Makefile
kvm.o:		 kvm.c kernel/kvm.h paths.h Makefile
config.o:	 config.c Makefile
proxy.o:	 proxy.c Makefile
version.o:	 version.c Makefile
crypto.o:        crypto.c Makefile
idecrypt.o:      idecrypt.c crypto.h Makefile

kernel.o:	 kernel/$(KSRC).c identd.h error.h kernel/kvm.h Makefile
	$(CC) -c -I. $(CFLAGS) kernel/$(KSRC).c
	@mv $(KSRC).o kernel.o

libkvm.a:	kvm.o
	@rm -f libkvm.a
	ar cq libkvm.a kvm.o

itest: itest.o kernel.o kvm.o
	$(CC) $(LDFLAGS) -o itest itest.o kernel.o kvm.o $(LIBS)
	@mv itest ..

clean:
	-rm -f in.identd core *.o *~ kernel/*~ \#* kernel/\#*
