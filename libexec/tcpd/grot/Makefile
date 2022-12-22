# @(#) Makefile 1.12 94/03/23 16:51:32

what:
	@echo
	@echo "Usage: edit the REAL_DAEMON_DIR definition in the Makefile then:"
	@echo
	@echo "	make sys-type"
	@echo
	@echo "If you are in a hurry you can try instead:"
	@echo
	@echo "	make REAL_DAEMON_DIR=/foo/bar sys-type"
	@echo
	@echo "And for a version with language extensions enabled:"
	@echo
	@echo "	make REAL_DAEMON_DIR=/foo/bar STYLE=-DPROCESS_OPTIONS sys-type"
	@echo
	@echo "This Makefile knows about the following sys-types:"
	@echo
	@echo "	generic (most bsd-ish systems with sys5 compatibility)"
	@echo "	386bsd aix alpha apollo convex-ultranet dell-gcc dgux dynix"
	@echo "	epix esix freebsd hpux irix4 isc(untested) linux"
	@echo "	mips(untested) netbsd next osf ptx-2.x ptx-generic pyramid"
	@echo "	sco sco-nis sco-od2 sunos4 sunos40 sunos5 sysv4 ultrix"
	@echo "	unicos(untested) uxp"
	@echo
	@echo "If none of these match your environment, edit the system"
	@echo "dependencies sections in the Makefile and do a 'make other'."
	@echo

#######################################################
# Choice between easy and advanced installation recipe.
# 
# Advanced installation: vendor-provided daemons are left alone, and the
# inetd configuration file is edited. In this case, the REAL_DAEMON_DIR
# macro should reflect the actual directory with (most of) your
# vendor-provided network daemons.  These names can be found in the
# inetd.conf file. Usually, the telnet, ftp and finger daemons all live
# in the same directory.
# 
# Uncomment the appropriate line if you are going to edit inetd.conf.
#
# Ultrix 4.x SunOS 4.x ConvexOS 10.x Dynix/ptx
#REAL_DAEMON_DIR=/usr/etc
#
# SysV.4 Solaris 2.x OSF AIX
#REAL_DAEMON_DIR=/usr/sbin
#
# BSD 4.4
#REAL_DAEMON_DIR=/usr/libexec
#
# HP-UX SCO
#REAL_DAEMON_DIR=/etc

# Easy installation: vendor-provided network daemons are moved to "some
# other" directory, and the tcpd wrapper fills in the "holes". For this
# mode of operation, the REAL_DAEMON_DIR macro should be set to the "some
# other" directory.  The "..." is here for historical reasons only; you
# should probably use some other name. 
# 
# Uncomment the appropriate line if you are going to move your daemons.
#
# Ultrix 4.x SunOS 4.x ConvexOS 10.x Dynix/ptx
#REAL_DAEMON_DIR=/usr/etc/...
#
# SysV.4 Solaris 2.x OSF AIX
#REAL_DAEMON_DIR=/usr/sbin/...
#
# BSD 4.4
#REAL_DAEMON_DIR=/usr/libexec/...
#
# HP-UX SCO
#REAL_DAEMON_DIR=/etc/...

# End of mandatory section
##########################

##########################################
# Ready-to-use system-dependent templates.
#
# Ready-to-use templates are available for many systems (see the "echo"
# commands at the start of this Makefile).  The templates take care of
# all system dependencies: after editing the REAL_DAEMON_DIR definition
# above, do a "make sunos4" (or whatever system type is appropriate).
#
# If your system is not listed (or something that comes close enough), you
# will have to edit the system depencies section below and do a "make other".  
#
# Send templates for other UNIX versions to wietse@wzv.win.tue.nl.

# This is good for many BSD+SYSV hybrids with NIS (formerly YP).
generic sunos4 aix osf alpha dynix:
	@make REAL_DAEMON_DIR=$(REAL_DAEMON_DIR) STYLE=$(STYLE) \
	LIBS= RANLIB=ranlib ARFLAGS=rv AUX_OBJ=environ.o \
	NETGROUP=-DNETGROUP TLI= all

# The NeXT loader needs "-m" or it barfs on redefined library functions.
next:
	@make REAL_DAEMON_DIR=$(REAL_DAEMON_DIR) STYLE=$(STYLE) \
	LIBS=-m RANLIB=ranlib ARFLAGS=rv AUX_OBJ=environ.o \
	NETGROUP=-DNETGROUP TLI= all

