/* $Header: /CVSROOT/scylla-charybdis/charybdis.c,v 1.3 2003/11/06 06:57:49 tino Exp $
 *
 * $Log: charybdis.c,v $
 * Revision 1.3  2003/11/06 06:57:49  tino
 * Customizeable timeout added (300 was not enough for me now ..)
 *
 * Revision 1.2  2003/11/02 22:21:37  tino
 * CVS keywords added
 *
 */
#define	HERE_CHARYBDIS

#define	CHARYBDIS_TIMEOUT	999	/* Increased timeout	*/
int	charybdis_timeout;

#include "util.h"

#include <netinet/tcp.h>
#include <time.h>

static unsigned long	totalcount, overflowcount;
static char		workmode;
static int		returnval;
static int		filecount, filecopy;

#define	INTERPOLATION 30

static void
alarm_ticker(void)
{
  static int	speedometer[INTERPOLATION], speed2[INTERPOLATION];
  static time_t	timing[INTERPOLATION];
  static unsigned long	lastcount, lastio;
  static unsigned long	gigabytes;
  static long	delta, delta2;
  static int	pos;
  time_t	now;
  long		seconds;
  double	bytes, bytes2, percent;

  pos			= (pos+1)%INTERPOLATION;

  time(&now);

  if (overflowcount)
    {
      lastcount		= totalcount-overflowcount;
      overflowcount	= 0;
    }
  if (copycount_out>=(1ul<<30))
    {
      gigabytes++;
      copycount_out	-= 1ul<<30;
      lastio		-= 1ul<<30;
    }
  delta2		-= speed2[pos];
  delta2		+=
    speed2[pos]		= copycount_out-lastio;
  lastio		= copycount_out;

  delta			-= speedometer[pos];
  delta			+= 
    speedometer[pos]	= copycount-lastcount;
  lastcount		= copycount;

  seconds		= now-(timing[pos] ? timing[pos] : timing[1]);
  timing[pos]		= now;
  if (seconds<=0)
    seconds		= 1;

  bytes			= (double)delta/seconds/1024.;
  bytes2		= (double)delta2/seconds/1024.;
  percent		= -1;
  if (totalcount)
    percent		= copycount*100./totalcount;

  printf("\r%c %7ld %6.2f%% %8.2f %8.2f %2d %d %d %lu.%05lx",
	 workmode, (totalcount-copycount)>>10,
	 percent, bytes, bytes2, alarm_timeout,
	 filecount, filecopy,
	 gigabytes, copycount_out>>10);
  fflush(stdout);
}

static void
getaddr(struct sockaddr_in *sin, const char *host, int port)
{
  memset(sin, 0, sizeof *sin);
  sin->sin_family	= AF_INET;
  sin->sin_addr.s_addr	= htonl(INADDR_ANY);
  sin->sin_port		= htons(port);
  if (host && !inet_aton(host, &sin->sin_addr))
    {
      struct hostent	*he;

      if ((he=gethostbyname(host))==0)
	ex(host);
      if (he->h_addrtype!=AF_INET || he->h_length!=sizeof sin->sin_addr)
	ex("unsupported host address");
      memcpy(&sin->sin_addr, he->h_addr, sizeof sin->sin_addr);
    }
}

static void
sockopen(const char *host, int port)
{
  struct sockaddr_in	sin;
  int			on;
  struct linger		linger;
  unsigned int		mss;

  sock	= socket( AF_INET, SOCK_STREAM, 0 );
  if (sock<0)
    ex("socket");

  on = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on))
    ex("setsockopt reuse");
  linger.l_onoff	= 1;
  linger.l_linger	= 65535;
  if (setsockopt(sock, SOL_SOCKET, SO_LINGER, &linger, sizeof linger))
    ex("setsockopt linger");
  on	= 102400;
  if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &on, sizeof on))
    ex("setsockopt sndbuf");
  on	= 102400;
  if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &on, sizeof on))
    ex("setsockopt rcvbuf");

  /* 2002-05-09 set the MTU
   * The kernel takes a lower value if it is not supported ;)
   * We add some bytes for the tcp packet overhead.
   */
  if (maxblock<1500)
    {
      mss = maxblock+48;
      if(setsockopt(sock, IPPROTO_TCP, TCP_MAXSEG, &mss, sizeof(mss)))
        ex("setsockopt maxseg");
    }

  getaddr(&sin, host, port);

  if (bind(sock, (struct sockaddr *)&sin, sizeof sin))
    ex("bind");

#if 0
  if (fcntl(sock, F_SETFL, O_NDELAY))
    ex("O_NDELAY");
#endif
}

static void
do_connect(const char *host, int port)
{
  struct sockaddr_in	sin;

  sockopen(NULL, 0);

  getaddr(&sin, host, port);
  if (connect(sock, (struct sockaddr *)&sin, sizeof sin))
    ex("connect");
}

/* We don't want to connect to scylla
 * except we have to do something.
 * Therefor this delayed connect function.
 * Additionally this prohibits a timeout
 * until find feeds us the files ;)
 */
