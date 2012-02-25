#include "util.h"

static void
alarm_ticker(void)
{
  /* do nothing */
}

char *
newname(const char *name)
{
  char		*buf;
  unsigned	i, j, k;
  struct stat	st;

  buf	= re_alloc(NULL, strlen(name)+10);

  i	= 0;
  for (i=1; i; i+=i)
    {
      sprintf(buf, "%s.~%02u~", name, i-1);
      if (stat(buf, &st))
	break;
    }
  fprintf(stderr, "newname i=%d\n", i);
  j	= i>>1;
  while (j<i)
    {
      k	= (i+j)>>1;
      sprintf(buf, "%s.~%02u~", name, k);
      if (stat(buf, &st))
	i	= k;
      else
	j	= k+1;
    }
  
  /* uh oh .. bug found */
  sprintf(buf, "%s.~%02u~", name, j);
  fprintf(stderr, "newname %s\n", buf);
  errno	= 0;
  if (!stat(buf, &st) || errno!=ENOENT)
    ex("new");
  return buf;
}

void
mkparent(const char *name)
{
  char		*buf, *tmp;
  struct stat	st;

  buf	= str_alloc(name);
  if ((tmp=strrchr(buf, '/'))!=0)
    {
      *tmp	= 0;
      if (stat(buf, &st))
	mkparent(buf);
      mkdir(buf, 0755);
      if (stat(buf, &st) ||
	  !S_ISDIR(st.st_mode))
	ex("!dir");
    }
  free(buf);
}

void
dofile(const char *name)
{
  int		fd;
  struct stat	st;
  char		*s;

  if ((fd=open(name, O_RDWR))<0)
    swrite("?");
  else if (fstat(fd, &st) ||
	   !S_ISREG(st.st_mode))
    ex("open");
  else
    swrite_free(md5sum(fd, st.st_size));

  s	= sread();
  switch (*s)
    {
    case 'o':
      close(fd);
      free(s);
      return;

    case 'a':
      if (fd<0)
	ex("!fd");
      if (lseek(fd, st.st_size, SEEK_SET)!=st.st_size)
	ex("lseek");
      break;

    case 'w':
      if (fd<0)
	ex("!fd");
      close(fd);
      fd	= -1;
      free(s);
      s	= newname(name);
      if (rename(name, s))
	ex("name");
    case 'c':
      if (fd>=0)
	ex("fd!");
      mkparent(name);
      if ((fd=open(name, O_WRONLY|O_CREAT|O_EXCL, 0644))<0)
	ex("create");
      break;

    default:
      ex("prot");
    }
  free(s);

  swrite("OK");

  if (docopy(sock, fd, 1))
    ex("broken stream");
  if (close(fd))
    ex("close");

  swrite("NEXT");
}

int
main(int argc, char **argv)
{
  freopen("/tmp/scylla.log", "a+", stderr);

  if (argc<3 || argc>4)
    {
      printf("Usage: %s directory password [ssl-key]\n"
	     "\tRun Scylla (this here) from inetd.\n"
	     "\tScylla assumes that Charybdis feeds a file [with SSL].\n"
	     "\tScylla writes the file into the given directory.\n"
	     "\tMissing sub-directories are created as needed.\n"
	     "\tExisting files are renamed.\n", argv[0]);
      return 1;
    }

  if (chdir(argv[1]))
    ex("chdir");

  sock	= 0;

#ifndef NO_SSL
  tino_ssl_server((argc>3 ? argv[3] : "/etc/scylla.pem"));
#endif

  alarm_init(30,10);

  if (sread_match(argv[2], 0))
    ex("auth fail");

  swrite("scylla " VERSION);

  do
    {
      char	*name;

      name	= sread();
      if (!strcmp(name, "..") || !strncmp(name, "../", 3) ||
	  *name=='/' || strstr(name, "/../"))
	ex("- illegal filename");

      dofile(name);
      free(name);

    }
  while (!sread_match("m", 0));

#ifndef NO_SSL
  tino_ssl_close();
#endif
  return 0;
}
