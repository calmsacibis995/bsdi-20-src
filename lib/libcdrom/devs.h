/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI devs.h,v 2.1 1995/02/03 06:56:39 polk Exp
 */

/*
 * This file defines functions that are private to libcdrom.
 */

#include <sys/cdefs.h>

struct cdfuncs {
	char	*name;
	struct	cdinfo __P(*(*probe)(int, char *, char *, char *, int));
	int	__P((*close)(struct cdinfo *));
	int	__P((*play)(struct cdinfo *, int, int));
	int	__P((*stop)(struct cdinfo *));
	int	__P((*status)(struct cdinfo *, struct cdstatus *));
	int	__P((*eject)(struct cdinfo *));
	int	__P((*volume)(struct cdinfo *, int));
};

struct	cdinfo *scsi2_probe __P((int, char *, char *, char *, int));
int	scsi2_close __P((struct cdinfo *));
int	scsi2_play __P((struct cdinfo *, int, int));
int	scsi2_stop __P((struct cdinfo *));
int	scsi2_status __P((struct cdinfo *, struct cdstatus *));
int	scsi2_eject __P((struct cdinfo *));
int	scsi2_volume __P((struct cdinfo *, int));

struct cdinfo *panasonic_probe __P((int, char *, char *, char *, int));
int	panasonic_close __P((struct cdinfo *));
int	panasonic_play __P((struct cdinfo *, int, int));
int	panasonic_stop __P((struct cdinfo *));
int	panasonic_status __P((struct cdinfo *, struct cdstatus *));
int	panasonic_eject __P((struct cdinfo *));
int	panasonic_volume __P((struct cdinfo *, int));

struct cdinfo *toshiba_probe __P((int, char *, char *, char *, int));
int	toshiba_close __P((struct cdinfo *));
int	toshiba_play __P((struct cdinfo *, int, int));
int	toshiba_stop __P((struct cdinfo *));
int	toshiba_status __P((struct cdinfo *, struct cdstatus *));
int	toshiba_eject __P((struct cdinfo *));
int	toshiba_volume __P((struct cdinfo *, int));

struct cdinfo *mitsumi_probe __P((int, char *, char *, char *, int));
int	mitsumi_close __P((struct cdinfo *));
int	mitsumi_play __P((struct cdinfo *, int, int));
int	mitsumi_stop __P((struct cdinfo *));
int	mitsumi_status __P((struct cdinfo *, struct cdstatus *));
int	mitsumi_eject __P((struct cdinfo *));
int	mitsumi_volume __P((struct cdinfo *, int));

struct cdinfo *make_cdinfo __P((int, int));
int	scsi_inquiry __P((int, char *, char *, char *, int *));
int	scsi_cmd __P((int, char *, char *, int));
int	scsi_cmd_write __P((int, char *, char *, int));

int	bcd_to_int __P((int));
int	int_to_bcd __P((int));
