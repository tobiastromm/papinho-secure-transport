#ifndef PST_BACKEND_NSS_H
#define PST_BACKEND_NSS_H
#include "pst_backend.h"
#include "pst_transport_internal.h"
#define PST_NSS_NATIVE_TRANSPORT_VERSION PST_NATIVE_TRANSPORT_VERSION
#define PST_NSS_NATIVE_TRANSPORT_KIND_WIN32_SOCKET PST_NATIVE_TRANSPORT_KIND_WIN32_SOCKET
typedef PST_NATIVE_TRANSPORT PST_NSS_NATIVE_TRANSPORT;
#define PST_NSS_NATIVE_TRANSPORT_MIN_SIZE PST_NATIVE_TRANSPORT_MIN_SIZE
const PST_BACKEND_DESCRIPTOR *pst_backend_nss_descriptor(void);
PST_RESULT pst_backend_nss_register(void);
PST_RESULT pst_backend_nss_normalize_error(pst_i32 native_error);
int pst_backend_nss_is_would_block(pst_i32 native_error);
PST_RESULT pst_backend_nss_classify_poll_flags(int read_ready, int write_ready,
    int error_ready, int hangup_ready, int invalid_ready,
    pst_u32 *ready_interest);
int pst_backend_nss_is_close_notify_alert(pst_u32 description);
void pst_backend_nss_observe_alert(pst_u32 description, pst_u32 *observed);
PST_RESULT pst_backend_nss_classify_eof(pst_u32 received_close_notify,
    PST_BACKEND_IO_RESULT *result);
PST_RESULT pst_backend_nss_alert_registration_result(int succeeded);
pst_i32 pst_backend_nss_last_error(const void *state);
pst_u32 pst_backend_nss_connection_protocol_version(const void *state);
void pst_backend_nss_trace_set_epoch(pst_u32 epoch);
#endif