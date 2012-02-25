#!/bin/bash
#
# $Header: /CVSROOT/scylla-charybdis/example/backuploophelper.sh,v 1.6 2003/11/06 07:02:30 tino Exp $
#
# This is a helper if you want to speed up backuploop.
# Just run this on the directory in parallel to backuploop.
#
# Needs the helper read-nonblocking in the directory it resides.
# (Sorry, cannot be implemented in bash easily)
#
# $Log: backuploophelper.sh,v $
# Revision 1.6  2003/11/06 07:02:30  tino
# still usage
#
# Revision 1.5  2003/11/06 07:01:03  tino
# usage corrected
#
# Revision 1.4  2003/11/04 03:22:19  tino
# Now it counts ups etc. and uses waitconnect like backuploop.sh
#
# Revision 1.3  2003/10/30 19:55:27  tino
# Seems to be ready for deploy
#
# Revision 1.2  2003/10/29 23:57:03  tino
# Common parts combined.  Calling syntax unified.  TEST VERSION!
#
# Revision 1.1  2003/10/27 20:41:30  tino
# Helper to backuploop to spawn several charybdis in parallel
# to speed up processing.  Only for real fast LAN connects.
#

HELPER="`dirname "$0"`/read-nonblocking"
if [ ! -x "$HELPER" ]
then
	echo "Cannot find helper $HELPER" >&2
	exit 1
fi

usage()
{
echo "Usage: echo Password |
       `basename "$0"` hostname:port[@throttle[:blocksize[:timeout]]] backuptimestamp parcount" >&2
exit 1
}

false; . "`dirname "$0"`/backuploopcommon.inc" || exit

[ 3 != "$#" ] && usage

setupbackuploop "$@"
checkdirs "$run" "$ME"

PARALLEL="$3"
MAXPARALLEL="`tput lines`"
let MAXPARALLEL--
if ! [ 1 -le "$PARALLEL" ] || [ "$MAXPARALLEL" -lt "$PARALLEL" ]
then
	echo "Parallel value $PARALLEL out of range (1..$MAXPARALLEL)" >&2
	exit 1
fi

tput clear
updatestats()
{
having=0
secs=0
while echo -n "   $secs "
do
	i=0
	missing=""
	while [ $i -lt $PARALLEL ]
	do
		d="${data[$i]}`$HELPER <&$((i+100))`"
		if [ 0 != $? ]
		then
			[ -n "$missing" ] || missing=$i
		else
			let having++
		fi
		data[$i]="${d//*}"
		tput cup $i 0
		printf "%2d %s: %s      " $i "${procs[$i]}" "${data[$i]}" || break
		let i=i+1
	done 2>/dev/null
	tput cup $i 0
	[ -n "$missing" ] && break
	let secs++
	sleep 1
done
[ 0 -lt $having ]
}

typeset -a procs
typeset -a data

oks=0
ups=0
xxs=0
unk=0
cnt=0
find "$run.tmp" -type f -print |
sort -r |
{
missing=0
while read a
do
	[ -f "$a" ] || continue

	let cnt++
	case "${data[$missing]//
}" in
	*\ up\ )	let ups++;;
	*\ xx\ )	let xxs++;;
	*\ ok\ )	let oks++;;
	'')	;;
	*)	let unk++;;
	esac
	data[$missing]=""
	procs[$missing]="`basename "$a"`"

	waitconnect

	find "$run.tmp" -type f -print | sort |
	(
	read b
	read c
	read d
	echo -n "        left `wc -l` a `wc -l < "$d"` files, run=$cnt ok=$oks up=$ups xx=$xxs ??=$unk "
	[ ".$d" != ".$a" ] &&
	[ ".$c" != ".$a" ] &&
	[ ".$b" != ".$a" ]
	) || break

	[ -f "$a" ] || continue

#	Beware (at least on my system):
#	FD10 is always open and points to stdin
	eval "exec $((missing+100))< <(runcharybdis \"\$a\")"

	updatestats || break
done

while	updatestats
do
	echo -n "        finish $having "
	sleep 1
done
}

echo
