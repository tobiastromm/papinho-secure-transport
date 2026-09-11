/* SPDX-License-Identifier: MPL-2.0 */
#include "papinho_secure_transport.h"
#include <stddef.h>
#include <stdio.h>
#define A(n,x) typedef char abi_##n[(x)?1:-1]
A(api,PST_API_VERSION==0x00020100UL);A(lib,PST_LIBRARY_VERSION==0x00000601UL);
A(role,offsetof(PST_CONNECTION_CONFIG,role)==8);A(known,PST_CAP_KNOWN_MASK==0x0007ffffUL);A(graceful_cap,PST_CAP_GRACEFUL_SHUTDOWN==0x00040000UL);A(sni_compat_zero,PST_SNI_MODE_COMPAT==0UL);
A(provider_info_size,sizeof(PST_PROVIDER_INFO)==60);
A(provider_server_caps,offsetof(PST_PROVIDER_INFO,server_capabilities)>offsetof(PST_PROVIDER_INFO,client_capabilities));
#if defined(_WIN64)
A(config_v20_size,PST_CONNECTION_CONFIG_V2_0_SIZE==184);A(config_size,sizeof(PST_CONNECTION_CONFIG)==208);A(selection_size,sizeof(PST_PROVIDER_SELECTION)==48);A(peer_size,sizeof(PST_PEER_INFO_SUMMARY)==128);
#else
A(config_v20_size,PST_CONNECTION_CONFIG_V2_0_SIZE==124);A(config_size,sizeof(PST_CONNECTION_CONFIG)==136);A(selection_size,sizeof(PST_PROVIDER_SELECTION)==28);A(peer_size,sizeof(PST_PEER_INFO_SUMMARY)==120);
#endif
int main(void){PST_VERSION_INFO version;PST_DIAGNOSTIC_INFO diagnostic;PST_LOG_CONFIG logging;if(pst_version_info_init(&version)!=PST_RESULT_OK||pst_get_version(&version)!=PST_RESULT_OK)return 1;if(version.api_major!=2||version.api_minor!=1||version.library_minor!=6||version.library_patch!=1)return 2;if(pst_diagnostic_info_init(&diagnostic)!=PST_RESULT_OK||pst_log_config_init(&logging)!=PST_RESULT_OK)return 3;printf("test_public_abi: PASS API=2.1 LIBRARY=0.6.1\n");return 0;}