void
delayed_connect(const char *host, int port, const char *pass)
{
  static char	*sav_host, *sav_pass;
  static int	sav_port;

  DP(("delayed_connect(%s,%d,%s)", host, port, pass));
  if (host)
    {
      sav_host	= str_alloc(host);
      sav_port	= port;
      sav_pass	= str_alloc(pass);
      sock	= -1;
      return;
    }
  if (sock>=0)
    return;

  workmode	= '0';
  alarm_init(1 /* see below */, charybdis_timeout);

  /* Hack: Set a connect timeout to 5 minutes
   */
  alarm(300);
  do_connect(sav_host, sav_port);

#ifndef NO_SSL
  alarm_process();
  alarm(300);
  tino_ssl_client();
#endif

  /* Reset timeout to alarm processing
   */
  alarm_reset();
  alarm_process();
  alarm(1 /* see above */);

  workmode	= '1';

  /* send password */
  swrite(sav_pass);

  if (sread_match("scylla", 6))
    ex("where is my dear scylla?");
  /* leave version control for future
   * we are using version 1.0 protocol
   * (plain password)
   */
}

void
dofile(const char *file)
{
  int		fd;
  char		*t;
  unsigned long	len;
  struct stat	st;

  DP(("dofile(%s)", file));

  filecount++;
  alarm_reset();
  alarm_process();

  if ((fd=open(file, O_RDONLY))<0)
    {
      perror(file);
      DP(("dofile() !file"));
      returnval	|= 1;
      return;
    }
  if (fstat(fd, &st) ||
      !S_ISREG(st.st_mode))
    {
      perror(file);
      DP(("dofile() !fstat"));
      close(fd);
      returnval	|= 1;
      return;
    }

  /* Say there is more on streaming mode
   * On the first file delayed_connect is done,
   * thus sock<0 here
   */
  if (sock>=0)
    swrite("m");

  /* Now that we have something to do: connect
   */
  delayed_connect(NULL, 0, NULL);

  workmode	= '2';
  swrite(file);

  t	= sread();
  if (!isdigit(*t))
    {
      len	= 0;
      swrite("c");
      workmode	= 'c';
    }
  else
    {
      char	*m;

      sscanf(t, "%lu", &len);
      m	= md5sum(fd, len);
      if (!m)
	ex("sum");

      if (!strcmp(m, t))
	{
	  if (st.st_size==len)
	    {
	      free(m);
	      free(t);
	      close(fd);
	      swrite("o");
	      /* unlink(argv[4]); */
	      return;
	    }
	  swrite("a");
	  workmode	= 'a';
	}
      else
	{
	  len	= 0;
	  swrite("w");
	  workmode	= 'w';
	}
      free(m);
    }
  free(t);

  filecopy++;

  if (lseek(fd, len, SEEK_SET)!=len)
    ex("lseek");
  overflowcount	+= copycount;
  copycount	= 0;
  totalcount	= st.st_size-len;

  if (sread_match("OK", 0))
    ex("protocol failure");

  if (docopy(fd, sock, 0))
    ex("copy error");
  if (close(fd))
    ex("close error");

  returnval	|= 2;
  workmode	= 'e';
  /* Wait for transfer flush */

  if (sread_match("NEXT", 0))
    {
      fprintf(stderr, "protocol mismatch: missing next\n");
      exit(returnval);
    }
}

/* TODO in 2.3:
 * We are not allowed to buffer, as if something breaks,
 * we must leave the other files on stdin.
 * Therefor we must loop over bytes.
 * Fix this with S&C version 3 (or 4?).
 */
void
stdinfileloop(void)
{
  char	buf[BUFSIZ];

  000;
  DP(("stdinfileloop"));
  while (fgets(buf, sizeof buf-1, stdin))
    {
      size_t	len;

      len	= strlen(buf);
      if (len>=sizeof buf-1)
	ex("filename too long");
      if (len && buf[len-1]=='\n')
	buf[--len]	= 0; 
      if (len<1)
	continue;

      dofile(buf);
    }
}

int
main(int argc, char **argv)
{
  DP(("main(%d,%p)", argc, argv));
  if (argc<5)
    {
      printf("Usage: %s host port password filename [throttle [blocksize [timeout]]]\n"
	     "\tCharybdis (this here) assumes that Scylla is at host:port\n"
	     "\tCharybdis authenticates at Scylla with the password.\n"
	     "\tWhen the file is uploaded to Scylla it returns 2 or\n"
	     "\tif Scylla already has the file completely it returns 0.\n"
	     "\tSo run this in a shell loop until true is returned\n"
	     "\tto restart broken uploads seamlessly.\n"
	     "\tThe bandwidth can be controlled with trottle@blocksize\n"
	     "\t\t(throttle is in KB/s)\n",
	     argv[0]);
      return 1;
    }

  returnval	= 0;
  if (argc>5)
    {
      throttle	= atoi(argv[5]);
      maxblock	= 512;
      if (argc>6)
	maxblock	= atoi(argv[6]);
    }
  if (maxblock<100)
    maxblock	= BUFSIZ;
  else if (maxblock<256)
    maxblock	= 256;

  charybdis_timeout	= CHARYBDIS_TIMEOUT;
  if (argc>7)
    charybdis_timeout	= atoi(argv[7]);
  if (charybdis_timeout<=0)
    charybdis_timeout	= CHARYBDIS_TIMEOUT;

  delayed_connect(argv[1], atoi(argv[2]), argv[3]);

  if (strcmp(argv[4], "-"))
    dofile(argv[4]);
  else
    stdinfileloop();

  if (sock>=0)
    {
      swrite("e");

#ifndef NO_SSL
      tino_ssl_close();
#endif
      close(sock);
    }
  return returnval;
}
