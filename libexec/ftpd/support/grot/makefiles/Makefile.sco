CC     = cc
AR     = ar clq
RANLIB = /bin/true
LIBC   = /lib/libc.a
IFLAGS = 
LFLAGS = 
CFLAGS = -O ${IFLAGS} ${LFLAGS}

SRCS   = getusershell.c fnmatch.c strcasestr.c strsep.c \
		 authuser.c ftw.c sco.c
OBJS   = getusershell.o fnmatch.o strcasestr.o strsep.o \
		 authuser.o ftw.o sco.o
 
all: $(OBJS)
	-rm -f libsupport.a
	${AR} libsupport.a $(OBJS)
	${RANLIB} libsupport.a

clean:
	-rm -f *.o libsupport.a

ftp.h:
	install -c -m 444 ftp.h /usr/include/arpa

paths.h:
	install -c -m 444 paths.h /usr/include

fnmatch.o: fnmatch.c
	${CC} ${CFLAGS} -c fnmatch.c

getusershell.o: getusershell.c
	${CC} ${CFLAGS} -c getusershell.c

strerror.o: strerror.c
	${CC} ${CFLAGS} -c strerror.c

strdup.o: strdup.c
	${CC} ${CFLAGS} -c strdup.c

strcasestr.o: strcasestr.c
	${CC} ${CFLAGS} -c strcasestr.c

strsep.o: strsep.c
	${CC} ${CFLAGS} -c strsep.c

authuser.o: authuser.c
	${CC} ${CFLAGS} -c authuser.c

ftw.o: ftw.c
	${CC} ${CFLAGS} -c ftw.c

ftruncate.o: ftruncate.c
	${CC} ${CFLAGS} -c ftruncate.c

initgroups.o: initgroups.c
	${CC} ${CFLAGS} -c initgroups.c

sco.o: sco.c
	${CC} ${CFLAGS} -c sco.c
