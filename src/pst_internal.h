#ifndef PST_INTERNAL_H
#define PST_INTERNAL_H
#include "papinho_secure_transport.h"
#include "pst_diagnostic.h"
typedef struct pst_internal_operation_context {
    pst_internal_diagnostic diagnostic;
} pst_internal_operation_context;
void pst_internal_operation_context_initialize(pst_internal_operation_context *context);
void pst_internal_operation_context_reset(pst_internal_operation_context *context);
void pst_internal_operation_context_diagnostic_copy(const pst_internal_operation_context *context,pst_internal_diagnostic *out);
PST_RESULT pst_runtime_create_internal(const PST_RUNTIME_OPTIONS *options,pst_runtime **out_runtime,pst_internal_operation_context *context);
PST_RESULT pst_connection_create_internal(pst_runtime *runtime,pst_config *config,pst_connection **out_connection,pst_internal_operation_context *context);
void pst_runtime_diagnostic_copy(const pst_runtime *runtime,pst_internal_diagnostic *out);
void pst_connection_diagnostic_copy(const pst_connection *connection,pst_internal_diagnostic *out);
PST_RESULT pst_validate_public_struct(const void *value, pst_u32 minimum_size);
#endif
