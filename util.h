#include "sc-include.h"
#include "sc-lib.h"
#include "sc-ssl.h"

static int	throttle;
static int	maxblock	= BUFSIZ;
static int	alarm_seconds;
static int	alarm_count;
static int	alarm_timeout;
static int	alarm_signal;

static unsigned long	copycount, copycount_in, copycount_out;
static int	copy_filestart;

static void	alarm_ticker(void);

#define	USE_SIGACTION

static void
alarmhandler(int sig)
{
#ifndef USE_SIGACTION
  if (signal(SIGALRM, alarmhandler))
    ex("signal");
#endif
  alarm(alarm_seconds);
  if (++alarm_signal>alarm_count)
    ex("alarm");
}

static void
alarm_init(int s, int c)
{
#ifdef USE_SIGACTION
  struct sigaction sa;

  memset(&sa, 0, sizeof sa);
  sa.sa_handler	= alarmhandler;

  if (sigaction(SIGALRM, &sa, NULL))
    ex("sigaction");
#else
  signal(SIGALRM, alarmhandler);
#endif

  alarm_seconds	= s;
  alarm_count	= c;
  alarm_timeout	= 0;
  alarm_signal	= 0;

  alarm(s);
}

static void
alarm_process(void)
{
  if (alarm_signal)
    {
      if (++alarm_timeout>alarm_count)
	ex("timeout");

      alarm_ticker();

      alarm_signal	= 0;
    }
}

static void
alarm_reset(void)
{
  alarm_timeout	= 0;
}

static void
writen(const char *buf, size_t len, int fo)
{
  static long		delta;
  static struct timeval	last;
  int			pos, put;

  alarm_reset();
  for (pos=0; pos<len; pos+=put)
    {
      size_t	max;

      max	= len-pos;
      if (max>maxblock)
	max	= maxblock;

      put	= tino_ssl_write(fo, buf+pos, max);
      alarm_process();

      if (put>0)
	{
	  if (fo==sock)
	    copycount_out	+= put;
	  if (put>max)
	    ex("oops");
	  alarm_reset();
	  if (throttle)
	    {
	      struct timeval	now;
	      long		hold, elapsed;

	      /* This code is self synchronizing
	       * to any type of problem.
	       */
	      gettimeofday(&now, NULL);
	      elapsed	=  (now.tv_sec-last.tv_sec)*1000+(now.tv_usec-last.tv_usec)/1000;
	      last	=  now;
	      if (elapsed<0)
		elapsed	= 0;
	      if (elapsed>1000000)
		elapsed	= 1000000;
	      if (copy_filestart)
		{
		  copy_filestart	= 0;
		  elapsed		= 0;
		}
	      delta	+= put;
	      delta	-= elapsed*throttle;
	      if (delta<0)
		{
		  if (delta<-maxblock)
		    delta	=  -maxblock;
		}
	      else
		{
	          hold	=  delta*1000/throttle;
	          if (hold>10000)
		    {
#if 0
		      fprintf(stderr, " t=%ld d=%ld n=%ld\r", hold/1000, delta, elapsed);
#endif
		      fflush(stderr);
		      usleep(hold);
		    }
                }
	    }
	  continue;
	}

      if (!put)
	ex("premature EOF on output");

      put	= 0;
      if (errno!=EINTR && errno!=EAGAIN)
	ex("output");
    }
}

static int
readn(int fd, void *buf, size_t max)
{
  alarm_reset();
  for (;;)
    {
      int	got;

      got	= tino_ssl_read(fd, buf, max);
      alarm_process();

      if (got>=0)
	{
	  alarm_reset();
	  if (fd==sock)
	    copycount_in	+= got;
	  return got;
	}

      if (errno!=EINTR && errno!=EAGAIN)
	ex("input");
    }
}

