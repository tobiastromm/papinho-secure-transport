/* SPDX-License-Identifier: MPL-2.0 */
#include "pst_backend.h"
#include <stddef.h>
#include <stdio.h>
#define A(n,x) typedef char spi_##n[(x)?1:-1]
A(version,PST_BACKEND_SPI_VERSION==0x00030000UL);A(role_offset,offsetof(PST_BACKEND_CONNECTION_OPTIONS,role)==8);A(config_after_caps,offsetof(PST_BACKEND_CONNECTION_OPTIONS,configuration)>offsetof(PST_BACKEND_CONNECTION_OPTIONS,required_capabilities));
#if defined(_WIN64)
A(options_size,sizeof(PST_BACKEND_CONNECTION_OPTIONS)==24);A(vtable_size,sizeof(PST_BACKEND_VTABLE)==160);A(descriptor_size,sizeof(PST_BACKEND_DESCRIPTOR)==48);
#else
A(options_size,sizeof(PST_BACKEND_CONNECTION_OPTIONS)==20);A(vtable_size,sizeof(PST_BACKEND_VTABLE)==84);A(descriptor_size,sizeof(PST_BACKEND_DESCRIPTOR)==28);
#endif
A(client_cap,PST_BACKEND_CAP_ROLE_CLIENT==PST_CAP_ROLE_CLIENT);A(server_cap,PST_BACKEND_CAP_ROLE_SERVER==PST_CAP_ROLE_SERVER);A(alpn_client,PST_BACKEND_CAP_ALPN_CLIENT==PST_CAP_ALPN_CLIENT);A(name_cap,PST_BACKEND_CAP_PEER_NAME_VERIFY==PST_CAP_PEER_NAME_VERIFY);
int main(void){printf("test_provider_spi_abi: PASS SPI=3.0\n");return 0;}