# SunOS for the 386 was frozen at release 4.0.x.
sunos40:
	@make REAL_DAEMON_DIR=$(REAL_DAEMON_DIR) STYLE=$(STYLE) \
	LIBS= RANLIB=ranlib ARFLAGS=rv AUX_OBJ="environ.o strcasecmp.o" \
	NETGROUP=-DNETGROUP TLI= all

# Ultrix is like sunos4, aix, next, etc., but has miscd, too.
ultrix:
	@make REAL_DAEMON_DIR=$(REAL_DAEMON_DIR) STYLE=$(STYLE) \
	LIBS= RANLIB=ranlib ARFLAGS=rv AUX_OBJ=environ.o \
	NETGROUP=-DNETGROUP TLI= all miscd

# This works on EP/IX 1.4.3 and will likely work on Mips (reggers@julian.uwo.ca)
epix:
	@make REAL_DAEMON_DIR=$(REAL_DAEMON_DIR) STYLE=$(STYLE) \
	LIBS= RANLIB=ranlib ARFLAGS=rv AUX_OBJ=environ.o \
	NETGROUP=-DNETGROUP TLI= SYSTYPE="-systype bsd43" all

# Free bsd and linux by default have no NIS.
386bsd netbsd freebsd:
	@make REAL_DAEMON_DIR=$(REAL_DAEMON_DIR) STYLE=$(STYLE) \
	LIBS= RANLIB=ranlib ARFLAGS=rv AUX_OBJ= NETGROUP= TLI= all

linux:
	@make REAL_DAEMON_DIR=$(REAL_DAEMON_DIR) STYLE=$(STYLE) \
	LIBS= RANLIB=ranlib ARFLAGS=rv AUX_OBJ=environ.o \
	NETGROUP= TLI= EXTRA_CFLAGS="-DBROKEN_SO_LINGER" all

# This is good for many SYSV+BSD hybrids with NIS, probably also for HP-UX 7.x.
hpux hpux8 hpux9:
	@make REAL_DAEMON_DIR=$(REAL_DAEMON_DIR) STYLE=$(STYLE) \
	LIBS= RANLIB=echo ARFLAGS=rv AUX_OBJ=environ.o \
	NETGROUP=-DNETGROUP TLI= all

# ConvexOS-10.x with UltraNet support (ukkonen@csc.fi).
convex-ultranet:
	@make REAL_DAEMON_DIR=$(REAL_DAEMON_DIR) STYLE=$(STYLE) \
	LIBS=-lulsock RANLIB=ranlib ARFLAGS=rv AUX_OBJ=environ.o \
	NETGROUP=-DNETGROUP TLI= all

# Generic support for the Dynix/PTX version of TLI.
ptx-generic:
	@make REAL_DAEMON_DIR=$(REAL_DAEMON_DIR) STYLE=$(STYLE) \
	LIBS="-lsocket -linet -lnsl" RANLIB=echo ARFLAGS=rv \
	AUX_OBJ="environ.o strcasecmp.o ptx.o" NETGROUP= TLI=-DPTX all

# With UDP support optimized for PTX 2.x (timw@sequent.com).
ptx-2.x:
	@make REAL_DAEMON_DIR=$(REAL_DAEMON_DIR) STYLE=$(STYLE) \
	LIBS="-lsocket -linet -lnsl" RANLIB=echo ARFLAGS=rv \
	AUX_OBJ="environ.o strcasecmp.o tli-sequent.o" NETGROUP= \
	TLI=-DTLI_SEQUENT all

# IRIX 4.0.x has a special ar(1) flag.
irix4:
	@make REAL_DAEMON_DIR=$(REAL_DAEMON_DIR) STYLE=$(STYLE) \
	LIBS=-lsun RANLIB=echo ARFLAGS=rvs AUX_OBJ=environ.o \
	NETGROUP=-DNETGROUP TLI= all

# SunOS 5.x is close enough to generic System V.4
sunos5 esix sysv4:
	@make REAL_DAEMON_DIR=$(REAL_DAEMON_DIR) STYLE=$(STYLE) \
	LIBS="-lsocket -lnsl" RANLIB=echo ARFLAGS=rv \
	NETGROUP=-DNETGROUP AUX_OBJ=environ.o TLI=-DTLI all

