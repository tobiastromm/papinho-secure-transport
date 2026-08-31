#ifndef PST_TRANSPORT_INTERNAL_H
#define PST_TRANSPORT_INTERNAL_H
#include "papinho_secure_transport.h"
struct pst_transport { const char *backend_id; void *native; void (*destroy)(pst_transport *,int); };
#endif
