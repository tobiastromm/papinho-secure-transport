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
#include <stdarg.h>
#include <errno.h>
#if defined(_MSC_VER) && _MSC_VER == 1200
# pragma warning(pop)
# pragma warning(disable:4514)
#endif

#define MAX_STEPS 80
#define WAIT_MS 125UL
#define CONTROL_WAIT_MS 2000UL
static const char g_expected[] = "pst-phase7b-data-before-close";
static const char g_client_write[] = "pst-phase7b-client-write";
static DWORD g_epoch;
static FILE *g_client_log;

static void timeline(const char *format, ...)
{
    va_list arguments;
    if (g_client_log == NULL) return;
    fprintf(g_client_log, "T_MS=%010lu ",
            (unsigned long)(GetTickCount() - g_epoch));
    va_start(arguments, format);
    vfprintf(g_client_log, format, arguments);
    va_end(arguments);
    fprintf(g_client_log, "\n");
    fflush(g_client_log);
}

static void console_marker(const char *format, ...)
{
    va_list arguments;
    printf("T_MS=%010lu ", (unsigned long)(GetTickCount() - g_epoch));
    va_start(arguments, format);
    vprintf(format, arguments);
    va_end(arguments);
    printf("\n");
    fflush(stdout);
}

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

static SOCKET socket4_create(int *native_error)
{
    SOCKET value;
    value = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (value == INVALID_SOCKET) {
        *native_error = WSAGetLastError();
        return INVALID_SOCKET;
    }
    *native_error = 0;
    return value;
}

static int connect4(SOCKET value, const char *host, unsigned short port,
                    int *native_error)
{
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = inet_addr(host);
    if (connect(value, (struct sockaddr *)&address, sizeof(address)) != 0) {
        *native_error = WSAGetLastError();
        return 0;
    }
    *native_error = 0;
    return 1;
}

static int control_receive(SOCKET value, char expected, DWORD *elapsed)
{
    fd_set readable;
    struct timeval timeout;
    DWORD start;
    char marker;
    int result;
    readable.fd_count = 1;
    readable.fd_array[0] = value;
    timeout.tv_sec = (long)(CONTROL_WAIT_MS / 1000UL);
    timeout.tv_usec = (long)((CONTROL_WAIT_MS % 1000UL) * 1000UL);
    start = GetTickCount();
    result = select(0, &readable, NULL, NULL, &timeout);
    *elapsed = GetTickCount() - start;
    if (result != 1 || !FD_ISSET(value, &readable)) return 0;
    return recv(value, &marker, 1, 0) == 1 && marker == expected;
}

static int wait_once(pst_connection *connection, PST_RESULT *failure,
                     const char *loop, int step)
{
    PST_WAIT_RESULT wait_result;
    PST_RESULT result;
    pst_u32 interest;
    DWORD start;
    interest = 0;
    result = pst_connection_get_interest(connection, &interest);
    timeline("LOOP=%s STEP=%d BEFORE_WAIT STATE=ACTIVE INTEREST_RESULT=%ld INTEREST=0x%08lx",
        loop, step, (long)result, (unsigned long)interest);
    if (step == 0)
        console_marker("BEFORE_WAIT LOOP=%s STEP=%d INTEREST=0x%08lx",
            loop, step, (unsigned long)interest);
    memset(&wait_result, 0, sizeof(wait_result));
    start = GetTickCount();
    result = pst_connection_wait(connection, WAIT_MS, &wait_result);
    timeline("LOOP=%s STEP=%d AFTER_WAIT WAIT_RESULT=%ld READY=0x%08lx TIMED_OUT=%lu DURATION_MS=%lu",
        loop, step, (long)result, (unsigned long)wait_result.ready_interest,
        (unsigned long)wait_result.timed_out,
        (unsigned long)(GetTickCount() - start));
    if (step == 0 || result != PST_RESULT_OK || wait_result.timed_out)
        console_marker("AFTER_WAIT LOOP=%s STEP=%d RESULT=%ld READY=0x%08lx TIMED_OUT=%lu DURATION_MS=%lu",
            loop, step, (long)result,
            (unsigned long)wait_result.ready_interest,
            (unsigned long)wait_result.timed_out,
            (unsigned long)(GetTickCount() - start));
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
    pst_peer_info *peer;
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
    peer = NULL;
    if (pst_connection_get_peer_info(connection, &peer) !=
        PST_RESULT_INVALID_STATE || peer != NULL) return 0;
    return 1;
}

