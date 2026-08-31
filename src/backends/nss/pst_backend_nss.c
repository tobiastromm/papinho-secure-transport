#include "pst_backend_nss.h"
#if defined(_MSC_VER) && _MSC_VER == 1200
# pragma warning(push)
# pragma warning(disable:4115 4068)
#endif
#include <windows.h>
#include <winsock.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "nss.h"
#include "ssl.h"
#include "sslerr.h"
#include "sslproto.h"
#include "cert.h"
#include "certdb.h"
#include "keyhi.h"
#include "pk11pub.h"
#include "secoid.h"
#include "pst_identity_internal.h"
#include "pst_transport_internal.h"
#include "papinho_secure_transport_win32.h"
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
typedef SECStatus (*pst_ssl_channel_info_fn)(PRFileDesc *, SSLChannelInfo *, PRUintn);
typedef SECStatus (*pst_ssl_client_auth_hook_fn)(PRFileDesc *, SSLGetClientAuthData, void *);
typedef CERTCertificate *(*pst_ssl_peer_certificate_fn)(PRFileDesc *);
typedef SECStatus (*pst_ssl_version_set_fn)(PRFileDesc *, const SSLVersionRange *);
typedef SECStatus (*pst_ssl_set_alpn_fn)(PRFileDesc *, const unsigned char *, unsigned int);
typedef SECStatus (*pst_ssl_get_alpn_fn)(PRFileDesc *, SSLNextProtoState *, unsigned char *, unsigned int *, unsigned int);
typedef int (*pst_ssl_data_pending_fn)(PRFileDesc *);
typedef CERTCertDBHandle *(*pst_cert_default_db_fn)(void);
typedef CERTCertificate *(*pst_cert_new_temp_fn)(CERTCertDBHandle *, SECItem *, char *, PRBool, PRBool);
typedef SECStatus (*pst_cert_change_trust_fn)(CERTCertDBHandle *, CERTCertificate *, CERTCertTrust *);
typedef void (*pst_cert_destroy_fn)(CERTCertificate *);
typedef CERTCertificate *(*pst_cert_dup_fn)(CERTCertificate *);
typedef PK11SlotInfo *(*pst_pk11_get_slot_fn)(void);
typedef void (*pst_pk11_free_slot_fn)(PK11SlotInfo *);
typedef SECStatus (*pst_pk11_import_key_fn)(PK11SlotInfo *, SECItem *, SECItem *, SECItem *, PRBool, PRBool, unsigned int, SECKEYPrivateKey **, void *);
typedef SECStatus (*pst_pk11_hash_fn)(SECOidTag, unsigned char *, const unsigned char *, PRInt32);
typedef SECKEYPrivateKey *(*pst_key_copy_fn)(const SECKEYPrivateKey *);
typedef void (*pst_key_destroy_fn)(SECKEYPrivateKey *);
typedef struct pst_nss_backend_state {
    pst_i32 last_error;
    HMODULE nspr_module;
    HMODULE nss_module;
    HMODULE ssl_module;
    int initialized;
    int has_database;
    CERTCertificate *test_ca;
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
    pst_ssl_channel_info_fn ssl_channel_info;
    pst_ssl_client_auth_hook_fn ssl_client_auth_hook;
    pst_ssl_peer_certificate_fn ssl_peer_certificate;
    pst_ssl_version_set_fn ssl_version_set;
    pst_ssl_set_alpn_fn ssl_set_alpn;
    pst_ssl_get_alpn_fn ssl_get_alpn;
    pst_ssl_data_pending_fn ssl_data_pending;
    pst_cert_default_db_fn cert_default_db;
    pst_cert_new_temp_fn cert_new_temp;
    pst_cert_change_trust_fn cert_change_trust;
    pst_cert_destroy_fn cert_destroy;
    pst_cert_dup_fn cert_dup;
    pst_pk11_get_slot_fn pk11_get_slot;
    pst_pk11_free_slot_fn pk11_free_slot;
    pst_pk11_import_key_fn pk11_import_key;
    pst_pk11_hash_fn pk11_hash;
    pst_key_copy_fn key_copy;
    pst_key_destroy_fn key_destroy;
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
    CERTCertificate *local_certificate;
    SECKEYPrivateKey *local_key;
    CERTCertificate *trust_anchor;
    char *expected_hostname;
    pst_u32 require_peer;
    int handshake_complete;
    pst_u32 minimum_version, maximum_version, alpn_requirement;
    pst_u8 *alpn; pst_size alpn_size;
} pst_nss_connection_state;
static int pst_nss_active;
static DWORD pst_nss_trace_epoch;
static int pst_nss_trace_epoch_set;
void pst_backend_nss_trace_set_epoch(pst_u32 epoch)
{
    pst_nss_trace_epoch = (DWORD)epoch;
    pst_nss_trace_epoch_set = 1;
}
static void pst_nss_trace(const char *event, const char *detail)
{
    char path[1024];
    DWORD length;
    FILE *file;
    length = GetEnvironmentVariableA("PST_NSS_TRACE_FILE", path, sizeof(path));
    if (length == 0 || length >= sizeof(path)) return;
    file = fopen(path, "a");
    if (file == NULL) return;
    if (!pst_nss_trace_epoch_set) {
        pst_nss_trace_epoch = GetTickCount();
        pst_nss_trace_epoch_set = 1;
    }
    fprintf(file, "T_MS=%010lu %s%s%s\n",
            (unsigned long)(GetTickCount() - pst_nss_trace_epoch), event,
            detail == NULL ? "" : " ", detail == NULL ? "" : detail);
    fclose(file);
}
static void pst_nss_trace_module(const char *name, HMODULE module)
{
    char path[1024];
    DWORD length;
    if (module == NULL) return;
    length = GetModuleFileNameA(module, path, sizeof(path));
    if (length != 0 && length < sizeof(path)) pst_nss_trace(name, path);
}
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
    s->cert_new_temp = (pst_cert_new_temp_fn)pst_nss_symbol(s->nss_module, "CERT_NewTempCertificate");
    s->cert_change_trust = (pst_cert_change_trust_fn)pst_nss_symbol(s->nss_module, "CERT_ChangeCertTrust");
    s->cert_destroy = (pst_cert_destroy_fn)pst_nss_symbol(s->nss_module, "CERT_DestroyCertificate");
    s->ssl_import_fd = (pst_ssl_import_fd_fn)pst_nss_symbol(s->ssl_module, "SSL_ImportFD");
    s->ssl_reset_handshake = (pst_ssl_reset_handshake_fn)pst_nss_symbol(s->ssl_module, "SSL_ResetHandshake");
    s->ssl_force_handshake = (pst_ssl_force_handshake_fn)pst_nss_symbol(s->ssl_module, "SSL_ForceHandshake");
    s->ssl_option_set = (pst_ssl_option_set_fn)pst_nss_symbol(s->ssl_module, "SSL_OptionSet");
    s->ssl_set_url = (pst_ssl_set_url_fn)pst_nss_symbol(s->ssl_module, "SSL_SetURL");
    s->ssl_auth_hook = (pst_ssl_auth_hook_fn)pst_nss_symbol(s->ssl_module, "SSL_AuthCertificateHook");
    s->ssl_auth_certificate = (pst_ssl_auth_certificate_fn)pst_nss_symbol(s->ssl_module, "SSL_AuthCertificate");
    s->ssl_channel_info = (pst_ssl_channel_info_fn)pst_nss_symbol(s->ssl_module, "SSL_GetChannelInfo");
    s->ssl_client_auth_hook = (pst_ssl_client_auth_hook_fn)pst_nss_symbol(s->ssl_module, "SSL_GetClientAuthDataHook");
    s->ssl_peer_certificate = (pst_ssl_peer_certificate_fn)pst_nss_symbol(s->ssl_module, "SSL_PeerCertificate");
    s->ssl_version_set = (pst_ssl_version_set_fn)pst_nss_symbol(s->ssl_module, "SSL_VersionRangeSet");
    s->ssl_set_alpn = (pst_ssl_set_alpn_fn)pst_nss_symbol(s->ssl_module, "SSL_SetNextProtoNego");
    s->ssl_get_alpn = (pst_ssl_get_alpn_fn)pst_nss_symbol(s->ssl_module, "SSL_GetNextProto");
    s->ssl_data_pending = (pst_ssl_data_pending_fn)pst_nss_symbol(s->ssl_module, "SSL_DataPending");
    s->cert_dup = (pst_cert_dup_fn)pst_nss_symbol(s->nss_module, "CERT_DupCertificate");
    s->pk11_get_slot = (pst_pk11_get_slot_fn)pst_nss_symbol(s->nss_module, "PK11_GetInternalKeySlot");
    s->pk11_free_slot = (pst_pk11_free_slot_fn)pst_nss_symbol(s->nss_module, "PK11_FreeSlot");
    s->pk11_import_key = (pst_pk11_import_key_fn)pst_nss_symbol(s->nss_module, "PK11_ImportDERPrivateKeyInfoAndReturnKey");
    s->pk11_hash = (pst_pk11_hash_fn)pst_nss_symbol(s->nss_module, "PK11_HashBuf");
    s->key_copy = (pst_key_copy_fn)pst_nss_symbol(s->nss_module, "SECKEY_CopyPrivateKey");
    s->key_destroy = (pst_key_destroy_fn)pst_nss_symbol(s->nss_module, "SECKEY_DestroyPrivateKey");
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
        s->ssl_channel_info != NULL && s->ssl_client_auth_hook != NULL &&
        s->ssl_peer_certificate != NULL && s->ssl_version_set != NULL &&
        s->ssl_set_alpn != NULL && s->ssl_get_alpn != NULL && s->cert_dup != NULL &&
        s->pk11_get_slot != NULL && s->pk11_free_slot != NULL &&
        s->pk11_import_key != NULL && s->pk11_hash != NULL &&
        s->key_copy != NULL && s->key_destroy != NULL &&
        s->cert_default_db != NULL && s->cert_new_temp != NULL &&
        s->cert_change_trust != NULL && s->cert_destroy != NULL;
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
static PST_RESULT pst_nss_install_test_ca(pst_nss_backend_state *s,
                                          const char *path)
{
    FILE *file;
    long length;
    unsigned char *data;
    SECItem item;
    CERTCertTrust trust;
    CERTCertDBHandle *database;
    file = fopen(path, "rb");
    if (file == NULL) return PST_RESULT_INVALID_ARGUMENT;
    if (fseek(file, 0, SEEK_END) != 0) { fclose(file); return PST_RESULT_RESOURCE_FAILURE; }
    length = ftell(file);
    if (length <= 0 || (unsigned long)length > (unsigned long)UINT_MAX) {
        fclose(file); return PST_RESULT_INVALID_ARGUMENT;
    }
    if (fseek(file, 0, SEEK_SET) != 0) { fclose(file); return PST_RESULT_RESOURCE_FAILURE; }
    data = (unsigned char *)malloc((size_t)length);
    if (data == NULL) { fclose(file); return PST_RESULT_OUT_OF_MEMORY; }
    if (fread(data, 1, (size_t)length, file) != (size_t)length) {
        free(data); fclose(file); return PST_RESULT_RESOURCE_FAILURE;
    }
    fclose(file); item.type = siDERCertBuffer; item.data = data;
    item.len = (unsigned int)length; database = s->cert_default_db();
    s->test_ca = s->cert_new_temp(database, &item, "pst-phase3-test-ca",
                                  PR_FALSE, PR_TRUE);
    free(data);
    if (s->test_ca == NULL) return PST_RESULT_AUTH_FAILURE;
    memset(&trust, 0, sizeof(trust));
    trust.sslFlags = CERTDB_VALID_CA | CERTDB_TRUSTED_CA |
                     CERTDB_TRUSTED_CLIENT_CA;
    if (s->cert_change_trust(database, s->test_ca, &trust) != SECSuccess) {
        s->cert_destroy(s->test_ca); s->test_ca = NULL;
        return PST_RESULT_AUTH_FAILURE;
    }
    s->has_database = 1; pst_nss_trace("test_ca", path);
    return PST_RESULT_OK;
}static PST_RESULT pst_nss_initialize(void **out_state)
{
    pst_nss_backend_state *s;
    char db_dir[1024];
    char ca_path[1024];
    DWORD length;
    SECStatus status;
    PST_RESULT result;
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
    length = GetEnvironmentVariableA("PST_NSS_TEST_CA_DER", ca_path, sizeof(ca_path));
    if (length >= sizeof(ca_path)) {
        s->nss_shutdown(); s->pr_cleanup(); pst_nss_unload(s); free(s);
        return PST_RESULT_INVALID_ARGUMENT;
    }
    if (length != 0) {
        result = pst_nss_install_test_ca(s, ca_path);
        if (result != PST_RESULT_OK) {
            s->nss_shutdown(); s->pr_cleanup(); pst_nss_unload(s); free(s);
            return result;
        }
    }
    pst_nss_trace_module("module_nspr4", s->nspr_module);
    pst_nss_trace_module("module_nss3", s->nss_module);
    pst_nss_trace_module("module_ssl3", s->ssl_module);
    pst_nss_trace_module("module_nssutil3", GetModuleHandleA("nssutil3.dll"));
    pst_nss_trace_module("module_plc4", GetModuleHandleA("plc4.dll"));
    pst_nss_trace_module("module_plds4", GetModuleHandleA("plds4.dll"));
    pst_nss_trace_module("module_softokn3", GetModuleHandleA("softokn3.dll"));
    pst_nss_trace_module("module_freebl3", GetModuleHandleA("freebl3.dll"));
    s->initialized = 1; pst_nss_active = 1; *out_state = s;
    return PST_RESULT_OK;
}
static void pst_nss_shutdown(void *state)
{
    pst_nss_backend_state *s = (pst_nss_backend_state *)state;
    if (s == NULL) return;
    if (s->initialized) {
        if (s->test_ca != NULL) {
            s->cert_destroy(s->test_ca); s->test_ca = NULL;
        }
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
        PST_BACKEND_CAP_NONBLOCKING | PST_BACKEND_CAP_BACKEND_WAIT |
        PST_BACKEND_CAP_HOSTNAME_VERIFY | PST_BACKEND_CAP_CLIENT_AUTH |
        PST_BACKEND_CAP_CUSTOM_TRUST | PST_BACKEND_CAP_PEER_INFO |
        PST_BACKEND_CAP_ALPN;
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
    c->ssl_fd = NULL;
    if (c->local_certificate != NULL) c->runtime->backend->cert_destroy(c->local_certificate);
    if (c->local_key != NULL) c->runtime->backend->key_destroy(c->local_key);
    if (c->trust_anchor != NULL) c->runtime->backend->cert_destroy(c->trust_anchor);
    free(c->expected_hostname); free(c->alpn); free(c);
}
static SECStatus PR_CALLBACK pst_nss_client_auth(void *arg, PRFileDesc *fd,
 CERTDistNames *names, CERTCertificate **cert, SECKEYPrivateKey **key)
{
 pst_nss_connection_state *c=(pst_nss_connection_state *)arg;
 (void)fd;(void)names;*cert=NULL;*key=NULL;
 if(c==NULL||c->local_certificate==NULL||c->local_key==NULL)return SECFailure;
 *cert=c->runtime->backend->cert_dup(c->local_certificate);
 *key=c->runtime->backend->key_copy(c->local_key);
 if(*cert==NULL||*key==NULL){if(*cert)c->runtime->backend->cert_destroy(*cert);if(*key)c->runtime->backend->key_destroy(*key);*cert=NULL;*key=NULL;return SECFailure;}
 return SECSuccess;
}
static PST_RESULT pst_nss_configure_identity(void *state,const pst_config *config)
{
 pst_nss_connection_state *c=(pst_nss_connection_state*)state;pst_nss_backend_state *s;
 const pst_trust *trust;const pst_credentials *credentials;const pst_u8 *data;SECItem item;CERTCertTrust flags;PK11SlotInfo *slot;const char *host;pst_size n;
 if(!c||!config||!pst_config_is_frozen(config)||c->ssl_fd||c->expected_hostname)return PST_RESULT_INVALID_ARGUMENT;
 s=c->runtime->backend;trust=pst_config_trust(config);credentials=pst_config_credentials(config);
 if(trust){if(pst_trust_kind(trust)==PST_TRUST_SOURCE_SYSTEM)return PST_RESULT_UNSUPPORTED;if(pst_trust_kind(trust)!=PST_TRUST_SOURCE_CUSTOM_CA_DER)return PST_RESULT_UNSUPPORTED;
  data=pst_trust_data(trust,&n);if(n>(pst_size)UINT_MAX)return PST_RESULT_INVALID_ARGUMENT;item.type=siDERCertBuffer;item.data=(unsigned char*)data;item.len=(unsigned int)n;
  c->trust_anchor=s->cert_new_temp(s->cert_default_db(),&item,"pst-custom-trust",PR_FALSE,PR_TRUE);if(!c->trust_anchor)return PST_RESULT_AUTH_FAILURE;
  memset(&flags,0,sizeof(flags));flags.sslFlags=CERTDB_VALID_CA|CERTDB_TRUSTED_CA|CERTDB_TRUSTED_CLIENT_CA;
  if(s->cert_change_trust(s->cert_default_db(),c->trust_anchor,&flags)!=SECSuccess)return PST_RESULT_AUTH_FAILURE;s->has_database=1;}
 if(credentials){data=pst_credentials_certificate_der(credentials,&n);if(n>(pst_size)UINT_MAX)return PST_RESULT_INVALID_ARGUMENT;item.type=siDERCertBuffer;item.data=(unsigned char*)data;item.len=(unsigned int)n;
  c->local_certificate=s->cert_new_temp(s->cert_default_db(),&item,"pst-local-credential",PR_FALSE,PR_TRUE);if(!c->local_certificate)return PST_RESULT_AUTH_FAILURE;
  data=pst_credentials_private_key_der(credentials,&n);if(n>(pst_size)UINT_MAX)return PST_RESULT_INVALID_ARGUMENT;item.type=siBuffer;item.data=(unsigned char*)data;item.len=(unsigned int)n;slot=s->pk11_get_slot();if(!slot)return PST_RESULT_BACKEND_FAILURE;
  if(s->pk11_import_key(slot,&item,NULL,NULL,PR_FALSE,PR_TRUE,KU_ALL,&c->local_key,NULL)!=SECSuccess){s->pk11_free_slot(slot);return PST_RESULT_AUTH_FAILURE;}s->pk11_free_slot(slot);}
 host=pst_config_expected_hostname(config);if(host){n=strlen(host);c->expected_hostname=(char*)malloc(n+1);if(!c->expected_hostname)return PST_RESULT_OUT_OF_MEMORY;memcpy(c->expected_hostname,host,n+1);}
 data=pst_config_alpn_wire(config,&n);if(n){c->alpn=(pst_u8*)malloc(n);if(!c->alpn)return PST_RESULT_OUT_OF_MEMORY;memcpy(c->alpn,data,n);c->alpn_size=n;}c->minimum_version=pst_config_minimum_version(config);c->maximum_version=pst_config_maximum_version(config);c->alpn_requirement=pst_config_alpn_requirement(config);
 c->require_peer=pst_config_require_peer_authentication(config);pst_nss_trace("PST_identity_config","ok");return PST_RESULT_OK;
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
        (c->expected_hostname == NULL && (t->hostname == NULL || t->hostname[0] == '\0')))
        return PST_RESULT_INVALID_ARGUMENT;
    s = c->runtime->backend; socket_value = (SOCKET)t->native_socket;
    nonblocking = 1UL;
    if (ioctlsocket(socket_value, FIONBIO, &nonblocking) != 0) {
        c->last_error = (pst_i32)WSAGetLastError();
        return PST_RESULT_TRANSPORT_FAILURE;
    }
    pst_nss_trace("FIONBIO", "ok");
    tcp_fd = s->pr_import_tcp((PROsfd)socket_value);
    if (tcp_fd == NULL) {
        pst_nss_capture_error(s, &c->last_error);
        return pst_backend_nss_normalize_error(c->last_error);
    }
    *ownership_accepted = 1UL;
    pst_nss_trace("PR_ImportTCPSocket", "ok ownership_accepted=1");
    option.option = PR_SockOpt_Nonblocking;
    option.value.non_blocking = PR_TRUE;
    if (s->pr_set_socket_option(tcp_fd, &option) != PR_SUCCESS) {
        pst_nss_capture_error(s, &c->last_error); s->pr_close(tcp_fd);
        return pst_backend_nss_normalize_error(c->last_error);
    }
    pst_nss_trace("PR_SockOpt_Nonblocking", "ok");
    ssl_fd = s->ssl_import_fd(NULL, tcp_fd);
    if (ssl_fd == NULL) {
        pst_nss_capture_error(s, &c->last_error); s->pr_close(tcp_fd);
        return pst_backend_nss_normalize_error(c->last_error);
    }
    pst_nss_trace("SSL_ImportFD", "ok");
    { SSLVersionRange range; range.min=(PRUint16)(c->minimum_version==PST_TLS_VERSION_1_3?SSL_LIBRARY_VERSION_TLS_1_3:SSL_LIBRARY_VERSION_TLS_1_2);range.max=(PRUint16)(c->maximum_version==PST_TLS_VERSION_1_3?SSL_LIBRARY_VERSION_TLS_1_3:SSL_LIBRARY_VERSION_TLS_1_2);if(s->ssl_version_set(ssl_fd,&range)!=SECSuccess||(c->alpn_size&&s->ssl_set_alpn(ssl_fd,c->alpn,(unsigned int)c->alpn_size)!=SECSuccess)){pst_nss_capture_error(s,&c->last_error);s->pr_close(ssl_fd);return pst_backend_nss_normalize_error(c->last_error);}}
    if (s->ssl_option_set(ssl_fd, SSL_SECURITY, PR_TRUE) != SECSuccess ||
        s->ssl_option_set(ssl_fd, SSL_HANDSHAKE_AS_CLIENT, PR_TRUE) != SECSuccess ||
        s->ssl_option_set(ssl_fd, SSL_ENABLE_SSL3, PR_FALSE) != SECSuccess ||
        (c->alpn_size != 0 && s->ssl_option_set(ssl_fd, SSL_ENABLE_ALPN, PR_TRUE) != SECSuccess) ||
        s->ssl_set_url(ssl_fd, c->expected_hostname != NULL ? c->expected_hostname : t->hostname) != SECSuccess ||
        s->ssl_reset_handshake(ssl_fd, PR_FALSE) != SECSuccess) {
        pst_nss_capture_error(s, &c->last_error); s->pr_close(ssl_fd);
        return pst_backend_nss_normalize_error(c->last_error);
    }
    if ((s->has_database || c->require_peer) &&
        s->ssl_auth_hook(ssl_fd, (SSLAuthCertificate)s->ssl_auth_certificate,
                         s->cert_default_db()) != SECSuccess) {
        pst_nss_capture_error(s, &c->last_error); s->pr_close(ssl_fd);
        return pst_backend_nss_normalize_error(c->last_error);
    }
    if (c->local_certificate != NULL &&
        s->ssl_client_auth_hook(ssl_fd,pst_nss_client_auth,c)!=SECSuccess) {
        pst_nss_capture_error(s,&c->last_error);s->pr_close(ssl_fd);
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
    SECStatus handshake_status;
    DWORD handshake_start, handshake_end;
    char detail[192];
    if (c == NULL || c->ssl_fd == NULL || operation == NULL || error == NULL)
        return PST_RESULT_INVALID_ARGUMENT;
    s = c->runtime->backend;
    sprintf(detail, "BEGIN state=%s interest=0x%08lx",
            c->handshake_complete ? "established" : "handshaking",
            (unsigned long)c->interest);
    pst_nss_trace("SSL_ForceHandshake", detail);
    handshake_start = GetTickCount();
    handshake_status = s->ssl_force_handshake(c->ssl_fd);
    handshake_end = GetTickCount();
    if (handshake_status != SECSuccess)
        pst_nss_capture_error(s, &c->last_error);
    sprintf(detail, "END status=%d error=%ld would_block=%d duration_ms=%lu",
            (int)handshake_status,
            handshake_status == SECSuccess ? 0L : (long)c->last_error,
            handshake_status != SECSuccess &&
                pst_backend_nss_is_would_block(c->last_error),
            (unsigned long)(handshake_end - handshake_start));
    pst_nss_trace("SSL_ForceHandshake", detail);
    if (handshake_status == SECSuccess) {
        if(c->alpn_requirement==PST_FEATURE_REQUIRED){unsigned char value[256];unsigned int n=0;SSLNextProtoState st;char alpn_detail[64];SECStatus ar=s->ssl_get_alpn(c->ssl_fd,&st,value,&n,sizeof(value));sprintf(alpn_detail,"status=%d state=%d length=%u",(int)ar,(int)st,n);pst_nss_trace("ALPN",alpn_detail);if(ar!=SECSuccess||(st!=SSL_NEXT_PROTO_NEGOTIATED&&st!=SSL_NEXT_PROTO_SELECTED)){c->interest=PST_BACKEND_INTEREST_NONE;*operation=PST_BACKEND_OPERATION_FAILED;*error=PST_RESULT_POLICY_VIOLATION;return PST_RESULT_OK;}}
        c->interest = PST_BACKEND_INTEREST_NONE;
        *operation = PST_BACKEND_OPERATION_COMPLETE; *error = PST_RESULT_OK;
        c->handshake_complete = 1;
        pst_nss_trace("SSL_ForceHandshake_class", "COMPLETE");
        pst_nss_trace_module("module_softokn3", GetModuleHandleA("softokn3.dll"));
        pst_nss_trace_module("module_freebl3", GetModuleHandleA("freebl3.dll"));
        return PST_RESULT_OK;
    }
    if (pst_backend_nss_is_would_block(c->last_error)) {
        c->interest = PST_BACKEND_INTEREST_READ | PST_BACKEND_INTEREST_WRITE;
        *operation = PST_BACKEND_OPERATION_NEED_READ_WRITE; *error = PST_RESULT_OK;
        pst_nss_trace("SSL_ForceHandshake_class", "PR_WOULD_BLOCK_ERROR");
        pst_nss_trace_module("module_softokn3", GetModuleHandleA("softokn3.dll"));
        pst_nss_trace_module("module_freebl3", GetModuleHandleA("freebl3.dll"));
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
    DWORD poll_start, poll_end;
    int pending;
    char detail[256];
    if (c == NULL || c->ssl_fd == NULL || result == NULL)
        return PST_RESULT_INVALID_ARGUMENT;
    s = c->runtime->backend; result->ready_interest = 0UL; result->timed_out = 0UL;
    poll_desc.fd = c->ssl_fd; poll_desc.in_flags = 0; poll_desc.out_flags = 0;
    if ((interest & PST_BACKEND_INTEREST_READ) != 0UL) poll_desc.in_flags |= PR_POLL_READ;
    if ((interest & PST_BACKEND_INTEREST_WRITE) != 0UL) poll_desc.in_flags |= PR_POLL_WRITE;
    pending = s->ssl_data_pending == NULL ? -1 : s->ssl_data_pending(c->ssl_fd);
    sprintf(detail, "BEGIN state=%s interest=0x%08lx in=0x%04x timeout_ms=%lu pending=%d",
            c->handshake_complete ? "established" : "handshaking",
            (unsigned long)interest, (unsigned int)poll_desc.in_flags,
            (unsigned long)timeout_ms, pending);
    pst_nss_trace("PR_Poll", detail);
    poll_start = GetTickCount();
    count = s->pr_poll(&poll_desc, 1, s->pr_ms_interval((PRUint32)timeout_ms));
    poll_end = GetTickCount();
    if (count < 0) pst_nss_capture_error(s, &c->last_error);
    pending = s->ssl_data_pending == NULL ? -1 : s->ssl_data_pending(c->ssl_fd);
    sprintf(detail, "END result=%ld duration_ms=%lu out=0x%04x read=%d write=%d err=%d hup=%d nval=%d pending=%d error=%ld",
            (long)count, (unsigned long)(poll_end - poll_start),
            (unsigned int)poll_desc.out_flags,
            (poll_desc.out_flags & PR_POLL_READ) != 0,
            (poll_desc.out_flags & PR_POLL_WRITE) != 0,
            (poll_desc.out_flags & PR_POLL_ERR) != 0,
            (poll_desc.out_flags & PR_POLL_HUP) != 0,
            (poll_desc.out_flags & PR_POLL_NVAL) != 0, pending,
            count < 0 ? (long)c->last_error : 0L);
    pst_nss_trace("PR_Poll", detail);
    if (count == 0) {
        result->timed_out = 1UL; pst_nss_trace("PR_Poll_class", "timeout result=OK");
        return PST_RESULT_OK;
    }
    if (count < 0) {
        pst_nss_trace("PR_Poll_class", "error result=normalized");
        return pst_backend_nss_normalize_error(c->last_error);
    }
    if ((poll_desc.out_flags & PR_POLL_READ) != 0) result->ready_interest |= PST_BACKEND_INTEREST_READ;
    if ((poll_desc.out_flags & PR_POLL_WRITE) != 0) result->ready_interest |= PST_BACKEND_INTEREST_WRITE;
    if ((poll_desc.out_flags & (PR_POLL_ERR | PR_POLL_NVAL)) != 0) {
        pst_nss_trace("PR_Poll_class", "ERR_OR_NVAL result=TRANSPORT_FAILURE");
        return PST_RESULT_TRANSPORT_FAILURE;
    }
    if ((poll_desc.out_flags & PR_POLL_HUP) != 0) {
        pst_nss_trace("PR_Poll_class", "HUP result=CLOSED");
        return PST_RESULT_CLOSED;
    }
    pst_nss_trace("PR_Poll_class", "ready result=OK");
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
    PRInt32 amount, requested;
    DWORD read_start, read_end;
    int pending;
    char detail[256];
    if (c == NULL || c->ssl_fd == NULL || buffer == NULL || result == NULL)
        return PST_RESULT_INVALID_ARGUMENT;
    pst_nss_io_reset(result); s = c->runtime->backend;
    requested = capacity > (pst_size)INT_MAX ? INT_MAX : (PRInt32)capacity;
    pending = s->ssl_data_pending == NULL ? -1 : s->ssl_data_pending(c->ssl_fd);
    sprintf(detail, "BEGIN requested=%ld interest=0x%08lx pending=%d",
            (long)requested, (unsigned long)c->interest, pending);
    pst_nss_trace("PR_Read", detail);
    read_start = GetTickCount();
    amount = s->pr_read(c->ssl_fd, buffer, requested);
    read_end = GetTickCount();
    if (amount < 0) pst_nss_capture_error(s, &c->last_error);
    pending = s->ssl_data_pending == NULL ? -1 : s->ssl_data_pending(c->ssl_fd);
    sprintf(detail, "END ret=%ld error=%ld would_block=%d pending=%d duration_ms=%lu",
            (long)amount, amount < 0 ? (long)c->last_error : 0L,
            amount < 0 && pst_backend_nss_is_would_block(c->last_error), pending,
            (unsigned long)(read_end - read_start));
    pst_nss_trace("PR_Read", detail);
    if (amount > 0) {
        result->bytes_transferred = (pst_size)amount;
        result->operation = PST_BACKEND_OPERATION_COMPLETE;
        pst_nss_trace("PR_Read_class", "COMPLETE");
        c->interest = PST_BACKEND_INTEREST_NONE; return PST_RESULT_OK;
    }
    if (amount == 0) {
        result->operation = PST_BACKEND_OPERATION_CLOSED;
        result->close_kind = PST_BACKEND_CLOSE_CLEAN;
        pst_nss_trace("PR_Read_class", "CLOSED clean");
        c->interest = PST_BACKEND_INTEREST_NONE; return PST_RESULT_OK;
    }
    if (pst_backend_nss_is_would_block(c->last_error)) {
        result->operation = PST_BACKEND_OPERATION_NEED_READ_WRITE;
        c->interest = PST_BACKEND_INTEREST_READ | PST_BACKEND_INTEREST_WRITE;
        pst_nss_trace("PR_Read_class", "NEED_READ_WRITE");
        return PST_RESULT_OK;
    }
    result->operation = PST_BACKEND_OPERATION_FAILED;
    result->error = pst_backend_nss_normalize_error(c->last_error);
    if (result->error == PST_RESULT_TRUNCATED)
        result->close_kind = PST_BACKEND_CLOSE_TRUNCATED;
    pst_nss_trace("PR_Read_class", "FAILED normalized");
    c->interest = PST_BACKEND_INTEREST_NONE; return PST_RESULT_OK;
}
static PST_RESULT pst_nss_write(void *state, const void *buffer, pst_size length,
                                PST_BACKEND_IO_RESULT *result)
{
    pst_nss_connection_state *c = (pst_nss_connection_state *)state;
    pst_nss_backend_state *s;
    PRInt32 amount, requested;
    DWORD write_start, write_end;
    char detail[192];
    if (c == NULL || c->ssl_fd == NULL || buffer == NULL || result == NULL)
        return PST_RESULT_INVALID_ARGUMENT;
    pst_nss_io_reset(result); s = c->runtime->backend;
    requested = length > (pst_size)INT_MAX ? INT_MAX : (PRInt32)length;
    sprintf(detail, "BEGIN requested=%ld interest=0x%08lx",
            (long)requested, (unsigned long)c->interest);
    pst_nss_trace("PR_Write", detail);
    write_start = GetTickCount();
    amount = s->pr_write(c->ssl_fd, buffer, requested);
    write_end = GetTickCount();
    if (amount < 0) pst_nss_capture_error(s, &c->last_error);
    sprintf(detail, "END ret=%ld error=%ld would_block=%d duration_ms=%lu",
            (long)amount, amount < 0 ? (long)c->last_error : 0L,
            amount < 0 && pst_backend_nss_is_would_block(c->last_error),
            (unsigned long)(write_end - write_start));
    pst_nss_trace("PR_Write", detail);
    if (amount >= 0) {
        result->bytes_transferred = (pst_size)amount;
        result->operation = amount == 0 && length != 0 ?
            PST_BACKEND_OPERATION_NEED_READ_WRITE : PST_BACKEND_OPERATION_COMPLETE;
        c->interest = amount == 0 && length != 0 ?
            PST_BACKEND_INTEREST_READ | PST_BACKEND_INTEREST_WRITE : PST_BACKEND_INTEREST_NONE;
        pst_nss_trace("PR_Write_class", amount > 0 ? "progress" : "zero");
        return PST_RESULT_OK;
    }
    if (pst_backend_nss_is_would_block(c->last_error)) {
        result->operation = PST_BACKEND_OPERATION_NEED_READ_WRITE;
        c->interest = PST_BACKEND_INTEREST_READ | PST_BACKEND_INTEREST_WRITE;
        return PST_RESULT_OK;
    }
    result->operation = PST_BACKEND_OPERATION_FAILED;
    result->error = pst_backend_nss_normalize_error(c->last_error);
    c->interest = PST_BACKEND_INTEREST_NONE; return PST_RESULT_OK;
}
pst_u32 pst_backend_nss_connection_protocol_version(const void *state)
{
    const pst_nss_connection_state *c = (const pst_nss_connection_state *)state;
    SSLChannelInfo info;
    if (c == NULL || c->ssl_fd == NULL) return 0UL;
    memset(&info, 0, sizeof(info));
    if (c->runtime->backend->ssl_channel_info(c->ssl_fd, &info,
                                              sizeof(info)) != SECSuccess)
        return 0UL;
    return (pst_u32)info.protocolVersion;
}
static PST_RESULT pst_nss_peer_info_create(void *state, void **out)
{
    pst_nss_connection_state *c = (pst_nss_connection_state *)state;
    CERTCertificate *cert; SSLChannelInfo info;
    PST_PEER_INFO_SUMMARY summary; PST_RESULT result;
    if (c == NULL || out == NULL) return PST_RESULT_INVALID_ARGUMENT;
    *out = NULL; if (!c->handshake_complete) return PST_RESULT_INVALID_STATE;
    memset(&info, 0, sizeof(info));
    if (c->runtime->backend->ssl_channel_info(c->ssl_fd, &info, sizeof(info)) != SECSuccess)
        return PST_RESULT_BACKEND_FAILURE;
    cert = c->runtime->backend->ssl_peer_certificate(c->ssl_fd);
    if (cert == NULL) return PST_RESULT_UNAVAILABLE;
    memset(&summary, 0, sizeof(summary)); summary.struct_size = sizeof(summary);
    summary.api_version = PST_API_VERSION;
    summary.certificate_present = PST_KNOWN_TRUE;
    summary.chain_validated = c->require_peer ? PST_KNOWN_TRUE : PST_KNOWN_UNKNOWN;
    summary.hostname_validated = c->require_peer ? PST_KNOWN_TRUE : PST_KNOWN_UNKNOWN;
    summary.peer_authenticated = c->require_peer ? PST_KNOWN_TRUE : PST_KNOWN_UNKNOWN;
    summary.tls_version = (pst_u32)info.protocolVersion;
    summary.cipher_suite = (pst_u32)info.cipherSuite;
    summary.alpn_available = PST_KNOWN_UNSUPPORTED;
    summary.session_resumed = info.resumed ? PST_KNOWN_TRUE : PST_KNOWN_FALSE;
    summary.early_data_accepted = PST_KNOWN_UNSUPPORTED;
    summary.certificate_sha256_size = 32;
    summary.leaf_der_size = (pst_size)cert->derCert.len;
    if (c->runtime->backend->pk11_hash(SEC_OID_SHA256,
        summary.certificate_sha256, cert->derCert.data,
        (PRInt32)cert->derCert.len) != SECSuccess) {
        c->runtime->backend->cert_destroy(cert); return PST_RESULT_BACKEND_FAILURE;
    }
    result = pst_peer_info_create_snapshot(&summary, cert->derCert.data,
                                            (pst_peer_info **)out);
    c->runtime->backend->cert_destroy(cert); return result;
}
static void pst_nss_peer_info_destroy(void *value)
{
    pst_peer_info_release((pst_peer_info *)value);
}
static PST_RESULT pst_nss_get_alpn(void *state,pst_u8 *buffer,pst_size capacity,pst_size *out_size){pst_nss_connection_state *c=(pst_nss_connection_state*)state;SSLNextProtoState st;unsigned int n=0;if(!c||!out_size)return PST_RESULT_INVALID_ARGUMENT;*out_size=0;if(capacity>(pst_size)UINT_MAX)return PST_RESULT_INVALID_ARGUMENT;if(c->runtime->backend->ssl_get_alpn(c->ssl_fd,&st,buffer,&n,(unsigned int)capacity)!=SECSuccess)return PST_RESULT_BACKEND_FAILURE;*out_size=n;if(st==SSL_NEXT_PROTO_NO_SUPPORT||st==SSL_NEXT_PROTO_NO_OVERLAP)return PST_RESULT_UNAVAILABLE;return PST_RESULT_OK;}
static PST_RESULT pst_nss_shutdown_step(void *state, pst_u32 *operation, PST_RESULT *error)
{
    pst_nss_connection_state *c = (pst_nss_connection_state *)state;
    pst_nss_backend_state *s;
    if (c == NULL || c->ssl_fd == NULL || operation == NULL || error == NULL)
        return PST_RESULT_INVALID_ARGUMENT;
    s = c->runtime->backend;
    if (s->pr_shutdown(c->ssl_fd, PR_SHUTDOWN_BOTH) == PR_SUCCESS) {
        *operation = PST_BACKEND_OPERATION_COMPLETE; *error = PST_RESULT_OK;
        c->interest = PST_BACKEND_INTEREST_NONE;
        pst_nss_trace("PR_Shutdown", "complete"); return PST_RESULT_OK;
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
    pst_nss_write, pst_nss_shutdown_step, pst_nss_peer_info_create,
    pst_nss_peer_info_destroy, pst_nss_configure_identity, pst_nss_get_alpn
};
static const PST_BACKEND_DESCRIPTOR pst_nss_descriptor = {
    sizeof(PST_BACKEND_DESCRIPTOR), PST_BACKEND_SPI_VERSION,
    "retrozilla-nss", "RetroZilla NSS/NSPR",
    PST_BACKEND_CAP_TLS_1_2 | PST_BACKEND_CAP_TLS_1_3 |
    PST_BACKEND_CAP_HOSTNAME_VERIFY | PST_BACKEND_CAP_NONBLOCKING |
    PST_BACKEND_CAP_BACKEND_WAIT | PST_BACKEND_CAP_CLIENT_AUTH |
    PST_BACKEND_CAP_CUSTOM_TRUST | PST_BACKEND_CAP_PEER_INFO |
    PST_BACKEND_CAP_ALPN, &pst_nss_vtable
};
const PST_BACKEND_DESCRIPTOR *pst_backend_nss_descriptor(void)
{
    return &pst_nss_descriptor;
}
PST_RESULT pst_backend_nss_register(void)
{
    return pst_backend_register(&pst_nss_descriptor);
}
typedef struct pst_win32_transport { pst_transport base; PST_NSS_NATIVE_TRANSPORT native; } pst_win32_transport;
static void pst_win32_transport_destroy(pst_transport *base,int consumed){pst_win32_transport *t=(pst_win32_transport*)base;if(!consumed)closesocket((SOCKET)t->native.native_socket);free(t);}
PST_RESULT PST_CALL pst_win32_register_retrozilla_nss(void){PST_RESULT r=pst_backend_nss_register();return r==PST_RESULT_INVALID_STATE?PST_RESULT_OK:r;}
PST_RESULT PST_CALL pst_win32_socket_transport_create(pst_size socket_value,pst_transport **out){pst_win32_transport *t;if(!out)return PST_RESULT_INVALID_ARGUMENT;*out=NULL;t=(pst_win32_transport*)calloc(1,sizeof(*t));if(!t)return PST_RESULT_OUT_OF_MEMORY;t->base.backend_id="retrozilla-nss";t->base.native=&t->native;t->base.destroy=pst_win32_transport_destroy;t->native.struct_size=sizeof(t->native);t->native.version=PST_NSS_NATIVE_TRANSPORT_VERSION;t->native.kind=PST_NSS_NATIVE_TRANSPORT_KIND_WIN32_SOCKET;t->native.native_socket=socket_value;*out=&t->base;return PST_RESULT_OK;}
