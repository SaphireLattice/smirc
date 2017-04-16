#ifndef SMIRC_SSL_H
#define SMIRC_SSL_H

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

void InitializeSSL();
int verify_callback(int, X509_STORE_CTX *);
void ShowCerts(SSL* ssl);

#endif //SMIRC_SSL_H