# DG/UX 5.4.1 is like SYSV.4 but slightly different.
dgux:
	@make REAL_DAEMON_DIR=$(REAL_DAEMON_DIR) STYLE=$(STYLE) \
	LIBS=-lnsl RANLIB=echo ARFLAGS=rv \
	NETGROUP=-DNETGROUP AUX_OBJ=environ.o TLI=-DTLI \
	BUGS="$(BUGS) -DINET_ADDR_BUG" all

# UXP/DS System V.4 clone (vic@uida0.uida.es).
uxp:
	@make LIBS="-L/usr/ucblib -lsocket -lnsl -lucb" \
	RANLIB=echo ARFLAGS=rv NETGROUP=-DNETGROUP \
	AUX_OBJ=environ.o TLI="-DTLI -DDRS_XTI" all

# DELL System V.4 Issue 2.2 using gcc (kim@grendel.lut.fi, jurban@norden1.com)
dell-gcc:
	@make LIBS="-lsocket -lnsl" RANLIB=ranlib ARFLAGS=rv CC=gcc \
	AUX_OBJ="environ.o strcasecmp.o" TLI=-DTLI all

# SCO 3.2v4.1 no frills (jedwards@sol1.solinet.net).
sco:
	@make REAL_DAEMON_DIR=$(REAL_DAEMON_DIR) STYLE=$(STYLE) \
	LIBS="-lsocket -lnsl_s" RANLIB=echo ARFLAGS=rv \
	NETGROUP= AUX_OBJ=environ.o TLI= all

# SCO OpenDesktop 2.0, release 3.2 (peter@midnight.com).
sco-od2:
	@make REAL_DAEMON_DIR=$(REAL_DAEMON_DIR) STYLE=$(STYLE) \
	LIBS="-lrpcsvc -lrpc -lyp -lrpc -lrpcsvc -lsocket" \
	RANLIB=echo ARFLAGS=rv AUX_OBJ=environ.o \
	NETGROUP=-DNETGROUP TLI= all

# SCO 3.2v4.2 with TCP/IP 1.2.1 (Eduard.Vopicka@vse.cz)
sco-nis:
	@make REAL_DAEMON_DIR=$(REAL_DAEMON_DIR) STYLE=$(STYLE) \
	LIBS="-lyp -lrpc -lsocket -lyp -lc_s -lc" \
	RANLIB=echo ARFLAGS=rv AUX_OBJ=environ.o \
	NETGROUP=-DNETGROUP TLI= EXTRA_CFLAGS="-nointl" all

# Domain SR10.4. Build under bsd, run under either sysv3 or bsd43.
apollo:
	@make REAL_DAEMON_DIR=$(REAL_DAEMON_DIR) STYLE=$(STYLE) \
	LIBS= RANLIB=ranlib ARFLAGS=rv AUX_OBJ=environ.o \
	NETGROUP=-DNETGROUP TLI= SYSTYPE="-A run,any -A sys,any" all

# Pyramid OSx 5.1, using the BSD universe.
pyramid:
	@make REAL_DAEMON_DIR=$(REAL_DAEMON_DIR) STYLE=$(STYLE) \
	LIBS= RANLIB=ranlib ARFLAGS=rv AUX_OBJ="environ.o vfprintf.o" \
	STRINGS="-Dstrchr=index -Dstrrchr=rindex -Dmemcmp=bcmp -Dno_memcpy" \
	NETGROUP="-DNETGROUP -DUSE_GETDOMAIN" TLI= all

# Untested.
mips:
	@echo "Warning: some definitions may be wrong."
	make REAL_DAEMON_DIR=$(REAL_DAEMON_DIR) STYLE=$(STYLE) \
	LIBS= RANLIB=ranlib ARFLAGS=rv AUX_OBJ=environ.o \
	NETGROUP=-DNETGROUP TLI= SYSTYPE="-sysname bsd43" all

# Untested.
isc:
	@echo "Warning: some definitions may be wrong."
	make REAL_DAEMON_DIR=$(REAL_DAEMON_DIR) STYLE=$(STYLE) \
	LIBS="-linet -lnsl_s -ldbm" RANLIB=echo ARFLAGS=rv \
	AUX_OBJ="environ.o strcasecmp.o" NETGROUP= TLI= all

