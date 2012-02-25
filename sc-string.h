static char *
strxcpy(char *s, const char *src, size_t max)
{
  if (max)
    {
      strncpy(s, src, max);
      s[max-1]	= 0;
    }
  return s;
}

static char *
strxcat(char *s, const char *src, size_t max)
{
  size_t len;

  for (len=0; len<max && s[len]; len++);
  strxcpy(s+len, src, max-len);
  return s;
}

