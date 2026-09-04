#include "papinho_secure_transport.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define ABI_ASSERT(name, expression) typedef char name[(expression) ? 1 : -1]

ABI_ASSERT(abi_result_4, sizeof(PST_RESULT) == 4);
ABI_ASSERT(abi_bool_u32_4, sizeof(pst_u32) == 4);
ABI_ASSERT(abi_diagnostic_56, sizeof(PST_DIAGNOSTIC_INFO) == 56);
ABI_ASSERT(abi_log_event_60, sizeof(PST_LOG_EVENT) == 60);
ABI_ASSERT(abi_version_32, sizeof(PST_VERSION_INFO) == 32);
ABI_ASSERT(abi_wait_8, sizeof(PST_WAIT_RESULT) == 8);
ABI_ASSERT(abi_diag_backend_offset_24, offsetof(PST_DIAGNOSTIC_INFO, backend_id) == 24);
ABI_ASSERT(abi_log_backend_offset_28, offsetof(PST_LOG_EVENT, backend_id) == 28);
ABI_ASSERT(abi_diag_result_offset_16, offsetof(PST_DIAGNOSTIC_INFO, normalized_result) == 16);
ABI_ASSERT(abi_log_result_offset_20, offsetof(PST_LOG_EVENT, normalized_result) == 20);
ABI_ASSERT(abi_version_library_patch_offset_28, offsetof(PST_VERSION_INFO, library_patch) == 28);
ABI_ASSERT(abi_peer_hash_offset_44, offsetof(PST_PEER_INFO_SUMMARY, certificate_sha256) == 44);
ABI_ASSERT(abi_wait_timeout_offset_4, offsetof(PST_WAIT_RESULT, timed_out) == 4);

