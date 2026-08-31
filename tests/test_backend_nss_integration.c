#include "backends/nss/pst_backend_nss.h"
#if defined(_MSC_VER) && _MSC_VER == 1200
# pragma warning(push)
# pragma warning(disable:4115 4514)
#endif
#include <windows.h>
#include <winsock.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_MSC_VER) && _MSC_VER == 1200
# pragma warning(pop)
# pragma warning(disable:4514)
#endif
static int connect_ipv4(const char *host, unsigned short port, SOCKET *out_socket)
{
    struct sockaddr_in address;
    struct hostent *entry;
    SOCKET socket_value;
    unsigned long numeric;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET; address.sin_port = htons(port);
    numeric = inet_addr(host);
    if (numeric == INADDR_NONE) {
        entry = gethostbyname(host);
        if (entry == NULL || entry->h_addrtype != AF_INET) return 0;
        memcpy(&address.sin_addr, entry->h_addr, sizeof(address.sin_addr));
    } else address.sin_addr.s_addr = numeric;
    socket_value = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_value == INVALID_SOCKET) return 0;
    if (connect(socket_value, (struct sockaddr *)&address, sizeof(address)) != 0) {
        closesocket(socket_value); return 0;
    }
    *out_socket = socket_value; return 1;
}
int main(int argc, char **argv)
{
    WSADATA winsock_data;
    const PST_BACKEND_DESCRIPTOR *descriptor;
    void *backend_state;
    void *runtime_state;
    void *connection_state;
    PST_NSS_NATIVE_TRANSPORT transport;
    PST_BACKEND_WAIT_RESULT wait_result;
    SOCKET socket_value;
    pst_u32 accepted;
    pst_u32 operation;
    pst_u32 interest;
    PST_RESULT error;
    PST_RESULT result;
    unsigned short port;
    int complete;
    int i;
    if (argc != 4) {
        fprintf(stderr, "usage: test_backend_nss_integration host port certificate-hostname\n");
        return 2;
    }
    port = (unsigned short)atoi(argv[2]);
    if (port == 0) return 3;
    if (WSAStartup(MAKEWORD(2, 0), &winsock_data) != 0) return 4;
    if (!connect_ipv4(argv[1], port, &socket_value)) { WSACleanup(); return 5; }
    descriptor = pst_backend_nss_descriptor(); error = PST_RESULT_OK; backend_state = NULL;
    runtime_state = NULL; connection_state = NULL; accepted = 0UL; complete = 0;
    result = descriptor->vtable->initialize(&backend_state);
    if (result != PST_RESULT_OK) { closesocket(socket_value); WSACleanup(); return 6; }
    result = descriptor->vtable->runtime_create(backend_state, &runtime_state);
    if (result != PST_RESULT_OK) { closesocket(socket_value); descriptor->vtable->shutdown(backend_state); WSACleanup(); return 7; }
    result = descriptor->vtable->connection_create(runtime_state, &connection_state);
    if (result != PST_RESULT_OK) { closesocket(socket_value); descriptor->vtable->runtime_destroy(runtime_state); descriptor->vtable->shutdown(backend_state); WSACleanup(); return 8; }
    memset(&transport, 0, sizeof(transport));
    transport.struct_size = sizeof(transport); transport.version = PST_NSS_NATIVE_TRANSPORT_VERSION;
    transport.kind = PST_NSS_NATIVE_TRANSPORT_KIND_WIN32_SOCKET;
    transport.native_socket = (pst_size)socket_value; transport.hostname = argv[3];
    result = descriptor->vtable->attach_transport(connection_state, &transport,
        PST_BACKEND_OWNERSHIP_TRANSFERRED, &accepted);
    if (result != PST_RESULT_OK) {
        if (!accepted) closesocket(socket_value);
        descriptor->vtable->connection_destroy(connection_state);
        descriptor->vtable->runtime_destroy(runtime_state);
        descriptor->vtable->shutdown(backend_state); WSACleanup(); return 9;
    }
    socket_value = INVALID_SOCKET;
    for (i = 0; i < 200; ++i) {
        result = descriptor->vtable->handshake_step(connection_state, &operation, &error);
        if (result != PST_RESULT_OK || operation == PST_BACKEND_OPERATION_FAILED) break;
        if (operation == PST_BACKEND_OPERATION_COMPLETE) { complete = 1; break; }
        result = descriptor->vtable->get_interest(connection_state, &interest);
        if (result != PST_RESULT_OK) break;
        result = descriptor->vtable->wait(connection_state, interest, 250UL, &wait_result);
        if (result != PST_RESULT_OK) break;
    }
    if (complete) {
        descriptor->vtable->shutdown_step(connection_state, &operation, &error);
        printf("test_backend_nss_integration: PASS\n");
    } else {
        fprintf(stderr, "handshake failed: result=%ld native=%ld\n",
                (long)error, (long)pst_backend_nss_last_error(connection_state));
    }
    descriptor->vtable->connection_destroy(connection_state);
    descriptor->vtable->runtime_destroy(runtime_state);
    descriptor->vtable->shutdown(backend_state); WSACleanup();
    return complete ? 0 : 10;
}