# Untested. Borrow me a Cray?
unicos:
	@echo "Warning: some definitions may be wrong."
	make REAL_DAEMON_DIR=$(REAL_DAEMON_DIR) STYLE=$(STYLE) \
	LIBS=-lnet RANLIB=echo ARFLAGS=rv \
	AUX_OBJ="environ.o strcasecmp.o" NETGROUP= TLI= all

###############################################################
# System dependencies: TLI (transport-level interface) support.
# 
# Uncomment the following macro if your system has System V.4-style TLI
# support (/usr/include/sys/timod.h, /etc/netconfig, and the netdir(3)
# routines).
#
#TLI	= -DTLI

###############################################################################
# System dependencies: differences between ranlib(1) and ar(1) implementations.
#
# Some C compilers (Ultrix 4.x) insist that ranlib(1) be run on an object
# library; some don't care as long as the modules are in the right order;
# some systems don't even have a ranlib(1) command. Make your choice.

RANLIB	= ranlib	# have ranlib (BSD-ish UNIX)
#RANLIB	= echo		# no ranlib (SYSV-ish UNIX)

ARFLAGS	= rv		# most systems
#ARFLAGS= rvs		# IRIX 4.0.x

#############################################################################
# System dependencies: routines that are not present in the system libraries.
# 
# If your system library does not have set/putenv(), strcasecmp() or strtok(),
# use the ones provided with this source distribution. The environ.c module
# implements setenv(), getenv(), and putenv().

AUX_OBJ= environ.o
#AUX_OBJ= environ.o strcasecmp.o
#AUX_OBJ= environ.o strcasecmp.o strtok.o

# Uncomment the following if your C library does not provide the
# strchr/strrchr/memcmp routines, but comes with index/rindex/bcmp.  If
# that is the case, you probably also do not have strtok() (see above).
#
#STRINGS= -Dstrchr=index -Dstrrchr=rindex -Dmemcmp=bcmp -Dno_memcpy

#################################################################
# System dependencies: selection of non-default object libraries.
#
# Most System V implementations require that you explicitly specify the
# networking libraries. There is no general consensus, though.
#
#LIBS	= -lsocket -lnsl		# SysV.4 Solaris 2.x
#LIBS	= -lsun				# IRIX
#LIBS	= -lsocket -linet -lnsl -lnfs	# PTX
#LIBS	= -linet -lnsl_s -ldbm		# ISC
#LIBS	= -lnet				# Unicos
#LIBS	= -linet -lsyslog -ldbm
#LIBS	= -lsyslog -lsocket -lnsl

######################################################
# System dependencies: system-specific compiler flags.
#
# Apollo Domain/OS offers both bsd and sys5 environments, sometimes
# on the same machine.  If your Apollo is primarily sys5.3 and also
# has bsd4.3, uncomment the following to build under bsd and run under
# either environment.
#
#SYSTYPE= -A run,any -A sys,any

# For MIPS RISC/os 4_52.p3, uncomment the following definition.
#
#SYSTYPE= -sysname bsd43

##################################################
# System dependencies: working around system bugs.
#
# -DGETPEERNAME_BUG works around a getpeername(2) bug in some versions of
# Apollo or SYSV.4 UNIX:  the wrapper would report that all UDP requests
# come from address 0.0.0.0. The workaround does no harm on other systems.
#
# -DBROKEN_FGETS works around an fgets(3) bug in some System V versions
# (IRIX):  fgets() gives up too fast when reading from a network socket.
# The workaround does no harm on other systems.
#
# DG/UX 5.4.1 comes with an inet_ntoa() function that returns a structure
# instead of a long integer. Compile with -DINET_ADDR_BUG to work around
# this mutant behavour.

BUGS = -DGETPEERNAME_BUG -DBROKEN_FGETS
#BUGS = -DGETPEERNAME_BUG -DBROKEN_FGETS -DINET_ADDR_BUG

##########################################################################
# System dependencies: whether or not your system has NIS (or YP) support.
#
# If your system supports NIS or YP-style netgroups, enable the following
# macro definition. Netgroups are used only for host access control.
#
#NETGROUP= -DNETGROUP

# End of the system dependencies.
#################################

##############################
# Start of the optional stuff.

