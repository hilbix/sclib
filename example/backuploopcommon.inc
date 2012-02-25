#!/bin/bash
#
# $Header: /CVSROOT/scylla-charybdis/example/backuploopcommon.inc,v 1.3 2003/11/06 23:11:40 tino Exp $
#
# $Log: backuploopcommon.inc,v $
# Revision 1.3  2003/11/06 23:11:40  tino
# There was a small typo -> runts instead of runs
#
# Revision 1.2  2003/11/06 12:05:45  tino
# host:port@throttle:blocksize:timeout should work now ..
#
# Revision 1.1  2003/10/29 23:57:03  tino
# Common parts combined.  Calling syntax unified.  TEST VERSION!
#
#

ME="`hostname -f`"
rundir="/tmp/scylla.backup/"
runs="`date +%Y%m%d%H%M%S`"

setupbackuploop()
{
[ 2 -gt $# ] && usage

CONNECT="${1//:/ }"
case "$CONNECT" in
*@*)	THROTTLE="`expr match "$CONNECT" '[^@]*@\(.*\)$'`";
	CONNECT="`expr match "$CONNECT" '\([^@]*\)@'`";;
esac

PASSWORD=""
read -rt1 PASSWORD || { echo "Use echo etc. to pipe password into this script." >&2; usage; }

run=""
case "`basename "$2" .tmp`" in
[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9])
	case "`dirname "$2"`/" in
	./|*$rundir)	run="$rundir`basename "$2" .tmp`";;
	esac;;
esac
}

checkdirs()
{
if [ ! -f "$1.files" -o ! -L "$2" -o ! -d "$2" -o ! -d "$1.ok" -o ! -d "$1.fail" -o ! -d "$1.tmp" ]
then
	cat <<EOF
Directories or softlink are missing.  Check if you are in the right place:
$2
$1.fail
$1.files
$1.ok
$1.tmp
EOF
	exit 1
fi
}

# We wait until the connection can be established.
# This is for SSH dialup usage like in following script:
# 
##!/bin/sh
#
#HostToBackup="$1"
#while true
#do
#	# wait until dialup is established
#	if netcat -zw10 "$HostToBackup" ssh
#	then
#		date
#		# Establish connection and start backup service
#		# Assumes empty passphrase and charybdis script at other side
#		ssh -R1234:localhost:999 "charybdis@$HostToBackup"
#		date
#	else
#		echo -n "."
#	done
#done

waitconnect()
{
#UNCOMMENT next line if you don't want to use netcat
#return 0
oops=false
while ! netcat -zw10 $CONNECT
do
	echo -n .
	sleep 10
	oops=:
done
$oops && echo
}

: runcharybdis
runcharybdis()
{
trap exit 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
trap '' 18 19 20
if /usr/local/bin/charybdis $CONNECT $PASSWORD - $THROTTLE < "$1"
then
	echo " ok "
	mv -f "$1" "$run.ok"
elif [ 2 = $? ]
then
	echo " up "
	rm -f "$1"
else
	echo " xx "
	mv -f "$1" "$run.fail"
fi
}

true
