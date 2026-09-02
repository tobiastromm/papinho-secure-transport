#ifndef PST_TRANSPORT_INTERNAL_H
#define PST_TRANSPORT_INTERNAL_H
#include "papinho_secure_transport.h"
#define PST_NATIVE_TRANSPORT_VERSION 0x00010000UL
#define PST_NATIVE_TRANSPORT_KIND_WIN32_SOCKET 1UL
typedef struct PST_NATIVE_TRANSPORT {
    pst_u32 struct_size; pst_u32 version; pst_u32 kind;
    pst_size native_socket; const char *hostname;
} PST_NATIVE_TRANSPORT;
#define PST_NATIVE_TRANSPORT_MIN_SIZE ((pst_u32)sizeof(PST_NATIVE_TRANSPORT))
struct pst_transport { const char *backend_id; void *native; void (*destroy)(pst_transport *,int); };
#endif
