#ifndef PST_BACKEND_OPENSSL_H
#define PST_BACKEND_OPENSSL_H
#include "pst_backend.h"
const PST_BACKEND_DESCRIPTOR *pst_backend_openssl_descriptor(void);
PST_RESULT pst_backend_openssl_register(void);
#endif
