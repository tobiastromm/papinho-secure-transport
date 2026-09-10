/* SPDX-License-Identifier: MPL-2.0 */
#include "papinho_secure_transport.h"
#include "papinho_secure_transport_win32.h"
#include <stdio.h>
#include <string.h>
int main(void)
{
    PST_RUNTIME_OPTIONS options;
    PST_RUNTIME_INFO info;
    PST_CONNECTION_CONFIG config;
    pst_runtime *runtime;
    if (pst_api_version() != 0x00020100UL || pst_library_version() != 0x00000600UL) return 10;
    if (pst_win32_register_builtin_providers() != PST_RESULT_OK) return 11;
    memset(&options, 0, sizeof(options)); options.struct_size = sizeof(options); options.api_version = PST_API_VERSION;
    if (pst_runtime_create(&options, &runtime) != PST_RESULT_OK) return 12;
    memset(&info, 0, sizeof(info)); info.struct_size = sizeof(info); info.api_version = PST_API_VERSION;
    if (pst_runtime_get_info(runtime, &info) != PST_RESULT_OK || info.provider_count == 0) { pst_runtime_release(runtime); return 13; }
    memset(&config, 0, sizeof(config));config.struct_size=sizeof(config);config.api_version=PST_API_VERSION;
#ifdef PST_RELEASE_SERVER_CONSUMER
    config.role=PST_CONNECTION_ROLE_SERVER;
#else
    config.role=PST_CONNECTION_ROLE_CLIENT;
#endif
    config.provider_selection.struct_size=sizeof(config.provider_selection);config.provider_selection.api_version=PST_API_VERSION;config.provider_selection.mode=PST_BACKEND_SELECTION_AUTOMATIC;
    config.local_identity.struct_size=sizeof(config.local_identity);config.local_identity.api_version=PST_API_VERSION;
    config.peer_authentication.struct_size=sizeof(config.peer_authentication);config.peer_authentication.api_version=PST_API_VERSION;config.peer_authentication.certificate_mode=PST_PEER_CERTIFICATE_DISABLED;
    config.tls.struct_size=sizeof(config.tls);config.tls.api_version=PST_API_VERSION;config.tls.minimum_version=PST_TLS_VERSION_1_2;config.tls.maximum_version=PST_TLS_VERSION_1_2;
    config.alpn.struct_size=sizeof(config.alpn);config.alpn.api_version=PST_API_VERSION;config.alpn.mode=PST_FEATURE_DISABLED;
    pst_runtime_release(runtime);
    printf("PACKAGE_CONSUMER API=2.1.0 LIBRARY=0.6.0 ROLE=%s PASS\n",config.role==PST_CONNECTION_ROLE_SERVER?"SERVER":"CLIENT");
    return 0;
}