#if defined(_WIN64)
ABI_ASSERT(abi_size_8, sizeof(pst_size) == 8);
ABI_ASSERT(abi_log_config_32, sizeof(PST_LOG_CONFIG) == 32);
ABI_ASSERT(abi_credential_48, sizeof(PST_CREDENTIAL_SOURCE) == 48);
ABI_ASSERT(abi_trust_32, sizeof(PST_TRUST_SOURCE) == 32);
ABI_ASSERT(abi_identity_48, sizeof(PST_IDENTITY_CONFIG) == 48);
ABI_ASSERT(abi_peer_summary_96, sizeof(PST_PEER_INFO_SUMMARY) == 96);
ABI_ASSERT(abi_runtime_options_48, sizeof(PST_RUNTIME_OPTIONS) == 48);
ABI_ASSERT(abi_runtime_info_24, sizeof(PST_RUNTIME_INFO) == 24);
ABI_ASSERT(abi_alpn_16, sizeof(PST_ALPN_PROTOCOL) == 16);
ABI_ASSERT(abi_tls_policy_48, sizeof(PST_TLS_POLICY) == 48);
ABI_ASSERT(abi_io_result_24, sizeof(PST_IO_RESULT) == 24);
ABI_ASSERT(abi_log_callback_offset_16, offsetof(PST_LOG_CONFIG, callback) == 16);
ABI_ASSERT(abi_credential_cert_offset_16, offsetof(PST_CREDENTIAL_SOURCE, certificate_der) == 16);
ABI_ASSERT(abi_runtime_exact_offset_16, offsetof(PST_RUNTIME_OPTIONS, exact_backend_id) == 16);
ABI_ASSERT(abi_log_context_offset_24, offsetof(PST_LOG_CONFIG, user_context) == 24);
ABI_ASSERT(abi_credential_cert_size_offset_24, offsetof(PST_CREDENTIAL_SOURCE, certificate_der_size) == 24);
ABI_ASSERT(abi_credential_key_offset_32, offsetof(PST_CREDENTIAL_SOURCE, private_key_der) == 32);
ABI_ASSERT(abi_credential_key_size_offset_40, offsetof(PST_CREDENTIAL_SOURCE, private_key_der_size) == 40);
ABI_ASSERT(abi_trust_data_offset_16, offsetof(PST_TRUST_SOURCE, data) == 16);
ABI_ASSERT(abi_trust_size_offset_24, offsetof(PST_TRUST_SOURCE, data_size) == 24);
ABI_ASSERT(abi_identity_credentials_offset_8, offsetof(PST_IDENTITY_CONFIG, credentials) == 8);
ABI_ASSERT(abi_identity_trust_offset_16, offsetof(PST_IDENTITY_CONFIG, trust) == 16);
ABI_ASSERT(abi_identity_hostname_offset_24, offsetof(PST_IDENTITY_CONFIG, expected_hostname) == 24);
ABI_ASSERT(abi_identity_hostname_size_offset_32, offsetof(PST_IDENTITY_CONFIG, expected_hostname_size) == 32);
ABI_ASSERT(abi_identity_peer_offset_40, offsetof(PST_IDENTITY_CONFIG, require_peer_authentication) == 40);
ABI_ASSERT(abi_peer_hash_size_offset_80, offsetof(PST_PEER_INFO_SUMMARY, certificate_sha256_size) == 80);
ABI_ASSERT(abi_peer_leaf_size_offset_88, offsetof(PST_PEER_INFO_SUMMARY, leaf_der_size) == 88);
ABI_ASSERT(abi_runtime_preferred_offset_24, offsetof(PST_RUNTIME_OPTIONS, preferred_backend_ids) == 24);
ABI_ASSERT(abi_runtime_count_offset_32, offsetof(PST_RUNTIME_OPTIONS, preferred_backend_count) == 32);
ABI_ASSERT(abi_runtime_caps_offset_40, offsetof(PST_RUNTIME_OPTIONS, required_capabilities) == 40);
ABI_ASSERT(abi_runtime_info_id_offset_8, offsetof(PST_RUNTIME_INFO, backend_id) == 8);
ABI_ASSERT(abi_runtime_info_caps_offset_16, offsetof(PST_RUNTIME_INFO, capabilities) == 16);
ABI_ASSERT(abi_alpn_size_offset_8, offsetof(PST_ALPN_PROTOCOL, size) == 8);
ABI_ASSERT(abi_tls_alpn_offset_16, offsetof(PST_TLS_POLICY, alpn_protocols) == 16);
ABI_ASSERT(abi_tls_alpn_count_offset_24, offsetof(PST_TLS_POLICY, alpn_protocol_count) == 24);
ABI_ASSERT(abi_tls_requirement_offset_32, offsetof(PST_TLS_POLICY, alpn_requirement) == 32);
ABI_ASSERT(abi_tls_graceful_offset_44, offsetof(PST_TLS_POLICY, require_graceful_shutdown) == 44);
ABI_ASSERT(abi_io_operation_offset_8, offsetof(PST_IO_RESULT, operation) == 8);
ABI_ASSERT(abi_io_error_offset_16, offsetof(PST_IO_RESULT, error) == 16);
#else
ABI_ASSERT(abi_size_4, sizeof(pst_size) == 4);
ABI_ASSERT(abi_log_config_20, sizeof(PST_LOG_CONFIG) == 20);
ABI_ASSERT(abi_credential_28, sizeof(PST_CREDENTIAL_SOURCE) == 28);
ABI_ASSERT(abi_trust_20, sizeof(PST_TRUST_SOURCE) == 20);
ABI_ASSERT(abi_identity_32, sizeof(PST_IDENTITY_CONFIG) == 32);
ABI_ASSERT(abi_peer_summary_84, sizeof(PST_PEER_INFO_SUMMARY) == 84);
ABI_ASSERT(abi_runtime_options_28, sizeof(PST_RUNTIME_OPTIONS) == 28);
ABI_ASSERT(abi_runtime_info_16, sizeof(PST_RUNTIME_INFO) == 16);
ABI_ASSERT(abi_alpn_8, sizeof(PST_ALPN_PROTOCOL) == 8);
ABI_ASSERT(abi_tls_policy_40, sizeof(PST_TLS_POLICY) == 40);
ABI_ASSERT(abi_io_result_16, sizeof(PST_IO_RESULT) == 16);
ABI_ASSERT(abi_log_callback_offset_12, offsetof(PST_LOG_CONFIG, callback) == 12);
ABI_ASSERT(abi_credential_cert_offset_12, offsetof(PST_CREDENTIAL_SOURCE, certificate_der) == 12);
ABI_ASSERT(abi_runtime_exact_offset_12, offsetof(PST_RUNTIME_OPTIONS, exact_backend_id) == 12);
ABI_ASSERT(abi_log_context_offset_16, offsetof(PST_LOG_CONFIG, user_context) == 16);
ABI_ASSERT(abi_credential_cert_size_offset_16, offsetof(PST_CREDENTIAL_SOURCE, certificate_der_size) == 16);
ABI_ASSERT(abi_credential_key_offset_20, offsetof(PST_CREDENTIAL_SOURCE, private_key_der) == 20);
ABI_ASSERT(abi_credential_key_size_offset_24, offsetof(PST_CREDENTIAL_SOURCE, private_key_der_size) == 24);
ABI_ASSERT(abi_trust_data_offset_12, offsetof(PST_TRUST_SOURCE, data) == 12);
ABI_ASSERT(abi_trust_size_offset_16, offsetof(PST_TRUST_SOURCE, data_size) == 16);
ABI_ASSERT(abi_identity_credentials_offset_8, offsetof(PST_IDENTITY_CONFIG, credentials) == 8);
ABI_ASSERT(abi_identity_trust_offset_12, offsetof(PST_IDENTITY_CONFIG, trust) == 12);
ABI_ASSERT(abi_identity_hostname_offset_16, offsetof(PST_IDENTITY_CONFIG, expected_hostname) == 16);
ABI_ASSERT(abi_identity_hostname_size_offset_20, offsetof(PST_IDENTITY_CONFIG, expected_hostname_size) == 20);
ABI_ASSERT(abi_identity_peer_offset_24, offsetof(PST_IDENTITY_CONFIG, require_peer_authentication) == 24);
ABI_ASSERT(abi_peer_hash_size_offset_76, offsetof(PST_PEER_INFO_SUMMARY, certificate_sha256_size) == 76);
ABI_ASSERT(abi_peer_leaf_size_offset_80, offsetof(PST_PEER_INFO_SUMMARY, leaf_der_size) == 80);
ABI_ASSERT(abi_runtime_preferred_offset_16, offsetof(PST_RUNTIME_OPTIONS, preferred_backend_ids) == 16);
ABI_ASSERT(abi_runtime_count_offset_20, offsetof(PST_RUNTIME_OPTIONS, preferred_backend_count) == 20);
ABI_ASSERT(abi_runtime_caps_offset_24, offsetof(PST_RUNTIME_OPTIONS, required_capabilities) == 24);
ABI_ASSERT(abi_runtime_info_id_offset_8, offsetof(PST_RUNTIME_INFO, backend_id) == 8);
ABI_ASSERT(abi_runtime_info_caps_offset_12, offsetof(PST_RUNTIME_INFO, capabilities) == 12);
ABI_ASSERT(abi_alpn_size_offset_4, offsetof(PST_ALPN_PROTOCOL, size) == 4);
ABI_ASSERT(abi_tls_alpn_offset_16, offsetof(PST_TLS_POLICY, alpn_protocols) == 16);
ABI_ASSERT(abi_tls_alpn_count_offset_20, offsetof(PST_TLS_POLICY, alpn_protocol_count) == 20);
ABI_ASSERT(abi_tls_requirement_offset_24, offsetof(PST_TLS_POLICY, alpn_requirement) == 24);
ABI_ASSERT(abi_tls_graceful_offset_36, offsetof(PST_TLS_POLICY, require_graceful_shutdown) == 36);
ABI_ASSERT(abi_io_operation_offset_4, offsetof(PST_IO_RESULT, operation) == 4);
ABI_ASSERT(abi_io_error_offset_12, offsetof(PST_IO_RESULT, error) == 12);
#endif

