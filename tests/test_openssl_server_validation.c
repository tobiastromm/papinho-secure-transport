/* SPDX-License-Identifier: MPL-2.0 */
#include "papinho_secure_transport.h"
#include "papinho_secure_transport_win32.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    printf("test_openssl_server_validation: FAIL line %d\n", __LINE__); \
    return 20; } } while (0)

static unsigned char *load_file(const char *path, pst_size *size)
{
    FILE *file;
    long length;
    unsigned char *data;
    *size = 0;
    file = fopen(path, "rb");
    if (file == NULL) return NULL;
    if (fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) <= 0 ||
        fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }
    data = (unsigned char *)malloc((size_t)length);
    if (data == NULL || fread(data, 1, (size_t)length, file) !=
        (size_t)length) {
        free(data);
        fclose(file);
        return NULL;
    }
    fclose(file);
    *size = (pst_size)length;
    return data;
}

static void config_init(PST_CONNECTION_CONFIG *config,
                        pst_credentials *credentials)
{
    memset(config, 0, sizeof(*config));
    config->struct_size = sizeof(*config);
    config->api_version = PST_API_VERSION;
    config->role = PST_CONNECTION_ROLE_SERVER;
    config->provider_selection.struct_size =
        sizeof(config->provider_selection);
    config->provider_selection.api_version = PST_API_VERSION;
    config->provider_selection.mode = PST_BACKEND_SELECTION_EXACT;
    config->provider_selection.exact_provider_id = "openssl";
    config->local_identity.struct_size = sizeof(config->local_identity);
    config->local_identity.api_version = PST_API_VERSION;
    config->local_identity.credentials = credentials;
    config->peer_authentication.struct_size =
        sizeof(config->peer_authentication);
    config->peer_authentication.api_version = PST_API_VERSION;
    config->peer_authentication.certificate_mode =
        PST_PEER_CERTIFICATE_DISABLED;
    config->tls.struct_size = sizeof(config->tls);
    config->tls.api_version = PST_API_VERSION;
    config->tls.minimum_version = PST_TLS_VERSION_1_2;
    config->tls.maximum_version = PST_TLS_VERSION_1_3;
    config->alpn.struct_size = sizeof(config->alpn);
    config->alpn.api_version = PST_API_VERSION;
    config->alpn.mode = PST_FEATURE_DISABLED;
}

static pst_credentials *make_credentials(const unsigned char *cert,
    pst_size cert_size, const unsigned char *chain, pst_size chain_size,
    const unsigned char *key, pst_size key_size)
{
    PST_CREDENTIAL_SOURCE source;
    PST_DER_ITEM certificates[2];
    pst_credentials *credentials = NULL;
    memset(&source, 0, sizeof(source));
    source.struct_size = sizeof(source);
    source.api_version = PST_API_VERSION;
    source.kind = PST_CREDENTIAL_SOURCE_CERT_CHAIN_DER_PKCS8_DER;
    certificates[0].data = cert;
    certificates[0].size = cert_size;
    certificates[1].data = chain;
    certificates[1].size = chain_size;
    source.certificate_chain = certificates;
    source.certificate_count = chain == NULL ? 1 : 2;
    source.private_key_der = key;
    source.private_key_der_size = key_size;
    if (pst_credentials_create(&source, &credentials) != PST_RESULT_OK)
        return NULL;
    return credentials;
}

static int expect_failure(pst_runtime *runtime, pst_credentials *credentials,
                          PST_RESULT expected)
{
    PST_CONNECTION_CONFIG config;
    PST_DIAGNOSTIC_INFO diagnostic;
    pst_connection *connection = NULL;
    PST_RESULT result;
    config_init(&config, credentials);
    memset(&diagnostic, 0, sizeof(diagnostic));
    diagnostic.struct_size = sizeof(diagnostic);
    diagnostic.api_version = PST_API_VERSION;
    result = pst_connection_create_ex(runtime, &config, &connection,
                                      &diagnostic);
    printf("IDENTITY_CASE EXPECTED=%ld RESULT=%ld CONNECTION=%d "
           "DIAG_VALID=%lu DIAG_RESULT=%ld BACKEND=%s\n",
           (long)expected, (long)result, connection != NULL,
           (unsigned long)diagnostic.valid,
           (long)diagnostic.normalized_result, diagnostic.backend_id);
    if (connection != NULL) pst_connection_release(connection);
    return result == expected && connection == NULL && diagnostic.valid &&
        diagnostic.normalized_result == expected &&
        (expected == PST_RESULT_POLICY_VIOLATION ?
         diagnostic.backend_id[0] == '\0' :
         strcmp(diagnostic.backend_id, "openssl") == 0);
}