###########################################
# Optional: Turning on language extensions
#
# Instead of the default access control language that is documented in
# the hosts_access.5 document, the wrappers can be configured to
# implement an extensible language documented in the hosts_options.5
# document.  This language is implemented by the "options.c" source
# module, which also gives hints on how to add your own extensions.
# Uncomment the next definition to turn on the language extensions.
# 
#STYLE	= -DPROCESS_OPTIONS	# Enable language extensions.

################################################################
# Optional: Changing the default disposition of logfile records
#
# By default, logfile entries are written to the same file as used for
# sendmail transaction logs. See your /etc/syslog.conf file for actual
# path names of logfiles. The tutorial section in the README file
# gives a brief introduction to the syslog daemon.
# 
# Change the FACILITY definition below if you disagree with the default
# disposition. Some syslog versions (including Ultrix 4.x) do not provide
# this flexibility.
# 
# If nothing shows up on your system, it may be that the syslog records
# are sent to a dedicated loghost. It may also be that no syslog daemon
# is running at all. The README file gives pointers to surrogate syslog
# implementations for systems that have no syslog library routines or
# no syslog daemons. When changing the syslog.conf file, remember that
# there must be TABs between fields.
#
# The LOG_XXX names below are taken from the /usr/include/syslog.h file.

FACILITY= LOG_MAIL	# LOG_MAIL is what most sendmail daemons use

# The syslog priority at which successful connections are logged.

SEVERITY= LOG_INFO	# LOG_INFO is normally not logged to the console

###########################
# Optional: Reduce DNS load
#
# When looking up the address for a host.domain name, the typical DNS
# server will first append substrings of your own domain, so it tries
# host.domain.your.own.domain, then host.domain.own.domain, and then
# host.domain. The APPEND_DOT feature stops this waste of cycles. It is
# off by default because it causes problems on sites that don't use DNS.
#
# This feature has undergone light testing and it may go away if there
# are problems with it. If you enable it let me know if it helps.
#
# DOT= -DAPPEND_DOT

##################################################
# Optional: Always attempt remote username lookups
#
# By default, the wrappers look up the remote username only when the
# access control rules require them to do so.
#
# Username lookups require that the remote host runs a daemon that
# supports an RFC 931 like protocol.  Remote user name lookups are not
# possible for UDP-based connections, and can cause noticeable delays
# with connections from non-UNIX PCs.  On some systems, remote username
# lookups can trigger a kernel bug, causing loss of service. The README
# file describes how to find out if your UNIX kernel has that problem.
# 
# Uncomment the following definition if the wrappers should always
# attempt to get the remote user name. If this is not enabled you can
# still do selective username lookups as documented in the hosts_access.5
# and hosts_options.5 manual pages (`nroff -man' format).
#
#AUTH	= -DALWAYS_RFC931
#
# The default username lookup timeout is 10 seconds. This may not be long
# enough for slow hosts or networks, but is enough to irritate PC users.

RFC931_TIMEOUT = 10

######################################################
# Optional: Changing the default file protection mask
#
# On many systems, network daemons and other system processes are started
# with a zero umask value, so that world-writable files may be produced.
# It is a good idea to edit your /etc/rc* files so that they begin with
# an explicit umask setting.  On our site we use `umask 022' because it
# does not break anything yet gives adequate protection against tampering.
# 
# The following macro specifies the default umask for processes run under
# control of the daemon wrappers. Comment it out only if you are certain
# that inetd and its children are started with a safe umask value.

UMASK	= -DDAEMON_UMASK=022

#######################################
# Optional: Turning off access control
#
# By default, host access control is enabled.  To disable host access
# control, comment out the following definition.  Host access control
# can also be turned off at runtime by providing no or empty access
# control tables.

ACCESS	= -DHOSTS_ACCESS

########################################################
# Optional: Changing the access control table pathnames
#
# The HOSTS_ALLOW and HOSTS_DENY macros define where the programs will
# look for access control information. Watch out for the quotes and
# backslashes when you make changes.

TABLES	= -DHOSTS_DENY=\"/etc/hosts.deny\" -DHOSTS_ALLOW=\"/etc/hosts.allow\"

###########################################
# Optional: Turning off host NAME checking
#
# By default, the software tries to protect against hosts that claim to
# have someone elses host name. This is relevant for network services
# whose authentication depends on host names, such as rsh and rlogin.
#
# With paranoid mode on, connections will be rejected when the host name
# does not match the host address. Connections will also be rejected when
# the host name is available but cannot be verified.
#
# Comment out the following definition if you do not need this additional
# protection. If paranoid mode is off, and a host name check fails, the
# daemon wrappers will use only the host address, but your daemons may
# still use the host name.

