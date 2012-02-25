static void
ex(const char *s)
{
  alarm(0);
  alarm(0);
  perror(s);
  sleep(1);
  exit(3);
}

static void
exs(const char *s)
{
  alarm(0);
  alarm(0);
  perror(s);
  tino_ssl_errprint(stderr);
  sleep(1);
  exit(3);
}

static void *
re_alloc(void *buf, size_t len)
{
  if (!len)
    len	= 1;
  if ((buf=(buf ? realloc(buf, len) : malloc(len)))==NULL)
    ex("malloc");
  return buf;
}

static void *
alloc(size_t len)
{
  return re_alloc(NULL, len);
}

static char *
str_alloc(const char *s)
{
  size_t	len;
  char		*buf;

  len	= strlen(s)+1;
  buf	= alloc(len);
  strncpy(buf, s, len);
  buf[len-1]	= 0;
  return buf;
}
