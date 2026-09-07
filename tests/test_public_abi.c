/* SPDX-License-Identifier: MPL-2.0 */
#include "papinho_secure_transport.h"
#include <stddef.h>
#include <stdio.h>
#define A(n,x) typedef char abi_##n[(x)?1:-1]
A(api,PST_API_VERSION==0x00020000UL);A(lib,PST_LIBRARY_VERSION==0x00000500UL);
A(role,offsetof(PST_CONNECTION_CONFIG,role)==8);A(known,PST_CAP_KNOWN_MASK==0x0001ffffUL);
#if defined(_WIN64)
A(config_size,sizeof(PST_CONNECTION_CONFIG)==184);A(selection_size,sizeof(PST_PROVIDER_SELECTION)==48);A(peer_size,sizeof(PST_PEER_INFO_SUMMARY)==128);
#else
A(config_size,sizeof(PST_CONNECTION_CONFIG)==124);A(selection_size,sizeof(PST_PROVIDER_SELECTION)==28);A(peer_size,sizeof(PST_PEER_INFO_SUMMARY)==120);
#endif
int main(void){PST_VERSION_INFO version;PST_DIAGNOSTIC_INFO diagnostic;PST_LOG_CONFIG logging;if(pst_version_info_init(&version)!=PST_RESULT_OK||pst_get_version(&version)!=PST_RESULT_OK)return 1;if(version.api_major!=2||version.library_minor!=5)return 2;if(pst_diagnostic_info_init(&diagnostic)!=PST_RESULT_OK||pst_log_config_init(&logging)!=PST_RESULT_OK)return 3;printf("test_public_abi: PASS API=2.0 LIBRARY=0.5\n");return 0;}
