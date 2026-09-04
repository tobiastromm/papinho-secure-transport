#include "papinho_secure_transport.h"
#include "papinho_secure_transport_win32.h"

#include <stdio.h>
#include <string.h>

static int wait_then_retry(pst_connection *connection)
{
    PST_WAIT_RESULT wait_result;
    memset(&wait_result, 0, sizeof(wait_result));
    return pst_connection_wait(connection, 5000UL, &wait_result) == PST_RESULT_OK &&
           !wait_result.timed_out && wait_result.ready_interest != PST_INTEREST_NONE;
}

static PST_RESULT attach_socket(pst_connection *connection, pst_size socket_value)
{
    pst_transport *transport = NULL;
    pst_u32 accepted = 0;
    PST_RESULT result;
    result = pst_win32_socket_transport_create(socket_value, &transport);
    if (result != PST_RESULT_OK) return result;
    result = pst_connection_attach(connection, transport,
                                   PST_OWNERSHIP_TRANSFERRED, &accepted);
    if (!accepted) pst_transport_release(transport);
    return result;
}

static PST_RESULT drive_handshake(pst_connection *connection)
{
    pst_u32 operation;
    PST_RESULT error;
    PST_RESULT result;
    unsigned int steps;
    for (steps = 0; steps < 100U; ++steps) {
        result = pst_connection_handshake(connection, &operation, &error);
        if (result != PST_RESULT_OK) return result;
        if (operation == PST_OPERATION_COMPLETE) return PST_RESULT_OK;
        if (operation == PST_OPERATION_FAILED) return error;
        if (!wait_then_retry(connection)) return PST_RESULT_TRANSPORT_FAILURE;
    }
    return PST_RESULT_RESOURCE_FAILURE;
}

static PST_RESULT write_all(pst_connection *connection,
                            const unsigned char *data, pst_size length)
{
    PST_IO_RESULT io;
    PST_RESULT result;
    pst_size total = 0;
    unsigned int steps;
    for (steps = 0; steps < 100U && total < length; ++steps) {
        memset(&io, 0, sizeof(io));
        result = pst_connection_write(connection, data + total,
                                      length - total, &io);
        if (result != PST_RESULT_OK) return result;
        total += io.bytes_transferred;
        if (io.operation == PST_OPERATION_FAILED) return io.error;
        if (io.operation == PST_OPERATION_CLOSED) return PST_RESULT_CLOSED;
        if (total < length && !wait_then_retry(connection))
            return PST_RESULT_TRANSPORT_FAILURE;
    }
    return total == length ? PST_RESULT_OK : PST_RESULT_RESOURCE_FAILURE;
}

static PST_RESULT read_some(pst_connection *connection, unsigned char *data,
                            pst_size capacity, pst_size *received)
{
    PST_IO_RESULT io;
    PST_RESULT result;
    unsigned int steps;
    *received = 0;
    for (steps = 0; steps < 100U; ++steps) {
        memset(&io, 0, sizeof(io));
        result = pst_connection_read(connection, data, capacity, &io);
        if (result != PST_RESULT_OK) return result;
        if (io.bytes_transferred != 0) {
            *received = io.bytes_transferred;
            return PST_RESULT_OK;
        }
        if (io.operation == PST_OPERATION_FAILED) return io.error;
        if (io.operation == PST_OPERATION_CLOSED) return PST_RESULT_CLOSED;
        if (!wait_then_retry(connection)) return PST_RESULT_TRANSPORT_FAILURE;
    }
    return PST_RESULT_RESOURCE_FAILURE;
}

static PST_RESULT drive_shutdown(pst_connection *connection)
{
    pst_u32 operation;
    PST_RESULT error;
    PST_RESULT result;
    unsigned int steps;
    for (steps = 0; steps < 40U; ++steps) {
        result = pst_connection_shutdown(connection, &operation, &error);
        if (result != PST_RESULT_OK) return result;
        if (operation == PST_OPERATION_COMPLETE) return PST_RESULT_OK;
        if (operation == PST_OPERATION_FAILED) return error;
        if (!wait_then_retry(connection)) return PST_RESULT_TRANSPORT_FAILURE;
    }
    return PST_RESULT_RESOURCE_FAILURE;
}

int main(int argc, char **argv)
{
    PST_RUNTIME_OPTIONS options;
    pst_runtime *runtime = NULL;
    unsigned char buffer[64];
    pst_size received;
    static const unsigned char request[] = "application protocol request";
    (void)argv;

    if (pst_win32_register_builtin_providers() != PST_RESULT_OK) return 1;
    memset(&options, 0, sizeof(options));
    options.struct_size = sizeof(options);
    options.api_version = PST_API_VERSION;
    options.selection = PST_BACKEND_SELECTION_AUTOMATIC;
    options.required_capabilities = PST_CAP_TLS_1_2 |
                                    PST_CAP_HOSTNAME_VERIFY |
                                    PST_CAP_NONBLOCKING;
    if (pst_runtime_create(&options, &runtime) != PST_RESULT_OK) return 2;

    printf("Built-in provider selected. Create authenticated trust/config and a TCP socket before using the bounded helpers.\n");
    if (argc == 999) {
        (void)attach_socket(NULL, 0);
        (void)drive_handshake(NULL);
        (void)write_all(NULL, request, sizeof(request) - 1);
        (void)read_some(NULL, buffer, sizeof(buffer), &received);
        (void)drive_shutdown(NULL);
    }
    pst_runtime_release(runtime);
    return 0;
}