#ifndef PAPINHO_SECURE_TRANSPORT_H
#define PAPINHO_SECURE_TRANSPORT_H

#include <limits.h>
#include <stddef.h>

#if defined(_MSC_VER)
# if defined(PST_BUILD_DLL)
#  define PST_API __declspec(dllexport)
# elif defined(PST_USE_DLL)
#  define PST_API __declspec(dllimport)
# else
#  define PST_API
# endif
# define PST_CALL __cdecl
#else
# define PST_API
# define PST_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define PST_API_VERSION_MAJOR 1UL
#define PST_API_VERSION_MINOR 0UL
#define PST_API_VERSION_PATCH 0UL
#define PST_API_VERSION 0x00010000UL
#define PST_LIBRARY_VERSION_MAJOR 0UL
#define PST_LIBRARY_VERSION_MINOR 1UL
#define PST_LIBRARY_VERSION_PATCH 0UL
#define PST_LIBRARY_VERSION 0x00000100UL

typedef unsigned char pst_u8;
#if USHRT_MAX == 0xffffU
typedef unsigned short pst_u16;
#else
# error PapinhoSecureTransport requires a 16-bit unsigned integer type
#endif
#if UINT_MAX == 0xffffffffUL
typedef unsigned int pst_u32;
typedef signed int pst_i32;
#elif ULONG_MAX == 0xffffffffUL
typedef unsigned long pst_u32;
typedef signed long pst_i32;
#else
# error PapinhoSecureTransport requires a 32-bit integer type
#endif
typedef size_t pst_size;

typedef char pst_check_u8_is_1[(sizeof(pst_u8) == 1) ? 1 : -1];
typedef char pst_check_u16_is_2[(sizeof(pst_u16) == 2) ? 1 : -1];
typedef char pst_check_u32_is_4[(sizeof(pst_u32) == 4) ? 1 : -1];
typedef char pst_check_i32_is_4[(sizeof(pst_i32) == 4) ? 1 : -1];

typedef pst_i32 PST_RESULT;
#define PST_RESULT_OK                  ((PST_RESULT)0)
#define PST_RESULT_INVALID_ARGUMENT    ((PST_RESULT)1)
#define PST_RESULT_INVALID_STATE       ((PST_RESULT)2)
#define PST_RESULT_UNSUPPORTED         ((PST_RESULT)3)
#define PST_RESULT_UNAVAILABLE         ((PST_RESULT)4)
#define PST_RESULT_OUT_OF_MEMORY       ((PST_RESULT)5)
#define PST_RESULT_RESOURCE_FAILURE    ((PST_RESULT)6)
#define PST_RESULT_TRANSPORT_FAILURE   ((PST_RESULT)7)
#define PST_RESULT_PROTOCOL_FAILURE    ((PST_RESULT)8)
#define PST_RESULT_AUTH_FAILURE        ((PST_RESULT)9)
#define PST_RESULT_HOSTNAME_MISMATCH   ((PST_RESULT)10)
#define PST_RESULT_POLICY_VIOLATION    ((PST_RESULT)11)
#define PST_RESULT_BACKEND_FAILURE     ((PST_RESULT)12)
#define PST_RESULT_TRUNCATED           ((PST_RESULT)13)
#define PST_RESULT_CLOSED              ((PST_RESULT)14)
#define PST_RESULT_INCOMPATIBLE_API    ((PST_RESULT)15)

typedef struct pst_runtime pst_runtime;
typedef struct pst_config pst_config;
typedef struct pst_credentials pst_credentials;
typedef struct pst_trust pst_trust;
typedef struct pst_connection pst_connection;
typedef struct pst_peer_info pst_peer_info;
typedef struct pst_transport pst_transport;

typedef struct PST_VERSION_INFO {
    pst_u32 struct_size;
    pst_u32 api_version;
    pst_u32 api_major;
    pst_u32 api_minor;
    pst_u32 api_patch;
    pst_u32 library_major;
    pst_u32 library_minor;
    pst_u32 library_patch;
} PST_VERSION_INFO;

#define PST_VERSION_INFO_MIN_SIZE ((pst_u32)sizeof(PST_VERSION_INFO))

