#include "pst_backend_nss.h"
#if defined(_MSC_VER) && _MSC_VER == 1200
# pragma warning(push)
# pragma warning(disable:4115 4068)
#endif
#include <windows.h>
#include <winsock.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "nss.h"
#include "ssl.h"
#include "sslerr.h"
#include "cert.h"
#include "secerr.h"
#include "prinit.h"
#include "prio.h"
#include "private/pprio.h"
#include "prerror.h"
#include "prerr.h"
#include "prinrval.h"
#if defined(_MSC_VER) && _MSC_VER == 1200
# pragma warning(pop)
# pragma warning(disable:4514)
#endif
typedef void (*pst_pr_init_fn)(PRThreadType, PRThreadPriority, PRUintn);
typedef PRStatus (*pst_pr_cleanup_fn)(void);
typedef PRErrorCode (*pst_pr_get_error_fn)(void);
typedef PRFileDesc *(*pst_pr_import_tcp_fn)(PROsfd);
typedef PRStatus (*pst_pr_set_socket_option_fn)(PRFileDesc *, const PRSocketOptionData *);
typedef PRStatus (*pst_pr_close_fn)(PRFileDesc *);
typedef PRInt32 (*pst_pr_poll_fn)(PRPollDesc *, PRIntn, PRIntervalTime);
typedef PRInt32 (*pst_pr_read_fn)(PRFileDesc *, void *, PRInt32);
typedef PRInt32 (*pst_pr_write_fn)(PRFileDesc *, const void *, PRInt32);
typedef PRStatus (*pst_pr_shutdown_fn)(PRFileDesc *, PRShutdownHow);
typedef PRIntervalTime (*pst_pr_ms_interval_fn)(PRUint32);
typedef SECStatus (*pst_nss_init_fn)(const char *);
typedef SECStatus (*pst_nss_shutdown_fn)(void);
typedef PRFileDesc *(*pst_ssl_import_fd_fn)(PRFileDesc *, PRFileDesc *);
typedef SECStatus (*pst_ssl_reset_handshake_fn)(PRFileDesc *, PRBool);
typedef SECStatus (*pst_ssl_force_handshake_fn)(PRFileDesc *);
typedef SECStatus (*pst_ssl_option_set_fn)(PRFileDesc *, PRInt32, PRIntn);
typedef SECStatus (*pst_ssl_set_url_fn)(PRFileDesc *, const char *);
typedef SECStatus (*pst_ssl_auth_hook_fn)(PRFileDesc *, SSLAuthCertificate, void *);
typedef SECStatus (*pst_ssl_auth_certificate_fn)(void *, PRFileDesc *, PRBool, PRBool);
typedef CERTCertDBHandle *(*pst_cert_default_db_fn)(void);
typedef struct pst_nss_backend_state {
    pst_i32 last_error;
    HMODULE nspr_module;
    HMODULE nss_module;
    HMODULE ssl_module;
    int initialized;
    int has_database;
    pst_pr_init_fn pr_init;
    pst_pr_cleanup_fn pr_cleanup;
    pst_pr_get_error_fn pr_get_error;
    pst_pr_import_tcp_fn pr_import_tcp;
    pst_pr_set_socket_option_fn pr_set_socket_option;
    pst_pr_close_fn pr_close;
    pst_pr_poll_fn pr_poll;
    pst_pr_read_fn pr_read;
    pst_pr_write_fn pr_write;
    pst_pr_shutdown_fn pr_shutdown;
    pst_pr_ms_interval_fn pr_ms_interval;
    pst_nss_init_fn nss_init;
    pst_nss_init_fn nss_nodb_init;
    pst_nss_shutdown_fn nss_shutdown;
    pst_ssl_import_fd_fn ssl_import_fd;
    pst_ssl_reset_handshake_fn ssl_reset_handshake;
    pst_ssl_force_handshake_fn ssl_force_handshake;
    pst_ssl_option_set_fn ssl_option_set;
    pst_ssl_set_url_fn ssl_set_url;
    pst_ssl_auth_hook_fn ssl_auth_hook;
    pst_ssl_auth_certificate_fn ssl_auth_certificate;
    pst_cert_default_db_fn cert_default_db;
} pst_nss_backend_state;
typedef struct pst_nss_runtime_state {
    pst_i32 last_error;
    pst_nss_backend_state *backend;
} pst_nss_runtime_state;
typedef struct pst_nss_connection_state {
    pst_i32 last_error;
    pst_nss_runtime_state *runtime;
    PRFileDesc *ssl_fd;
    pst_u32 interest;
    pst_u32 ownership;
} pst_nss_connection_state;
static int pst_nss_active;
static FARPROC pst_nss_symbol(HMODULE module, const char *name)
{
    if (module == NULL) return NULL;
    return GetProcAddress(module, name);
}
static void pst_nss_unload(pst_nss_backend_state *s)
{
    if (s->ssl_module != NULL) FreeLibrary(s->ssl_module);
    if (s->nss_module != NULL) FreeLibrary(s->nss_module);
    if (s->nspr_module != NULL) FreeLibrary(s->nspr_module);
    s->ssl_module = NULL; s->nss_module = NULL; s->nspr_module = NULL;
}
static int pst_nss_load(pst_nss_backend_state *s)
{
    s->nspr_module = LoadLibraryA("nspr4.dll");
    if (s->nspr_module == NULL) return 0;
    s->nss_module = LoadLibraryA("nss3.dll");
    if (s->nss_module == NULL) return 0;
    s->ssl_module = LoadLibraryA("ssl3.dll");
    if (s->ssl_module == NULL) return 0;
    s->pr_init = (pst_pr_init_fn)pst_nss_symbol(s->nspr_module, "PR_Init");
    s->pr_cleanup = (pst_pr_cleanup_fn)pst_nss_symbol(s->nspr_module, "PR_Cleanup");
    s->pr_get_error = (pst_pr_get_error_fn)pst_nss_symbol(s->nspr_module, "PR_GetError");
    s->pr_import_tcp = (pst_pr_import_tcp_fn)pst_nss_symbol(s->nspr_module, "PR_ImportTCPSocket");
    s->pr_set_socket_option = (pst_pr_set_socket_option_fn)pst_nss_symbol(s->nspr_module, "PR_SetSocketOption");
    s->pr_close = (pst_pr_close_fn)pst_nss_symbol(s->nspr_module, "PR_Close");
    s->pr_poll = (pst_pr_poll_fn)pst_nss_symbol(s->nspr_module, "PR_Poll");
    s->pr_read = (pst_pr_read_fn)pst_nss_symbol(s->nspr_module, "PR_Read");
    s->pr_write = (pst_pr_write_fn)pst_nss_symbol(s->nspr_module, "PR_Write");
    s->pr_shutdown = (pst_pr_shutdown_fn)pst_nss_symbol(s->nspr_module, "PR_Shutdown");
    s->pr_ms_interval = (pst_pr_ms_interval_fn)pst_nss_symbol(s->nspr_module, "PR_MillisecondsToInterval");
    s->nss_init = (pst_nss_init_fn)pst_nss_symbol(s->nss_module, "NSS_Init");
    s->nss_nodb_init = (pst_nss_init_fn)pst_nss_symbol(s->nss_module, "NSS_NoDB_Init");
    s->nss_shutdown = (pst_nss_shutdown_fn)pst_nss_symbol(s->nss_module, "NSS_Shutdown");
    s->cert_default_db = (pst_cert_default_db_fn)pst_nss_symbol(s->nss_module, "CERT_GetDefaultCertDB");
    s->ssl_import_fd = (pst_ssl_import_fd_fn)pst_nss_symbol(s->ssl_module, "SSL_ImportFD");
    s->ssl_reset_handshake = (pst_ssl_reset_handshake_fn)pst_nss_symbol(s->ssl_module, "SSL_ResetHandshake");
    s->ssl_force_handshake = (pst_ssl_force_handshake_fn)pst_nss_symbol(s->ssl_module, "SSL_ForceHandshake");
    s->ssl_option_set = (pst_ssl_option_set_fn)pst_nss_symbol(s->ssl_module, "SSL_OptionSet");
    s->ssl_set_url = (pst_ssl_set_url_fn)pst_nss_symbol(s->ssl_module, "SSL_SetURL");
    s->ssl_auth_hook = (pst_ssl_auth_hook_fn)pst_nss_symbol(s->ssl_module, "SSL_AuthCertificateHook");
    s->ssl_auth_certificate = (pst_ssl_auth_certificate_fn)pst_nss_symbol(s->ssl_module, "SSL_AuthCertificate");
    return s->pr_init != NULL && s->pr_cleanup != NULL &&
        s->pr_get_error != NULL && s->pr_import_tcp != NULL &&
        s->pr_set_socket_option != NULL && s->pr_close != NULL &&
        s->pr_poll != NULL && s->pr_read != NULL && s->pr_write != NULL &&
        s->pr_shutdown != NULL && s->pr_ms_interval != NULL &&
        s->nss_init != NULL && s->nss_nodb_init != NULL &&
        s->nss_shutdown != NULL && s->ssl_import_fd != NULL &&
        s->ssl_reset_handshake != NULL && s->ssl_force_handshake != NULL &&
        s->ssl_option_set != NULL && s->ssl_set_url != NULL &&
        s->ssl_auth_hook != NULL && s->ssl_auth_certificate != NULL &&
        s->cert_default_db != NULL;
}
int pst_backend_nss_is_would_block(pst_i32 error)
{
    return error == (pst_i32)PR_WOULD_BLOCK_ERROR;
}
PST_RESULT pst_backend_nss_normalize_error(pst_i32 error)
{
    if (pst_backend_nss_is_would_block(error)) return PST_RESULT_OK;
    if (error == (pst_i32)PR_CONNECT_RESET_ERROR ||
        error == (pst_i32)PR_END_OF_FILE_ERROR) return PST_RESULT_TRUNCATED;
    if (error == (pst_i32)PR_IO_ERROR ||
        error == (pst_i32)PR_NETWORK_UNREACHABLE_ERROR ||
        error == (pst_i32)PR_CONNECT_ABORTED_ERROR ||
        error == (pst_i32)PR_CONNECT_REFUSED_ERROR ||
        error == (pst_i32)PR_HOST_UNREACHABLE_ERROR)
        return PST_RESULT_TRANSPORT_FAILURE;
    if (error == (pst_i32)SSL_ERROR_BAD_CERT_DOMAIN)
        return PST_RESULT_HOSTNAME_MISMATCH;
    if (error == (pst_i32)SSL_ERROR_BAD_CERTIFICATE ||
        error == (pst_i32)SSL_ERROR_BAD_CERT_ALERT ||
        error == (pst_i32)SSL_ERROR_REVOKED_CERT_ALERT ||
        error == (pst_i32)SSL_ERROR_EXPIRED_CERT_ALERT ||
        error == (pst_i32)SSL_ERROR_UNSUPPORTED_CERT_ALERT ||
        error == (pst_i32)SSL_ERROR_CERTIFICATE_UNKNOWN_ALERT ||
        error == (pst_i32)SEC_ERROR_BAD_SIGNATURE ||
        error == (pst_i32)SEC_ERROR_EXPIRED_CERTIFICATE ||
        error == (pst_i32)SEC_ERROR_REVOKED_CERTIFICATE ||
        error == (pst_i32)SEC_ERROR_UNKNOWN_ISSUER ||
        error == (pst_i32)SEC_ERROR_UNTRUSTED_ISSUER ||
        error == (pst_i32)SEC_ERROR_UNTRUSTED_CERT ||
        error == (pst_i32)SEC_ERROR_CERT_NOT_VALID ||
        error == (pst_i32)SEC_ERROR_CA_CERT_INVALID)
        return PST_RESULT_AUTH_FAILURE;
    if (error == (pst_i32)SEC_ERROR_NO_MEMORY) return PST_RESULT_OUT_OF_MEMORY;
    if (IS_SSL_ERROR(error)) return PST_RESULT_PROTOCOL_FAILURE;
    if (IS_SEC_ERROR(error)) return PST_RESULT_BACKEND_FAILURE;
    return PST_RESULT_BACKEND_FAILURE;
}
pst_i32 pst_backend_nss_last_error(const void *state)
{
    if (state == NULL) return 0;
    return *(const pst_i32 *)state;
}
static void pst_nss_capture_error(pst_nss_backend_state *s, pst_i32 *target)
{
    *target = s->pr_get_error != NULL ? (pst_i32)s->pr_get_error() : 0;
}
static PST_RESULT pst_nss_initialize(void **out_state)
{
    pst_nss_backend_state *s;
    char db_dir[1024];
    DWORD length;
    SECStatus status;
    if (out_state == NULL) return PST_RESULT_INVALID_ARGUMENT;
    *out_state = NULL;
    if (pst_nss_active) return PST_RESULT_INVALID_STATE;
    s = (pst_nss_backend_state *)calloc(1, sizeof(*s));
    if (s == NULL) return PST_RESULT_OUT_OF_MEMORY;
    if (!pst_nss_load(s)) {
        s->last_error = (pst_i32)GetLastError();
        pst_nss_unload(s); free(s); return PST_RESULT_UNAVAILABLE;
    }
    s->pr_init(PR_SYSTEM_THREAD, PR_PRIORITY_NORMAL, 1);
    length = GetEnvironmentVariableA("PST_NSS_DB_DIR", db_dir, sizeof(db_dir));
    if (length >= sizeof(db_dir)) {
        s->pr_cleanup(); pst_nss_unload(s); free(s);
        return PST_RESULT_INVALID_ARGUMENT;
    }
    if (length == 0) status = s->nss_nodb_init(NULL);
    else { status = s->nss_init(db_dir); s->has_database = 1; }
    if (status != SECSuccess) {
        pst_nss_capture_error(s, &s->last_error);
        s->pr_cleanup(); pst_nss_unload(s); free(s);
        return PST_RESULT_BACKEND_FAILURE;
    }
    s->initialized = 1; pst_nss_active = 1; *out_state = s;
    return PST_RESULT_OK;
}
static void pst_nss_shutdown(void *state)
{
    pst_nss_backend_state *s = (pst_nss_backend_state *)state;
    if (s == NULL) return;
    if (s->initialized) {
        if (s->nss_shutdown() != SECSuccess)
            pst_nss_capture_error(s, &s->last_error);
        s->initialized = 0;
        s->pr_cleanup();
    }
    pst_nss_active = 0; pst_nss_unload(s); free(s);
}
static PST_RESULT pst_nss_runtime_create(void *state, void **out_runtime)
{
    pst_nss_backend_state *s = (pst_nss_backend_state *)state;
    pst_nss_runtime_state *runtime;
    if (s == NULL || !s->initialized || out_runtime == NULL)
        return PST_RESULT_INVALID_ARGUMENT;
    runtime = (pst_nss_runtime_state *)calloc(1, sizeof(*runtime));
    if (runtime == NULL) return PST_RESULT_OUT_OF_MEMORY;
    runtime->backend = s; *out_runtime = runtime; return PST_RESULT_OK;
}
static void pst_nss_runtime_destroy(void *runtime) { free(runtime); }
static PST_RESULT pst_nss_query(void *state, pst_u32 *capabilities)
{
    pst_nss_backend_state *s = (pst_nss_backend_state *)state;
    if (s == NULL || !s->initialized || capabilities == NULL)
        return PST_RESULT_INVALID_ARGUMENT;
    *capabilities = PST_BACKEND_CAP_TLS_1_2 | PST_BACKEND_CAP_TLS_1_3 |
        PST_BACKEND_CAP_NONBLOCKING | PST_BACKEND_CAP_BACKEND_WAIT;
    if (s->has_database) *capabilities |= PST_BACKEND_CAP_HOSTNAME_VERIFY;
    return PST_RESULT_OK;
}
static PST_RESULT pst_nss_validate_requirements(void *state, pst_u32 required)
{
    pst_nss_runtime_state *runtime = (pst_nss_runtime_state *)state;
    pst_u32 available;
    if (runtime == NULL || runtime->backend == NULL) return PST_RESULT_INVALID_ARGUMENT;
    pst_nss_query(runtime->backend, &available);
    return (required & ~available) == 0UL ? PST_RESULT_OK : PST_RESULT_UNSUPPORTED;
}
static PST_RESULT pst_nss_connection_create(void *state, void **out_connection)
{
    pst_nss_runtime_state *runtime = (pst_nss_runtime_state *)state;
    pst_nss_connection_state *connection;
    if (runtime == NULL || out_connection == NULL) return PST_RESULT_INVALID_ARGUMENT;
    connection = (pst_nss_connection_state *)calloc(1, sizeof(*connection));
    if (connection == NULL) return PST_RESULT_OUT_OF_MEMORY;
    connection->runtime = runtime; *out_connection = connection;
    return PST_RESULT_OK;
}
static void pst_nss_connection_destroy(void *state)
{
    pst_nss_connection_state *c = (pst_nss_connection_state *)state;
    if (c == NULL) return;
    if (c->ssl_fd != NULL) c->runtime->backend->pr_close(c->ssl_fd);
    c->ssl_fd = NULL; free(c);
}
static PST_RESULT pst_nss_attach(void *state, void *transport, pst_u32 ownership,
                                 pst_u32 *ownership_accepted)
{
    pst_nss_connection_state *c = (pst_nss_connection_state *)state;
    PST_NSS_NATIVE_TRANSPORT *t = (PST_NSS_NATIVE_TRANSPORT *)transport;
    pst_nss_backend_state *s;
    PRFileDesc *tcp_fd;
    PRFileDesc *ssl_fd;
    PRSocketOptionData option;
    u_long nonblocking;
    SOCKET socket_value;
    if (ownership_accepted == NULL) return PST_RESULT_INVALID_ARGUMENT;
    *ownership_accepted = 0UL;
    if (c == NULL || t == NULL || c->ssl_fd != NULL)
        return PST_RESULT_INVALID_ARGUMENT;
    if (ownership != PST_BACKEND_OWNERSHIP_TRANSFERRED)
        return PST_RESULT_UNSUPPORTED;
    if (t->struct_size < PST_NSS_NATIVE_TRANSPORT_MIN_SIZE ||
        t->version != PST_NSS_NATIVE_TRANSPORT_VERSION ||
        t->kind != PST_NSS_NATIVE_TRANSPORT_KIND_WIN32_SOCKET ||
        t->hostname == NULL || t->hostname[0] == '\0')
        return PST_RESULT_INVALID_ARGUMENT;
    s = c->runtime->backend; socket_value = (SOCKET)t->native_socket;
    nonblocking = 1UL;
    if (ioctlsocket(socket_value, FIONBIO, &nonblocking) != 0) {
        c->last_error = (pst_i32)WSAGetLastError();
        return PST_RESULT_TRANSPORT_FAILURE;
    }
    tcp_fd = s->pr_import_tcp((PROsfd)socket_value);
    if (tcp_fd == NULL) {
        pst_nss_capture_error(s, &c->last_error);
        return pst_backend_nss_normalize_error(c->last_error);
    }
    *ownership_accepted = 1UL;
    option.option = PR_SockOpt_Nonblocking;
    option.value.non_blocking = PR_TRUE;
    if (s->pr_set_socket_option(tcp_fd, &option) != PR_SUCCESS) {
        pst_nss_capture_error(s, &c->last_error); s->pr_close(tcp_fd);
        return pst_backend_nss_normalize_error(c->last_error);
    }
    ssl_fd = s->ssl_import_fd(NULL, tcp_fd);
    if (ssl_fd == NULL) {
        pst_nss_capture_error(s, &c->last_error); s->pr_close(tcp_fd);
        return pst_backend_nss_normalize_error(c->last_error);
    }
    if (s->ssl_option_set(ssl_fd, SSL_SECURITY, PR_TRUE) != SECSuccess ||
        s->ssl_option_set(ssl_fd, SSL_HANDSHAKE_AS_CLIENT, PR_TRUE) != SECSuccess ||
        s->ssl_option_set(ssl_fd, SSL_ENABLE_SSL3, PR_FALSE) != SECSuccess ||
        s->ssl_set_url(ssl_fd, t->hostname) != SECSuccess ||
        s->ssl_reset_handshake(ssl_fd, PR_FALSE) != SECSuccess) {
        pst_nss_capture_error(s, &c->last_error); s->pr_close(ssl_fd);
        return pst_backend_nss_normalize_error(c->last_error);
    }
    if (s->has_database &&
        s->ssl_auth_hook(ssl_fd, (SSLAuthCertificate)s->ssl_auth_certificate,
                         s->cert_default_db()) != SECSuccess) {
        pst_nss_capture_error(s, &c->last_error); s->pr_close(ssl_fd);
        return pst_backend_nss_normalize_error(c->last_error);
    }
    c->ssl_fd = ssl_fd; c->ownership = ownership;
    c->interest = PST_BACKEND_INTEREST_READ | PST_BACKEND_INTEREST_WRITE;
    return PST_RESULT_OK;
}
static PST_RESULT pst_nss_handshake(void *state, pst_u32 *operation, PST_RESULT *error)
{
    pst_nss_connection_state *c = (pst_nss_connection_state *)state;
    pst_nss_backend_state *s;
    if (c == NULL || c->ssl_fd == NULL || operation == NULL || error == NULL)
        return PST_RESULT_INVALID_ARGUMENT;
    s = c->runtime->backend;
    if (s->ssl_force_handshake(c->ssl_fd) == SECSuccess) {
        c->interest = PST_BACKEND_INTEREST_NONE;
        *operation = PST_BACKEND_OPERATION_COMPLETE; *error = PST_RESULT_OK;
        return PST_RESULT_OK;
    }
    pst_nss_capture_error(s, &c->last_error);
    if (pst_backend_nss_is_would_block(c->last_error)) {
        c->interest = PST_BACKEND_INTEREST_READ | PST_BACKEND_INTEREST_WRITE;
        *operation = PST_BACKEND_OPERATION_NEED_READ_WRITE; *error = PST_RESULT_OK;
        return PST_RESULT_OK;
    }
    c->interest = PST_BACKEND_INTEREST_NONE;
    *operation = PST_BACKEND_OPERATION_FAILED;
    *error = pst_backend_nss_normalize_error(c->last_error);
    return PST_RESULT_OK;
}
static PST_RESULT pst_nss_interest(void *state, pst_u32 *interest)
{
    pst_nss_connection_state *c = (pst_nss_connection_state *)state;
    if (c == NULL || c->ssl_fd == NULL || interest == NULL)
        return PST_RESULT_INVALID_ARGUMENT;
    *interest = c->interest; return PST_RESULT_OK;
}
static PST_RESULT pst_nss_wait(void *state, pst_u32 interest, pst_u32 timeout_ms,
                               PST_BACKEND_WAIT_RESULT *result)
{
    pst_nss_connection_state *c = (pst_nss_connection_state *)state;
    pst_nss_backend_state *s;
    PRPollDesc poll_desc;
    PRInt32 count;
    if (c == NULL || c->ssl_fd == NULL || result == NULL)
        return PST_RESULT_INVALID_ARGUMENT;
    s = c->runtime->backend; result->ready_interest = 0UL; result->timed_out = 0UL;
    poll_desc.fd = c->ssl_fd; poll_desc.in_flags = 0; poll_desc.out_flags = 0;
    if ((interest & PST_BACKEND_INTEREST_READ) != 0UL) poll_desc.in_flags |= PR_POLL_READ;
    if ((interest & PST_BACKEND_INTEREST_WRITE) != 0UL) poll_desc.in_flags |= PR_POLL_WRITE;
    count = s->pr_poll(&poll_desc, 1, s->pr_ms_interval((PRUint32)timeout_ms));
    if (count == 0) { result->timed_out = 1UL; return PST_RESULT_OK; }
    if (count < 0) {
        pst_nss_capture_error(s, &c->last_error);
        return pst_backend_nss_normalize_error(c->last_error);
    }
    if ((poll_desc.out_flags & PR_POLL_READ) != 0) result->ready_interest |= PST_BACKEND_INTEREST_READ;
    if ((poll_desc.out_flags & PR_POLL_WRITE) != 0) result->ready_interest |= PST_BACKEND_INTEREST_WRITE;
    if ((poll_desc.out_flags & (PR_POLL_ERR | PR_POLL_NVAL)) != 0)
        return PST_RESULT_TRANSPORT_FAILURE;
    if ((poll_desc.out_flags & PR_POLL_HUP) != 0) return PST_RESULT_CLOSED;
    return PST_RESULT_OK;
}
static void pst_nss_io_reset(PST_BACKEND_IO_RESULT *result)
{
    result->bytes_transferred = 0; result->operation = PST_BACKEND_OPERATION_FAILED;
    result->close_kind = PST_BACKEND_CLOSE_NONE; result->error = PST_RESULT_OK;
}
static PST_RESULT pst_nss_read(void *state, void *buffer, pst_size capacity,
                               PST_BACKEND_IO_RESULT *result)
{
    pst_nss_connection_state *c = (pst_nss_connection_state *)state;
    pst_nss_backend_state *s;
    PRInt32 amount;
    if (c == NULL || c->ssl_fd == NULL || buffer == NULL || result == NULL)
        return PST_RESULT_INVALID_ARGUMENT;
    pst_nss_io_reset(result); s = c->runtime->backend;
    amount = s->pr_read(c->ssl_fd, buffer,
                        capacity > (pst_size)INT_MAX ? INT_MAX : (PRInt32)capacity);
    if (amount > 0) {
        result->bytes_transferred = (pst_size)amount;
        result->operation = PST_BACKEND_OPERATION_COMPLETE;
        c->interest = PST_BACKEND_INTEREST_NONE; return PST_RESULT_OK;
    }
    if (amount == 0) {
        result->operation = PST_BACKEND_OPERATION_CLOSED;
        result->close_kind = PST_BACKEND_CLOSE_CLEAN;
        c->interest = PST_BACKEND_INTEREST_NONE; return PST_RESULT_OK;
    }
    pst_nss_capture_error(s, &c->last_error);
    if (pst_backend_nss_is_would_block(c->last_error)) {
        result->operation = PST_BACKEND_OPERATION_NEED_READ_WRITE;
        c->interest = PST_BACKEND_INTEREST_READ | PST_BACKEND_INTEREST_WRITE;
        return PST_RESULT_OK;
    }
    result->operation = PST_BACKEND_OPERATION_FAILED;
    result->error = pst_backend_nss_normalize_error(c->last_error);
    if (result->error == PST_RESULT_TRUNCATED)
        result->close_kind = PST_BACKEND_CLOSE_TRUNCATED;
    c->interest = PST_BACKEND_INTEREST_NONE; return PST_RESULT_OK;
}
static PST_RESULT pst_nss_write(void *state, const void *buffer, pst_size length,
                                PST_BACKEND_IO_RESULT *result)
{
    pst_nss_connection_state *c = (pst_nss_connection_state *)state;
    pst_nss_backend_state *s;
    PRInt32 amount;
    if (c == NULL || c->ssl_fd == NULL || buffer == NULL || result == NULL)
        return PST_RESULT_INVALID_ARGUMENT;
    pst_nss_io_reset(result); s = c->runtime->backend;
    amount = s->pr_write(c->ssl_fd, buffer,
                         length > (pst_size)INT_MAX ? INT_MAX : (PRInt32)length);
    if (amount >= 0) {
        result->bytes_transferred = (pst_size)amount;
        result->operation = amount == 0 && length != 0 ?
            PST_BACKEND_OPERATION_NEED_READ_WRITE : PST_BACKEND_OPERATION_COMPLETE;
        c->interest = amount == 0 && length != 0 ?
            PST_BACKEND_INTEREST_READ | PST_BACKEND_INTEREST_WRITE : PST_BACKEND_INTEREST_NONE;
        return PST_RESULT_OK;
    }
    pst_nss_capture_error(s, &c->last_error);
    if (pst_backend_nss_is_would_block(c->last_error)) {
        result->operation = PST_BACKEND_OPERATION_NEED_READ_WRITE;
        c->interest = PST_BACKEND_INTEREST_READ | PST_BACKEND_INTEREST_WRITE;
        return PST_RESULT_OK;
    }
    result->operation = PST_BACKEND_OPERATION_FAILED;
    result->error = pst_backend_nss_normalize_error(c->last_error);
    c->interest = PST_BACKEND_INTEREST_NONE; return PST_RESULT_OK;
}
static PST_RESULT pst_nss_shutdown_step(void *state, pst_u32 *operation, PST_RESULT *error)
{
    pst_nss_connection_state *c = (pst_nss_connection_state *)state;
    pst_nss_backend_state *s;
    if (c == NULL || c->ssl_fd == NULL || operation == NULL || error == NULL)
        return PST_RESULT_INVALID_ARGUMENT;
    s = c->runtime->backend;
    if (s->pr_shutdown(c->ssl_fd, PR_SHUTDOWN_BOTH) == PR_SUCCESS) {
        *operation = PST_BACKEND_OPERATION_COMPLETE; *error = PST_RESULT_OK;
        c->interest = PST_BACKEND_INTEREST_NONE; return PST_RESULT_OK;
    }
    pst_nss_capture_error(s, &c->last_error);
    if (pst_backend_nss_is_would_block(c->last_error)) {
        *operation = PST_BACKEND_OPERATION_NEED_READ_WRITE; *error = PST_RESULT_OK;
        c->interest = PST_BACKEND_INTEREST_READ | PST_BACKEND_INTEREST_WRITE;
        return PST_RESULT_OK;
    }
    *operation = PST_BACKEND_OPERATION_FAILED;
    *error = pst_backend_nss_normalize_error(c->last_error);
    c->interest = PST_BACKEND_INTEREST_NONE; return PST_RESULT_OK;
}
static const PST_BACKEND_VTABLE pst_nss_vtable = {
    sizeof(PST_BACKEND_VTABLE), PST_BACKEND_SPI_VERSION,
    pst_nss_initialize, pst_nss_shutdown, pst_nss_runtime_create,
    pst_nss_runtime_destroy, pst_nss_query, pst_nss_validate_requirements,
    pst_nss_connection_create, pst_nss_connection_destroy, pst_nss_attach,
    pst_nss_handshake, pst_nss_interest, pst_nss_wait, pst_nss_read,
    pst_nss_write, pst_nss_shutdown_step, NULL, NULL
};
static const PST_BACKEND_DESCRIPTOR pst_nss_descriptor = {
    sizeof(PST_BACKEND_DESCRIPTOR), PST_BACKEND_SPI_VERSION,
    "retrozilla-nss", "RetroZilla NSS/NSPR",
    PST_BACKEND_CAP_TLS_1_2 | PST_BACKEND_CAP_TLS_1_3 |
    PST_BACKEND_CAP_HOSTNAME_VERIFY | PST_BACKEND_CAP_NONBLOCKING |
    PST_BACKEND_CAP_BACKEND_WAIT, &pst_nss_vtable
};
const PST_BACKEND_DESCRIPTOR *pst_backend_nss_descriptor(void)
{
    return &pst_nss_descriptor;
}
PST_RESULT pst_backend_nss_register(void)
{
    return pst_backend_register(&pst_nss_descriptor);
}