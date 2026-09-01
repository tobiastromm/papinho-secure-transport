#ifndef PST_INTERNAL_H
#define PST_INTERNAL_H
#include "papinho_secure_transport.h"
#include "pst_diagnostic.h"
void pst_runtime_diagnostic_copy(const pst_runtime *runtime,pst_internal_diagnostic *out);
void pst_connection_diagnostic_copy(const pst_connection *connection,pst_internal_diagnostic *out);
PST_RESULT pst_validate_public_struct(const void *value, pst_u32 minimum_size);
#endif
