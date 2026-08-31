#ifndef PST_BACKEND_H
#define PST_BACKEND_H
#include "papinho_secure_transport.h"
#define PST_BACKEND_SPI_VERSION_MAJOR 1UL
#define PST_BACKEND_SPI_VERSION_MINOR 0UL
#define PST_BACKEND_SPI_VERSION 0x00010000UL
#define PST_BACKEND_CAP_TLS_1_2 0x00000001UL
#define PST_BACKEND_CAP_TLS_1_3 0x00000002UL
#define PST_BACKEND_CAP_CLIENT_AUTH 0x00000004UL
#define PST_BACKEND_CAP_ALPN 0x00000008UL
#define PST_BACKEND_CAP_CUSTOM_TRUST 0x00000010UL
#define PST_BACKEND_CAP_SYSTEM_TRUST 0x00000020UL
#define PST_BACKEND_CAP_HOSTNAME_VERIFY 0x00000040UL
#define PST_BACKEND_CAP_RESUMPTION 0x00000080UL
#define PST_BACKEND_CAP_EARLY_DATA 0x00000100UL
#define PST_BACKEND_CAP_PEER_INFO 0x00000200UL
#define PST_BACKEND_CAP_NONBLOCKING 0x00000400UL
#define PST_BACKEND_CAP_BACKEND_WAIT 0x00000800UL
#define PST_BACKEND_OPERATION_COMPLETE 0UL
#define PST_BACKEND_OPERATION_NEED_READ 1UL
#define PST_BACKEND_OPERATION_NEED_WRITE 2UL
#define PST_BACKEND_OPERATION_NEED_READ_WRITE 3UL
#define PST_BACKEND_OPERATION_CLOSED 4UL
#define PST_BACKEND_OPERATION_FAILED 5UL
#define PST_BACKEND_INTEREST_NONE 0UL
#define PST_BACKEND_INTEREST_READ 0x00000001UL
#define PST_BACKEND_INTEREST_WRITE 0x00000002UL
#define PST_BACKEND_CLOSE_NONE 0UL
#define PST_BACKEND_CLOSE_CLEAN 1UL
#define PST_BACKEND_CLOSE_TRUNCATED 2UL
#define PST_BACKEND_OWNERSHIP_BORROWED 0UL
#define PST_BACKEND_OWNERSHIP_TRANSFERRED 1UL
#define PST_BACKEND_OWNERSHIP_RETAINED 2UL
typedef struct PST_BACKEND_IO_RESULT {
    pst_size bytes_transferred;
    pst_u32 operation;
    pst_u32 close_kind;
    PST_RESULT error;
} PST_BACKEND_IO_RESULT;
typedef struct PST_BACKEND_WAIT_RESULT {
    pst_u32 ready_interest;
    pst_u32 timed_out;
} PST_BACKEND_WAIT_RESULT;
typedef struct PST_BACKEND_VTABLE {
    pst_u32 struct_size;
    pst_u32 spi_version;
    PST_RESULT (*initialize)(void **backend_state);
    void (*shutdown)(void *backend_state);
    PST_RESULT (*runtime_create)(void *backend_state, void **runtime_state);
    void (*runtime_destroy)(void *runtime_state);
    PST_RESULT (*query_capabilities)(void *backend_state, pst_u32 *capabilities);
    PST_RESULT (*validate_requirements)(void *runtime_state, pst_u32 required_capabilities);
    PST_RESULT (*connection_create)(void *runtime_state, void **connection_state);
    void (*connection_destroy)(void *connection_state);
    PST_RESULT (*attach_transport)(void *connection_state, void *transport, pst_u32 ownership);
    PST_RESULT (*handshake_step)(void *connection_state, pst_u32 *operation, PST_RESULT *error);
    PST_RESULT (*get_interest)(void *connection_state, pst_u32 *interest);
    PST_RESULT (*wait)(void *connection_state, pst_u32 interest, pst_u32 timeout_ms, PST_BACKEND_WAIT_RESULT *result);
    PST_RESULT (*read)(void *connection_state, void *buffer, pst_size capacity, PST_BACKEND_IO_RESULT *result);
    PST_RESULT (*write)(void *connection_state, const void *buffer, pst_size length, PST_BACKEND_IO_RESULT *result);
    PST_RESULT (*shutdown_step)(void *connection_state, pst_u32 *operation, PST_RESULT *error);
    PST_RESULT (*peer_info_create)(void *connection_state, void **peer_info);
    void (*peer_info_destroy)(void *peer_info);
} PST_BACKEND_VTABLE;
typedef struct PST_BACKEND_DESCRIPTOR {
    pst_u32 struct_size;
    pst_u32 spi_version;
    const char *id;
    const char *name;
    pst_u32 capabilities;
    const PST_BACKEND_VTABLE *vtable;
} PST_BACKEND_DESCRIPTOR;
#define PST_BACKEND_VTABLE_MIN_SIZE ((pst_u32)sizeof(PST_BACKEND_VTABLE))
#define PST_BACKEND_DESCRIPTOR_MIN_SIZE ((pst_u32)sizeof(PST_BACKEND_DESCRIPTOR))
PST_RESULT pst_backend_validate(const PST_BACKEND_DESCRIPTOR *descriptor);
PST_RESULT pst_backend_register(const PST_BACKEND_DESCRIPTOR *descriptor);
PST_RESULT pst_backend_unregister(const char *id);
const PST_BACKEND_DESCRIPTOR *pst_backend_find(const char *id);
pst_size pst_backend_count(void);
void pst_backend_registry_reset(void);
#endif