/* SPDX-License-Identifier: MPL-2.0 */
#include <winsock.h>
#include "papinho_secure_transport.h"
#include "papinho_secure_transport_win32.h"

#include <stdio.h>
#include <string.h>

/* The application owns its listener. Pass only an accepted socket here. */
static PST_RESULT serve_accepted_socket(pst_runtime *runtime,
                                        pst_size accepted_socket,
                                        pst_credentials *identity,
                                        pst_trust *client_trust)
{
    PST_CONNECTION_CONFIG config;
    PST_WAIT_RESULT wait_result;
    PST_IO_RESULT io;
    PST_DIAGNOSTIC_INFO diagnostic;
    PST_PEER_INFO_SUMMARY summary;
    pst_connection *connection = NULL;
    pst_transport *transport = NULL;
    pst_peer_info *peer = NULL;
    pst_u32 ownership_accepted = 0;
    pst_u32 operation = PST_OPERATION_FAILED;
    PST_RESULT error = PST_RESULT_OK;
    PST_RESULT result;
    unsigned char buffer[256];
    unsigned int steps;

    memset(&config, 0, sizeof(config));
    config.struct_size = sizeof(config);
    config.api_version = PST_API_VERSION;
    config.role = PST_CONNECTION_ROLE_SERVER;
    config.provider_selection.struct_size = sizeof(config.provider_selection);
    config.provider_selection.api_version = PST_API_VERSION;
    config.provider_selection.mode = PST_BACKEND_SELECTION_AUTOMATIC;
    config.local_identity.struct_size = sizeof(config.local_identity);
    config.local_identity.api_version = PST_API_VERSION;
    config.local_identity.credentials = identity;
    config.peer_authentication.struct_size = sizeof(config.peer_authentication);
    config.peer_authentication.api_version = PST_API_VERSION;
    config.peer_authentication.certificate_mode = PST_PEER_CERTIFICATE_REQUIRED;
    config.peer_authentication.trust = client_trust;
    config.tls.struct_size = sizeof(config.tls);
    config.tls.api_version = PST_API_VERSION;
    config.tls.minimum_version = PST_TLS_VERSION_1_2;
    config.tls.maximum_version = PST_TLS_VERSION_1_3;
    config.tls.require_graceful_shutdown = PST_FEATURE_REQUIRED;
    config.alpn.struct_size = sizeof(config.alpn);
    config.alpn.api_version = PST_API_VERSION;
    config.alpn.mode = PST_FEATURE_DISABLED;

    result = pst_connection_create(runtime, &config, &connection);
    if (result != PST_RESULT_OK) return result;
    result = pst_win32_socket_transport_create(accepted_socket, &transport);
    if (result != PST_RESULT_OK) {
        closesocket((SOCKET)accepted_socket);
        pst_connection_release(connection);
        return result;
    }
    if (result == PST_RESULT_OK)
        result = pst_connection_attach(connection, transport,
                                       PST_OWNERSHIP_TRANSFERRED,
                                       &ownership_accepted);
    if (!ownership_accepted && transport != NULL) pst_transport_release(transport);

    for (steps = 0; result == PST_RESULT_OK && steps < 100U; ++steps) {
        result = pst_connection_handshake(connection, &operation, &error);
        if (result != PST_RESULT_OK || operation == PST_OPERATION_COMPLETE) break;
        if (operation == PST_OPERATION_FAILED) { result = error; break; }
        memset(&wait_result, 0, sizeof(wait_result));
        result = pst_connection_wait(connection, 5000UL, &wait_result);
        if (result == PST_RESULT_OK && wait_result.timed_out)
            result = PST_RESULT_TRANSPORT_FAILURE;
    }
    if (result == PST_RESULT_OK && operation == PST_OPERATION_COMPLETE) {
        memset(&summary, 0, sizeof(summary));
        summary.struct_size = sizeof(summary);
        summary.api_version = PST_API_VERSION;
        if (pst_connection_get_peer_info(connection, &peer) == PST_RESULT_OK) {
            (void)pst_peer_info_get_summary(peer, &summary);
            printf("peer provider=%s authenticated=%lu TLS=%lu\n",
                   summary.provider_id,
                   (unsigned long)summary.peer_authenticated,
                   (unsigned long)summary.tls_version);
            pst_peer_info_release(peer);
        }
        memset(&io, 0, sizeof(io));
        result = pst_connection_read(connection, buffer, sizeof(buffer), &io);
        if (result == PST_RESULT_OK && io.bytes_transferred != 0) {
            pst_size received = io.bytes_transferred;
            memset(&io, 0, sizeof(io));
            result = pst_connection_write(connection, buffer, received, &io);
        }
        /* Production code should continue incrementally for partial I/O. */
    }
    for (steps = 0; result == PST_RESULT_OK && steps < 40U; ++steps) {
        result = pst_connection_shutdown(connection, &operation, &error);
        if (result != PST_RESULT_OK || operation == PST_OPERATION_COMPLETE) break;
        if (operation == PST_OPERATION_FAILED) { result = error; break; }
        memset(&wait_result, 0, sizeof(wait_result));
        result = pst_connection_wait(connection, 5000UL, &wait_result);
        if (result == PST_RESULT_OK && wait_result.timed_out)
            result = PST_RESULT_TRANSPORT_FAILURE;
    }
    if (result != PST_RESULT_OK) {
        memset(&diagnostic, 0, sizeof(diagnostic));
        diagnostic.struct_size = sizeof(diagnostic);
        diagnostic.api_version = PST_API_VERSION;
        if (pst_connection_copy_diagnostic(connection, &diagnostic) == PST_RESULT_OK &&
            diagnostic.valid)
            printf("failure result=%ld operation=%lu provider=%s\n",
                   (long)diagnostic.normalized_result,
                   (unsigned long)diagnostic.operation,
                   diagnostic.backend_id);
    }
    pst_connection_release(connection);
    return result;
}

