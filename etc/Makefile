#	BSDI	Makefile,v 2.1 1995/02/03 05:53:26 polk Exp
#	@(#)Makefile	5.11 (Berkeley) 5/21/91

# utmp is group writable by UTMPGRP
UTMPGRP=utmp

# Make everything owned by root/wheel in general
BINOWN=root
BINGRP=wheel

NOOBJ=	oobj

# disktab may be wrong -- hcx9 is a tahoe, but gets its own.
# -rw-r--r--
BIN=	aliases changelist csh.cshrc csh.login csh.logout crontab \
	daily daily.local dm.conf ftpusers gettytab group hosts \
	hosts.lpd inetd.conf inetd.conf.nowrappers man.conf \
	monthly monthly.local motd netgroup netstart.sample phones printcap \
	protocols rc rc.local remote security services shells \
	syslog.conf ttys weekly weekly.local mtoolsrc networks \
	etc.${MACHINE}/disktab

MTREE=	BSDI.root.dist BSDI.usr.dist BSDI.var.dist
KRB=	README README.install krb.conf krb.realms
NAMEDB=	localhost.rev named.boot root.cache
PCS=	pcs750.bin
WCS1=	wcs fppwcs poc poc1 poc2 fppoc
WCS2=	fpevent fppwcs fppwcs_dual hdcwcs load_diags start_fpp wcs wcs_dual
RBOOTD=	rbootd.conf

all clean cleandir depend etc install lint:

distribution:
	touch ${DESTDIR}/CONFIGME
	install -c -o ${BINOWN} -g ${BINGRP} -m 644 ${BIN} ${DESTDIR}/etc
	install -c -o root -g wheel -m 600 master.passwd ${DESTDIR}/etc
	install -c -o ${BINOWN} -g ${BINGRP} -m 600 hosts.equiv ${DESTDIR}/etc
	install -c -o ${BINOWN} -g ${BINGRP} -m 555 \
	     MAKEDEV.local etc.${MACHINE}/MAKEDEV ${DESTDIR}/dev
	(cd root; \
		install -c -o root -g wheel -m 644 dot.cshrc \
		    ${DESTDIR}/root/.cshrc; \
		install -c -o root -g wheel -m 600 dot.klogin \
		    ${DESTDIR}/root/.klogin; \
		install -c -o root -g wheel -m 644 dot.login \
		    ${DESTDIR}/root/.login; \
		install -c -o root -g wheel -m 644 dot.profile \
		    ${DESTDIR}/root/.profile; \
		rm -f ${DESTDIR}/.profile; \
		ln ${DESTDIR}/root/.profile ${DESTDIR}/.profile)
	cd mtree; install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${MTREE} \
	    ${DESTDIR}/etc/mtree
	cd mtree; install -c -o ${BINOWN} -g ${BINGRP} -m 600 special \
	    ${DESTDIR}/etc/mtree
	cd namedb; install -c -o ${BINOWN} -g ${BINGRP} -m 644 ${NAMEDB} \
	    ${DESTDIR}/etc/namedb
	cd kerberosIV; install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${KRB} \
	    ${DESTDIR}/etc/kerberosIV
	install -c -o ${BINOWN} -g operator -m 664 /dev/null \
	    ${DESTDIR}/etc/dumpdates
	install -c -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
	    ${DESTDIR}/etc/sendmail.cw
	install -c -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
	    ${DESTDIR}/var/log/messages
	install -c -o ${BINOWN} -g ${BINGRP} -m 600 /dev/null \
	    ${DESTDIR}/var/log/secure
	install -c -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
	    ${DESTDIR}/var/log/ftp.log
	install -c -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
	    ${DESTDIR}/var/spool/uucp/errors
	install -c -o news -g news -m 664 /dev/null \
	    ${DESTDIR}/var/log/news/news.log
	install -c -o news -g news -m 664 /dev/null \
	    ${DESTDIR}/var/log/news/news.err
	install -c -o news -g news -m 664 /dev/null \
	    ${DESTDIR}/var/log/news/news.crit
	install -c -o news -g news -m 664 /dev/null \
	    ${DESTDIR}/var/log/news/news.notice
	install -c -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
	    ${DESTDIR}/var/log/maillog
	install -c -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
	    ${DESTDIR}/var/log/lpd-errs
	install -c -o ${BINOWN} -g ${UTMPGRP} -m 664 /dev/null \
	    ${DESTDIR}/var/run/utmp
	install -c -o ${BINOWN} -g ${UTMPGRP} -m 664 /dev/null \
	    ${DESTDIR}/var/log/cron
	install -c -o nobody -g nogroup -m 664 /dev/null \
	    ${DESTDIR}/var/db/locate.database
	install -c -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
	    ${DESTDIR}/var/log/lastlog
	install -c -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
	    ${DESTDIR}/var/log/wtmp
	install -c -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
	    ${DESTDIR}/var/account/acct
	(cd etc.${MACHINE}; install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
	    fstab.* ${DESTDIR}/etc)
	(cd amd_samples; install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
	    start-amd amd-* ${DESTDIR}/etc/amd_samples)
.if ${MACHINE} == "tahoe"
	(cd etc.tahoe; install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${WCS1} \
	    ${DESTDIR}/)
.endif
.if ${MACHINE} == "vax"
	(cd etc.vax; install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${PCS} \
	    ${DESTDIR}/)
.endif

hcx9-distribution:
	(cd etc.tahoe; install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${WCS2} \
	    ${DESTDIR}/)

.include <bsd.prog.mk>