#define PST_CREDENTIAL_SOURCE_CERT_DER_PKCS8_DER 1UL
#define PST_TRUST_SOURCE_CUSTOM_CA_DER 1UL
#define PST_TRUST_SOURCE_SYSTEM 2UL
#define PST_REQUIREMENT_DISABLED 0UL
#define PST_REQUIREMENT_REQUIRED 1UL
#define PST_KNOWN_UNKNOWN 0UL
#define PST_KNOWN_FALSE 1UL
#define PST_KNOWN_TRUE 2UL
#define PST_KNOWN_UNSUPPORTED 3UL
typedef struct PST_CREDENTIAL_SOURCE {
    pst_u32 struct_size; pst_u32 api_version; pst_u32 kind;
    const pst_u8 *certificate_der; pst_size certificate_der_size;
    const pst_u8 *private_key_der; pst_size private_key_der_size;
} PST_CREDENTIAL_SOURCE;
typedef struct PST_TRUST_SOURCE {
    pst_u32 struct_size; pst_u32 api_version; pst_u32 kind;
    const pst_u8 *data; pst_size data_size;
} PST_TRUST_SOURCE;
typedef struct PST_IDENTITY_CONFIG {
    pst_u32 struct_size; pst_u32 api_version;
    pst_credentials *credentials; pst_trust *trust;
    const char *expected_hostname; pst_size expected_hostname_size;
    pst_u32 require_peer_authentication;
    pst_u32 require_client_authentication;
} PST_IDENTITY_CONFIG;
typedef struct PST_PEER_INFO_SUMMARY {
    pst_u32 struct_size; pst_u32 api_version;
    pst_u32 certificate_present; pst_u32 chain_validated;
    pst_u32 hostname_validated; pst_u32 peer_authenticated;
    pst_u32 tls_version; pst_u32 cipher_suite;
    pst_u32 alpn_available; pst_u32 session_resumed;
    pst_u32 early_data_accepted;
    pst_u8 certificate_sha256[32];
    pst_size certificate_sha256_size; pst_size leaf_der_size;
} PST_PEER_INFO_SUMMARY;
#define PST_CREDENTIAL_SOURCE_MIN_SIZE ((pst_u32)sizeof(PST_CREDENTIAL_SOURCE))
#define PST_TRUST_SOURCE_MIN_SIZE ((pst_u32)sizeof(PST_TRUST_SOURCE))
#define PST_IDENTITY_CONFIG_MIN_SIZE ((pst_u32)sizeof(PST_IDENTITY_CONFIG))
#define PST_PEER_INFO_SUMMARY_MIN_SIZE ((pst_u32)sizeof(PST_PEER_INFO_SUMMARY))

#define PST_BACKEND_SELECTION_EXACT 1UL
#define PST_BACKEND_SELECTION_ORDERED 2UL
#define PST_BACKEND_SELECTION_AUTOMATIC 3UL
#define PST_CAP_TLS_1_2 0x00000001UL
#define PST_CAP_TLS_1_3 0x00000002UL
#define PST_CAP_CLIENT_AUTH 0x00000004UL
#define PST_CAP_ALPN 0x00000008UL
#define PST_CAP_CUSTOM_TRUST 0x00000010UL
#define PST_CAP_SYSTEM_TRUST 0x00000020UL
#define PST_CAP_HOSTNAME_VERIFY 0x00000040UL
#define PST_CAP_RESUMPTION 0x00000080UL
#define PST_CAP_EARLY_DATA 0x00000100UL
#define PST_CAP_PEER_INFO 0x00000200UL
#define PST_CAP_NONBLOCKING 0x00000400UL
#define PST_CAP_BACKEND_WAIT 0x00000800UL
#define PST_TLS_VERSION_1_2 12UL
#define PST_TLS_VERSION_1_3 13UL
#define PST_FEATURE_DISABLED 0UL
#define PST_FEATURE_OPTIONAL 1UL
#define PST_FEATURE_REQUIRED 2UL
#define PST_OPERATION_COMPLETE 0UL
#define PST_OPERATION_NEED_READ 1UL
#define PST_OPERATION_NEED_WRITE 2UL
#define PST_OPERATION_NEED_READ_WRITE 3UL
#define PST_OPERATION_CLOSED 4UL
#define PST_OPERATION_FAILED 5UL
#define PST_INTEREST_NONE 0UL
#define PST_INTEREST_READ 1UL
#define PST_INTEREST_WRITE 2UL
#define PST_CLOSE_NONE 0UL
#define PST_CLOSE_CLEAN 1UL
#define PST_CLOSE_TRUNCATED 2UL
#define PST_OWNERSHIP_TRANSFERRED 1UL
typedef struct PST_RUNTIME_OPTIONS {
 pst_u32 struct_size; pst_u32 api_version; pst_u32 selection;
 const char *exact_backend_id; const char *const *preferred_backend_ids;
 pst_size preferred_backend_count; pst_u32 required_capabilities;
} PST_RUNTIME_OPTIONS;
typedef struct PST_RUNTIME_INFO {
 pst_u32 struct_size; pst_u32 api_version; const char *backend_id;
 pst_u32 capabilities;
} PST_RUNTIME_INFO;
typedef struct PST_ALPN_PROTOCOL { const pst_u8 *data; pst_size size; } PST_ALPN_PROTOCOL;
typedef struct PST_TLS_POLICY {
 pst_u32 struct_size; pst_u32 api_version; pst_u32 minimum_version;
 pst_u32 maximum_version; const PST_ALPN_PROTOCOL *alpn_protocols;
 pst_size alpn_protocol_count; pst_u32 alpn_requirement;
 pst_u32 resumption; pst_u32 early_data; pst_u32 require_graceful_shutdown;
} PST_TLS_POLICY;
typedef struct PST_IO_RESULT { pst_size bytes_transferred; pst_u32 operation; pst_u32 close_kind; PST_RESULT error; } PST_IO_RESULT;
typedef struct PST_WAIT_RESULT { pst_u32 ready_interest; pst_u32 timed_out; } PST_WAIT_RESULT;
#define PST_RUNTIME_OPTIONS_MIN_SIZE ((pst_u32)sizeof(PST_RUNTIME_OPTIONS))
#define PST_RUNTIME_INFO_MIN_SIZE ((pst_u32)sizeof(PST_RUNTIME_INFO))
#define PST_TLS_POLICY_MIN_SIZE ((pst_u32)sizeof(PST_TLS_POLICY))

