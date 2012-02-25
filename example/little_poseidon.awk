#!/bin/gawk
#
# $Header: /CVSROOT/scylla-charybdis/example/little_poseidon.awk,v 1.3 2003/10/14 01:10:55 tino Exp $
#
# Take Odysseus Output from stdin
# and write shell script to stdout.
#
# Run this shell script in the root directory
# of the restored computer after transferring the files
# to the computer.
#
# If you break you computer with the output of this script,
# you are responsible.
#
# YOU HAVE BEEN WARNED!
#
# $Log: little_poseidon.awk,v $
# Revision 1.3  2003/10/14 01:10:55  tino
# Warning message improved, now only idiots can accidentially destroy data
# with little_poseidon.awk
#
# Revision 1.2  2003/10/07 19:46:39  tino
# Removed the echoes from little_poseidon
#

BEGIN {
warn="\n"
warn=warn "DANGER AHEAD!\n"
warn=warn "IF YOU DON'T KNOW WHAT YOU ARE DOING *EXACTLY*,\n"
warn=warn "THIS SCRIPT WILL DESTROY DATA!\n"
warn=warn "\n"
warn=warn "The output of this script is thought to be run from a\n"
warn=warn "recovery system, out of the directory where the freshly\n"
warn=warn "restored root is mounted.\n"
warn=warn "\n"
warn=warn "This here has no nuts'n'bolts, there is no safety net.\n"
warn=warn "It's likely that you break something with this,\n"
warn=warn "if you don't know what you are doing.\n"
warn=warn "\n"
warn=warn "So be sure you can repeat the restore process at any time!\n"
warn=warn "YOU HAVE BEEN WARNED!\n"
warn=warn "\n"

print warn > "/dev/stderr"

print "#"
print "cat << EOF\n"
print warn
print "\nEnter DESTROY KILL ERASE to continue:"
print "EOF"
print "read a || exit"
print "[ '.DESTROY KILL ERASE' = ".$a" ] || exit"
}

# Input looks like
# 1 flags (octal)
# 2 xdev-inode (reference)
# 3 nlinks (number of references)
# 4 blocks (ignored)
# 5 uid
# 6 gid
# 7 atime (hex, ignored)
# 8 mtime (hex)
# 9 ctime (hex)
# 	[rdev]		block/char special
#	{len string}	softlink
# name (which does not support /n in it)

function escape(x)
{
  gsub(/'/, "'\"'\"'", x)
  return "'" x "'"
}

function shift(x)
{
  while (--x>=0)
    sub(/^[^ ]*  */,"");
}

function hex(s)
{
  return strtonum("0x" s)
}

function tim(t)
{
  return strftime("'%Y-%m-%d %H:%M:%S'", hex(t))
}

function hardlink(n)
{
  if (inot[devino]=="d")
    {
      printf("hardlink from/to directory %s not possible: %s\n", ino[devino], n) >"/dev/stderr"
      return
    }
  if (ino[devino]!="")
    {
      if (type=="d")
	{
	  printf("hardlink from/to directory %s not possible: %s\n", ino[devino], n) >"/dev/stderr"
	  return
	}
      printf("h %s%s\n", n, ino[devino]);
    }
  ino[devino]	= ino[devino] " " n
  inot[devino]	= type
}

BEGIN		{
		types["01"]	= "p"
		types["02"]	= "c"
		types["04"]	= "d"
		types["06"]	= "b"
		types["10"]	= "f"
		types["12"]	= "l"
		types["14"]	= "s"

		com["b"]="mknod %s b %s"
		com["c"]="mknod %s c %s"
		com["p"]="mknod %s c"
		com["d"]="mkdir -p %s"
		}

		{
		type	= types[substr($1,1,2)];
		if (type=="")
		  {
		    printf("unknown type %s\n", flags) >"/dev/stderr"
		    next
		  }

		if ((++ent % 10000)==0)
		  {
		    printf("%d\r", ent) >"/dev/stderr"
		    fflush("/dev/stderr")
		  }

		flags	= $1
		devino	= $2
		nlinks	= $3
		blocks	= $4
		uid	= $5
		gid	= $6
		atime	= $7
		mtime	= $8
		ctime	= $9

		s	= sprintf("%s %d %d %s %s", substr(flags,3), uid, gid, tim(mtime), tim(atime))
		shift(9)
		arg=""
		}

