/* SPDX-License-Identifier: MPL-2.0 */
#ifndef PST_INTERNAL_H
#define PST_INTERNAL_H
#include "papinho_secure_transport.h"
#include "pst_diagnostic.h"
#include "pst_log.h"
typedef struct pst_internal_operation_context {
    pst_internal_diagnostic diagnostic;
} pst_internal_operation_context;
void pst_internal_operation_context_initialize(pst_internal_operation_context *context);
void pst_internal_operation_context_reset(pst_internal_operation_context *context);
void pst_internal_operation_context_diagnostic_copy(const pst_internal_operation_context *context,pst_internal_diagnostic *out);
PST_RESULT pst_runtime_create_internal(const PST_RUNTIME_OPTIONS *options,pst_runtime **out_runtime,pst_internal_operation_context *context);
PST_RESULT pst_connection_create_internal(pst_runtime *runtime,const PST_CONNECTION_CONFIG *config,pst_connection **out_connection,pst_internal_operation_context *context);
void pst_runtime_diagnostic_copy(const pst_runtime *runtime,pst_internal_diagnostic *out);
void pst_connection_diagnostic_copy(const pst_connection *connection,pst_internal_diagnostic *out);
PST_RESULT pst_connection_wait_set_bind(pst_connection *,pst_wait_set *);
void pst_connection_wait_set_unbind(pst_connection *,pst_wait_set *);
int pst_connection_is_terminal(const pst_connection *,PST_RESULT *);
typedef PST_RESULT (*pst_external_source_poll_fn)(pst_external_source *,pst_u32,pst_u32 *);
typedef PST_RESULT (*pst_external_source_native_fn)(pst_external_source *,pst_size *);
typedef void (*pst_external_source_destroy_fn)(pst_external_source *);
struct pst_external_source {
    pst_external_source_poll_fn poll;
    pst_external_source_native_fn native_source;
    pst_external_source_destroy_fn destroy;
    pst_wait_set *wait_set;
};
typedef struct pst_platform_wait_entry {
    pst_size native_source;
    pst_u32 interests;
    pst_u32 ready_interest;
} pst_platform_wait_entry;
PST_RESULT pst_platform_wait_set_create(void **);
void pst_platform_wait_set_destroy(void *);
PST_RESULT pst_platform_wait_set_wake(void *);
PST_RESULT pst_platform_wait_set_begin(void *);
void pst_platform_wait_set_end(void *);
int pst_platform_wait_set_is_owner(void *);
int pst_platform_wait_set_is_active(void *);
PST_RESULT pst_platform_wait_set_wait(void *,pst_platform_wait_entry *,pst_size,pst_u32,pst_u32 *);
pst_u32 pst_platform_monotonic_ms(void);
PST_RESULT pst_connection_native_wait_source(const pst_connection *,pst_size *);
void pst_wait_set_test_fail_next_growth(void);
PST_RESULT pst_validate_public_struct(const void *value, pst_u32 minimum_size);
#endif