static pst_trust *make_trust(pst_u32 kind, const unsigned char *anchor,
                             pst_size anchor_size)
{
    PST_TRUST_SOURCE source;
    PST_DER_ITEM item;
    pst_trust *trust = NULL;
    memset(&source, 0, sizeof(source));
    source.struct_size = sizeof(source);
    source.api_version = PST_API_VERSION;
    source.kind = kind;
    if (kind == PST_TRUST_SOURCE_CUSTOM_CA_DER) {
        item.data = anchor;
        item.size = anchor_size;
        source.anchors = &item;
        source.anchor_count = 1;
    }
    if (pst_trust_create(&source, &trust) != PST_RESULT_OK) return NULL;
    return trust;
}

static int expect_role_result(pst_runtime *runtime,
    pst_credentials *credentials, pst_trust *trust, pst_u32 mode,
    const char *const *ordered, pst_size ordered_count, PST_RESULT expected,
    int expect_backend)
{
    PST_CONNECTION_CONFIG config;
    PST_DIAGNOSTIC_INFO diagnostic;
    pst_connection *connection = NULL;
    PST_RESULT result;
    config_init(&config, credentials);
    config.provider_selection.mode = mode;
    config.provider_selection.exact_provider_id =
        mode == PST_BACKEND_SELECTION_EXACT ? "openssl" : NULL;
    config.provider_selection.ordered_provider_ids = ordered;
    config.provider_selection.ordered_provider_count = ordered_count;
    config.peer_authentication.certificate_mode =
        PST_PEER_CERTIFICATE_REQUIRED;
    config.peer_authentication.trust = trust;
    memset(&diagnostic, 0, sizeof(diagnostic));
    diagnostic.struct_size = sizeof(diagnostic);
    diagnostic.api_version = PST_API_VERSION;
    result = pst_connection_create_ex(runtime, &config, &connection,
                                      &diagnostic);
    printf("ROLE_SCOPE MODE=%lu RESULT=%ld CONNECTION=%d BACKEND=%s\n",
           (unsigned long)mode, (long)result, connection != NULL,
           diagnostic.backend_id);
    if (connection != NULL) pst_connection_release(connection);
    return result == expected && ((connection != NULL) == expect_backend) &&
        (expect_backend || (diagnostic.valid &&
         diagnostic.normalized_result == expected &&
         diagnostic.backend_id[0] == '\0'));
}

static int expect_server_peer_name_rejected(pst_runtime *runtime,
                                             pst_credentials *credentials,
                                             pst_trust *trust)
{
    PST_CONNECTION_CONFIG config;
    PST_DIAGNOSTIC_INFO diagnostic;
    pst_connection *connection = NULL;
    PST_RESULT result;
    config_init(&config, credentials);
    config.peer_authentication.certificate_mode =
        PST_PEER_CERTIFICATE_REQUIRED;
    config.peer_authentication.trust = trust;
    config.peer_authentication.expected_peer_name = "client.example";
    config.peer_authentication.expected_peer_name_size = 14;
    memset(&diagnostic, 0, sizeof(diagnostic));
    diagnostic.struct_size = sizeof(diagnostic);
    diagnostic.api_version = PST_API_VERSION;
    result = pst_connection_create_ex(runtime, &config, &connection,
                                      &diagnostic);
    return result == PST_RESULT_POLICY_VIOLATION && connection == NULL &&
        diagnostic.valid && diagnostic.backend_id[0] == '\0';
}