PARANOID= -DPARANOID

#############################################
# Optional: Turning on host ADDRESS checking
#
# Optionally, the software tries to protect against hosts that pretend to
# have someone elses host address. This is relevant for network services
# whose authentication depends on host names, such as rsh and rlogin,
# because the network address is used to look up the remote host name.
# 
# The protection is effective only when the offending host claims to have
# a network address that lies outside its own network.
#
# My site has been running rlogind and rshd daemons that implement this
# feature for more than 2 years, and without any ill effects.
#
# This feature cannot be used with SunOS 4.x because of a kernel bug in
# the implementation of the getsockopt() system call. Kernel panics have
# been observed for SunOS 4.1.[1-3]. Symptoms are "BAD TRAP" and "Data
# fault" while executing the tcp_ctloutput() kernel function.
#
# Reportedly, Sun patch 100804-03 fixes the problem for SunOS 4.1.x.
#
# Uncomment the following macro definition if your getsockopt() is OK.
#
# KILL_OPT= -DKILL_IP_OPTIONS

## End configuration options
############################

# Protection against weird shells or weird make programs.

SHELL	= /bin/sh
.c.o:;	$(CC) $(CFLAGS) -c $*.c

CFLAGS	= -O -DFACILITY=$(FACILITY) $(ACCESS) $(PARANOID) $(NETGROUP) \
	$(BUGS) $(SYSTYPE) $(AUTH) $(UMASK) \
	-DREAL_MISCD=\"$(REAL_DAEMON_DIR)/miscd\" \
	-DREAL_DAEMON_DIR=\"$(REAL_DAEMON_DIR)\" $(STYLE) $(KILL_OPT) \
	-DSEVERITY=$(SEVERITY) -DRFC931_TIMEOUT=$(RFC931_TIMEOUT) \
	$(UCHAR) $(TABLES) $(STRINGS) $(TLI) $(EXTRA_CFLAGS) $(DOT)

LIB_OBJ= hosts_access.o options.o shell_cmd.o rfc931.o hosts_info.o \
	hosts_ctl.o refuse.o percent_x.o clean_exit.o $(AUX_OBJ) \
	fromhost.o fix_options.o socket.o tli.o workarounds.o init_client.o \
	6compat.o

KIT	= README miscd.c tcpd.c fromhost.c hosts_access.c shell_cmd.c \
	log_tcp.h try.c refuse.c Makefile hosts_access.5 strcasecmp.c \
	BLURB rfc931.c tcpd.8 hosts_info.c hosts_access.3 hosts_ctl.c \
	percent_x.c options.c clean_exit.c environ.c patchlevel.h strtok.c \
	fix_options.c workarounds.c socket.c tli.c DISCLAIMER \
	fakelog.c safe_finger.c hosts_options.5 options.h CHANGES \
	try-from.c init_client.c 6compat.c ptx.c vfprintf.c tli-sequent.c \
	tli-sequent.h

LIB	= libwrap.a

all other: config-check tcpd try try-from safe_finger

config-check:
	@set +e; test -n "$(REAL_DAEMON_DIR)" || { make; exit 1; }

$(LIB):	$(LIB_OBJ)
	rm -f $(LIB)
	ar $(ARFLAGS) $(LIB) $(LIB_OBJ)
	-$(RANLIB) $(LIB)

tcpd:	tcpd.o fromhost.o $(LIB)
	$(CC) $(CFLAGS) -o $@ tcpd.o fromhost.o $(LIB) $(LIBS)

miscd:	miscd.o fromhost.o $(LIB)
	$(CC) $(CFLAGS) -o $@ miscd.o fromhost.o $(LIB) $(LIBS)

safe_finger:
	$(CC) $(CFLAGS) -o $@ safe_finger.c $(LIB) $(LIBS)

try:	try.o fakelog.o $(LIB)
	$(CC) $(CFLAGS) -o $@ try.o fakelog.o $(LIB) $(LIBS)

try-from: try-from.c fakelog.o log_tcp.h Makefile $(LIB)
	$(CC) $(CFLAGS) -DTEST -o $@ try-from.c fakelog.o $(LIB) $(LIBS)

