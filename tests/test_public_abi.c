/* SPDX-License-Identifier: MPL-2.0 */
#include "papinho_secure_transport.h"
#include <stddef.h>
#include <stdio.h>
#define A(n,x) typedef char abi_##n[(x)?1:-1]
A(api,PST_API_VERSION==0x00030000UL);A(lib,PST_LIBRARY_VERSION==0x00000605UL);A(release_info,sizeof(PST_RELEASE_INFO)==28UL);
A(release_struct_size,offsetof(PST_RELEASE_INFO,struct_size)==0UL);A(release_api,offsetof(PST_RELEASE_INFO,api_version)==4UL);A(release_disposition,offsetof(PST_RELEASE_INFO,disposition)==8UL);A(release_result,offsetof(PST_RELEASE_INFO,result)==12UL);A(release_native_domain,offsetof(PST_RELEASE_INFO,native_domain)==16UL);A(release_native_code,offsetof(PST_RELEASE_INFO,native_code)==20UL);A(release_native_valid,offsetof(PST_RELEASE_INFO,native_code_valid)==24UL);
A(role,offsetof(PST_CONNECTION_CONFIG,role)==8);A(known,PST_CAP_KNOWN_MASK==0x0007ffffUL);A(graceful_cap,PST_CAP_GRACEFUL_SHUTDOWN==0x00040000UL);A(sni_compat_zero,PST_SNI_MODE_COMPAT==0UL);
A(provider_info_size,sizeof(PST_PROVIDER_INFO)==60);
A(provider_server_caps,offsetof(PST_PROVIDER_INFO,server_capabilities)>offsetof(PST_PROVIDER_INFO,client_capabilities));
#if defined(_WIN64)
A(config_v20_size,PST_CONNECTION_CONFIG_V2_0_SIZE==184);A(config_size,sizeof(PST_CONNECTION_CONFIG)==208);A(selection_size,sizeof(PST_PROVIDER_SELECTION)==48);A(peer_size,sizeof(PST_PEER_INFO_SUMMARY)==128);
#else
A(config_v20_size,PST_CONNECTION_CONFIG_V2_0_SIZE==124);A(config_size,sizeof(PST_CONNECTION_CONFIG)==136);A(selection_size,sizeof(PST_PROVIDER_SELECTION)==28);A(peer_size,sizeof(PST_PEER_INFO_SUMMARY)==120);
#endif
int main(void){PST_VERSION_INFO version;PST_DIAGNOSTIC_INFO diagnostic;PST_LOG_CONFIG logging;if(pst_version_info_init(&version)!=PST_RESULT_OK||pst_get_version(&version)!=PST_RESULT_OK)return 1;if(version.api_major!=3||version.api_minor!=0||version.library_minor!=6||version.library_patch!=5)return 2;if(pst_diagnostic_info_init(&diagnostic)!=PST_RESULT_OK||pst_log_config_init(&logging)!=PST_RESULT_OK)return 3;printf("test_public_abi: PASS API=3.0 LIBRARY=0.6.5\n");return 0;}
