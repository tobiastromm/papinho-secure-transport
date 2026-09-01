#include "papinho_secure_transport.h"
#include "papinho_secure_transport_win32.h"
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

#define MAX_STEPS 80
#define WAIT_MS 125UL
static const char g_expected[] = "pst-phase7b-data-before-close";
static const char g_client_write[] = "pst-phase7b-client-write";

typedef struct failure_log {
    pst_u32 total;
    pst_u32 errors;
    pst_u32 warnings;
} failure_log;

static void PST_CALL capture_log(void *context, const PST_LOG_EVENT *event)
{
    failure_log *log = (failure_log *)context;
    if (log == NULL || event == NULL) return;
    ++log->total;
    if (event->level == PST_LOG_LEVEL_ERROR) ++log->errors;
    if (event->level == PST_LOG_LEVEL_WARN) ++log->warnings;
}

static unsigned char *load_file(const char *path, pst_size *size)
{
    FILE *file;
    long amount;
    unsigned char *data;
    *size = 0;
    file = fopen(path, "rb");
    if (file == NULL) return NULL;
    if (fseek(file, 0, SEEK_END) != 0 ||
        (amount = ftell(file)) <= 0 ||
        fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }
    data = (unsigned char *)malloc((size_t)amount);
    if (data == NULL ||
        fread(data, 1, (size_t)amount, file) != (size_t)amount) {
        free(data);
        fclose(file);
        return NULL;
    }
    fclose(file);
    *size = (pst_size)amount;
    return data;
}

static int connect4(const char *host, unsigned short port, SOCKET *out)
{
    struct sockaddr_in address;
    SOCKET socket_value;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = inet_addr(host);
    socket_value = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_value == INVALID_SOCKET) return 0;
    if (connect(socket_value, (struct sockaddr *)&address, sizeof(address)) != 0) {
        closesocket(socket_value);
        return 0;
    }
    *out = socket_value;
    return 1;
}

static int wait_once(pst_connection *connection, PST_RESULT *failure)
{
    PST_WAIT_RESULT wait_result;
    PST_RESULT result;
    memset(&wait_result, 0, sizeof(wait_result));
    result = pst_connection_wait(connection, WAIT_MS, &wait_result);
    if (result != PST_RESULT_OK) {
        *failure = result;
        return 0;
    }
    return 1;
}

static int terminal_rejects_operations(pst_connection *connection)
{
    PST_IO_RESULT io;
    PST_WAIT_RESULT wait_result;
    PST_RESULT error;
    pst_u32 operation;
    char byte;
    if (pst_connection_handshake(connection, &operation, &error) !=
        PST_RESULT_INVALID_STATE) return 0;
    if (pst_connection_read(connection, &byte, 1, &io) !=
        PST_RESULT_INVALID_STATE) return 0;
    if (pst_connection_write(connection, &byte, 1, &io) !=
        PST_RESULT_INVALID_STATE) return 0;
    if (pst_connection_wait(connection, 0, &wait_result) !=
        PST_RESULT_INVALID_STATE) return 0;
    if (pst_connection_shutdown(connection, &operation, &error) !=
        PST_RESULT_INVALID_STATE) return 0;
    return 1;
}

