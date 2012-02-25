#include "md5.h"

static char *
md5sum(int fd, unsigned long max)
{
  MD5_CTX	ctx;
  char		buf[BUFSIZ], digest[16];
  int		got, i;
  char		*out, *ptr;
  unsigned long	total;

  MD5_Init(&ctx);
  total	= 0;
  while (total<max && (got=readn(fd, buf, sizeof buf))>0)
    {
      unsigned long tmp;

      if (got>(tmp=max-total))
	got	= (int)tmp;
      MD5_Update(&ctx, buf, got);
      total	+= got;
    }
  MD5_Final(digest, &ctx);

  out	= alloc(32+32);
  sprintf(out, "%lu ", total);
  ptr	= out+strlen(out);
  for (i=0; i<16; i++)
    {
      *ptr++	= "0123456789abcdef"[(digest[i]>>4)&0xf];
      *ptr++	= "0123456789abcdef"[(digest[i]>>0)&0xf];
    }
  *ptr	= 0;
  return out;
}
