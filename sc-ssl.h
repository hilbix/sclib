static int sock;

#ifdef	NO_SSL
static void
tino_ssl_errprint(FILE *fd)
{
  /* noting to do */
}
#else
#define OPENSSL_NO_KRB5

#include "rsa.h"       /* SSLeay stuff */
#include "crypto.h"
#include "x509.h"
#include "pem.h"
#include "ssl.h"
#include "err.h"

static SSL	*ssl;
static SSL_CTX	*ssl_ctx;
static int	ssl_active;

static int
tino_ssl_write(int fd, const void *buf, size_t len)
{
  if (ssl_active && sock==fd)
    return SSL_write(ssl, buf, len);
  return write(fd, buf, len);
}

static int
tino_ssl_read(int fd, void *buf, size_t len)
{
  if (ssl_active && sock==fd)
    return SSL_read(ssl, buf, len);
  return read(fd, buf, len);
}

static void
tino_ssl_errprint(FILE *fd)
{
  if (!ssl_active)
    return;
  ERR_print_errors_fp(fd);
}

static void
tino_ssl_close(void)
{
  if (!ssl_active)
    return;

  SSL_free(ssl);

  SSL_CTX_free(ssl_ctx);
}

static void
tino_ssl_client(void)
{
  SSLeay_add_all_algorithms();
  SSLeay_add_ssl_algorithms();
  SSL_load_error_strings();

  ssl_ctx = SSL_CTX_new(SSLv23_client_method());
  if (!ssl_ctx)
    ex("SSL_CTX_new");
  
  ssl = SSL_new(ssl_ctx);
  if (!ssl)
    exs("SSL_new");

  SSL_set_fd(ssl, sock);

  if (SSL_connect(ssl)!=1)
    exs("SSL_connect");

  ssl_active	= 1;
}

static void
tino_ssl_server(char *cert)
{
  SSLeay_add_all_algorithms();
  SSLeay_add_ssl_algorithms();
  SSL_load_error_strings();

  ssl_ctx = SSL_CTX_new(SSLv23_server_method());
  if (!ssl_ctx)
    ex("SSL_CTX_new");
  
  if (SSL_CTX_use_certificate_file(ssl_ctx, cert, SSL_FILETYPE_PEM)==-1)
    exs("SSL_CTX_use_cert...");

  if (SSL_CTX_use_PrivateKey_file(ssl_ctx, cert, SSL_FILETYPE_PEM)==-1)
    exs("SSL_CTX_use_Priv...");
  
  ssl = SSL_new(ssl_ctx);
  if (!ssl)
    exs("SSL_new");

  SSL_set_fd(ssl, sock);

  if (SSL_accept(ssl)==-1)
    exs("SSL_accept");

  ssl_active	= 1;
}
#endif
