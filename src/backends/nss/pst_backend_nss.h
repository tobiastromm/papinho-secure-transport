#ifndef PST_BACKEND_NSS_H
#define PST_BACKEND_NSS_H
#include "pst_backend.h"
#define PST_NSS_NATIVE_TRANSPORT_VERSION 0x00010000UL
#define PST_NSS_NATIVE_TRANSPORT_KIND_WIN32_SOCKET 1UL
typedef struct PST_NSS_NATIVE_TRANSPORT {
    pst_u32 struct_size;
    pst_u32 version;
    pst_u32 kind;
    pst_size native_socket;
    const char *hostname;
} PST_NSS_NATIVE_TRANSPORT;
#define PST_NSS_NATIVE_TRANSPORT_MIN_SIZE ((pst_u32)sizeof(PST_NSS_NATIVE_TRANSPORT))
const PST_BACKEND_DESCRIPTOR *pst_backend_nss_descriptor(void);
PST_RESULT pst_backend_nss_register(void);
PST_RESULT pst_backend_nss_normalize_error(pst_i32 native_error);
int pst_backend_nss_is_would_block(pst_i32 native_error);
pst_i32 pst_backend_nss_last_error(const void *state);
#endif