int main(int argc, char **argv)
{
    unsigned char malformed[4] = { 0x30, 0x02, 0x01, 0x00 };
    unsigned char *leaf, *intermediate, *key, *wrong_key;
    pst_size leaf_size, intermediate_size, key_size, wrong_key_size;
    pst_credentials *credentials;
    pst_credentials *valid_credentials;
    pst_trust *system_trust, *custom_trust;
    pst_runtime *runtime = NULL;
    PST_RUNTIME_OPTIONS options;
    PST_PROVIDER_INFO provider_info;
    const char *ordered[1] = { "openssl" };
    if (argc != 5) {
        fprintf(stderr, "usage: server.der intermediate.der server.pk8 wrong.pk8\n");
        return 2;
    }
    leaf = load_file(argv[1], &leaf_size);
    intermediate = load_file(argv[2], &intermediate_size);
    key = load_file(argv[3], &key_size);
    wrong_key = load_file(argv[4], &wrong_key_size);
    CHECK(leaf != NULL && intermediate != NULL && key != NULL &&
          wrong_key != NULL);
    CHECK(pst_win32_register_builtin_providers() == PST_RESULT_OK);
    memset(&options, 0, sizeof(options));
    options.struct_size = sizeof(options);
    options.api_version = PST_API_VERSION;
    CHECK(pst_runtime_create(&options, &runtime) == PST_RESULT_OK);

    valid_credentials = make_credentials(leaf, leaf_size, intermediate,
                                         intermediate_size, key, key_size);
    system_trust = make_trust(PST_TRUST_SOURCE_SYSTEM, NULL, 0);
    custom_trust = make_trust(PST_TRUST_SOURCE_CUSTOM_CA_DER, intermediate,
                              intermediate_size);
    CHECK(valid_credentials != NULL && system_trust != NULL &&
          custom_trust != NULL);
    CHECK(expect_role_result(runtime, valid_credentials, system_trust,
          PST_BACKEND_SELECTION_EXACT, NULL, 0, PST_RESULT_OK, 1));
    CHECK(expect_role_result(runtime, valid_credentials, system_trust,
          PST_BACKEND_SELECTION_ORDERED, ordered, 1,
          PST_RESULT_OK, 1));
    CHECK(expect_role_result(runtime, valid_credentials, system_trust,
          PST_BACKEND_SELECTION_AUTOMATIC, NULL, 0,
          PST_RESULT_OK, 1));
    CHECK(expect_server_peer_name_rejected(runtime, valid_credentials,
                                           custom_trust));
    memset(&provider_info, 0, sizeof(provider_info));
    provider_info.struct_size = sizeof(provider_info);
    provider_info.api_version = PST_API_VERSION;
    CHECK(pst_runtime_get_provider_info(runtime, 0, &provider_info) ==
          PST_RESULT_OK && provider_info.initialized);
    CHECK(expect_role_result(runtime, valid_credentials, custom_trust,
          PST_BACKEND_SELECTION_EXACT, NULL, 0, PST_RESULT_OK, 1));
    pst_trust_release(system_trust);
    pst_trust_release(custom_trust);
    pst_credentials_release(valid_credentials);

    CHECK(expect_failure(runtime, NULL, PST_RESULT_POLICY_VIOLATION));
    credentials = make_credentials(malformed, sizeof(malformed), NULL, 0,
                                   key, key_size);
    CHECK(credentials != NULL);
    CHECK(expect_failure(runtime, credentials, PST_RESULT_AUTH_FAILURE));
    pst_credentials_release(credentials);
    credentials = make_credentials(leaf, leaf_size, NULL, 0, malformed,
                                   sizeof(malformed));
    CHECK(credentials != NULL);
    CHECK(expect_failure(runtime, credentials, PST_RESULT_AUTH_FAILURE));
    pst_credentials_release(credentials);
    credentials = make_credentials(leaf, leaf_size, intermediate,
                                   intermediate_size, wrong_key,
                                   wrong_key_size);
    CHECK(credentials != NULL);
    CHECK(expect_failure(runtime, credentials, PST_RESULT_AUTH_FAILURE));
    pst_credentials_release(credentials);
    credentials = make_credentials(leaf, leaf_size, malformed,
                                   sizeof(malformed), key, key_size);
    CHECK(credentials != NULL);
    CHECK(expect_failure(runtime, credentials, PST_RESULT_AUTH_FAILURE));
    pst_credentials_release(credentials);

    pst_runtime_release(runtime);
    free(leaf);
    free(intermediate);
    free(key);
    free(wrong_key);
    printf("OPENSSL_SERVER_KEY_MATCH_VALIDATION=PASS\n");
    printf("OPENSSL_ROLE_SCOPED_ELIGIBILITY=PASS EXACT=PASS ORDERED=PASS "
           "AUTOMATIC=PASS PRE_BINDING=PASS SERVER_SYSTEM_TRUST=ELIGIBLE "
           "SERVER_CUSTOM_TRUST=ELIGIBLE SERVER_PEER_NAME=POLICY_REJECTED\n");
    printf("OPENSSL_SERVER_IDENTITY_NEGATIVE_MATRIX=PASS "
           "MISSING=PASS MALFORMED_CERT=PASS MALFORMED_KEY=PASS "
           "KEY_MISMATCH=PASS MALFORMED_CHAIN=PASS\n");
    return 0;
}