/*
 * Listener ownership never crosses into PST. The application may apply its
 * own admission policy after accept() and before this handoff.
 */
static PST_RESULT run_one_server_connection(pst_runtime *runtime,
                                            unsigned short port,
                                            pst_credentials *identity,
                                            pst_trust *client_trust)
{
    struct sockaddr_in address;
    SOCKET listener;
    SOCKET accepted;
    PST_RESULT result;

    listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == INVALID_SOCKET) return PST_RESULT_TRANSPORT_FAILURE;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);
    if (bind(listener, (const struct sockaddr *)&address,
             sizeof(address)) == SOCKET_ERROR ||
        listen(listener, 8) == SOCKET_ERROR) {
        closesocket(listener);
        return PST_RESULT_TRANSPORT_FAILURE;
    }
    accepted = accept(listener, NULL, NULL);
    closesocket(listener); /* The listener is always application-owned. */
    if (accepted == INVALID_SOCKET) return PST_RESULT_TRANSPORT_FAILURE;
    result = serve_accepted_socket(runtime, (pst_size)accepted,
                                   identity, client_trust);
    /* serve_accepted_socket either transfers or closes the accepted socket. */
    return result;
}

int main(int argc, char **argv)
{
    PST_RUNTIME_OPTIONS options;
    pst_runtime *runtime = NULL;
    (void)argv;
    if (pst_win32_register_builtin_providers() != PST_RESULT_OK) return 1;
    memset(&options, 0, sizeof(options));
    options.struct_size = sizeof(options);
    options.api_version = PST_API_VERSION;
    if (pst_runtime_create(&options, &runtime) != PST_RESULT_OK) return 2;
    printf("SERVER runtime ready; application owns bind/listen/accept.\n");
    /* Supply deployment credentials/trust before enabling this sample call. */
    if (argc == 999)
        (void)run_one_server_connection(runtime, 8443, NULL, NULL);
    pst_runtime_release(runtime);
    return 0;
}
