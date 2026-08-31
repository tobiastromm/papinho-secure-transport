#ifndef PST_DIAGNOSTIC_H
#define PST_DIAGNOSTIC_H
#include "papinho_secure_transport.h"
#define PST_DIAGNOSTIC_BACKEND_ID_CAPACITY 32
#define PST_DIAGNOSTIC_DOMAIN_NONE 0UL
#define PST_DIAGNOSTIC_DOMAIN_NSPR 1UL
#define PST_DIAGNOSTIC_DOMAIN_NSS 2UL
#define PST_DIAGNOSTIC_DOMAIN_WIN32 3UL
#define PST_DIAGNOSTIC_DOMAIN_WINSOCK 4UL
#define PST_DIAGNOSTIC_DOMAIN_BACKEND 5UL
#define PST_DIAGNOSTIC_PHASE_NONE 0UL
#define PST_DIAGNOSTIC_PHASE_BACKEND_INITIALIZE 1UL
#define PST_DIAGNOSTIC_PHASE_RUNTIME_CREATE 2UL
#define PST_DIAGNOSTIC_PHASE_CAPABILITY_VALIDATE 3UL
#define PST_DIAGNOSTIC_PHASE_TRANSPORT_ATTACH 4UL
#define PST_DIAGNOSTIC_PHASE_TLS_CONFIGURE 5UL
#define PST_DIAGNOSTIC_PHASE_HANDSHAKE 6UL
#define PST_DIAGNOSTIC_PHASE_PEER_AUTHENTICATE 7UL
#define PST_DIAGNOSTIC_PHASE_HOSTNAME_VERIFY 8UL
#define PST_DIAGNOSTIC_PHASE_ALPN 9UL
#define PST_DIAGNOSTIC_PHASE_READ 10UL
#define PST_DIAGNOSTIC_PHASE_WRITE 11UL
#define PST_DIAGNOSTIC_PHASE_WAIT 12UL
#define PST_DIAGNOSTIC_PHASE_SHUTDOWN 13UL
#define PST_DIAGNOSTIC_PHASE_PEER_INFO 14UL
#define PST_DIAGNOSTIC_PHASE_IDENTITY_SETUP 15UL
#define PST_DIAGNOSTIC_FLAG_NATIVE 0x00000001UL
#define PST_DIAGNOSTIC_FLAG_SECONDARY 0x00000002UL
typedef struct pst_internal_diagnostic { PST_RESULT result; pst_u32 phase; pst_u32 native_domain; pst_i32 native_code; pst_i32 secondary_native_code; pst_u32 flags; pst_u32 generation; pst_u32 valid; char backend_id[PST_DIAGNOSTIC_BACKEND_ID_CAPACITY]; } pst_internal_diagnostic;
void pst_diagnostic_initialize(pst_internal_diagnostic *diagnostic);
void pst_diagnostic_clear(pst_internal_diagnostic *diagnostic);
void pst_diagnostic_capture(pst_internal_diagnostic *diagnostic,PST_RESULT result,pst_u32 phase,const char *backend_id,pst_u32 native_domain,pst_i32 native_code,pst_i32 secondary_native_code,pst_u32 flags);
#endif