shar:	$(KIT)
	@shar $(KIT)

kit:	$(KIT)
	@makekit $(KIT)

archive:
	$(ARCHIVE) $(KIT)

clean:
	rm -f tcpd miscd safe_finger try try-from *.[oa] core

# Enable all bells and whistles for linting.

lint: tcpd_lint miscd_lint try_lint

tcpd_lint:
	lint -DFACILITY=LOG_MAIL -DHOSTS_ACCESS -DPARANOID -DNETGROUP \
	-DGETPEERNAME_BUG -DDAEMON_UMASK=022 -DSEVERITY=$(SEVERITY) \
	$(TABLES) -DKILL_IP_OPTIONS -DPROCESS_OPTIONS \
	-DRFC931_TIMEOUT=$(RFC931_TIMEOUT) -DALWAYS_RFC931 \
	-DREAL_DAEMON_DIR=\"$(REAL_DAEMON_DIR)\" \
	tcpd.c fromhost.c socket.c tli.c hosts_access.c shell_cmd.c \
	refuse.c rfc931.c hosts_info.c percent_x.c clean_exit.c options.c \
	environ.c fix_options.c workarounds.c init_client.c

miscd_lint:
	lint -DFACILITY=LOG_MAIL -DHOSTS_ACCESS -DPARANOID -DNETGROUP \
	-DGETPEERNAME_BUG -DDAEMON_UMASK=022 -DSEVERITY=$(SEVERITY) \
	$(TABLES) -DKILL_IP_OPTIONS -DPROCESS_OPTIONS \
	-DRFC931_TIMEOUT=$(RFC931_TIMEOUT) -DALWAYS_RFC931 \
	-DREAL_MISCD=\"$(REAL_DAEMON_DIR)/miscd\" \
	miscd.c fromhost.c socket.c tli.c hosts_access.c shell_cmd.c \
	refuse.c rfc931.c hosts_info.c percent_x.c clean_exit.c options.c \
	environ.c fix_options.c workarounds.c init_client.c

try_lint:
	lint -DFACILITY=LOG_MAIL -DSEVERITY=$(SEVERITY) -DHOSTS_ACCESS \
	$(TABLES) -DNETGROUP -DPROCESS_OPTIONS -DRFC931_TIMEOUT=10 \
	try.c hosts_ctl.c hosts_access.c hosts_info.c percent_x.c options.c \
	workarounds.c init_client.c

printfck:
	printfck -f printf.ck tcpd.c fromhost.c socket.c tli.c hosts_access.c shell_cmd.c \
	refuse.c rfc931.c hosts_info.c percent_x.c clean_exit.c options.c \
	environ.c fix_options.c workarounds.c init_client.c >aap.c
	lint -DFACILITY=LOG_MAIL -DHOSTS_ACCESS -DPARANOID -DNETGROUP \
	-DGETPEERNAME_BUG -DDAEMON_UMASK=022 -DSEVERITY=$(SEVERITY) \
	$(TABLES) -DKILL_IP_OPTIONS -DPROCESS_OPTIONS \
	-DRFC931_TIMEOUT=$(RFC931_TIMEOUT) -DALWAYS_RFC931 \
	-DREAL_DAEMON_DIR=\"$(REAL_DAEMON_DIR)\" aap.c
	rm -f aap.c

# Compilation dependencies.

clean_exit.o: log_tcp.h Makefile
environ.o: Makefile
fix_options.o: log_tcp.h Makefile
fromhost.o: log_tcp.h Makefile
hosts_access.o: options.h log_tcp.h Makefile
hosts_ctl.o: log_tcp.h Makefile
hosts_info.o: log_tcp.h Makefile
init_client.o: log_tcp.h Makefile
miscd.o: patchlevel.h log_tcp.h Makefile
options.o: options.h log_tcp.h Makefile
percent_x.o: log_tcp.h Makefile
refuse.o: log_tcp.h Makefile
rfc931.o: log_tcp.h Makefile
shell_cmd.o: log_tcp.h Makefile
socket.o: log_tcp.h Makefile
tcpd.o: patchlevel.h log_tcp.h Makefile
tli.o: log_tcp.h Makefile
tli-sequent.o: tli-sequent.h log_tcp.h Makefile
try-from: log_tcp.h Makefile
try.o: log_tcp.h Makefile
workarounds.o: log_tcp.h Makefile