int main(int argc, char **argv)
{
    WSADATA winsock;
    SOCKET native_socket;
    unsigned char *ca_data;
    unsigned char *cert_data;
    unsigned char *key_data;
    pst_size ca_size;
    pst_size cert_size;
    pst_size key_size;
    PST_TRUST_SOURCE trust_source;
    PST_CREDENTIAL_SOURCE credential_source;
    PST_IDENTITY_CONFIG identity;
    PST_TLS_POLICY policy;
    PST_ALPN_PROTOCOL protocol;
    PST_RUNTIME_OPTIONS runtime_options;
    PST_LOG_CONFIG log_config;
    PST_DIAGNOSTIC_INFO diagnostic;
    PST_IO_RESULT io;
    failure_log log;
    pst_trust *trust;
    pst_credentials *credentials;
    pst_config *config;
    pst_runtime *runtime;
    pst_transport *transport;
    pst_connection *connection;
    pst_u32 accepted;
    pst_u32 operation;
    PST_RESULT error;
    PST_RESULT result;
    PST_RESULT final_result;
    pst_u32 final_close;
    pst_size total_read;
    pst_size total_written;
    char received[64];
    int established;
    int terminal;
    int content_match;
    int ok;
    int step;
    int mode_read;
    int mode_write;
    int mode_shutdown;
    const char *mode;

    if (argc != 11) {
        fprintf(stderr, "usage: host port hostname ca.der client.der key.pk8 tls alpn mode log-level\n");
        return 2;
    }
    mode = argv[9];
    ca_data = load_file(argv[4], &ca_size);
    cert_data = load_file(argv[5], &cert_size);
    key_data = load_file(argv[6], &key_size);
    if (ca_data == NULL || cert_data == NULL || key_data == NULL) return 3;

    trust = NULL;
    credentials = NULL;
    config = NULL;
    runtime = NULL;
    transport = NULL;
    connection = NULL;
    native_socket = INVALID_SOCKET;
    memset(&log, 0, sizeof(log));
    memset(&trust_source, 0, sizeof(trust_source));
    trust_source.struct_size = sizeof(trust_source);
    trust_source.api_version = PST_API_VERSION;
    trust_source.kind = PST_TRUST_SOURCE_CUSTOM_CA_DER;
    trust_source.data = ca_data;
    trust_source.data_size = ca_size;
    if (pst_trust_create(&trust_source, &trust) != PST_RESULT_OK) return 4;

    memset(&credential_source, 0, sizeof(credential_source));
    credential_source.struct_size = sizeof(credential_source);
    credential_source.api_version = PST_API_VERSION;
    credential_source.kind = PST_CREDENTIAL_SOURCE_CERT_DER_PKCS8_DER;
    credential_source.certificate_der = cert_data;
    credential_source.certificate_der_size = cert_size;
    credential_source.private_key_der = key_data;
    credential_source.private_key_der_size = key_size;
    if (pst_credentials_create(&credential_source, &credentials) != PST_RESULT_OK)
        return 5;
    memset(key_data, 0, key_size);
    free(key_data);
    key_data = NULL;

    if (pst_config_create(&config) != PST_RESULT_OK) return 6;
    memset(&identity, 0, sizeof(identity));
    identity.struct_size = sizeof(identity);
    identity.api_version = PST_API_VERSION;
    identity.credentials = credentials;
    identity.trust = trust;
    identity.expected_hostname = argv[3];
    identity.expected_hostname_size = strlen(argv[3]);
    identity.require_peer_authentication = 1;
    identity.require_client_authentication = 1;
    if (pst_config_set_identity(config, &identity) != PST_RESULT_OK) return 7;
    memset(&policy, 0, sizeof(policy));
    policy.struct_size = sizeof(policy);
    policy.api_version = PST_API_VERSION;
    policy.minimum_version = (pst_u32)atoi(argv[7]);
    policy.maximum_version = policy.minimum_version;
    protocol.data = (const pst_u8 *)argv[8];
    protocol.size = strlen(argv[8]);
    policy.alpn_protocols = &protocol;
    policy.alpn_protocol_count = 1;
    policy.alpn_requirement = PST_FEATURE_REQUIRED;
    policy.early_data = PST_FEATURE_DISABLED;
    if (pst_config_set_tls_policy(config, &policy) != PST_RESULT_OK ||
        pst_config_freeze(config) != PST_RESULT_OK) return 8;

    if (pst_win32_register_retrozilla_nss() != PST_RESULT_OK) return 9;
    memset(&runtime_options, 0, sizeof(runtime_options));
    runtime_options.struct_size = sizeof(runtime_options);
    runtime_options.api_version = PST_API_VERSION;
    runtime_options.selection = PST_BACKEND_SELECTION_EXACT;
    runtime_options.exact_backend_id = "retrozilla-nss";
    if (pst_log_config_init(&log_config) != PST_RESULT_OK) return 10;
    log_config.level = (pst_u32)atoi(argv[10]);
    log_config.callback = capture_log;
    log_config.user_context = &log;
    if (pst_runtime_create_with_logging(&runtime_options, &log_config,
        &runtime, NULL) != PST_RESULT_OK) return 11;

    if (WSAStartup(MAKEWORD(2, 0), &winsock) != 0 ||
        !connect4(argv[1], (unsigned short)atoi(argv[2]), &native_socket))
        return 12;
    if (pst_win32_socket_transport_create((pst_size)native_socket,
        &transport) != PST_RESULT_OK) return 13;
    if (pst_connection_create(runtime, config, &connection) != PST_RESULT_OK)
        return 14;
    accepted = 0;
    result = pst_connection_attach(connection, transport,
        PST_OWNERSHIP_TRANSFERRED, &accepted);
    if (result != PST_RESULT_OK || accepted != 1UL) return 15;
    transport = NULL;
    native_socket = INVALID_SOCKET;

    established = 0;
    final_result = PST_RESULT_OK;
    final_close = PST_CLOSE_NONE;
    for (step = 0; step < MAX_STEPS; ++step) {
        result = pst_connection_handshake(connection, &operation, &error);
        if (result != PST_RESULT_OK) {
            final_result = result;
            break;
        }
        if (operation == PST_OPERATION_COMPLETE) {
            established = 1;
            break;
        }
        if (operation == PST_OPERATION_FAILED) {
            final_result = error;
            break;
        }
        if (!wait_once(connection, &final_result)) break;
    }

    mode_read = !strcmp(mode, "clean_close") ||
        !strcmp(mode, "abrupt_close") ||
        !strcmp(mode, "read_clean") ||
        !strcmp(mode, "read_abrupt") ||
        !strcmp(mode, "data_then_close");
    mode_write = !strcmp(mode, "close_around_write");
    mode_shutdown = !strcmp(mode, "shutdown_abort");
    total_read = 0;
    total_written = 0;
    memset(received, 0, sizeof(received));

    if (established && mode_read) {
        for (step = 0; step < MAX_STEPS; ++step) {
            memset(&io, 0, sizeof(io));
            result = pst_connection_read(connection, received + total_read,
                sizeof(received) - total_read, &io);
            total_read += io.bytes_transferred;
            if (result != PST_RESULT_OK) {
                final_result = result;
                break;
            }
            if (io.operation == PST_OPERATION_CLOSED) {
                final_result = PST_RESULT_CLOSED;
                final_close = io.close_kind;
                break;
            }
            if (io.operation == PST_OPERATION_FAILED) {
                final_result = io.error;
                final_close = io.close_kind;
                break;
            }
            if (!wait_once(connection, &final_result)) break;
        }
    } else if (established && mode_write) {
        for (step = 0; step < MAX_STEPS &&
            total_written < sizeof(g_client_write) - 1; ++step) {
            memset(&io, 0, sizeof(io));
            result = pst_connection_write(connection,
                g_client_write + total_written,
                sizeof(g_client_write) - 1 - total_written, &io);
            total_written += io.bytes_transferred;
            if (result != PST_RESULT_OK ||
                io.operation == PST_OPERATION_FAILED) {
                final_result = result != PST_RESULT_OK ? result : io.error;
                break;
            }
            if (io.operation != PST_OPERATION_COMPLETE &&
                !wait_once(connection, &final_result)) break;
        }
        if (final_result == PST_RESULT_OK) {
            for (step = 0; step < MAX_STEPS; ++step) {
                memset(&io, 0, sizeof(io));
                result = pst_connection_read(connection, received, 1, &io);
                if (result != PST_RESULT_OK) {
                    final_result = result;
                    break;
                }
                if (io.operation == PST_OPERATION_FAILED) {
                    final_result = io.error;
                    final_close = io.close_kind;
                    break;
                }
                if (io.operation == PST_OPERATION_CLOSED) {
                    final_result = PST_RESULT_CLOSED;
                    final_close = io.close_kind;
                    break;
                }
                if (!wait_once(connection, &final_result)) break;
            }
        }
    } else if (established && mode_shutdown) {
        for (step = 0; step < MAX_STEPS &&
            total_written < sizeof(g_client_write) - 1; ++step) {
            memset(&io, 0, sizeof(io));
            result = pst_connection_write(connection,
                g_client_write + total_written,
                sizeof(g_client_write) - 1 - total_written, &io);
            total_written += io.bytes_transferred;
            if (result != PST_RESULT_OK ||
                io.operation == PST_OPERATION_FAILED) {
                final_result = result != PST_RESULT_OK ? result : io.error;
                break;
            }
            if (io.operation != PST_OPERATION_COMPLETE &&
                !wait_once(connection, &final_result)) break;
        }
        if (final_result == PST_RESULT_OK) {
            for (step = 0; step < MAX_STEPS; ++step) {
                result = pst_connection_shutdown(connection, &operation, &error);
                if (result != PST_RESULT_OK) {
                    final_result = result;
                    break;
                }
                if (operation == PST_OPERATION_COMPLETE) {
                    final_result = PST_RESULT_CLOSED;
                    final_close = PST_CLOSE_CLEAN;
                    break;
                }
                if (operation == PST_OPERATION_FAILED) {
                    final_result = error;
                    break;
                }
                if (!wait_once(connection, &final_result)) break;
            }
        }
    }

    memset(&diagnostic, 0, sizeof(diagnostic));
    diagnostic.struct_size = sizeof(diagnostic);
    diagnostic.api_version = PST_API_VERSION;
    pst_connection_copy_diagnostic(connection, &diagnostic);
    terminal = terminal_rejects_operations(connection);
    content_match = total_read == sizeof(g_expected) - 1 &&
        memcmp(received, g_expected, total_read) == 0;

    ok = step < MAX_STEPS && terminal;
    if (!established) {
        ok = ok && final_result != PST_RESULT_OK &&
            diagnostic.valid &&
            diagnostic.normalized_result == final_result;
    } else if (!strcmp(mode, "data_then_close")) {
        ok = ok && content_match &&
            final_result == PST_RESULT_CLOSED &&
            final_close == PST_CLOSE_CLEAN &&
            !diagnostic.valid;
    } else if (!strcmp(mode, "clean_close") ||
        !strcmp(mode, "read_clean")) {
        ok = ok && final_result == PST_RESULT_CLOSED &&
            final_close == PST_CLOSE_CLEAN &&
            !diagnostic.valid;
    } else if (!strcmp(mode, "abrupt_close") ||
        !strcmp(mode, "read_abrupt")) {
        ok = ok && final_result == PST_RESULT_TRUNCATED &&
            final_close == PST_CLOSE_TRUNCATED &&
            diagnostic.valid &&
            diagnostic.normalized_result == final_result;
    } else if (mode_write || mode_shutdown) {
        ok = ok && total_written <= sizeof(g_client_write) - 1 &&
            final_result != PST_RESULT_OK;
    }
    if (log_config.level == PST_LOG_LEVEL_OFF)
        ok = ok && log.total == 0UL;
    if (log_config.level == PST_LOG_LEVEL_ERROR && final_result != PST_RESULT_CLOSED)
        ok = ok && log.errors == 1UL && log.warnings == 0UL;

    printf("MODE=%s ESTABLISHED=%d FINAL=%ld CLOSE_KIND=%lu READ=%lu WRITE=%lu CONTENT_MATCH=%d TERMINAL=%d DIAG_VALID=%lu DIAG_RESULT=%ld DIAG_OPERATION=%lu LOG_TOTAL=%lu LOG_ERROR=%lu LOG_WARN=%lu STEPS=%d PASS=%d\n",
        mode, established, (long)final_result, (unsigned long)final_close,
        (unsigned long)total_read, (unsigned long)total_written, content_match,
        terminal, (unsigned long)diagnostic.valid,
        (long)diagnostic.normalized_result,
        (unsigned long)diagnostic.operation,
        (unsigned long)log.total, (unsigned long)log.errors,
        (unsigned long)log.warnings, step, ok);

    pst_connection_release(connection);
    pst_config_release(config);
    pst_credentials_release(credentials);
    pst_trust_release(trust);
    pst_runtime_release(runtime);
    if (transport != NULL) pst_transport_release(transport);
    if (native_socket != INVALID_SOCKET) closesocket(native_socket);
    WSACleanup();
    free(ca_data);
    free(cert_data);
    if (key_data != NULL) {
        memset(key_data, 0, key_size);
        free(key_data);
    }
    return ok ? 0 : 20;
}