PST_API pst_u32 PST_CALL pst_api_version(void);
PST_API pst_u32 PST_CALL pst_library_version(void);
PST_API PST_RESULT PST_CALL pst_version_info_init(PST_VERSION_INFO *info);
PST_API PST_RESULT PST_CALL pst_get_version(PST_VERSION_INFO *info);
PST_API const char *PST_CALL pst_result_string(PST_RESULT result);
PST_API PST_RESULT PST_CALL pst_credentials_create(const PST_CREDENTIAL_SOURCE *source, pst_credentials **out_credentials);
PST_API void PST_CALL pst_credentials_release(pst_credentials *credentials);
PST_API PST_RESULT PST_CALL pst_trust_create(const PST_TRUST_SOURCE *source, pst_trust **out_trust);
PST_API void PST_CALL pst_trust_release(pst_trust *trust);
PST_API PST_RESULT PST_CALL pst_config_create(pst_config **out_config);
PST_API PST_RESULT PST_CALL pst_config_set_identity(pst_config *config, const PST_IDENTITY_CONFIG *identity);
PST_API PST_RESULT PST_CALL pst_config_freeze(pst_config *config);
PST_API void PST_CALL pst_config_release(pst_config *config);
PST_API PST_RESULT PST_CALL pst_peer_info_get_summary(const pst_peer_info *peer_info, PST_PEER_INFO_SUMMARY *summary);
PST_API PST_RESULT PST_CALL pst_peer_info_copy_leaf_der(const pst_peer_info *peer_info, pst_u8 *buffer, pst_size capacity, pst_size *out_size);
PST_API void PST_CALL pst_peer_info_release(pst_peer_info *peer_info);
PST_API PST_RESULT PST_CALL pst_runtime_create(const PST_RUNTIME_OPTIONS *options, pst_runtime **out_runtime);
PST_API void PST_CALL pst_runtime_release(pst_runtime *runtime);
PST_API PST_RESULT PST_CALL pst_runtime_get_info(const pst_runtime *runtime, PST_RUNTIME_INFO *info);
PST_API PST_RESULT PST_CALL pst_config_set_tls_policy(pst_config *config, const PST_TLS_POLICY *policy);
PST_API PST_RESULT PST_CALL pst_connection_create(pst_runtime *runtime, pst_config *config, pst_connection **out_connection);
PST_API PST_RESULT PST_CALL pst_connection_attach(pst_connection *connection, pst_transport *transport, pst_u32 ownership, pst_u32 *ownership_accepted);
PST_API PST_RESULT PST_CALL pst_connection_handshake(pst_connection *connection, pst_u32 *operation, PST_RESULT *error);
PST_API PST_RESULT PST_CALL pst_connection_get_interest(pst_connection *connection, pst_u32 *interest);
PST_API PST_RESULT PST_CALL pst_connection_wait(pst_connection *connection, pst_u32 timeout_ms, PST_WAIT_RESULT *result);
PST_API PST_RESULT PST_CALL pst_connection_read(pst_connection *connection, void *buffer, pst_size capacity, PST_IO_RESULT *result);
PST_API PST_RESULT PST_CALL pst_connection_write(pst_connection *connection, const void *buffer, pst_size length, PST_IO_RESULT *result);
PST_API PST_RESULT PST_CALL pst_connection_get_peer_info(pst_connection *connection, pst_peer_info **out_peer_info);
PST_API PST_RESULT PST_CALL pst_connection_get_negotiated_alpn(pst_connection *connection, pst_u8 *buffer, pst_size capacity, pst_size *out_size);
PST_API PST_RESULT PST_CALL pst_connection_shutdown(pst_connection *connection, pst_u32 *operation, PST_RESULT *error);
PST_API void PST_CALL pst_connection_release(pst_connection *connection);
PST_API void PST_CALL pst_transport_release(pst_transport *transport);

#ifdef __cplusplus
}
#endif
#endif