type=="b" || type=="c"	{
		if ($1!~/^\[[0-9a-z][0-9a-z][0-9a-z][0-9a-z]\]$/)
		  {
		    printf("unknown char/block mode %s\n", $1) >"/dev/stderr"
		    next
                  }
		arg=sprintf("%d %d", hex(substr($1,2,2)), hex(substr($1,4,2)))
		shift(1)
		}

# softlink
type=="l"	{
		if ($1=="{-1")
		  {
		    sub(/^.*\} /,"");
		    printf("softlink with error %s\n", $0) >"/dev/stderr"
		    next
		  }
		len=substr($1,2)
		shift(1)
		arg=escape(substr($0,1,len))
		$0=substr($0,len+1)
		if ($1!="}")
		  {
		    sub(/^.*\} /,"");
		    printf("missing } at end of softlink name %s%s\n", arg, $0) >"/dev/stderr"
		    next
		  }
		shift(1)
		}

/\/\.$/ || /\/\.\.$/	{
		if (type!="d")
		  printf("name ending on . or .. not directory: %s\n", $0) >"/dev/stderr"
		  next
		}

/^\/proc\//	{
		if (was_proc) next
		was_proc=1
		printf("ignoring /proc/...\n") >"/dev/stderr"
		next
		}

		{
		if (substr($0,1,1)!="/")
		  {
		    printf("name not absoulte %s\n", n) >"/dev/stderr"
		    next
		  }
		n=escape(substr($0,2))
		if (nlinks>1)
		  hardlink(n)
		printf("%s %s %s %s\n", type, n, s, arg);
		}

BEGIN {
s=""
# Ordinary file
s=s "f()\n"
s=s "{\n"
s=s "chmod -- \"$2\" \"$1\"\n"
s=s "chown -- \"$3:$4\" \"$1\"\n"
s=s "touch -cd \"$5\" -- \"$1\"\n"
s=s "touch -acd \"$6\" -- \"$1\"\n"
s=s "}\n"
# Directory
s=s "d()\n"
s=s "{\n"
s=s "mkdir -p -- \"$1\"\n"
s=s "f \"$@\"\n"
s=s "}\n"
# Pipe/fifo
s=s "p()\n"
s=s "{\n"
s=s "mkfifo -- \"$1\"\n"
s=s "f \"$@\"\n"
s=s "}\n"
# Character special
s=s "c()\n"
s=s "{\n"
s=s "mknod -- \"$1\" c \"$7\" \"$8\"\n"
s=s "f \"$@\"\n"
s=s "}\n"
# Block special
s=s "b()\n"
s=s "{\n"
s=s "mknod -- \"$1\" b \"$7\" \"$8\"\n"
s=s "f \"$@\"\n"
s=s "}\n"
# softLink
s=s "l()\n"
s=s "{\n"
s=s "ln -s -- \"$7\" \"$1\"\n"
s=s "}\n"
# Socket: We could create it with gAWK ..
s=s "s()\n"
s=s "{\n"
s=s "echo \"ignored: socket $1\"\n"
s=s "}\n"
s=s "\n"
# Hardlink
s=s "h()\n"
s=s "{\n"
s=s "l=\"$1\"\n"
s=s "shift\n"
s=s "for a\n"
s=s "do\n"
s=s "  if [ -e \"$a\" ]\n"
s=s "  then\n"
s=s "    if [ ! -f \"$l\" -o ! -f \"$a\" ] \\|\\| cmp \"$a\" \"$l\"\n"
s=s "    then\n"
s=s "      ln -f -- \"$a\" \"$l\"\n"
s=s "      return\n"
s=s "    fi\n"
s=s "  fi\n"
s=s "done\n"
s=s "}\n"

print s
}