int main(int argc, char **argv)
{
    WSADATA winsock;
    SOCKET native_socket;
    SOCKET control_socket;
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
    int winsock_started;
    int exit_code;
    int setup_failed;
    int native_error;
    int control_ready;
    int abort_confirmed;
    int shutdown_calls;
    const char *mode;
    const char *client_log_path;
    DWORD loop_start;
    DWORD loop_end;
    DWORD control_elapsed;

    if (argc != 11) {
        fprintf(stderr, "usage: host port hostname ca.der client.der key.pk8 tls alpn mode log-level\n");
        return 2;
    }
    g_epoch = GetTickCount();
    pst_backend_nss_trace_set_epoch((pst_u32)g_epoch);
    mode = argv[9];
    client_log_path = getenv("PST_FAILURE_CLIENT_LOG");
    if (client_log_path == NULL || client_log_path[0] == '\0')
        client_log_path = "failure-client.log";
    errno = 0;
    g_client_log = fopen(client_log_path, "w");
    if (g_client_log == NULL)
        console_marker("CLIENT_LOG_OPEN=0 FILE=%s ERRNO=%d",
            client_log_path, errno);
    else
        console_marker("CLIENT_LOG_OPEN=1 FILE=%s ERRNO=0",
            client_log_path);
    timeline("MODE=%s PROCESS_EPOCH=%lu", mode, (unsigned long)g_epoch);
    timeline("BOUNDS HANDSHAKE_MAX_STEPS=%d HANDSHAKE_WAIT_MS=%lu READ_MAX_STEPS=%d READ_WAIT_MS=%lu SHUTDOWN_MAX_STEPS=%d SHUTDOWN_WAIT_MS=%lu",
        MAX_STEPS, (unsigned long)WAIT_MS, MAX_STEPS, (unsigned long)WAIT_MS,
        MAX_STEPS, (unsigned long)WAIT_MS);
    timeline("ENV PST_FAILURE_CLIENT_LOG_PRESENT=%d PST_NSS_TRACE_FILE_PRESENT=%d PST_NSS_MODULE_FILE_PRESENT=%d",
        getenv("PST_FAILURE_CLIENT_LOG") != NULL,
        getenv("PST_NSS_TRACE_FILE") != NULL,
        getenv("PST_NSS_MODULE_FILE") != NULL);
    timeline("ENV_SAFE CLIENT=%d BACKEND=%d MODULES=%d",
        getenv("PST_FAILURE_CLIENT_LOG") != NULL &&
            strcmp(getenv("PST_FAILURE_CLIENT_LOG"), "failure-client.log") == 0,
        getenv("PST_NSS_TRACE_FILE") != NULL &&
            strcmp(getenv("PST_NSS_TRACE_FILE"), "failure-backend.log") == 0,
        getenv("PST_NSS_MODULE_FILE") != NULL &&
            strcmp(getenv("PST_NSS_MODULE_FILE"), "failure-modules.log") == 0);

    ca_data = NULL;
    cert_data = NULL;
    key_data = NULL;
    ca_size = cert_size = key_size = 0;
    trust = NULL;
    credentials = NULL;
    config = NULL;
    runtime = NULL;
    transport = NULL;
    connection = NULL;
    native_socket = INVALID_SOCKET;
    control_socket = INVALID_SOCKET;
    winsock_started = 0;
    exit_code = 20;
    setup_failed = 0;
    native_error = 0;
    control_ready = 0;
    abort_confirmed = 0;
    shutdown_calls = 0;
    control_elapsed = 0;
    accepted = 0;
    established = 0;
    terminal = 0;
    content_match = 0;
    ok = 0;
    step = 0;
    final_result = PST_RESULT_OK;
    final_close = PST_CLOSE_NONE;
    total_read = 0;
    total_written = 0;
    memset(&log, 0, sizeof(log));

    timeline("FIXTURE_LOAD_BEGIN");
    ca_data = load_file(argv[4], &ca_size);
    cert_data = load_file(argv[5], &cert_size);
    key_data = load_file(argv[6], &key_size);
    result = ca_data != NULL && cert_data != NULL && key_data != NULL ?
        PST_RESULT_OK : PST_RESULT_RESOURCE_FAILURE;
    timeline("FIXTURE_LOAD_END result=%ld", (long)result);
    if (result != PST_RESULT_OK) {
        timeline("SETUP_FAIL stage=FIXTURE_LOAD result=%ld", (long)result);
        console_marker("SETUP_FAIL stage=FIXTURE_LOAD result=%ld", (long)result);
        exit_code = 3; setup_failed = 1; goto cleanup;
    }

    memset(&trust_source, 0, sizeof(trust_source));
    trust_source.struct_size = sizeof(trust_source);
    trust_source.api_version = PST_API_VERSION;
    trust_source.kind = PST_TRUST_SOURCE_CUSTOM_CA_DER;
    trust_source.data = ca_data;
    trust_source.data_size = ca_size;
    timeline("TRUST_CREATE_BEGIN");
    result = pst_trust_create(&trust_source, &trust);
    timeline("TRUST_CREATE_END result=%ld", (long)result);
    if (result != PST_RESULT_OK) {
        timeline("SETUP_FAIL stage=TRUST_CREATE result=%ld", (long)result);
        console_marker("SETUP_FAIL stage=TRUST_CREATE result=%ld", (long)result);
        exit_code = 4; setup_failed = 1; goto cleanup;
    }

    memset(&credential_source, 0, sizeof(credential_source));
    credential_source.struct_size = sizeof(credential_source);
    credential_source.api_version = PST_API_VERSION;
    credential_source.kind = PST_CREDENTIAL_SOURCE_CERT_DER_PKCS8_DER;
    credential_source.certificate_der = cert_data;
    credential_source.certificate_der_size = cert_size;
    credential_source.private_key_der = key_data;
    credential_source.private_key_der_size = key_size;
    timeline("CREDENTIAL_CREATE_BEGIN");
    result = pst_credentials_create(&credential_source, &credentials);
    timeline("CREDENTIAL_CREATE_END result=%ld", (long)result);
    if (result != PST_RESULT_OK) {
        timeline("SETUP_FAIL stage=CREDENTIAL_CREATE result=%ld", (long)result);
        console_marker("SETUP_FAIL stage=CREDENTIAL_CREATE result=%ld", (long)result);
        exit_code = 5; setup_failed = 1; goto cleanup;
    }
    memset(key_data, 0, key_size);
    free(key_data);
    key_data = NULL;

    timeline("CONFIG_CREATE_BEGIN");
    result = pst_config_create(&config);
    timeline("CONFIG_CREATE_END result=%ld", (long)result);
    if (result != PST_RESULT_OK) {
        timeline("SETUP_FAIL stage=CONFIG_CREATE result=%ld", (long)result);
        console_marker("SETUP_FAIL stage=CONFIG_CREATE result=%ld", (long)result);
        exit_code = 6; setup_failed = 1; goto cleanup;
    }

    memset(&identity, 0, sizeof(identity));
    identity.struct_size = sizeof(identity);
    identity.api_version = PST_API_VERSION;
    identity.credentials = credentials;
    identity.trust = trust;
    identity.expected_hostname = argv[3];
    identity.expected_hostname_size = strlen(argv[3]);
    identity.require_peer_authentication = 1;
    identity.require_client_authentication = 1;
    timeline("IDENTITY_CONFIG_BEGIN");
    result = pst_config_set_identity(config, &identity);
    timeline("IDENTITY_CONFIG_END result=%ld", (long)result);
    if (result != PST_RESULT_OK) {
        timeline("SETUP_FAIL stage=IDENTITY_CONFIG result=%ld", (long)result);
        console_marker("SETUP_FAIL stage=IDENTITY_CONFIG result=%ld", (long)result);
        exit_code = 7; setup_failed = 1; goto cleanup;
    }

    memset(&policy, 0, sizeof(policy));
    policy.struct_size = sizeof(policy);
    policy.api_version = PST_API_VERSION;
    policy.minimum_version = (pst_u32)atoi(argv[7]);
    policy.maximum_version = policy.minimum_version;
    timeline("ALPN_CONFIG_BEGIN");
    protocol.data = (const pst_u8 *)argv[8];
    protocol.size = strlen(argv[8]);
    policy.alpn_protocols = &protocol;
    policy.alpn_protocol_count = 1;
    policy.alpn_requirement = PST_FEATURE_REQUIRED;
    policy.early_data = PST_FEATURE_DISABLED;
    timeline("ALPN_CONFIG_END result=%ld", (long)PST_RESULT_OK);
    timeline("TLS_POLICY_BEGIN");
    result = pst_config_set_tls_policy(config, &policy);
    if (result == PST_RESULT_OK) result = pst_config_freeze(config);
    timeline("TLS_POLICY_END result=%ld", (long)result);
    if (result != PST_RESULT_OK) {
        timeline("SETUP_FAIL stage=TLS_POLICY result=%ld", (long)result);
        console_marker("SETUP_FAIL stage=TLS_POLICY result=%ld", (long)result);
        exit_code = 8; setup_failed = 1; goto cleanup;
    }

    timeline("BACKEND_REGISTER_BEGIN");
    result = pst_win32_register_retrozilla_nss();
    timeline("BACKEND_REGISTER_END result=%ld", (long)result);
    if (result != PST_RESULT_OK) {
        timeline("SETUP_FAIL stage=BACKEND_REGISTER result=%ld", (long)result);
        console_marker("SETUP_FAIL stage=BACKEND_REGISTER result=%ld", (long)result);
        exit_code = 9; setup_failed = 1; goto cleanup;
    }

    memset(&runtime_options, 0, sizeof(runtime_options));
    runtime_options.struct_size = sizeof(runtime_options);
    runtime_options.api_version = PST_API_VERSION;
    runtime_options.selection = PST_BACKEND_SELECTION_EXACT;
    runtime_options.exact_backend_id = "retrozilla-nss";
    result = pst_log_config_init(&log_config);
    if (result != PST_RESULT_OK) {
        timeline("SETUP_FAIL stage=LOG_CONFIG result=%ld", (long)result);
        console_marker("SETUP_FAIL stage=LOG_CONFIG result=%ld", (long)result);
        exit_code = 10; setup_failed = 1; goto cleanup;
    }
    log_config.level = (pst_u32)atoi(argv[10]);
    log_config.callback = capture_log;
    log_config.user_context = &log;
    timeline("RUNTIME_CREATE_BEGIN");
    console_marker("RUNTIME_CREATE_BEGIN");
    result = pst_runtime_create_with_logging(&runtime_options, &log_config,
        &runtime, NULL);
    timeline("RUNTIME_CREATE_END result=%ld", (long)result);
    console_marker("RUNTIME_CREATE_END result=%ld", (long)result);
    if (result != PST_RESULT_OK) {
        timeline("SETUP_FAIL stage=RUNTIME_CREATE result=%ld", (long)result);
        console_marker("SETUP_FAIL stage=RUNTIME_CREATE result=%ld", (long)result);
        exit_code = 11; setup_failed = 1; goto cleanup;
    }

    timeline("WINSOCK_STARTUP_BEGIN");
    native_error = WSAStartup(MAKEWORD(2, 0), &winsock);
    result = native_error == 0 ? PST_RESULT_OK : PST_RESULT_TRANSPORT_FAILURE;
    timeline("WINSOCK_STARTUP_END result=%ld native_error=%d",
        (long)result, native_error);
    if (result != PST_RESULT_OK) {
        timeline("SETUP_FAIL stage=WINSOCK_STARTUP result=%ld", (long)result);
        console_marker("SETUP_FAIL stage=WINSOCK_STARTUP result=%ld", (long)result);
        exit_code = 12; setup_failed = 1; goto cleanup;
    }
    winsock_started = 1;

    timeline("SOCKET_CREATE_BEGIN");
    native_socket = socket4_create(&native_error);
    result = native_socket != INVALID_SOCKET ?
        PST_RESULT_OK : PST_RESULT_TRANSPORT_FAILURE;
    timeline("SOCKET_CREATE_END result=%ld native_error=%d",
        (long)result, native_error);
    if (result != PST_RESULT_OK) {
        timeline("SETUP_FAIL stage=SOCKET_CREATE result=%ld", (long)result);
        console_marker("SETUP_FAIL stage=SOCKET_CREATE result=%ld", (long)result);
        exit_code = 12; setup_failed = 1; goto cleanup;
    }

    timeline("CONNECT_BEGIN");
    console_marker("CONNECT_BEGIN");
    if (!connect4(native_socket, argv[1], (unsigned short)atoi(argv[2]),
        &native_error)) result = PST_RESULT_TRANSPORT_FAILURE;
    else result = PST_RESULT_OK;
    timeline("CONNECT_END result=%ld native_error=%d",
        (long)result, native_error);
    console_marker("CONNECT_END result=%ld native_error=%d",
        (long)result, native_error);
    if (result != PST_RESULT_OK) {
        timeline("SETUP_FAIL stage=CONNECT result=%ld", (long)result);
        console_marker("SETUP_FAIL stage=CONNECT result=%ld", (long)result);
        exit_code = 12; setup_failed = 1; goto cleanup;
    }

    timeline("TRANSPORT_CREATE_BEGIN");
    result = pst_win32_socket_transport_create((pst_size)native_socket,
        &transport);
    timeline("TRANSPORT_CREATE_END result=%ld", (long)result);
    if (result != PST_RESULT_OK) {
        timeline("SETUP_FAIL stage=TRANSPORT_CREATE result=%ld", (long)result);
        console_marker("SETUP_FAIL stage=TRANSPORT_CREATE result=%ld", (long)result);
        exit_code = 13; setup_failed = 1; goto cleanup;
    }
    native_socket = INVALID_SOCKET;

    timeline("CONNECTION_CREATE_BEGIN");
    console_marker("CONNECTION_CREATE_BEGIN");
    result = pst_connection_create(runtime, config, &connection);
    timeline("CONNECTION_CREATE_END result=%ld", (long)result);
    console_marker("CONNECTION_CREATE_END result=%ld", (long)result);
    if (result != PST_RESULT_OK) {
        timeline("SETUP_FAIL stage=CONNECTION_CREATE result=%ld", (long)result);
        console_marker("SETUP_FAIL stage=CONNECTION_CREATE result=%ld", (long)result);
        exit_code = 14; setup_failed = 1; goto cleanup;
    }

    timeline("ATTACH_BEGIN");
    console_marker("ATTACH_BEGIN");
    accepted = 0;
    result = pst_connection_attach(connection, transport,
        PST_OWNERSHIP_TRANSFERRED, &accepted);
    timeline("ATTACH_END result=%ld ownership_accepted=%lu",
        (long)result, (unsigned long)accepted);
    console_marker("ATTACH_END result=%ld ownership_accepted=%lu",
        (long)result, (unsigned long)accepted);
    if (accepted == 1UL) transport = NULL;
    if (result != PST_RESULT_OK || accepted != 1UL) {
        timeline("SETUP_FAIL stage=ATTACH result=%ld", (long)result);
        console_marker("SETUP_FAIL stage=ATTACH result=%ld", (long)result);
        exit_code = 15; setup_failed = 1; goto cleanup;
    }

    timeline("CONNECTION_ATTACHED OWNERSHIP_ACCEPTED=%lu HANDSHAKE_START",
        (unsigned long)accepted);
    console_marker("HANDSHAKE_BEGIN MODE=%s", mode);
    established = 0;
    final_result = PST_RESULT_OK;
    final_close = PST_CLOSE_NONE;
    loop_start = GetTickCount();
    for (step = 0; step < MAX_STEPS; ++step) {
        timeline("LOOP=HANDSHAKE STEP=%d BEFORE_HANDSHAKE STATE=HANDSHAKING", step);
        result = pst_connection_handshake(connection, &operation, &error);
        timeline("LOOP=HANDSHAKE STEP=%d AFTER_HANDSHAKE RESULT=%ld OPERATION=%lu ERROR=%ld",
            step, (long)result, (unsigned long)operation, (long)error);
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
        if (!wait_once(connection, &final_result, "HANDSHAKE", step)) break;
    }
    loop_end = GetTickCount();
    timeline("LOOP=HANDSHAKE END STEPS=%d ELAPSED_MS=%lu ESTABLISHED=%d FINAL=%ld",
        step, (unsigned long)(loop_end - loop_start), established,
        (long)final_result);
    console_marker("HANDSHAKE_END STEPS=%d ELAPSED_MS=%lu ESTABLISHED=%d FINAL=%ld",
        step, (unsigned long)(loop_end - loop_start), established,
        (long)final_result);

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
        loop_start = GetTickCount();
        timeline("LOOP=READ START STATE=ESTABLISHED TOTAL_READ=%lu",
            (unsigned long)total_read);
        console_marker("READ_LOOP_BEGIN TOTAL_READ=%lu",
            (unsigned long)total_read);
        for (step = 0; step < MAX_STEPS; ++step) {
            timeline("LOOP=READ STEP=%d BEFORE_READ STATE=ESTABLISHED TOTAL_READ=%lu",
                step, (unsigned long)total_read);
            if (step == 0)
                console_marker("BEFORE_READ STEP=%d TOTAL_READ=%lu",
                    step, (unsigned long)total_read);
            memset(&io, 0, sizeof(io));
            result = pst_connection_read(connection, received + total_read,
                sizeof(received) - total_read, &io);
            total_read += io.bytes_transferred;
            timeline("LOOP=READ STEP=%d AFTER_READ READ_RESULT=%ld OPERATION=%lu BYTES=%lu TOTAL_READ=%lu CLOSE_KIND=%lu ERROR=%ld",
                step, (long)result, (unsigned long)io.operation,
                (unsigned long)io.bytes_transferred, (unsigned long)total_read,
                (unsigned long)io.close_kind, (long)io.error);
            if (step == 0 || result != PST_RESULT_OK ||
                io.operation == PST_OPERATION_CLOSED ||
                io.operation == PST_OPERATION_FAILED)
                console_marker("AFTER_READ STEP=%d RESULT=%ld OPERATION=%lu BYTES=%lu TOTAL_READ=%lu CLOSE_KIND=%lu",
                    step, (long)result, (unsigned long)io.operation,
                    (unsigned long)io.bytes_transferred,
                    (unsigned long)total_read, (unsigned long)io.close_kind);
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
            if (!wait_once(connection, &final_result, "READ", step)) break;
        }
        loop_end = GetTickCount();
        timeline("LOOP=READ END STEPS=%d ELAPSED_MS=%lu TOTAL_READ=%lu FINAL=%ld CLOSE_KIND=%lu",
            step, (unsigned long)(loop_end - loop_start),
            (unsigned long)total_read, (long)final_result,
            (unsigned long)final_close);
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
                !wait_once(connection, &final_result, "WRITE", step)) break;
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
                if (!wait_once(connection, &final_result, "WRITE", step)) break;
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
                !wait_once(connection, &final_result, "WRITE", step)) break;
        }
        if (final_result == PST_RESULT_OK) {
            control_socket = socket4_create(&native_error);
            if (control_socket == INVALID_SOCKET ||
                !connect4(control_socket, argv[1],
                    (unsigned short)atoi(argv[2]), &native_error) ||
                send(control_socket, "C", 1, 0) != 1) {
                timeline("CONTROL_CONNECT_FAIL native_error=%d", native_error);
                final_result = PST_RESULT_TRANSPORT_FAILURE;
            } else {
                control_ready = control_receive(control_socket, 'R',
                    &control_elapsed);
                timeline("CONTROL_READY=%d ELAPSED_MS=%lu BOUND_MS=%lu",
                    control_ready, (unsigned long)control_elapsed,
                    (unsigned long)CONTROL_WAIT_MS);
                if (!control_ready)
                    final_result = PST_RESULT_TRANSPORT_FAILURE;
            }
        }
        if (final_result == PST_RESULT_OK) {
            loop_start = GetTickCount();
            timeline("LOOP=SHUTDOWN START STATE=ESTABLISHED");
            console_marker("SHUTDOWN_BEGIN STATE=ESTABLISHED");
            for (step = 0; step < MAX_STEPS; ++step) {
                shutdown_calls = step + 1;
                timeline("LOOP=SHUTDOWN STEP=%d BEFORE_SHUTDOWN STATE=SHUTTING_DOWN",
                    step);
                result = pst_connection_shutdown(connection, &operation, &error);
                timeline("LOOP=SHUTDOWN STEP=%d AFTER_SHUTDOWN RESULT=%ld OPERATION=%lu ERROR=%ld",
                    step, (long)result, (unsigned long)operation, (long)error);
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
                if (!wait_once(connection, &final_result, "SHUTDOWN", step)) break;
            }
            loop_end = GetTickCount();
            timeline("LOOP=SHUTDOWN END STEPS=%d ELAPSED_MS=%lu FINAL=%ld",
                shutdown_calls, (unsigned long)(loop_end - loop_start),
                (long)final_result);
            console_marker("SHUTDOWN_END STEPS=%d ELAPSED_MS=%lu FINAL=%ld",
                shutdown_calls, (unsigned long)(loop_end - loop_start),
                (long)final_result);
            if (final_result == PST_RESULT_CLOSED) {
                abort_confirmed = control_receive(control_socket, 'A',
                    &control_elapsed);
                timeline("CONTROL_ABORT_CONFIRMED=%d ELAPSED_MS=%lu BOUND_MS=%lu",
                    abort_confirmed, (unsigned long)control_elapsed,
                    (unsigned long)CONTROL_WAIT_MS);
            }
        }
    }
    memset(&diagnostic, 0, sizeof(diagnostic));
    diagnostic.struct_size = sizeof(diagnostic);
    diagnostic.api_version = PST_API_VERSION;
    pst_connection_copy_diagnostic(connection, &diagnostic);
    timeline("TERMINAL_CHECK BEFORE STATE=%s",
        established ? "ESTABLISHED_OR_TERMINAL" : "FAILED");
    terminal = terminal_rejects_operations(connection);
    timeline("TERMINAL_CHECK AFTER TERMINAL=%d", terminal);
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
    } else if (mode_write) {
        ok = ok && total_written <= sizeof(g_client_write) - 1 &&
            final_result != PST_RESULT_OK;
    } else if (mode_shutdown) {
        ok = ok && total_written == sizeof(g_client_write) - 1 &&
            control_ready && abort_confirmed && shutdown_calls == 1 &&
            final_result == PST_RESULT_CLOSED &&
            final_close == PST_CLOSE_CLEAN && !diagnostic.valid;
    }
    if (log_config.level == PST_LOG_LEVEL_OFF)
        ok = ok && log.total == 0UL;
    if (log_config.level == PST_LOG_LEVEL_ERROR && final_result != PST_RESULT_CLOSED)
        ok = ok && log.errors == 1UL && log.warnings == 0UL;

    if (mode_shutdown)
        timeline("SHUTDOWN_PROOF MAX_STEPS=%d WAIT_MS=%lu CALLS=%d ELAPSED_MS=%lu CONTROL_READY=%d ABORT_CONFIRMED=%d CONTROL_BOUND_MS=%lu",
            MAX_STEPS, (unsigned long)WAIT_MS, shutdown_calls,
            (unsigned long)(loop_end - loop_start), control_ready,
            abort_confirmed, (unsigned long)CONTROL_WAIT_MS);
    timeline("MODE=%s ESTABLISHED=%d FINAL=%ld FINAL_STATE=%s CLOSE_KIND=%lu READ=%lu WRITE=%lu CONTENT_MATCH=%d TERMINAL=%d DIAG_VALID=%lu DIAG_RESULT=%ld DIAG_OPERATION=%lu LOG_TOTAL=%lu LOG_ERROR=%lu LOG_WARN=%lu STEPS=%d PASS=%d",
        mode, established, (long)final_result,
        final_result == PST_RESULT_CLOSED ? "CLOSED" : "FAILED",
        (unsigned long)final_close, (unsigned long)total_read,
        (unsigned long)total_written, content_match, terminal,
        (unsigned long)diagnostic.valid, (long)diagnostic.normalized_result,
        (unsigned long)diagnostic.operation, (unsigned long)log.total,
        (unsigned long)log.errors, (unsigned long)log.warnings, step, ok);

    exit_code = ok ? 0 : 20;

