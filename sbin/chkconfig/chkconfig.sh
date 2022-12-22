#!/bin/sh

#	BSDI	chkconfig.sh,v 2.1 1995/02/03 07:12:28 polk Exp

PATH=/bin:/usr/bin:/sbin:/usr/sbin

dmesg | sed -e 's/ \[got .* ticks\]//' > /tmp/ck$$.dmesg

old=/var/db/dmesg.boot
new=/tmp/ck$$.dmesg

if [ -r $old ]
then
	grep -v 'delay multiplier' $old > /tmp/ck$$.1
	grep -v 'delay multiplier' $new > /tmp/ck$$.2

	cat $old $new | awk '/^delay multiplier / {
		if (oldm == 0) {
			oldm = $3;
		} else {
			newm = $3;
			if (oldm != 0) {
				pct = (newm - oldm) * 100.0 / oldm;
				if (pct < 0)
					pct = -pct;
				if (pct >= 5)
					printf ("delay multiplier %d -> %d\n",
						oldm, newm);
			}
		
		}
	}' > /tmp/ck$$.3

	diff /tmp/ck$$.1 /tmp/ck$$.2 >> /tmp/ck$$.3

	if [ -s /tmp/ck$$.3 ]
	then
		(echo "Configuration changed:"; cat /tmp/ck$$.3) > /tmp/ck$$.4
		cat /tmp/ck$$.4
		logger < /tmp/ck$$.4
	fi
fi

mv $new $old
chmod 644 $old

rm -f /tmp/ck$$.*

exit 0
