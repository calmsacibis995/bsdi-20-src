BINDIR=		/usr/local/bin
ETCDIR=		/usr/local/etc
MANDIR=		/usr/local/man
MANEXT=		8

all:
	@ echo 'Use the "build" command (shell script) to make ftpd.'
	@ echo 'You can say "build help" for details on how it works.'

install: bin/ftpd bin/ftpcount bin/ftpshut
	-mv -f ${ETCDIR}/ftpd ${ETCDIR}/ftpd-old
	@echo Installing binaries.
	install -o bin -g bin -m 755 bin/ftpd ${ETCDIR}/ftpd
	install -o bin -g bin -m 755 bin/ftpshut ${BINDIR}/ftpshut
	install -o bin -g bin -m 755 bin/ftpcount ${BINDIR}/ftpcount
	install -o bin -g bin -m 755 bin/ftpwho ${BINDIR}/ftpwho
	@echo Installing manpages.
	install -o bin -g bin -m 755 doc/ftpd.8 ${MANDIR}/man8/ftpd.8
	install -o bin -g bin -m 755 doc/ftpcount.1 ${MANDIR}/man1/ftpcount.1
	install -o bin -g bin -m 755 doc/ftpwho.1 ${MANDIR}/man1/ftpwho.1
	install -o bin -g bin -m 755 doc/ftpshut.8 ${MANDIR}/man8/ftpshut.8
	install -o bin -g bin -m 755 doc/ftpaccess.5 ${MANDIR}/man5/ftpaccess.5
	install -o bin -g bin -m 755 doc/ftphosts.5 ${MANDIR}/man5/ftphosts.5
	install -o bin -g bin -m 755 doc/ftpconversions.5 ${MANDIR}/man5/ftpconversions.5
	install -o bin -g bin -m 755 doc/xferlog.5 ${MANDIR}/man5/xferlog.5
