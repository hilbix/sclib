#!/bin/bash
#
# $Header: /CVSROOT/scylla-charybdis/example/backuploop.sh,v 1.10 2003/11/06 23:12:29 tino Exp $
#
# $Log: backuploop.sh,v $
# Revision 1.10  2003/11/06 23:12:29  tino
# Missed a few more runts (rants?)
#
# Revision 1.9  2003/11/06 23:09:01  tino
# forgot to shift away fist argument (host)
# so find printed an error (which can be ignored)
#
# Revision 1.8  2003/11/06 07:02:30  tino
# still usage
#
# Revision 1.7  2003/11/06 07:01:03  tino
# usage corrected
#
# Revision 1.6  2003/11/06 04:32:53  tino
# $run.ko added for a special measure:
# If you want to restart a broken backuploop,
# just move files with *only* erronous lines from $run.tmp into $run.ko,
# then rerun backuploop.sh to skip this filenames.
# Needed this as Linux allows ? and * in filenames, Windows shares don't.
# (Yes, I backup Linux to Windows shares with S&C, don't ask)
#
# Revision 1.5  2003/11/05 08:45:03  tino
# A "little" bug fixed:  When too many files are present,
# the loop did never finish
#
# Revision 1.4  2003/10/29 23:57:03  tino
# Common parts combined.  Calling syntax unified.  TEST VERSION!
#
# Revision 1.3  2003/10/25 04:32:48  tino
# Changed the split multiplicator from 2 to 3/2
# Additionally switch sorting from normal to reverse on each second run.
# This is to elliminate (slowly) files with case insensitivity problems
# on drives which are case insensitive.
#

# Type of filesystems to backup
FILESYSTEMS="reiserfs ext3 ext2"

usage()
{
cat <<EOF
Usage: echo Password |
       `basename "$0"` hostname:port[@throttle[:blocksize[:timeout]]] backuptimestamp|path ...
Examples:
	echo PW | `basename "$0"` host:port /
		start backup from all data
	echo PW | `basename "$0"` host:port $runs
		restart a backup with timestamp $runs
		(look into $rundir to find valid timestamps)
EOF
exit 1
}

false; . "`dirname "$0"`/backuploopcommon.inc" || exit

setupbackuploop "$@"

if [ 2 != $# -o -z "$run" ]
then
	mkdir -p "$rundir" 2>/dev/null || exit
	run="$rundir$runs"
	mkdir "$run.ok" "$run.fail" "$run.tmp" "$run.odysseus" || exit

	if [ ! -L "$ME" ]
	then
		ln -fs / "$ME" 2>/dev/null
	fi

	notfs=""
	for a in $FILESYSTEMS
	do
		notfs="$notfs ! -fstype $a"
	done
	shift
	for a
	do
		# Create dummy odysseus files such that this files are backed up
		b="${a//\//-}"
		>>"$run.odysseus/odysseus.$b"
		>>"$run.odysseus/log.$b"
		find "$ME$a" "$ME$run.odysseus" -type f -print -o $notfs -prune
	done |
	sort -u > "$run.files"

	# As odysseus has a fundamentally different approach to find we run it afterwards ..
	for a
	do
		b="${a//\//-}"
		odysseus "$a" >>"$run.odysseus/odysseus.$b" 2>>"$run.odysseus/log.$b" &
	done
fi

checkdirs "$run" "$ME"

r=
split=2
while [ -n "$split" -a 0 -lt "$split" -a 50000 -gt "$split" ]
do
	for a in "$run.tmp"/*
	do
		[ -f "$a" ] || continue
		waitconnect
		echo -n "                                                        do `basename "$a"`"
		runcharybdis "$a"
	done

	split="`expr "$split" '*' 3 / 2`"
	echo "SPLIT $split"
	(
	cat "$run.files"
	for a in "$run.ok"/* "$run.ko"/*
	do
		cat "$a"
	done 2>/dev/null
	) |
	sort $r |
	uniq -u |
	(
	n="`expr 20000 / $split`"
	[ 1 -gt "$n" ] && n=1
	cd "$run.tmp" && split -l "$n" - "`date +%Y%m%d%H%M%S`"
	)
	if [ ".$r" = "." ]
	then
	    r=-r
	else
	    r=
	fi
done
