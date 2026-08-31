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
static PST_RESULT wait_for_backend(const PST_BACKEND_DESCRIPTOR *descriptor,
                                   void *connection)
{
    pst_u32 interest;
    PST_BACKEND_WAIT_RESULT wait_result;

    PST_RESULT result;
    result = descriptor->vtable->get_interest(connection, &interest);
    if (result != PST_RESULT_OK) return result;
    return descriptor->vtable->wait(connection, interest, 250UL, &wait_result);
}
static unsigned char *load_file(const char *path, pst_size *out_size)
{
    FILE *file; long length; unsigned char *data;
    *out_size = 0; file = fopen(path, "rb"); if (file == NULL) return NULL;
    if (fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) <= 0 ||
        fseek(file, 0, SEEK_SET) != 0) { fclose(file); return NULL; }
    data = (unsigned char *)malloc((size_t)length);
    if (data == NULL || fread(data, 1, (size_t)length, file) != (size_t)length) {
        free(data); fclose(file); return NULL;
    }
    fclose(file); *out_size = (pst_size)length; return data;
}
int main(int argc, char **argv)
{
    WSADATA winsock_data;
    const PST_BACKEND_DESCRIPTOR *descriptor;
    void *backend_state;
    void *runtime_state;
    void *connection_state;
    PST_NSS_NATIVE_TRANSPORT transport;
    PST_BACKEND_IO_RESULT io;
    PST_BACKEND_WAIT_RESULT wait_result;

    SOCKET socket_value;
    pst_u32 accepted;
    pst_u32 operation;
    pst_u32 interest;
    PST_RESULT error;
    PST_RESULT result;
    unsigned short port;
    int complete;
    int io_complete;
    const char message[] = "pst-phase3-functional-proof";
    char received[64];
    pst_size written;
    pst_size received_count;
    pst_u32 protocol_version;
    pst_u32 shutdown_operation;
    int i;
    unsigned char *ca_der, *client_der, *key_der; pst_size ca_n, client_n, key_n;
    pst_trust *trust; pst_credentials *credentials; pst_config *config;
    PST_TRUST_SOURCE trust_source; PST_CREDENTIAL_SOURCE credential_source;
    PST_IDENTITY_CONFIG identity; pst_peer_info *peer; PST_PEER_INFO_SUMMARY summary;
    if (argc != 4 && argc != 7) {
        fprintf(stderr, "usage: test_backend_nss_integration host port hostname [ca.der client.der key.pk8]\n");
        return 2;
    }
    port = (unsigned short)atoi(argv[2]);
    if (port == 0) return 3;
    if (WSAStartup(MAKEWORD(2, 0), &winsock_data) != 0) return 4;
    if (!connect_ipv4(argv[1], port, &socket_value)) { WSACleanup(); return 5; }
    descriptor = pst_backend_nss_descriptor(); error = PST_RESULT_OK; backend_state = NULL;
    runtime_state = NULL; connection_state = NULL; accepted = 0UL; complete = 0;
    io_complete = 0; written = 0; received_count = 0; shutdown_operation = 0UL;
    ca_der=NULL;client_der=NULL;key_der=NULL;ca_n=0;client_n=0;key_n=0;
    trust=NULL;credentials=NULL;config=NULL;peer=NULL;
    result = descriptor->vtable->initialize(&backend_state);
    if (result != PST_RESULT_OK) { closesocket(socket_value); WSACleanup(); return 6; }
    result = descriptor->vtable->runtime_create(backend_state, &runtime_state);
    if (result != PST_RESULT_OK) { closesocket(socket_value); descriptor->vtable->shutdown(backend_state); WSACleanup(); return 7; }
    result = descriptor->vtable->connection_create(runtime_state, &connection_state);
    if (result != PST_RESULT_OK) { closesocket(socket_value); descriptor->vtable->runtime_destroy(runtime_state); descriptor->vtable->shutdown(backend_state); WSACleanup(); return 8; }
    if (argc == 7) {
        ca_der=load_file(argv[4],&ca_n);client_der=load_file(argv[5],&client_n);key_der=load_file(argv[6],&key_n);
        if(!ca_der||!client_der||!key_der){result=PST_RESULT_INVALID_ARGUMENT;goto cleanup;}
        memset(&trust_source,0,sizeof(trust_source));trust_source.struct_size=sizeof(trust_source);trust_source.api_version=PST_API_VERSION;trust_source.kind=PST_TRUST_SOURCE_CUSTOM_CA_DER;trust_source.data=ca_der;trust_source.data_size=ca_n;
        result=pst_trust_create(&trust_source,&trust);if(result!=PST_RESULT_OK)goto cleanup;
        memset(&credential_source,0,sizeof(credential_source));credential_source.struct_size=sizeof(credential_source);credential_source.api_version=PST_API_VERSION;credential_source.kind=PST_CREDENTIAL_SOURCE_CERT_DER_PKCS8_DER;credential_source.certificate_der=client_der;credential_source.certificate_der_size=client_n;credential_source.private_key_der=key_der;credential_source.private_key_der_size=key_n;
        result=pst_credentials_create(&credential_source,&credentials);if(result!=PST_RESULT_OK)goto cleanup;
        memset(key_der,0,key_n);free(key_der);key_der=NULL;
        result=pst_config_create(&config);if(result!=PST_RESULT_OK)goto cleanup;
        memset(&identity,0,sizeof(identity));identity.struct_size=sizeof(identity);identity.api_version=PST_API_VERSION;identity.credentials=credentials;identity.trust=trust;identity.expected_hostname=argv[3];identity.expected_hostname_size=strlen(argv[3]);identity.require_peer_authentication=1;identity.require_client_authentication=1;
        result=pst_config_set_identity(config,&identity);if(result!=PST_RESULT_OK)goto cleanup;
        result=pst_config_freeze(config);if(result!=PST_RESULT_OK)goto cleanup;
        result=descriptor->vtable->connection_configure_identity(connection_state,config);if(result!=PST_RESULT_OK)goto cleanup;
    }
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
        protocol_version = pst_backend_nss_connection_protocol_version(connection_state);
        if (argc == 7) {
            result=descriptor->vtable->peer_info_create(connection_state,(void**)&peer);
            if(result!=PST_RESULT_OK){complete=0;goto cleanup;}
            memset(&summary,0,sizeof(summary));summary.struct_size=sizeof(summary);summary.api_version=PST_API_VERSION;
            result=pst_peer_info_get_summary(peer,&summary);
            if(result!=PST_RESULT_OK||summary.peer_authenticated!=PST_KNOWN_TRUE||summary.hostname_validated!=PST_KNOWN_TRUE||summary.certificate_sha256_size!=32){complete=0;goto cleanup;}
            printf("PEER_AUTH=%lu HOSTNAME=%lu CIPHER=0x%04lx LEAF_DER=%lu SHA256=32\n",(unsigned long)summary.peer_authenticated,(unsigned long)summary.hostname_validated,(unsigned long)summary.cipher_suite,(unsigned long)summary.leaf_der_size);
        }
        for (i = 0; i < 200 && written < sizeof(message) - 1; ++i) {
            result = descriptor->vtable->write(connection_state, message + written,
                (sizeof(message) - 1) - written, &io);
            if (result != PST_RESULT_OK || io.operation == PST_BACKEND_OPERATION_FAILED) break;
            written += io.bytes_transferred;
            if (io.operation != PST_BACKEND_OPERATION_COMPLETE &&
                wait_for_backend(descriptor, connection_state) != PST_RESULT_OK) break;
        }
        for (i = 0; i < 200 && received_count < sizeof(message) - 1; ++i) {
            result = descriptor->vtable->read(connection_state, received + received_count,
                (sizeof(message) - 1) - received_count, &io);
            if (result != PST_RESULT_OK || io.operation == PST_BACKEND_OPERATION_FAILED ||
                io.operation == PST_BACKEND_OPERATION_CLOSED) break;
            received_count += io.bytes_transferred;
            if (received_count < sizeof(message) - 1 &&
                wait_for_backend(descriptor, connection_state) != PST_RESULT_OK) break;
        }
        io_complete = written == sizeof(message) - 1 &&
            received_count == sizeof(message) - 1 &&
            memcmp(message, received, sizeof(message) - 1) == 0;
        descriptor->vtable->shutdown_step(connection_state, &shutdown_operation, &error);
        if (io_complete) {
            printf("TLS_VERSION=0x%04lx WRITE=%lu READ=%lu SHUTDOWN=%lu\n",
                   (unsigned long)protocol_version, (unsigned long)written,
                   (unsigned long)received_count, (unsigned long)shutdown_operation);
            printf("test_backend_nss_integration: PASS\n");
        }
    } else {
        fprintf(stderr, "handshake failed: result=%ld native=%ld\n",
                (long)error, (long)pst_backend_nss_last_error(connection_state));
    }
cleanup:
    if(peer)descriptor->vtable->peer_info_destroy(peer);
    pst_config_release(config);pst_credentials_release(credentials);pst_trust_release(trust);
    free(ca_der);free(client_der);if(key_der){memset(key_der,0,key_n);free(key_der);}
    descriptor->vtable->connection_destroy(connection_state);
    descriptor->vtable->runtime_destroy(runtime_state);
    descriptor->vtable->shutdown(backend_state); WSACleanup();
    return complete && io_complete ? 0 : 10;
}