cleanup:
    timeline("CLEANUP_BEGIN SETUP_FAILED=%d", setup_failed);
    console_marker("CLEANUP_BEGIN");
    if (connection != NULL) {
        timeline("CLEANUP BEFORE_CONNECTION_RELEASE");
        pst_connection_release(connection);
        timeline("CLEANUP AFTER_CONNECTION_RELEASE");
    }
    if (transport != NULL) {
        timeline("CLEANUP BEFORE_TRANSPORT_RELEASE");
        pst_transport_release(transport);
        timeline("CLEANUP AFTER_TRANSPORT_RELEASE");
    }
    if (native_socket != INVALID_SOCKET) {
        timeline("CLEANUP BEFORE_SOCKET_CLOSE");
        closesocket(native_socket);
        timeline("CLEANUP AFTER_SOCKET_CLOSE");
    }
    if (control_socket != INVALID_SOCKET) {
        timeline("CLEANUP BEFORE_CONTROL_SOCKET_CLOSE");
        closesocket(control_socket);
        control_socket = INVALID_SOCKET;
        timeline("CLEANUP AFTER_CONTROL_SOCKET_CLOSE");
    }
    if (config != NULL) {
        timeline("CLEANUP BEFORE_CONFIG_RELEASE");
        pst_config_release(config);
        timeline("CLEANUP AFTER_CONFIG_RELEASE");
    }
    if (credentials != NULL) {
        timeline("CLEANUP BEFORE_CREDENTIALS_RELEASE");
        pst_credentials_release(credentials);
        timeline("CLEANUP AFTER_CREDENTIALS_RELEASE");
    }
    if (trust != NULL) {
        timeline("CLEANUP BEFORE_TRUST_RELEASE");
        pst_trust_release(trust);
        timeline("CLEANUP AFTER_TRUST_RELEASE");
    }
    if (runtime != NULL) {
        timeline("CLEANUP BEFORE_RUNTIME_RELEASE");
        pst_runtime_release(runtime);
        timeline("CLEANUP AFTER_RUNTIME_RELEASE");
    }
    if (winsock_started) {
        timeline("CLEANUP BEFORE_WINSOCK_CLEANUP");
        WSACleanup();
        timeline("CLEANUP AFTER_WINSOCK_CLEANUP");
    }
    free(ca_data);
    free(cert_data);
    if (key_data != NULL) {
        memset(key_data, 0, key_size);
        free(key_data);
    }
    timeline("CLEANUP_END");
    console_marker("CLEANUP_END");
    timeline("TOTAL_ELAPSED_MS=%lu EXIT_CODE=%d",
        (unsigned long)(GetTickCount() - g_epoch), exit_code);
    if (g_client_log != NULL) {
        fclose(g_client_log);
        g_client_log = NULL;
    }
    console_marker("TOTAL_ELAPSED_MS=%lu EXIT_CODE=%d",
        (unsigned long)(GetTickCount() - g_epoch), exit_code);
    return exit_code;
}
