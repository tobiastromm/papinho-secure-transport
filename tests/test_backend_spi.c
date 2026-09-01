#include "pst_backend.h"
#include "pst_internal.h"
#include "pst_transport_internal.h"
#include <stdio.h>
#include <string.h>
typedef struct mock_backend_state { int initialized; pst_internal_diagnostic diagnostic; } mock_backend_state;
typedef struct mock_runtime_state { mock_backend_state *backend; pst_internal_diagnostic diagnostic; } mock_runtime_state;
typedef struct mock_connection_state {
    mock_runtime_state *runtime;
    void *transport;
    pst_u32 ownership;
    pst_u32 interest;
    pst_internal_diagnostic diagnostic;
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
static int g_readiness_scenario;
static int g_scenario_read_calls;
static int g_scenario_wait_calls;
static pst_u32 g_wait_interests[4];
static int g_transport_destroy_calls;
static PST_RESULT mock_initialize(void **state)
{
    ++g_initialize_calls; g_backend.initialized = 1; pst_diagnostic_initialize(&g_backend.diagnostic); *state = &g_backend;
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
    pst_diagnostic_initialize(&g_runtime.diagnostic); *runtime = &g_runtime; return PST_RESULT_OK;
}
static void mock_runtime_destroy(void *runtime)
{
    (void)runtime; ++g_runtime_destroy_calls;
}
static PST_RESULT mock_query(void *state, pst_u32 *capabilities)
{
    if (state != &g_backend || capabilities == NULL) return PST_RESULT_INVALID_ARGUMENT;
    *capabilities = PST_BACKEND_CAP_TLS_1_2 | PST_BACKEND_CAP_NONBLOCKING | PST_BACKEND_CAP_BACKEND_WAIT;
    return PST_RESULT_OK;
}
static PST_RESULT mock_validate_requirements(void *runtime, pst_u32 required)
{
    pst_u32 available;
    if (runtime != &g_runtime) return PST_RESULT_INVALID_ARGUMENT;
    available = PST_BACKEND_CAP_TLS_1_2 | PST_BACKEND_CAP_NONBLOCKING | PST_BACKEND_CAP_BACKEND_WAIT;
    return (required & ~available) == 0UL ? PST_RESULT_OK : PST_RESULT_UNSUPPORTED;
}
static PST_RESULT mock_connection_create(void *runtime, void **connection)
{
    if (runtime != &g_runtime) return PST_RESULT_INVALID_ARGUMENT;
    ++g_connection_create_calls; memset(&g_connection, 0, sizeof(g_connection));
    g_connection.runtime = &g_runtime; pst_diagnostic_initialize(&g_connection.diagnostic); *connection = &g_connection;
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
    if (g_next_operation == PST_BACKEND_OPERATION_NEED_READ)
        g_connection.interest = PST_BACKEND_INTEREST_READ;
    else if (g_next_operation == PST_BACKEND_OPERATION_NEED_WRITE)
        g_connection.interest = PST_BACKEND_INTEREST_WRITE;
    else if (g_next_operation == PST_BACKEND_OPERATION_NEED_READ_WRITE)
        g_connection.interest = PST_BACKEND_INTEREST_READ | PST_BACKEND_INTEREST_WRITE;
    else g_connection.interest = PST_BACKEND_INTEREST_NONE;
    *error = g_next_operation == PST_BACKEND_OPERATION_FAILED ?
        PST_RESULT_AUTH_FAILURE : PST_RESULT_OK;
    if (g_next_operation == PST_BACKEND_OPERATION_FAILED)
        pst_diagnostic_capture(&g_connection.diagnostic, PST_RESULT_AUTH_FAILURE,
            PST_DIAGNOSTIC_PHASE_PEER_AUTHENTICATE, "test-backend",
            PST_DIAGNOSTIC_DOMAIN_NSS, -8179, 0, PST_DIAGNOSTIC_FLAG_NATIVE);
    else if (g_next_operation == PST_BACKEND_OPERATION_COMPLETE)
        pst_diagnostic_clear(&g_connection.diagnostic);
    return PST_RESULT_OK;
}
static PST_RESULT mock_interest(void *connection, pst_u32 *interest)
{
    if (connection != &g_connection || interest == NULL) return PST_RESULT_INVALID_ARGUMENT;
    *interest = g_connection.interest;
    return PST_RESULT_OK;
}
static PST_RESULT mock_wait(void *connection, pst_u32 interest, pst_u32 timeout_ms,
                            PST_BACKEND_WAIT_RESULT *result)
{
    if (connection != &g_connection || result == NULL) return PST_RESULT_INVALID_ARGUMENT;
    if (g_readiness_scenario == 4) {
        result->timed_out = 0UL; result->ready_interest = PST_BACKEND_INTEREST_NONE;
        pst_diagnostic_capture(&g_connection.diagnostic, PST_RESULT_TRANSPORT_FAILURE,
            PST_DIAGNOSTIC_PHASE_WAIT, "test-backend",
            PST_DIAGNOSTIC_DOMAIN_NSPR, -5961, 10054, PST_DIAGNOSTIC_FLAG_NATIVE | PST_DIAGNOSTIC_FLAG_SECONDARY);
        return PST_RESULT_TRANSPORT_FAILURE;
    }
    if (g_readiness_scenario != 0) {
        g_wait_interests[g_scenario_wait_calls] = interest;
        ++g_scenario_wait_calls; result->timed_out = 0UL;
        if (g_readiness_scenario == 3)
            result->ready_interest = PST_BACKEND_INTEREST_READ | PST_BACKEND_INTEREST_WRITE;
        else if (g_readiness_scenario == 1 && g_scenario_wait_calls == 2)
            result->ready_interest = PST_BACKEND_INTEREST_READ;
        else result->ready_interest = PST_BACKEND_INTEREST_WRITE;
        return PST_RESULT_OK;
    }
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
    if (g_readiness_scenario != 0) {
        ++g_scenario_read_calls; result->bytes_transferred = 0;
        if ((g_readiness_scenario == 1 && g_scenario_read_calls < 3) ||
            (g_readiness_scenario != 1 && g_scenario_read_calls < 2)) {
            result->operation = PST_BACKEND_OPERATION_NEED_READ_WRITE;
            g_connection.interest = PST_BACKEND_INTEREST_READ | PST_BACKEND_INTEREST_WRITE;
        } else {
            ((char *)buffer)[0] = 'r'; result->bytes_transferred = 1;
            result->operation = PST_BACKEND_OPERATION_COMPLETE;
            g_connection.interest = PST_BACKEND_INTEREST_NONE;
        }
        result->close_kind = PST_BACKEND_CLOSE_NONE; result->error = PST_RESULT_OK;
        return PST_RESULT_OK;
    }
    amount = capacity < 3 ? capacity : 3;
    memset(buffer, 'r', amount); result->bytes_transferred = amount;
    result->operation = capacity > amount ? PST_BACKEND_OPERATION_NEED_READ : PST_BACKEND_OPERATION_COMPLETE;
    g_connection.interest = result->operation == PST_BACKEND_OPERATION_NEED_READ ? PST_BACKEND_INTEREST_READ : PST_BACKEND_INTEREST_NONE;
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
    g_connection.interest = result->operation == PST_BACKEND_OPERATION_NEED_WRITE ? PST_BACKEND_INTEREST_WRITE : PST_BACKEND_INTEREST_NONE;
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
static void mock_transport_destroy(pst_transport *transport,int consumed){(void)transport;(void)consumed;++g_transport_destroy_calls;}
static PST_RESULT mock_configure(void *connection,const pst_config *config){return connection==&g_connection&&config!=NULL?PST_RESULT_OK:PST_RESULT_INVALID_ARGUMENT;}
static PST_RESULT mock_alpn(void *connection,pst_u8 *buffer,pst_size capacity,pst_size *size){(void)buffer;(void)capacity;if(connection!=&g_connection||!size)return PST_RESULT_INVALID_ARGUMENT;*size=0;return PST_RESULT_UNAVAILABLE;}
static void mock_diagnostic_copy(const void *state,pst_internal_diagnostic *out){if(!out)return;if(state==&g_backend)pst_diagnostic_copy(out,&g_backend.diagnostic);else if(state==&g_runtime)pst_diagnostic_copy(out,&g_runtime.diagnostic);else if(state==&g_connection)pst_diagnostic_copy(out,&g_connection.diagnostic);else pst_diagnostic_initialize(out);}
static const PST_BACKEND_VTABLE g_vtable = {
    sizeof(PST_BACKEND_VTABLE), PST_BACKEND_SPI_VERSION,
    mock_initialize, mock_shutdown, mock_runtime_create, mock_runtime_destroy,
    mock_query, mock_validate_requirements, mock_connection_create,
    mock_connection_destroy, mock_attach, mock_handshake, mock_interest,
    mock_wait, mock_read, mock_write, mock_shutdown_step, NULL, NULL,
    mock_configure, mock_alpn, mock_diagnostic_copy
};
static const PST_BACKEND_DESCRIPTOR g_descriptor = {
    sizeof(PST_BACKEND_DESCRIPTOR), PST_BACKEND_SPI_VERSION,
    "test-backend", "SPI test backend",
    PST_BACKEND_CAP_TLS_1_2 | PST_BACKEND_CAP_NONBLOCKING | PST_BACKEND_CAP_BACKEND_WAIT, &g_vtable
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
    PST_RUNTIME_OPTIONS public_options;
    pst_runtime *public_runtime;
    pst_config *public_config;
    pst_connection *public_connection;
    pst_connection *public_connection_b;
    pst_transport public_transport;
    PST_IO_RESULT public_io;
    PST_WAIT_RESULT public_wait;
    pst_u32 public_accepted;
    pst_internal_diagnostic diagnostic;
    pst_internal_diagnostic saved_diagnostic;
    PST_DIAGNOSTIC_INFO public_diagnostic;
    PST_DIAGNOSTIC_INFO saved_public_diagnostic;
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
    public_runtime = NULL; public_config = NULL; public_connection = NULL;
    CHECK(pst_config_create(&public_config) == PST_RESULT_OK, 56);
    CHECK(pst_config_freeze(public_config) == PST_RESULT_OK, 57);
    memset(&public_options, 0, sizeof(public_options));
    public_options.struct_size = sizeof(public_options);
    public_options.api_version = PST_API_VERSION;
    public_options.selection = PST_BACKEND_SELECTION_EXACT;
    public_options.exact_backend_id = "test-backend";
    CHECK(pst_runtime_create(&public_options, &public_runtime) == PST_RESULT_OK, 58);
    memset(&public_transport, 0, sizeof(public_transport));
    public_transport.backend_id = "test-backend";
    public_transport.native = &transport_token;
    public_transport.destroy = mock_transport_destroy;
    CHECK(pst_connection_create(public_runtime, public_config, &public_connection) == PST_RESULT_OK, 59);
    public_accepted = 0UL;
    CHECK(pst_connection_attach(public_connection, &public_transport,
        PST_OWNERSHIP_TRANSFERRED, &public_accepted) == PST_RESULT_OK, 60);
    CHECK(public_accepted == 1UL, 61);
    g_next_operation = PST_BACKEND_OPERATION_COMPLETE;
    CHECK(pst_connection_handshake(public_connection, &operation, &error) == PST_RESULT_OK, 62);
    CHECK(operation == PST_OPERATION_COMPLETE, 63);
    CHECK(pst_connection_wait(public_connection, 0, &public_wait) == PST_RESULT_OK, 103);
    CHECK(public_wait.timed_out == 1UL, 104);
    pst_connection_diagnostic_copy(public_connection, &diagnostic);
    CHECK(!diagnostic.valid, 105);
    memset(&public_diagnostic,0,sizeof(public_diagnostic));public_diagnostic.struct_size=sizeof(public_diagnostic);public_diagnostic.api_version=PST_API_VERSION;
    CHECK(pst_connection_copy_diagnostic(public_connection,&public_diagnostic)==PST_RESULT_OK&&!public_diagnostic.valid,107);

    g_readiness_scenario = 1; g_scenario_read_calls = 0; g_scenario_wait_calls = 0;
    CHECK(pst_connection_read(public_connection, buffer, sizeof(buffer), &public_io) == PST_RESULT_OK, 64);
    CHECK(public_io.operation == PST_OPERATION_NEED_READ_WRITE && public_io.bytes_transferred == 0, 65);
    pst_connection_diagnostic_copy(public_connection, &diagnostic);
    CHECK(!diagnostic.valid, 106);
    CHECK(pst_connection_wait(public_connection, 250, &public_wait) == PST_RESULT_OK, 66);
    CHECK(g_wait_interests[0] == (PST_BACKEND_INTEREST_READ | PST_BACKEND_INTEREST_WRITE), 67);
    CHECK(public_wait.ready_interest == PST_BACKEND_INTEREST_WRITE, 68);
    CHECK(pst_connection_read(public_connection, buffer, sizeof(buffer), &public_io) == PST_RESULT_OK, 69);
    CHECK(public_io.operation == PST_OPERATION_NEED_READ_WRITE && public_io.bytes_transferred == 0, 70);
    CHECK(pst_connection_wait(public_connection, 250, &public_wait) == PST_RESULT_OK, 71);
    CHECK(g_wait_interests[1] == PST_BACKEND_INTEREST_READ, 72);
    CHECK(public_wait.ready_interest == PST_BACKEND_INTEREST_READ, 73);
    CHECK(pst_connection_read(public_connection, buffer, sizeof(buffer), &public_io) == PST_RESULT_OK, 74);
    CHECK(public_io.operation == PST_OPERATION_COMPLETE && public_io.bytes_transferred == 1, 75);

    g_readiness_scenario = 2; g_scenario_read_calls = 0; g_scenario_wait_calls = 0;
    CHECK(pst_connection_read(public_connection, buffer, sizeof(buffer), &public_io) == PST_RESULT_OK, 76);
    CHECK(pst_connection_wait(public_connection, 250, &public_wait) == PST_RESULT_OK, 77);
    CHECK(g_wait_interests[0] == (PST_BACKEND_INTEREST_READ | PST_BACKEND_INTEREST_WRITE), 78);
    CHECK(public_wait.ready_interest == PST_BACKEND_INTEREST_WRITE, 79);
    CHECK(pst_connection_read(public_connection, buffer, sizeof(buffer), &public_io) == PST_RESULT_OK, 80);
    CHECK(public_io.operation == PST_OPERATION_COMPLETE && public_io.bytes_transferred == 1, 81);

    g_readiness_scenario = 3; g_scenario_read_calls = 0; g_scenario_wait_calls = 0;
    CHECK(pst_connection_read(public_connection, buffer, sizeof(buffer), &public_io) == PST_RESULT_OK, 82);
    CHECK(pst_connection_wait(public_connection, 250, &public_wait) == PST_RESULT_OK, 83);
    CHECK(g_wait_interests[0] == (PST_BACKEND_INTEREST_READ | PST_BACKEND_INTEREST_WRITE), 84);
    CHECK(public_wait.ready_interest == (PST_BACKEND_INTEREST_READ | PST_BACKEND_INTEREST_WRITE), 85);
    CHECK(pst_connection_read(public_connection, buffer, sizeof(buffer), &public_io) == PST_RESULT_OK, 86);
    CHECK(public_io.operation == PST_OPERATION_COMPLETE && public_io.bytes_transferred == 1, 87);
    g_readiness_scenario = 4;
    g_connection.interest = PST_BACKEND_INTEREST_READ;
    CHECK(pst_connection_wait(public_connection, 250, &public_wait) ==
        PST_RESULT_TRANSPORT_FAILURE, 121);
    pst_connection_diagnostic_copy(public_connection, &diagnostic);
    CHECK(diagnostic.valid && diagnostic.result == PST_RESULT_TRANSPORT_FAILURE &&
        diagnostic.phase == PST_DIAGNOSTIC_PHASE_WAIT, 124);
    CHECK(pst_connection_get_interest(public_connection, &interest) ==
        PST_RESULT_INVALID_STATE, 122);
    CHECK(pst_connection_read(public_connection, buffer, sizeof(buffer),
        &public_io) == PST_RESULT_INVALID_STATE, 123);
    pst_connection_release(public_connection);

    CHECK(pst_connection_create(public_runtime, public_config, &public_connection) == PST_RESULT_OK, 89);
    public_connection_b=NULL;CHECK(pst_connection_create(public_runtime,public_config,&public_connection_b)==PST_RESULT_OK,113);
    CHECK(pst_connection_read(public_connection, buffer, sizeof(buffer), &public_io) == PST_RESULT_INVALID_STATE, 90);
    pst_connection_diagnostic_copy(public_connection, &diagnostic);
    CHECK(diagnostic.valid && diagnostic.result == PST_RESULT_INVALID_STATE, 91);
    CHECK(diagnostic.phase == PST_DIAGNOSTIC_PHASE_READ, 101);
    CHECK(diagnostic.native_domain == PST_DIAGNOSTIC_DOMAIN_NONE && diagnostic.native_code == 0, 102);
    public_diagnostic.struct_size=sizeof(public_diagnostic);public_diagnostic.api_version=PST_API_VERSION;
    CHECK(pst_connection_copy_diagnostic(public_connection,&public_diagnostic)==PST_RESULT_OK&&public_diagnostic.valid,115);
    CHECK(public_diagnostic.normalized_result==PST_RESULT_INVALID_STATE&&public_diagnostic.operation==PST_DIAGNOSTIC_OPERATION_READ,116);
    saved_public_diagnostic=public_diagnostic;
    public_accepted=0UL;CHECK(pst_connection_attach(public_connection,&public_transport,PST_OWNERSHIP_TRANSFERRED,&public_accepted)==PST_RESULT_OK&&public_accepted==1UL,117);
    g_next_operation=PST_BACKEND_OPERATION_COMPLETE;
    CHECK(pst_connection_handshake(public_connection,&operation,&error)==PST_RESULT_OK&&operation==PST_OPERATION_COMPLETE,118);
    public_diagnostic.struct_size=sizeof(public_diagnostic);public_diagnostic.api_version=PST_API_VERSION;
    CHECK(pst_connection_copy_diagnostic(public_connection,&public_diagnostic)==PST_RESULT_OK&&!public_diagnostic.valid,119);
    CHECK(saved_public_diagnostic.valid&&saved_public_diagnostic.normalized_result==PST_RESULT_INVALID_STATE,120);
    public_diagnostic.struct_size=sizeof(public_diagnostic);public_diagnostic.api_version=PST_API_VERSION;
    CHECK(pst_connection_copy_diagnostic(public_connection_b,&public_diagnostic)==PST_RESULT_OK&&!public_diagnostic.valid,114);
    pst_connection_release(public_connection_b);pst_connection_release(public_connection);

    CHECK(pst_connection_create(public_runtime, public_config, &public_connection) == PST_RESULT_OK, 92);
    public_accepted = 0UL;
    CHECK(pst_connection_attach(public_connection, &public_transport, PST_OWNERSHIP_TRANSFERRED, &public_accepted) == PST_RESULT_OK, 93);
    g_next_operation = PST_BACKEND_OPERATION_FAILED;
    CHECK(pst_connection_handshake(public_connection, &operation, &error) == PST_RESULT_OK, 94);
    CHECK(operation == PST_OPERATION_FAILED && error == PST_RESULT_AUTH_FAILURE, 95);
    pst_connection_diagnostic_copy(public_connection, &diagnostic);
    CHECK(diagnostic.valid && diagnostic.result == PST_RESULT_AUTH_FAILURE, 96);
    CHECK(diagnostic.phase == PST_DIAGNOSTIC_PHASE_PEER_AUTHENTICATE, 97);
    public_diagnostic.struct_size=sizeof(public_diagnostic);public_diagnostic.api_version=PST_API_VERSION;
    CHECK(pst_connection_copy_diagnostic(public_connection,&public_diagnostic)==PST_RESULT_OK,108);
    CHECK(public_diagnostic.valid&&public_diagnostic.normalized_result==PST_RESULT_AUTH_FAILURE,109);
    CHECK(public_diagnostic.operation==PST_DIAGNOSTIC_OPERATION_AUTHENTICATION&&!strcmp(public_diagnostic.backend_id,"test-backend"),110);
    saved_public_diagnostic=public_diagnostic;
    CHECK(diagnostic.native_domain == PST_DIAGNOSTIC_DOMAIN_NSS && diagnostic.native_code == -8179, 98);
    pst_diagnostic_copy(&saved_diagnostic, &diagnostic);
    pst_connection_release(public_connection);
    CHECK(saved_diagnostic.valid && saved_diagnostic.native_code == -8179, 99);
    CHECK(saved_public_diagnostic.valid&&saved_public_diagnostic.normalized_result==PST_RESULT_AUTH_FAILURE,111);
    CHECK(!strcmp(saved_public_diagnostic.backend_id,"test-backend"),112);
    pst_runtime_diagnostic_copy(public_runtime, &diagnostic);
    CHECK(!diagnostic.valid, 100);
    pst_config_release(public_config);
    pst_runtime_release(public_runtime);
    CHECK(g_transport_destroy_calls == 3, 88);
    CHECK(pst_backend_unregister("test-backend") == PST_RESULT_OK, 50);
    CHECK(pst_backend_count() == 0 && pst_backend_find("test-backend") == NULL, 51);
    CHECK(pst_backend_unregister("test-backend") == PST_RESULT_UNAVAILABLE, 52);
    printf("test_backend_spi: PASS\n"); return 0;
}
