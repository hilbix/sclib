#include "sc-include.h"
#include "sc-lib.h"
#include "sc-slist.h"
#include "sc-string.h"

#include <stdarg.h>

#include <sys/types.h>
#include <dirent.h>

static int errors;

static void
error(const char *s, ...)
{
  va_list	list;

  fprintf(stderr, "error: ");
  va_start(list, s);
  vfprintf(stderr, s, list);
  va_end(list);
  fprintf(stderr, ": %s\n", strerror(errno));
  errors++;
}

static void
tino_ssl_errprint(FILE *fd)
{
  000;
}

struct sc_dir_stat
  {
    const char	*name;	/* Full path	*/
    struct stat64	st;
  };

#define	SC_PATH_MAX	PATH_MAX

/* This gobbles CWD!
 */
static GLIST
sc_scandir(const char *name)
{
  SLIST		slist;
  GLIST		glist;
  DIR		*dir;
  struct dirent	*dp;
  char		buf[SC_PATH_MAX+2];
  const char	*s;

  xDP(("sc_scandir(%s)", name));
  /* Get into the directory
   */
  if (chdir(name))
    {
      error("cannot chdir: %s", name);
      return 0;
    }
  if (strcmp(getcwd(buf, SC_PATH_MAX), name))
    {
      size_t	len;

      len	= strlen(name);
      if (len<2 || name[len-1]!='.' ||
	  (name[len-2]!='/' && (len<3 || name[len-2]!='.' || name[len-3]!='/')))
	error("cwd disagrees with directory name: %s", name);
      return 0;
    }

  xDP(("sc_scandir() read"));

  /* Read it's contents
   */
  if ((dir=opendir("."))==NULL)
    {
      error("cannot open dir: %s", name);
      return 0;
    }
  slist	= slist_new();
  while ((dp=readdir(dir))!=NULL)
    {
      xDP(("sc_scandir() readdir %s", dp->d_name));
      slist_add(slist, dp->d_name);
    }
  if (closedir(dir))
    error("error reading dir: %s", name);

  /* Now do the stats
   */
  glist	= glist_new(sizeof (struct sc_dir_stat));
  for (; (s=slist_get(slist))!=NULL; slist_free(s))
    {
      struct stat64		st;
      struct sc_dir_stat	*ent;

      xDP(("sc_scandir() stat %s", s));
      if (lstat64(s, &st))
	{
	  error("cannot stat: %s/%s", name, s);
	  continue;
	}
      strxcpy(buf, name, sizeof buf);
      if (!*buf || buf[strlen(buf)-1]!='/')
	strxcat(buf, "/", sizeof buf);
      strxcat(buf, s, sizeof buf);
      if (strlen(buf)>=SC_PATH_MAX)
	{
	  error("filename too deep: %s/%s", name, s);
	  continue;
	}
      ent	= glist_add(glist)->data;
      ent->name	= str_alloc(buf);
      ent->st	= st;
    }
  xDP(("sc_scandir() ok"));
  return glist;
}

static void
sc_scan(int argc, char **argv)
{
  SLIST		dirlist;
  const char	*s;

  DP(("sc_scan(%d,%p)", argc, argv));
  for (dirlist=slist_init(argc, argv); (s=slist_get(dirlist))!=0; slist_free(s))
    {
      GLIST			glist;
      GLIST_ENT			ent;

      glist	= sc_scandir(s);
      xDP(("sc_scan() glist=%p", glist));
      while ((ent=glist_get(glist))!=0)
	{
	  struct sc_dir_stat	*ptr=ent->data;

	  xDP(("sc_scan() ent=%p", ent));
	  printf("%06o %04x-%08llx %4d %8llu %4x %4x %08llx %08llx %08llx",
		 ptr->st.st_mode,
		 (unsigned)ptr->st.st_dev,
		 (unsigned long long)ptr->st.st_ino,
		 ptr->st.st_nlink,
		 (unsigned long long)ptr->st.st_blocks,
		 ptr->st.st_uid,   ptr->st.st_gid,
		 (unsigned long long)ptr->st.st_atime,
		 (unsigned long long)ptr->st.st_mtime,
		 (unsigned long long)ptr->st.st_ctime);
	  if (S_ISCHR(ptr->st.st_mode) ||
	      S_ISBLK(ptr->st.st_mode))
	    printf(" [%04lx]", (unsigned long)ptr->st.st_rdev);
	  else if (S_ISLNK(ptr->st.st_mode))
	    {
	      int	n;
	      char	*dat;
	      size_t	dlen;

	      dlen	= ptr->st.st_size+1;
	      if (dlen<BUFSIZ)
		dlen	= BUFSIZ;
	      dat	= alloc(dlen);
	      n		= readlink(ptr->name, dat, dlen);
	      if (n==-1)
		printf(" {-1 softlink read error: %s}", strerror(errno));
	      else if (n>=dlen)
		printf(" {-1 softlink length mismatch: %d}", n);
	      else if ((dat[n]=0, strlen(dat))!=n)
		printf(" {-1 unsupported soflink}");
	      else
		printf(" {%d %s}", n, dat);
	      free(dat);
	    }
	  printf(" %s\n", ptr->name);
	  if (S_ISDIR(ptr->st.st_mode))
	    slist_add(dirlist, ptr->name);
	  glist_free(ent);
	}
      xDP(("sc_scan() next"));
    }
}

int
main(int argc, char **argv)
{
  if (argc<1)
    {
      printf("Usage: %s /initial/path\n"
	     "\tTry command 'help'\n", argv[0]);
      return 1;
    }
  sc_scan(argc-1, argv+1);
  return 0;
}

