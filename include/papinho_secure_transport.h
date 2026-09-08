/* SPDX-License-Identifier: MPL-2.0 */
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
#define PST_API_VERSION_MAJOR 2UL
#define PST_API_VERSION_MINOR 0UL
#define PST_API_VERSION_PATCH 0UL
#define PST_API_VERSION 0x00020000UL
#define PST_LIBRARY_VERSION_MAJOR 0UL
#define PST_LIBRARY_VERSION_MINOR 5UL
#define PST_LIBRARY_VERSION_PATCH 0UL
#define PST_LIBRARY_VERSION 0x00000500UL
typedef unsigned char pst_u8;
#if USHRT_MAX == 0xffffU
typedef unsigned short pst_u16;
#else
# error PapinhoSecureTransport requires a 16-bit unsigned integer type
#endif
#if UINT_MAX == 0xffffffffUL
typedef unsigned int pst_u32; typedef signed int pst_i32;
#elif ULONG_MAX == 0xffffffffUL
typedef unsigned long pst_u32; typedef signed long pst_i32;
#else
# error PapinhoSecureTransport requires a 32-bit integer type
#endif
typedef size_t pst_size;
typedef char pst_check_u8_is_1[(sizeof(pst_u8)==1)?1:-1];
typedef char pst_check_u16_is_2[(sizeof(pst_u16)==2)?1:-1];
typedef char pst_check_u32_is_4[(sizeof(pst_u32)==4)?1:-1];
typedef char pst_check_i32_is_4[(sizeof(pst_i32)==4)?1:-1];
typedef pst_i32 PST_RESULT;
#define PST_RESULT_OK ((PST_RESULT)0)
#define PST_RESULT_INVALID_ARGUMENT ((PST_RESULT)1)
#define PST_RESULT_INVALID_STATE ((PST_RESULT)2)
#define PST_RESULT_UNSUPPORTED ((PST_RESULT)3)
#define PST_RESULT_UNAVAILABLE ((PST_RESULT)4)
#define PST_RESULT_OUT_OF_MEMORY ((PST_RESULT)5)
#define PST_RESULT_RESOURCE_FAILURE ((PST_RESULT)6)
#define PST_RESULT_TRANSPORT_FAILURE ((PST_RESULT)7)
#define PST_RESULT_PROTOCOL_FAILURE ((PST_RESULT)8)
#define PST_RESULT_AUTH_FAILURE ((PST_RESULT)9)
#define PST_RESULT_PEER_NAME_MISMATCH ((PST_RESULT)10)
#define PST_RESULT_POLICY_VIOLATION ((PST_RESULT)11)
#define PST_RESULT_BACKEND_FAILURE ((PST_RESULT)12)
#define PST_RESULT_TRUNCATED ((PST_RESULT)13)
#define PST_RESULT_CLOSED ((PST_RESULT)14)
#define PST_RESULT_INCOMPATIBLE_API ((PST_RESULT)15)
typedef struct pst_runtime pst_runtime; typedef struct pst_credentials pst_credentials;
typedef struct pst_trust pst_trust; typedef struct pst_connection pst_connection;
typedef struct pst_peer_info pst_peer_info; typedef struct pst_transport pst_transport;
#define PST_CONNECTION_ROLE_INVALID 0UL
#define PST_CONNECTION_ROLE_CLIENT 1UL
#define PST_CONNECTION_ROLE_SERVER 2UL
#define PST_PEER_CERTIFICATE_DISABLED 0UL
#define PST_PEER_CERTIFICATE_OPTIONAL 1UL
#define PST_PEER_CERTIFICATE_REQUIRED 2UL
#define PST_FEATURE_DISABLED 0UL
#define PST_FEATURE_OPTIONAL 1UL
#define PST_FEATURE_REQUIRED 2UL
#define PST_KNOWN_UNKNOWN 0UL
#define PST_KNOWN_FALSE 1UL
#define PST_KNOWN_TRUE 2UL
#define PST_KNOWN_UNSUPPORTED 3UL
#define PST_KNOWN_NOT_APPLICABLE 4UL
#define PST_BACKEND_SELECTION_EXACT 1UL
#define PST_BACKEND_SELECTION_ORDERED 2UL
#define PST_BACKEND_SELECTION_AUTOMATIC 3UL
#define PST_CAP_TLS_1_2 0x00000001UL
#define PST_CAP_TLS_1_3 0x00000002UL
#define PST_CAP_ROLE_CLIENT 0x00000004UL
#define PST_CAP_ROLE_SERVER 0x00000008UL
#define PST_CAP_LOCAL_IDENTITY 0x00000010UL
#define PST_CAP_PEER_CERT_AUTH 0x00000020UL
#define PST_CAP_PEER_CERT_OPTIONAL 0x00000040UL
#define PST_CAP_ALPN_CLIENT 0x00000080UL
#define PST_CAP_ALPN_SERVER 0x00000100UL
#define PST_CAP_CUSTOM_TRUST 0x00000200UL
#define PST_CAP_SYSTEM_TRUST 0x00000400UL
#define PST_CAP_PEER_NAME_VERIFY 0x00000800UL
#define PST_CAP_PEER_INFO 0x00001000UL
#define PST_CAP_NONBLOCKING 0x00002000UL
#define PST_CAP_BACKEND_WAIT 0x00004000UL
#define PST_CAP_RESUMPTION 0x00008000UL
#define PST_CAP_EARLY_DATA 0x00010000UL
#define PST_CAP_KNOWN_MASK 0x0001ffffUL
#define PST_TLS_VERSION_1_2 12UL
#define PST_TLS_VERSION_1_3 13UL
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
#define PST_DIAGNOSTIC_BACKEND_ID_CAPACITY 32UL
#define PST_DIAGNOSTIC_OPERATION_NONE 0UL
#define PST_DIAGNOSTIC_OPERATION_RUNTIME 1UL
#define PST_DIAGNOSTIC_OPERATION_CONFIGURATION 2UL
#define PST_DIAGNOSTIC_OPERATION_TRANSPORT 3UL
#define PST_DIAGNOSTIC_OPERATION_CONNECTION 4UL
#define PST_DIAGNOSTIC_OPERATION_HANDSHAKE 5UL
#define PST_DIAGNOSTIC_OPERATION_AUTHENTICATION 6UL
#define PST_DIAGNOSTIC_OPERATION_READ 7UL
#define PST_DIAGNOSTIC_OPERATION_WRITE 8UL
#define PST_DIAGNOSTIC_OPERATION_WAIT 9UL
#define PST_DIAGNOSTIC_OPERATION_SHUTDOWN 10UL
#define PST_DIAGNOSTIC_OPERATION_PEER_INFO 11UL
#define PST_DIAGNOSTIC_REASON_NONE 0UL
#define PST_DIAGNOSTIC_REASON_PEER_CERT_ABSENT 1UL
#define PST_DIAGNOSTIC_REASON_PEER_CERT_INVALID 2UL
#define PST_DIAGNOSTIC_REASON_PEER_CERT_UNTRUSTED 3UL
#define PST_DIAGNOSTIC_REASON_PEER_NAME_MISMATCH 4UL
#define PST_DIAGNOSTIC_REASON_ALPN_MISMATCH 5UL
#define PST_DIAGNOSTIC_REASON_TLS_POLICY_MISMATCH 6UL
typedef struct PST_DIAGNOSTIC_INFO { pst_u32 struct_size,api_version,valid,generation; PST_RESULT normalized_result; pst_u32 operation,role,reason; char backend_id[PST_DIAGNOSTIC_BACKEND_ID_CAPACITY]; } PST_DIAGNOSTIC_INFO;
typedef pst_u32 PST_LOG_LEVEL;
#define PST_LOG_LEVEL_OFF 0UL
#define PST_LOG_LEVEL_ERROR 1UL
#define PST_LOG_LEVEL_WARN 2UL
#define PST_LOG_LEVEL_INFO 3UL
#define PST_LOG_LEVEL_DEBUG 4UL
#define PST_LOG_LEVEL_TRACE 5UL
#define PST_LOG_EVENT_RUNTIME_READY 1UL
#define PST_LOG_EVENT_RUNTIME_FAILURE 2UL
#define PST_LOG_EVENT_CONNECTION_SECURE 3UL
#define PST_LOG_EVENT_CONNECTION_FAILURE 4UL
#define PST_LOG_EVENT_AUTHENTICATION_FAILURE 5UL
#define PST_LOG_EVENT_CONNECTION_CLOSED 6UL
#define PST_LOG_EVENT_STATE_TRANSITION 7UL
#define PST_LOG_EVENT_OPERATION_PROGRESS 8UL
#define PST_LOG_CATEGORY_RUNTIME 1UL
#define PST_LOG_CATEGORY_BACKEND 2UL
#define PST_LOG_CATEGORY_CONNECTION 3UL
#define PST_LOG_CATEGORY_TLS 4UL
#define PST_LOG_CATEGORY_AUTHENTICATION 5UL
#define PST_LOG_CATEGORY_IO 6UL
#define PST_LOG_CATEGORY_READINESS 7UL
#define PST_LOG_CATEGORY_SHUTDOWN 8UL
typedef struct PST_LOG_EVENT { pst_u32 struct_size,api_version; PST_LOG_LEVEL level; pst_u32 event_id,category; PST_RESULT normalized_result; pst_u32 operation,role,peer_auth_fact,policy_fact; char backend_id[PST_DIAGNOSTIC_BACKEND_ID_CAPACITY]; } PST_LOG_EVENT;
typedef void (PST_CALL *PST_LOG_CALLBACK)(void *,const PST_LOG_EVENT *);
typedef struct PST_LOG_CONFIG { pst_u32 struct_size,api_version; PST_LOG_LEVEL level; PST_LOG_CALLBACK callback; void *user_context; } PST_LOG_CONFIG;
typedef struct PST_VERSION_INFO { pst_u32 struct_size,api_version,api_major,api_minor,api_patch,library_major,library_minor,library_patch; } PST_VERSION_INFO;
#define PST_CREDENTIAL_SOURCE_CERT_CHAIN_DER_PKCS8_DER 1UL
#define PST_TRUST_SOURCE_CUSTOM_CA_DER 1UL
#define PST_TRUST_SOURCE_SYSTEM 2UL
typedef struct PST_DER_ITEM { const pst_u8 *data; pst_size size; } PST_DER_ITEM;
typedef struct PST_CREDENTIAL_SOURCE { pst_u32 struct_size,api_version,kind; const PST_DER_ITEM *certificate_chain; pst_size certificate_count; const pst_u8 *private_key_der; pst_size private_key_der_size; } PST_CREDENTIAL_SOURCE;
typedef struct PST_TRUST_SOURCE { pst_u32 struct_size,api_version,kind; const PST_DER_ITEM *anchors; pst_size anchor_count; } PST_TRUST_SOURCE;
typedef struct PST_PROVIDER_SELECTION { pst_u32 struct_size,api_version,mode; const char *exact_provider_id; const char *const *ordered_provider_ids; pst_size ordered_provider_count; pst_u32 required_capabilities; } PST_PROVIDER_SELECTION;
typedef struct PST_LOCAL_IDENTITY { pst_u32 struct_size,api_version; pst_credentials *credentials; } PST_LOCAL_IDENTITY;
typedef struct PST_PEER_AUTH_CONFIG { pst_u32 struct_size,api_version,certificate_mode; pst_trust *trust; const char *expected_peer_name; pst_size expected_peer_name_size; } PST_PEER_AUTH_CONFIG;
typedef struct PST_ALPN_PROTOCOL { const pst_u8 *data; pst_size size; } PST_ALPN_PROTOCOL;
typedef struct PST_ALPN_CONFIG { pst_u32 struct_size,api_version,mode; const PST_ALPN_PROTOCOL *protocols; pst_size protocol_count; } PST_ALPN_CONFIG;
typedef struct PST_TLS_POLICY { pst_u32 struct_size,api_version,minimum_version,maximum_version,resumption,early_data,require_graceful_shutdown; } PST_TLS_POLICY;
typedef struct PST_CONNECTION_CONFIG { pst_u32 struct_size,api_version,role; PST_PROVIDER_SELECTION provider_selection; PST_LOCAL_IDENTITY local_identity; PST_PEER_AUTH_CONFIG peer_authentication; PST_TLS_POLICY tls; PST_ALPN_CONFIG alpn; } PST_CONNECTION_CONFIG;
typedef struct PST_RUNTIME_OPTIONS { pst_u32 struct_size,api_version; } PST_RUNTIME_OPTIONS;
typedef struct PST_RUNTIME_INFO { pst_u32 struct_size,api_version; pst_size provider_count; } PST_RUNTIME_INFO;
typedef struct PST_PROVIDER_INFO { pst_u32 struct_size,api_version,available,initialized,capabilities; char provider_id[PST_DIAGNOSTIC_BACKEND_ID_CAPACITY]; pst_u32 client_capabilities,server_capabilities; } PST_PROVIDER_INFO;
typedef struct PST_PEER_INFO_SUMMARY { pst_u32 struct_size,api_version,local_role,certificate_present,chain_validated,peer_name_validated,peer_authenticated,tls_version,cipher_suite,alpn_available,session_resumed,early_data_accepted; char provider_id[PST_DIAGNOSTIC_BACKEND_ID_CAPACITY]; pst_u8 certificate_sha256[32]; pst_size certificate_sha256_size,leaf_der_size; } PST_PEER_INFO_SUMMARY;
typedef struct PST_IO_RESULT { pst_size bytes_transferred; pst_u32 operation,close_kind; PST_RESULT error; } PST_IO_RESULT;
typedef struct PST_WAIT_RESULT { pst_u32 ready_interest,timed_out; } PST_WAIT_RESULT;
#define PST_DIAGNOSTIC_INFO_MIN_SIZE ((pst_u32)sizeof(PST_DIAGNOSTIC_INFO))
#define PST_LOG_CONFIG_MIN_SIZE ((pst_u32)sizeof(PST_LOG_CONFIG))
#define PST_LOG_EVENT_MIN_SIZE ((pst_u32)sizeof(PST_LOG_EVENT))
#define PST_VERSION_INFO_MIN_SIZE ((pst_u32)sizeof(PST_VERSION_INFO))
#define PST_CREDENTIAL_SOURCE_MIN_SIZE ((pst_u32)sizeof(PST_CREDENTIAL_SOURCE))
#define PST_TRUST_SOURCE_MIN_SIZE ((pst_u32)sizeof(PST_TRUST_SOURCE))
#define PST_PROVIDER_SELECTION_MIN_SIZE ((pst_u32)sizeof(PST_PROVIDER_SELECTION))
#define PST_LOCAL_IDENTITY_MIN_SIZE ((pst_u32)sizeof(PST_LOCAL_IDENTITY))
#define PST_PEER_AUTH_CONFIG_MIN_SIZE ((pst_u32)sizeof(PST_PEER_AUTH_CONFIG))
#define PST_ALPN_CONFIG_MIN_SIZE ((pst_u32)sizeof(PST_ALPN_CONFIG))
#define PST_TLS_POLICY_MIN_SIZE ((pst_u32)sizeof(PST_TLS_POLICY))
#define PST_CONNECTION_CONFIG_MIN_SIZE ((pst_u32)sizeof(PST_CONNECTION_CONFIG))
#define PST_RUNTIME_OPTIONS_MIN_SIZE ((pst_u32)sizeof(PST_RUNTIME_OPTIONS))
#define PST_RUNTIME_INFO_MIN_SIZE ((pst_u32)sizeof(PST_RUNTIME_INFO))
#define PST_PROVIDER_INFO_MIN_SIZE ((pst_u32)sizeof(PST_PROVIDER_INFO))
#define PST_PEER_INFO_SUMMARY_MIN_SIZE ((pst_u32)sizeof(PST_PEER_INFO_SUMMARY))
PST_API pst_u32 PST_CALL pst_api_version(void); PST_API pst_u32 PST_CALL pst_library_version(void);
PST_API PST_RESULT PST_CALL pst_version_info_init(PST_VERSION_INFO *); PST_API PST_RESULT PST_CALL pst_get_version(PST_VERSION_INFO *); PST_API const char *PST_CALL pst_result_string(PST_RESULT);
PST_API PST_RESULT PST_CALL pst_diagnostic_info_init(PST_DIAGNOSTIC_INFO *); PST_API PST_RESULT PST_CALL pst_log_config_init(PST_LOG_CONFIG *);
PST_API PST_RESULT PST_CALL pst_credentials_create(const PST_CREDENTIAL_SOURCE *,pst_credentials **); PST_API void PST_CALL pst_credentials_release(pst_credentials *);
PST_API PST_RESULT PST_CALL pst_trust_create(const PST_TRUST_SOURCE *,pst_trust **); PST_API void PST_CALL pst_trust_release(pst_trust *);
PST_API PST_RESULT PST_CALL pst_runtime_create(const PST_RUNTIME_OPTIONS *,pst_runtime **); PST_API PST_RESULT PST_CALL pst_runtime_create_ex(const PST_RUNTIME_OPTIONS *,pst_runtime **,PST_DIAGNOSTIC_INFO *);
PST_API PST_RESULT PST_CALL pst_runtime_create_with_logging(const PST_RUNTIME_OPTIONS *,const PST_LOG_CONFIG *,pst_runtime **,PST_DIAGNOSTIC_INFO *); PST_API PST_RESULT PST_CALL pst_runtime_copy_diagnostic(const pst_runtime *,PST_DIAGNOSTIC_INFO *);
PST_API PST_RESULT PST_CALL pst_runtime_get_info(const pst_runtime *,PST_RUNTIME_INFO *); PST_API PST_RESULT PST_CALL pst_runtime_get_provider_info(const pst_runtime *,pst_size,PST_PROVIDER_INFO *); PST_API void PST_CALL pst_runtime_release(pst_runtime *);
PST_API PST_RESULT PST_CALL pst_connection_create(pst_runtime *,const PST_CONNECTION_CONFIG *,pst_connection **); PST_API PST_RESULT PST_CALL pst_connection_create_ex(pst_runtime *,const PST_CONNECTION_CONFIG *,pst_connection **,PST_DIAGNOSTIC_INFO *);
PST_API PST_RESULT PST_CALL pst_connection_get_provider_info(const pst_connection *,PST_PROVIDER_INFO *); PST_API PST_RESULT PST_CALL pst_connection_copy_diagnostic(const pst_connection *,PST_DIAGNOSTIC_INFO *);
PST_API PST_RESULT PST_CALL pst_connection_attach(pst_connection *,pst_transport *,pst_u32,pst_u32 *); PST_API PST_RESULT PST_CALL pst_connection_handshake(pst_connection *,pst_u32 *,PST_RESULT *);
PST_API PST_RESULT PST_CALL pst_connection_get_interest(pst_connection *,pst_u32 *); PST_API PST_RESULT PST_CALL pst_connection_wait(pst_connection *,pst_u32,PST_WAIT_RESULT *);
PST_API PST_RESULT PST_CALL pst_connection_read(pst_connection *,void *,pst_size,PST_IO_RESULT *); PST_API PST_RESULT PST_CALL pst_connection_write(pst_connection *,const void *,pst_size,PST_IO_RESULT *);
PST_API PST_RESULT PST_CALL pst_connection_get_peer_info(pst_connection *,pst_peer_info **); PST_API PST_RESULT PST_CALL pst_connection_get_negotiated_alpn(pst_connection *,pst_u8 *,pst_size,pst_size *);
PST_API PST_RESULT PST_CALL pst_connection_shutdown(pst_connection *,pst_u32 *,PST_RESULT *); PST_API void PST_CALL pst_connection_release(pst_connection *);
PST_API PST_RESULT PST_CALL pst_peer_info_get_summary(const pst_peer_info *,PST_PEER_INFO_SUMMARY *); PST_API PST_RESULT PST_CALL pst_peer_info_copy_leaf_der(const pst_peer_info *,pst_u8 *,pst_size,pst_size *); PST_API void PST_CALL pst_peer_info_release(pst_peer_info *);
PST_API void PST_CALL pst_transport_release(pst_transport *);
#ifdef __cplusplus
}
#endif
#endif