static int constants_frozen(void)
{
    volatile pst_u32 value;
    volatile PST_RESULT result;
    value = PST_API_VERSION; if (value != 0x00010300UL) return 0;
    value = PST_LIBRARY_VERSION; if (value != 0x00000300UL) return 0;
    result = PST_RESULT_OK; if (result != 0) return 0;
    result = PST_RESULT_INCOMPATIBLE_API; if (result != 15) return 0;
    value = PST_TLS_VERSION_1_2; if (value != 12UL) return 0;
    value = PST_TLS_VERSION_1_3; if (value != 13UL) return 0;
    value = PST_CAP_TLS_1_2; if (value != 0x001UL) return 0;
    value = PST_CAP_BACKEND_WAIT; if (value != 0x800UL) return 0;
    value = PST_LOG_LEVEL_OFF; if (value != 0UL) return 0;
    value = PST_LOG_LEVEL_TRACE; if (value != 5UL) return 0;
    value = PST_OPERATION_COMPLETE; if (value != 0UL) return 0;
    value = PST_OPERATION_FAILED; if (value != 5UL) return 0;
    value = PST_CLOSE_NONE; if (value != 0UL) return 0;
    value = PST_CLOSE_TRUNCATED; if (value != 2UL) return 0;
    return 1;
}

