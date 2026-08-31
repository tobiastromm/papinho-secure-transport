#include "pst_backend.h"
#include <stdio.h>
#include <string.h>
typedef struct mock_backend_state { int initialized; } mock_backend_state;
typedef struct mock_runtime_state { mock_backend_state *backend; } mock_runtime_state;
typedef struct mock_connection_state {
    mock_runtime_state *runtime;
    void *transport;
    pst_u32 ownership;
} mock_connection_state;
static mock_backend_state g_backend;
static mock_runtime_state g_runtime;
static mock_connection_state g_connection;
static pst_u32 g_next_operation;
static int g_initialize_calls;
static int g_shutdown_calls;
static int g_runtime_create_calls;
static int g_runtime_destroy_calls;
static int g_connection_create_calls;
static int g_connection_destroy_calls;
static PST_RESULT mock_initialize(void **state)
{
    ++g_initialize_calls; g_backend.initialized = 1; *state = &g_backend;
    return PST_RESULT_OK;
}
static void mock_shutdown(void *state)
{
    mock_backend_state *s = (mock_backend_state *)state;
    ++g_shutdown_calls; s->initialized = 0;
}
static PST_RESULT mock_runtime_create(void *state, void **runtime)
{
    ++g_runtime_create_calls; g_runtime.backend = (mock_backend_state *)state;
    *runtime = &g_runtime; return PST_RESULT_OK;
}
static void mock_runtime_destroy(void *runtime)
{
    (void)runtime; ++g_runtime_destroy_calls;
}
static PST_RESULT mock_query(void *state, pst_u32 *capabilities)
{
    if (state != &g_backend || capabilities == NULL) return PST_RESULT_INVALID_ARGUMENT;
    *capabilities = PST_BACKEND_CAP_NONBLOCKING | PST_BACKEND_CAP_BACKEND_WAIT;
    return PST_RESULT_OK;
}
static PST_RESULT mock_validate_requirements(void *runtime, pst_u32 required)
{
    pst_u32 available;
    if (runtime != &g_runtime) return PST_RESULT_INVALID_ARGUMENT;
    available = PST_BACKEND_CAP_NONBLOCKING | PST_BACKEND_CAP_BACKEND_WAIT;
    return (required & ~available) == 0UL ? PST_RESULT_OK : PST_RESULT_UNSUPPORTED;
}
static PST_RESULT mock_connection_create(void *runtime, void **connection)
{
    if (runtime != &g_runtime) return PST_RESULT_INVALID_ARGUMENT;
    ++g_connection_create_calls; memset(&g_connection, 0, sizeof(g_connection));
    g_connection.runtime = &g_runtime; *connection = &g_connection;
    return PST_RESULT_OK;
}
static void mock_connection_destroy(void *connection)
{
    (void)connection; ++g_connection_destroy_calls;
}
static PST_RESULT mock_attach(void *connection, void *transport, pst_u32 ownership,
                              pst_u32 *ownership_accepted)
{
    mock_connection_state *c = (mock_connection_state *)connection;
    if (ownership_accepted == NULL) return PST_RESULT_INVALID_ARGUMENT;
    *ownership_accepted = 0UL;
    if (c != &g_connection || transport == NULL) return PST_RESULT_INVALID_ARGUMENT;
    if (ownership > PST_BACKEND_OWNERSHIP_RETAINED) return PST_RESULT_INVALID_ARGUMENT;
    c->transport = transport; c->ownership = ownership;
    *ownership_accepted = ownership == PST_BACKEND_OWNERSHIP_TRANSFERRED ? 1UL : 0UL;
    return PST_RESULT_OK;
}
static PST_RESULT mock_handshake(void *connection, pst_u32 *operation, PST_RESULT *error)
{
    if (connection != &g_connection || operation == NULL || error == NULL)
        return PST_RESULT_INVALID_ARGUMENT;
    *operation = g_next_operation;
    *error = g_next_operation == PST_BACKEND_OPERATION_FAILED ?
        PST_RESULT_PROTOCOL_FAILURE : PST_RESULT_OK;
    return PST_RESULT_OK;
}
static PST_RESULT mock_interest(void *connection, pst_u32 *interest)
{
    if (connection != &g_connection || interest == NULL) return PST_RESULT_INVALID_ARGUMENT;
    if (g_next_operation == PST_BACKEND_OPERATION_NEED_READ) *interest = PST_BACKEND_INTEREST_READ;
    else if (g_next_operation == PST_BACKEND_OPERATION_NEED_WRITE) *interest = PST_BACKEND_INTEREST_WRITE;
    else if (g_next_operation == PST_BACKEND_OPERATION_NEED_READ_WRITE)
        *interest = PST_BACKEND_INTEREST_READ | PST_BACKEND_INTEREST_WRITE;
    else *interest = PST_BACKEND_INTEREST_NONE;
    return PST_RESULT_OK;
}
static PST_RESULT mock_wait(void *connection, pst_u32 interest, pst_u32 timeout_ms,
                            PST_BACKEND_WAIT_RESULT *result)
{
    if (connection != &g_connection || result == NULL) return PST_RESULT_INVALID_ARGUMENT;
    result->timed_out = timeout_ms == 0UL ? 1UL : 0UL;
    result->ready_interest = result->timed_out ? PST_BACKEND_INTEREST_NONE : interest;
    return PST_RESULT_OK;
}
static PST_RESULT mock_read(void *connection, void *buffer, pst_size capacity,
                            PST_BACKEND_IO_RESULT *result)
{
    pst_size amount;
    if (connection != &g_connection || buffer == NULL || result == NULL)
        return PST_RESULT_INVALID_ARGUMENT;
    amount = capacity < 3 ? capacity : 3;
    memset(buffer, 'r', amount); result->bytes_transferred = amount;
    result->operation = capacity > amount ? PST_BACKEND_OPERATION_NEED_READ : PST_BACKEND_OPERATION_COMPLETE;
    result->close_kind = PST_BACKEND_CLOSE_NONE; result->error = PST_RESULT_OK;
    return PST_RESULT_OK;
}
static PST_RESULT mock_write(void *connection, const void *buffer, pst_size length,
                             PST_BACKEND_IO_RESULT *result)
{
    if (connection != &g_connection || buffer == NULL || result == NULL)
        return PST_RESULT_INVALID_ARGUMENT;
    result->bytes_transferred = length < 2 ? length : 2;
    result->operation = length > result->bytes_transferred ?
        PST_BACKEND_OPERATION_NEED_WRITE : PST_BACKEND_OPERATION_COMPLETE;
    result->close_kind = PST_BACKEND_CLOSE_NONE; result->error = PST_RESULT_OK;
    return PST_RESULT_OK;
}
static PST_RESULT mock_shutdown_step(void *connection, pst_u32 *operation, PST_RESULT *error)
{
    if (connection != &g_connection || operation == NULL || error == NULL)
        return PST_RESULT_INVALID_ARGUMENT;
    *operation = PST_BACKEND_OPERATION_COMPLETE; *error = PST_RESULT_OK;
    return PST_RESULT_OK;
}
static PST_RESULT mock_configure(void *connection,const pst_config *config){return connection==&g_connection&&config!=NULL?PST_RESULT_OK:PST_RESULT_INVALID_ARGUMENT;}
static PST_RESULT mock_alpn(void *connection,pst_u8 *buffer,pst_size capacity,pst_size *size){(void)buffer;(void)capacity;if(connection!=&g_connection||!size)return PST_RESULT_INVALID_ARGUMENT;*size=0;return PST_RESULT_UNAVAILABLE;}
static const PST_BACKEND_VTABLE g_vtable = {
    sizeof(PST_BACKEND_VTABLE), PST_BACKEND_SPI_VERSION,
    mock_initialize, mock_shutdown, mock_runtime_create, mock_runtime_destroy,
    mock_query, mock_validate_requirements, mock_connection_create,
    mock_connection_destroy, mock_attach, mock_handshake, mock_interest,
    mock_wait, mock_read, mock_write, mock_shutdown_step, NULL, NULL,
    mock_configure, mock_alpn
};
static const PST_BACKEND_DESCRIPTOR g_descriptor = {
    sizeof(PST_BACKEND_DESCRIPTOR), PST_BACKEND_SPI_VERSION,
    "test-backend", "SPI test backend",
    PST_BACKEND_CAP_NONBLOCKING | PST_BACKEND_CAP_BACKEND_WAIT, &g_vtable
};
#define CHECK(condition, code) if (!(condition)) return (code)
int main(void)
{
    PST_BACKEND_DESCRIPTOR d;
    PST_BACKEND_VTABLE v;
    const PST_BACKEND_DESCRIPTOR *found;
    void *backend_state;
    void *runtime_state;
    void *connection_state;
    pst_u32 capabilities;
    pst_u32 operation;
    pst_u32 interest;
    pst_u32 ownership_accepted;
    PST_RESULT error;
    PST_BACKEND_WAIT_RESULT wait_result;
    PST_BACKEND_IO_RESULT io;
    char buffer[8];
    int transport_token;
    pst_u32 states[4];
    int i;
    pst_backend_registry_reset();
    CHECK(pst_backend_validate(NULL) == PST_RESULT_INVALID_ARGUMENT, 1);
    d = g_descriptor; d.struct_size = PST_BACKEND_DESCRIPTOR_MIN_SIZE - 1UL;
    CHECK(pst_backend_validate(&d) == PST_RESULT_INVALID_ARGUMENT, 2);
    d = g_descriptor; d.spi_version = 0x00030000UL;
    CHECK(pst_backend_validate(&d) == PST_RESULT_INCOMPATIBLE_API, 3);
    d = g_descriptor; d.id = "";
    CHECK(pst_backend_validate(&d) == PST_RESULT_INVALID_ARGUMENT, 4);
    CHECK(pst_backend_register(&d) == PST_RESULT_INVALID_ARGUMENT, 53);
    d = g_descriptor; d.id = "Invalid ID";
    CHECK(pst_backend_validate(&d) == PST_RESULT_INVALID_ARGUMENT, 5);
    d = g_descriptor; d.vtable = NULL;
    CHECK(pst_backend_validate(&d) == PST_RESULT_INVALID_ARGUMENT, 6);
    v = g_vtable; v.struct_size = PST_BACKEND_VTABLE_MIN_SIZE - 1UL;
    d = g_descriptor; d.vtable = &v;
    CHECK(pst_backend_validate(&d) == PST_RESULT_INVALID_ARGUMENT, 54);
    v = g_vtable; v.handshake_step = NULL; d = g_descriptor; d.vtable = &v;
    CHECK(pst_backend_validate(&d) == PST_RESULT_INVALID_ARGUMENT, 7);
    v = g_vtable; v.spi_version = 0x00030000UL; d.vtable = &v;
    CHECK(pst_backend_validate(&d) == PST_RESULT_INCOMPATIBLE_API, 8);
    d = g_descriptor; d.capabilities = PST_BACKEND_CAP_EARLY_DATA;
    CHECK(pst_backend_validate(&d) == PST_RESULT_INVALID_ARGUMENT, 9);
    v = g_vtable; v.wait = NULL; d = g_descriptor; d.vtable = &v;
    CHECK(pst_backend_validate(&d) == PST_RESULT_INVALID_ARGUMENT, 10);
    d = g_descriptor; d.capabilities = PST_BACKEND_CAP_PEER_INFO;
    CHECK(pst_backend_validate(&d) == PST_RESULT_INVALID_ARGUMENT, 11);
    CHECK(pst_backend_register(&g_descriptor) == PST_RESULT_OK, 12);
    CHECK(pst_backend_count() == 1, 13);
    CHECK(pst_backend_register(&g_descriptor) == PST_RESULT_INVALID_STATE, 14);
    found = pst_backend_find("test-backend"); CHECK(found == &g_descriptor, 15);
    CHECK(pst_backend_find("missing") == NULL, 16);
    CHECK(found->vtable->initialize(&backend_state) == PST_RESULT_OK, 17);
    CHECK(backend_state == &g_backend && g_initialize_calls == 1, 18);
    CHECK(found->vtable->query_capabilities(backend_state, &capabilities) == PST_RESULT_OK, 19);
    CHECK(capabilities == found->capabilities, 20);
    CHECK(found->vtable->runtime_create(backend_state, &runtime_state) == PST_RESULT_OK, 21);
    CHECK(runtime_state == &g_runtime && g_runtime_create_calls == 1, 22);
    CHECK(found->vtable->validate_requirements(runtime_state, PST_BACKEND_CAP_NONBLOCKING) == PST_RESULT_OK, 23);
    CHECK(found->vtable->validate_requirements(runtime_state, PST_BACKEND_CAP_ALPN) == PST_RESULT_UNSUPPORTED, 24);
    CHECK(found->vtable->connection_create(runtime_state, &connection_state) == PST_RESULT_OK, 25);
    CHECK(connection_state == &g_connection && g_connection_create_calls == 1, 26);
    transport_token = 7;
    CHECK(found->vtable->attach_transport(connection_state, NULL, PST_BACKEND_OWNERSHIP_TRANSFERRED, &ownership_accepted) == PST_RESULT_INVALID_ARGUMENT, 27);
    CHECK(g_connection.transport == NULL && ownership_accepted == 0UL, 28);
    CHECK(found->vtable->attach_transport(connection_state, &transport_token, PST_BACKEND_OWNERSHIP_TRANSFERRED, &ownership_accepted) == PST_RESULT_OK, 29);
    CHECK(g_connection.transport == &transport_token && g_connection.ownership == PST_BACKEND_OWNERSHIP_TRANSFERRED, 30);
    CHECK(ownership_accepted == 1UL, 55);
    states[0] = PST_BACKEND_OPERATION_COMPLETE; states[1] = PST_BACKEND_OPERATION_NEED_READ;
    states[2] = PST_BACKEND_OPERATION_NEED_WRITE; states[3] = PST_BACKEND_OPERATION_NEED_READ_WRITE;
    for (i = 0; i < 4; ++i) {
        g_next_operation = states[i];
        CHECK(found->vtable->handshake_step(connection_state, &operation, &error) == PST_RESULT_OK, 31 + i);
        CHECK(operation == states[i] && error == PST_RESULT_OK, 35 + i);
    }
    g_next_operation = PST_BACKEND_OPERATION_NEED_READ_WRITE;
    CHECK(found->vtable->get_interest(connection_state, &interest) == PST_RESULT_OK, 39);
    CHECK(interest == (PST_BACKEND_INTEREST_READ | PST_BACKEND_INTEREST_WRITE), 40);
    CHECK(found->vtable->wait(connection_state, interest, 0, &wait_result) == PST_RESULT_OK, 41);
    CHECK(wait_result.timed_out == 1 && wait_result.ready_interest == 0, 42);
    CHECK(found->vtable->read(connection_state, buffer, sizeof(buffer), &io) == PST_RESULT_OK, 43);
    CHECK(io.bytes_transferred == 3 && io.operation == PST_BACKEND_OPERATION_NEED_READ, 44);
    CHECK(found->vtable->write(connection_state, buffer, sizeof(buffer), &io) == PST_RESULT_OK, 45);
    CHECK(io.bytes_transferred == 2 && io.operation == PST_BACKEND_OPERATION_NEED_WRITE, 46);
    CHECK(found->vtable->shutdown_step(connection_state, &operation, &error) == PST_RESULT_OK, 47);
    CHECK(operation == PST_BACKEND_OPERATION_COMPLETE && error == PST_RESULT_OK, 48);
    found->vtable->connection_destroy(connection_state);
    found->vtable->runtime_destroy(runtime_state);
    found->vtable->shutdown(backend_state);
    CHECK(g_connection_destroy_calls == 1 && g_runtime_destroy_calls == 1 && g_shutdown_calls == 1, 49);
    CHECK(pst_backend_unregister("test-backend") == PST_RESULT_OK, 50);
    CHECK(pst_backend_count() == 0 && pst_backend_find("test-backend") == NULL, 51);
    CHECK(pst_backend_unregister("test-backend") == PST_RESULT_UNAVAILABLE, 52);
    printf("test_backend_spi: PASS\n"); return 0;
}
