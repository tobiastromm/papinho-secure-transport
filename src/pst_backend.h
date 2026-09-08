/* SPDX-License-Identifier: MPL-2.0 */
#ifndef PST_BACKEND_H
#define PST_BACKEND_H
#include "papinho_secure_transport.h"
#include "pst_diagnostic.h"
#define PST_BACKEND_SPI_VERSION_MAJOR 3UL
#define PST_BACKEND_SPI_VERSION_MINOR 0UL
#define PST_BACKEND_SPI_VERSION 0x00030000UL
#define PST_BACKEND_ID_CAPACITY 32UL
#define PST_BACKEND_CAP_TLS_1_2 PST_CAP_TLS_1_2
#define PST_BACKEND_CAP_TLS_1_3 PST_CAP_TLS_1_3
#define PST_BACKEND_CAP_ROLE_CLIENT PST_CAP_ROLE_CLIENT
#define PST_BACKEND_CAP_ROLE_SERVER PST_CAP_ROLE_SERVER
#define PST_BACKEND_CAP_LOCAL_IDENTITY PST_CAP_LOCAL_IDENTITY
#define PST_BACKEND_CAP_PEER_CERT_AUTH PST_CAP_PEER_CERT_AUTH
#define PST_BACKEND_CAP_PEER_CERT_OPTIONAL PST_CAP_PEER_CERT_OPTIONAL
#define PST_BACKEND_CAP_ALPN_CLIENT PST_CAP_ALPN_CLIENT
#define PST_BACKEND_CAP_ALPN_SERVER PST_CAP_ALPN_SERVER
#define PST_BACKEND_CAP_CUSTOM_TRUST PST_CAP_CUSTOM_TRUST
#define PST_BACKEND_CAP_SYSTEM_TRUST PST_CAP_SYSTEM_TRUST
#define PST_BACKEND_CAP_PEER_NAME_VERIFY PST_CAP_PEER_NAME_VERIFY
#define PST_BACKEND_CAP_PEER_INFO PST_CAP_PEER_INFO
#define PST_BACKEND_CAP_NONBLOCKING PST_CAP_NONBLOCKING
#define PST_BACKEND_CAP_BACKEND_WAIT PST_CAP_BACKEND_WAIT
#define PST_BACKEND_CAP_RESUMPTION PST_CAP_RESUMPTION
#define PST_BACKEND_CAP_EARLY_DATA PST_CAP_EARLY_DATA
#define PST_BACKEND_OPERATION_COMPLETE PST_OPERATION_COMPLETE
#define PST_BACKEND_OPERATION_NEED_READ PST_OPERATION_NEED_READ
#define PST_BACKEND_OPERATION_NEED_WRITE PST_OPERATION_NEED_WRITE
#define PST_BACKEND_OPERATION_NEED_READ_WRITE PST_OPERATION_NEED_READ_WRITE
#define PST_BACKEND_OPERATION_CLOSED PST_OPERATION_CLOSED
#define PST_BACKEND_OPERATION_FAILED PST_OPERATION_FAILED
#define PST_BACKEND_INTEREST_NONE PST_INTEREST_NONE
#define PST_BACKEND_INTEREST_READ PST_INTEREST_READ
#define PST_BACKEND_INTEREST_WRITE PST_INTEREST_WRITE
#define PST_BACKEND_CLOSE_NONE PST_CLOSE_NONE
#define PST_BACKEND_CLOSE_CLEAN PST_CLOSE_CLEAN
#define PST_BACKEND_CLOSE_TRUNCATED PST_CLOSE_TRUNCATED
#define PST_BACKEND_OWNERSHIP_TRANSFERRED PST_OWNERSHIP_TRANSFERRED
typedef struct PST_BACKEND_IO_RESULT { pst_size bytes_transferred; pst_u32 operation,close_kind; PST_RESULT error; } PST_BACKEND_IO_RESULT;
typedef struct PST_BACKEND_WAIT_RESULT { pst_u32 ready_interest,timed_out; } PST_BACKEND_WAIT_RESULT;
typedef struct PST_BACKEND_CONNECTION_OPTIONS { pst_u32 struct_size,spi_version,role,required_capabilities; const PST_CONNECTION_CONFIG *configuration; } PST_BACKEND_CONNECTION_OPTIONS;
typedef struct PST_BACKEND_VTABLE {
 pst_u32 struct_size,spi_version; PST_RESULT (*initialize)(void **); void (*shutdown)(void *); PST_RESULT (*runtime_create)(void *,void **); void (*runtime_destroy)(void *); PST_RESULT (*query_capabilities)(void *,pst_u32 *); PST_RESULT (*validate_requirements)(void *,pst_u32);
 PST_RESULT (*connection_create)(void *,const PST_BACKEND_CONNECTION_OPTIONS *,void **); void (*connection_destroy)(void *); PST_RESULT (*attach_transport)(void *,void *,pst_u32,pst_u32 *); PST_RESULT (*handshake_step)(void *,pst_u32 *,PST_RESULT *); PST_RESULT (*get_interest)(void *,pst_u32 *); PST_RESULT (*wait)(void *,pst_u32,pst_u32,PST_BACKEND_WAIT_RESULT *); PST_RESULT (*read)(void *,void *,pst_size,PST_BACKEND_IO_RESULT *); PST_RESULT (*write)(void *,const void *,pst_size,PST_BACKEND_IO_RESULT *); PST_RESULT (*shutdown_step)(void *,pst_u32 *,PST_RESULT *); PST_RESULT (*peer_info_create)(void *,void **); void (*peer_info_destroy)(void *); PST_RESULT (*connection_get_alpn)(void *,pst_u8 *,pst_size,pst_size *); void (*diagnostic_copy)(const void *,pst_internal_diagnostic *);
} PST_BACKEND_VTABLE;
#define PST_BACKEND_METADATA_VERSION 0x00010000UL
#define PST_BACKEND_METADATA_COMPONENT_CAPACITY 2UL
#define PST_BACKEND_METADATA_NAME_CAPACITY 24UL
#define PST_BACKEND_METADATA_QUALIFIER_CAPACITY 16UL
#define PST_BACKEND_VERSION_AVAILABLE 1UL
typedef struct PST_BACKEND_COMPONENT_VERSION { pst_u32 flags,major,minor,patch; char name[PST_BACKEND_METADATA_NAME_CAPACITY]; char qualifier[PST_BACKEND_METADATA_QUALIFIER_CAPACITY]; } PST_BACKEND_COMPONENT_VERSION;
typedef struct PST_BACKEND_METADATA { pst_u32 struct_size,version; PST_BACKEND_COMPONENT_VERSION implementation; pst_u32 component_count; PST_BACKEND_COMPONENT_VERSION components[PST_BACKEND_METADATA_COMPONENT_CAPACITY]; } PST_BACKEND_METADATA;
typedef struct PST_BACKEND_DESCRIPTOR { pst_u32 struct_size,spi_version; const char *id,*name; pst_u32 capabilities; const PST_BACKEND_VTABLE *vtable; const PST_BACKEND_METADATA *metadata; pst_u32 client_capabilities,server_capabilities; } PST_BACKEND_DESCRIPTOR;
#define PST_BACKEND_CONNECTION_OPTIONS_MIN_SIZE ((pst_u32)sizeof(PST_BACKEND_CONNECTION_OPTIONS))
#define PST_BACKEND_METADATA_MIN_SIZE ((pst_u32)(offsetof(PST_BACKEND_METADATA,components)+sizeof(((PST_BACKEND_METADATA*)0)->components)))
#define PST_BACKEND_VTABLE_MIN_SIZE ((pst_u32)(offsetof(PST_BACKEND_VTABLE,shutdown_step)+sizeof(((PST_BACKEND_VTABLE*)0)->shutdown_step)))
#define PST_BACKEND_VTABLE_FIELD_SIZE(f) ((pst_u32)(offsetof(PST_BACKEND_VTABLE,f)+sizeof(((PST_BACKEND_VTABLE*)0)->f)))
#define PST_BACKEND_DESCRIPTOR_MIN_SIZE ((pst_u32)sizeof(PST_BACKEND_DESCRIPTOR))
PST_RESULT pst_backend_validate(const PST_BACKEND_DESCRIPTOR *); PST_RESULT pst_backend_register(const PST_BACKEND_DESCRIPTOR *); PST_RESULT pst_backend_register_manifest(const PST_BACKEND_DESCRIPTOR *const *,pst_size); void pst_backend_registry_seal(void); int pst_backend_registry_is_sealed(void); void pst_backend_test_manifest_fail_after(pst_size); PST_RESULT pst_backend_unregister(const char *); const PST_BACKEND_DESCRIPTOR *pst_backend_find(const char *); const PST_BACKEND_DESCRIPTOR *pst_backend_find_by_index(pst_size); pst_size pst_backend_count(void); void pst_backend_registry_reset(void);
#endif