#ifdef NO_ZLIB
static int
docopy(int fi, int fo, int ignore)
{
  char		*buf;
  int		got;
  size_t	bs;

  copy_filestart	= 1;
  bs	= maxblock;
  if (bs<BUFSIZ)
    bs	= BUFSIZ;
  buf	= re_alloc(NULL, bs);
  while ((got=readn(fi, buf, bs))!=0)
    {
      writen(buf, got, fo);
      copycount		+= got;
    }
  free(buf);
  return 0;
}
#else
#include "zlib.h"
static void *
dozalloc(void *ign, unsigned item, unsigned size)
{
  return re_alloc(NULL, (long)item*(long)size);
}
static void
dozfree(void *ign, void *p)
{
  free(p);
}
static int
docopy(int fi, int fo, int doinflate)
{
  char		*bi, *bo;
  size_t	bs;
  z_stream	str;
  int		init, mode, ret;

  copy_filestart	= 1;
  bs	= maxblock;
  if (bs<BUFSIZ)
    bs	= BUFSIZ;
  bi	= re_alloc(NULL, bs);
  bo	= re_alloc(NULL, bs);

  str.zalloc	= dozalloc;
  str.zfree	= dozfree;
  str.opaque	= 0;

  str.avail_in	= 0;
  str.next_in	= bi;

  str.avail_out	= bs;
  str.next_out	= bo;

  init		= 1;
  mode		= Z_NO_FLUSH;

  do
    {
      if (!str.avail_in && mode==Z_NO_FLUSH)
	{
	  if ((str.avail_in=readn(fi, str.next_in=bi, bs))==0)
	    mode	= Z_FINISH;
	  copycount	+= str.avail_in;
	}

      if (init)
	{
	  init	= 0;

	  if (doinflate)
	    {
	      if (inflateInit(&str)!=Z_OK)
		ex("inflate");
	    }
	  else if (deflateInit(&str, 9)!=Z_OK)
	    ex("deflate");
	}

      ret	= (doinflate ? inflate : deflate)(&str, mode);
#if 0
      fprintf(stderr, "\nmode=%d ret=%d\n", mode, ret);
#endif

      if (!str.avail_out || mode!=Z_NO_FLUSH || ret==Z_STREAM_END)
	{
	  size_t	put;

	  put	= bs-str.avail_out;
	  if (put)
	    writen(bo, put, fo);
	  else if (mode!=Z_NO_FLUSH && ret==Z_BUF_ERROR && !str.avail_in)
	    ex("starved");
#if 0
	  fprintf(stderr, "put=%d\n", put);
#endif
	  str.avail_out	= bs;
	  str.next_out	= bo;
	}
    }
  while (ret==Z_OK || ret==Z_BUF_ERROR);

  /* 2002-07-14: Memory leak resolved	*/
  if (!init && (doinflate ? inflateEnd : deflateEnd)(&str)!=Z_OK)
     ex("zlib err");

  free(bi);
  free(bo);

  return ret!=Z_STREAM_END;
}
#endif

/* That's not optimal, however we do not send big things this way.
 */
static void
swrite(const char *s)
{
  size_t	len;
  char		*tmp;

  len	= strlen(s);
  tmp	= alloc(len+1);
  memcpy(tmp, s, len);
  tmp[len]='\n';
  writen(tmp, len+1, sock);
  free(tmp);
}

static void
swrite2(const char *r, const char *s)
{
  size_t	len1, len2;
  char		*tmp;

  len1	= strlen(r);
  len2	= strlen(s);
  tmp	= alloc(len1+len2+1);
  memcpy(tmp, r, len1);
  memcpy(tmp+len1, s, len2);
  tmp[len1+len2]='\n';
  writen(tmp, len1+len2+1, sock);
  free(tmp);
}

static void
swrite_free(char *s)
{
  swrite(s);
  free(s);
}

static char *
sread(void)
{
  char		buf[1024];
  char		*out	= 0;
  size_t	len	= 0;
  int		got;

  while ((got=readn(sock, buf, sizeof buf))>0)
    {
      char	*tmp;

      out	= re_alloc(out, len+got+1);
      memcpy(out+len, buf, got);
      out[len+got]	= 0;
      if ((tmp=strchr(out+len, '\n'))!=0)
	{
	  *tmp	= 0;
	  return out;
	}
      len	+= got;
    }
  out		= re_alloc(out, len+1);
  out[len]	= 0;
  return out;
}

static int
sread_match(const char *s, int n)
{
  char	*t;
  int	res;

  if (!n)
    n	= strlen(s)+1;
  t	= sread();
  res	= strncmp(t, s, n);
  free(t);
  return res;
}

#include "sc-md5.h"