static int deterministic_failure_outputs(void)
{
    pst_u32 operation = 99UL, interest = 99UL;
    PST_RESULT error = (PST_RESULT)99;
    PST_WAIT_RESULT wait_result;
    PST_IO_RESULT io_result;
    memset(&wait_result, 0xa5, sizeof(wait_result));
    memset(&io_result, 0xa5, sizeof(io_result));
    if (pst_connection_handshake(NULL, &operation, &error) != PST_RESULT_INVALID_STATE) return 0;
    if (operation != PST_OPERATION_FAILED || error != PST_RESULT_INVALID_STATE) return 0;
    if (pst_connection_get_interest(NULL, &interest) != PST_RESULT_INVALID_STATE || interest != PST_INTEREST_NONE) return 0;
    if (pst_connection_wait(NULL, 1UL, &wait_result) != PST_RESULT_INVALID_STATE) return 0;
    if (wait_result.ready_interest != PST_INTEREST_NONE || wait_result.timed_out != 0UL) return 0;
    if (pst_connection_read(NULL, NULL, 0, &io_result) != PST_RESULT_INVALID_STATE) return 0;
    if (io_result.bytes_transferred != 0 || io_result.operation != PST_OPERATION_FAILED || io_result.close_kind != PST_CLOSE_NONE || io_result.error != PST_RESULT_INVALID_STATE) return 0;
    memset(&io_result, 0xa5, sizeof(io_result));
    if (pst_connection_write(NULL, NULL, 0, &io_result) != PST_RESULT_INVALID_STATE) return 0;
    if (io_result.bytes_transferred != 0 || io_result.operation != PST_OPERATION_FAILED || io_result.close_kind != PST_CLOSE_NONE || io_result.error != PST_RESULT_INVALID_STATE) return 0;
    operation = 99UL; error = (PST_RESULT)99;
    if (pst_connection_shutdown(NULL, &operation, &error) != PST_RESULT_INVALID_STATE) return 0;
    if (operation != PST_OPERATION_FAILED || error != PST_RESULT_INVALID_STATE) return 0;
    {
        PST_RUNTIME_INFO info;
        PST_PEER_INFO_SUMMARY summary;
        PST_DIAGNOSTIC_INFO diagnostic;
        pst_size required = 99;
        memset(&info, 0xa5, sizeof(info)); info.struct_size = sizeof(info); info.api_version = PST_API_VERSION;
        if (pst_runtime_get_info(NULL, &info) != PST_RESULT_INVALID_ARGUMENT || info.backend_id != NULL || info.capabilities != 0UL) return 0;
        memset(&summary, 0xa5, sizeof(summary)); summary.struct_size = sizeof(summary); summary.api_version = PST_API_VERSION;
        if (pst_peer_info_get_summary(NULL, &summary) != PST_RESULT_INVALID_ARGUMENT || summary.certificate_present != 0UL || summary.leaf_der_size != 0) return 0;
        if (pst_peer_info_copy_leaf_der(NULL, NULL, 0, &required) != PST_RESULT_INVALID_ARGUMENT || required != 0) return 0;
        memset(&diagnostic, 0xa5, sizeof(diagnostic)); diagnostic.struct_size = sizeof(diagnostic); diagnostic.api_version = PST_API_VERSION;
        if (pst_runtime_copy_diagnostic(NULL, &diagnostic) != PST_RESULT_INVALID_ARGUMENT || diagnostic.valid != 0UL) return 0;
        diagnostic.valid = 99UL;
        if (pst_connection_copy_diagnostic(NULL, &diagnostic) != PST_RESULT_INVALID_ARGUMENT || diagnostic.valid != 0UL) return 0;
    }
    return 1;
}

int main(void)
{
    if (!constants_frozen()) return 1;
    if (!deterministic_failure_outputs()) return 2;
    printf("test_public_abi: PASS PTR=%lu PST_SIZE=%lu\n", (unsigned long)sizeof(void *), (unsigned long)sizeof(pst_size));
    return 0;
}
