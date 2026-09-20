/* SPDX-License-Identifier: MPL-2.0 */
#include "papinho_secure_transport.h"
#include "papinho_secure_transport_win32.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    PST_RUNTIME_OPTIONS options;
    PST_CONNECTION_CONFIG config;
    pst_runtime *runtime = NULL;
    pst_connection *connection = NULL;
    PST_RESULT result;

    result = pst_win32_register_builtin_providers();
    if (result != PST_RESULT_OK) return 1;
    memset(&options, 0, sizeof(options));
    options.struct_size = sizeof(options);
    options.api_version = PST_API_VERSION;
    result = pst_runtime_create(&options, &runtime);
    if (result != PST_RESULT_OK || runtime == NULL) return 2;
    memset(&config, 0, sizeof(config));
    config.struct_size = sizeof(config);
    config.api_version = PST_API_VERSION;
    config.role = PST_CONNECTION_ROLE_CLIENT;
    config.provider_selection.struct_size = sizeof(config.provider_selection);
    config.provider_selection.api_version = PST_API_VERSION;
    config.provider_selection.mode = PST_BACKEND_SELECTION_EXACT;
    config.provider_selection.exact_provider_id = "retrozilla-nss";
    config.local_identity.struct_size = sizeof(config.local_identity);
    config.local_identity.api_version = PST_API_VERSION;
    config.peer_authentication.struct_size = sizeof(config.peer_authentication);
    config.peer_authentication.api_version = PST_API_VERSION;
    config.peer_authentication.certificate_mode = PST_PEER_CERTIFICATE_DISABLED;
    config.tls.struct_size = sizeof(config.tls);
    config.tls.api_version = PST_API_VERSION;
    config.tls.minimum_version = PST_TLS_VERSION_1_2;
    config.tls.maximum_version = PST_TLS_VERSION_1_2;
    config.alpn.struct_size = sizeof(config.alpn);
    config.alpn.api_version = PST_API_VERSION;
    result = pst_connection_create(runtime, &config, &connection);
    if (result != PST_RESULT_OK || connection == NULL) {
        pst_runtime_release(runtime);
        return 3;
    }
    pst_connection_release(connection);
    pst_runtime_release(runtime);
    printf("VC6_CRT_PUBLIC_CLIENT_API=PASS\n");
    return 0;
}
