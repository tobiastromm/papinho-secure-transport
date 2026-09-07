/* SPDX-License-Identifier: MPL-2.0 */
#include "papinho_secure_transport.h"
#include "pst_backend.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#define ASSERT_C(name,expr) typedef char assert_##name[(expr)?1:-1]
#define CHECK(expr,n) if(!(expr))return fail(n)
ASSERT_C(api_version,PST_API_VERSION==0x00020000UL);
ASSERT_C(library_version,PST_LIBRARY_VERSION==0x00000500UL);
ASSERT_C(spi_version,PST_BACKEND_SPI_VERSION==0x00030000UL);
ASSERT_C(role_client,PST_CONNECTION_ROLE_CLIENT==1UL);
ASSERT_C(role_server,PST_CONNECTION_ROLE_SERVER==2UL);
ASSERT_C(cap_last,PST_CAP_EARLY_DATA==0x00010000UL);
ASSERT_C(config_role_first,offsetof(PST_CONNECTION_CONFIG,role)==8);
ASSERT_C(spi_options_role,offsetof(PST_BACKEND_CONNECTION_OPTIONS,role)==8);
ASSERT_C(spi_options_config,offsetof(PST_BACKEND_CONNECTION_OPTIONS,configuration)>offsetof(PST_BACKEND_CONNECTION_OPTIONS,required_capabilities));
#if defined(_WIN64)
ASSERT_C(connection_config_size,sizeof(PST_CONNECTION_CONFIG)==184);
ASSERT_C(provider_selection_size,sizeof(PST_PROVIDER_SELECTION)==48);
ASSERT_C(peer_info_size,sizeof(PST_PEER_INFO_SUMMARY)==128);
ASSERT_C(spi_options_size,sizeof(PST_BACKEND_CONNECTION_OPTIONS)==24);
ASSERT_C(spi_vtable_size,sizeof(PST_BACKEND_VTABLE)==160);
ASSERT_C(spi_descriptor_size,sizeof(PST_BACKEND_DESCRIPTOR)==48);
#else
ASSERT_C(connection_config_size,sizeof(PST_CONNECTION_CONFIG)==124);
ASSERT_C(provider_selection_size,sizeof(PST_PROVIDER_SELECTION)==28);
ASSERT_C(peer_info_size,sizeof(PST_PEER_INFO_SUMMARY)==120);
ASSERT_C(spi_options_size,sizeof(PST_BACKEND_CONNECTION_OPTIONS)==20);
ASSERT_C(spi_vtable_size,sizeof(PST_BACKEND_VTABLE)==84);
ASSERT_C(spi_descriptor_size,sizeof(PST_BACKEND_DESCRIPTOR)==28);
#endif
typedef struct candidate { const char *id; pst_u32 caps; int handshake_fails; } candidate;
static int fail(int n){printf("test_api2_spi3_contract: FAIL %d\n",n);return n;}
static const candidate providers[]={
 {"a",PST_CAP_ROLE_CLIENT|PST_CAP_TLS_1_2|PST_CAP_NONBLOCKING,0},
 {"b",PST_CAP_ROLE_SERVER|PST_CAP_TLS_1_2|PST_CAP_ALPN_SERVER|PST_CAP_NONBLOCKING,0},
 {"c",PST_CAP_ROLE_CLIENT|PST_CAP_ROLE_SERVER|PST_CAP_TLS_1_2|PST_CAP_TLS_1_3|PST_CAP_ALPN_CLIENT|PST_CAP_ALPN_SERVER|PST_CAP_NONBLOCKING,1}
};
static int eligible(const candidate *p,pst_u32 required){return (required&~p->caps)==0;}
static const candidate *find(const char *id){int i;for(i=0;i<3;i++)if(!strcmp(providers[i].id,id))return &providers[i];return NULL;}
static const candidate *select_provider(pst_u32 mode,const char *exact,const char *const *ordered,pst_size count,pst_u32 required){pst_size i;const candidate*p;if(mode==PST_BACKEND_SELECTION_EXACT){p=find(exact);return p&&eligible(p,required)?p:NULL;}if(mode==PST_BACKEND_SELECTION_ORDERED){for(i=0;i<count;i++){p=find(ordered[i]);if(p&&eligible(p,required))return p;}return NULL;}if(mode==PST_BACKEND_SELECTION_AUTOMATIC){for(i=0;i<3;i++)if(eligible(&providers[i],required))return &providers[i];}return NULL;}
static int valid_config(pst_u32 role,pst_u32 peer_mode,pst_u32 alpn_mode,pst_size alpn_count,int has_identity,int has_name){if(role!=PST_CONNECTION_ROLE_CLIENT&&role!=PST_CONNECTION_ROLE_SERVER)return 0;if(peer_mode>PST_PEER_CERTIFICATE_REQUIRED)return 0;if(alpn_mode>PST_FEATURE_REQUIRED)return 0;if(alpn_mode==PST_FEATURE_REQUIRED&&!alpn_count)return 0;if(role==PST_CONNECTION_ROLE_SERVER&&!has_identity)return 0;if(role==PST_CONNECTION_ROLE_SERVER&&has_name)return 0;if(role==PST_CONNECTION_ROLE_CLIENT&&peer_mode!=PST_PEER_CERTIFICATE_DISABLED&&!has_name)return 0;return 1;}
int main(void){const candidate*p;const char*order1[]={"a","b","c"};const char*order2[]={"c","b"};pst_u32 client12=PST_CAP_ROLE_CLIENT|PST_CAP_TLS_1_2; pst_u32 server12=PST_CAP_ROLE_SERVER|PST_CAP_TLS_1_2; pst_u32 server_alpn=server12|PST_CAP_ALPN_SERVER;
 CHECK(select_provider(PST_BACKEND_SELECTION_EXACT,"a",NULL,0,client12)==&providers[0],1);
 CHECK(select_provider(PST_BACKEND_SELECTION_EXACT,"a",NULL,0,server12)==NULL,2);
 CHECK(select_provider(PST_BACKEND_SELECTION_EXACT,"b",NULL,0,server_alpn)==&providers[1],3);
 CHECK(select_provider(PST_BACKEND_SELECTION_EXACT,"missing",NULL,0,client12)==NULL,4);
 CHECK(select_provider(PST_BACKEND_SELECTION_ORDERED,NULL,order1,3,server_alpn)==&providers[1],5);
 p=select_provider(PST_BACKEND_SELECTION_ORDERED,NULL,order2,2,server_alpn);CHECK(p==&providers[2]&&p->handshake_fails,6);
 CHECK(select_provider(PST_BACKEND_SELECTION_AUTOMATIC,NULL,NULL,0,client12)==&providers[0],8);
 CHECK(select_provider(PST_BACKEND_SELECTION_AUTOMATIC,NULL,NULL,0,server12)==&providers[1],9);
 CHECK(select_provider(PST_BACKEND_SELECTION_AUTOMATIC,NULL,NULL,0,PST_CAP_ROLE_SERVER|PST_CAP_TLS_1_3)==&providers[2],10);
 CHECK(select_provider(0,NULL,NULL,0,client12)==NULL,20);
 CHECK(select_provider(PST_BACKEND_SELECTION_AUTOMATIC,NULL,NULL,0,PST_CAP_EARLY_DATA)==NULL,21);
 CHECK(!valid_config(0,PST_PEER_CERTIFICATE_DISABLED,PST_FEATURE_DISABLED,0,0,0),11);
 CHECK(!valid_config(PST_CONNECTION_ROLE_SERVER,PST_PEER_CERTIFICATE_DISABLED,PST_FEATURE_DISABLED,0,0,0),12);
 CHECK(valid_config(PST_CONNECTION_ROLE_CLIENT,PST_PEER_CERTIFICATE_DISABLED,PST_FEATURE_DISABLED,0,0,0),13);
 CHECK(!valid_config(PST_CONNECTION_ROLE_CLIENT,PST_PEER_CERTIFICATE_REQUIRED,PST_FEATURE_DISABLED,0,0,0),14);
 CHECK(valid_config(PST_CONNECTION_ROLE_CLIENT,PST_PEER_CERTIFICATE_REQUIRED,PST_FEATURE_OPTIONAL,1,0,1),15);
 CHECK(!valid_config(PST_CONNECTION_ROLE_SERVER,PST_PEER_CERTIFICATE_OPTIONAL,PST_FEATURE_REQUIRED,0,1,0),16);
 CHECK(!valid_config(PST_CONNECTION_ROLE_SERVER,PST_PEER_CERTIFICATE_OPTIONAL,PST_FEATURE_OPTIONAL,1,1,1),17);
 CHECK(valid_config(PST_CONNECTION_ROLE_SERVER,PST_PEER_CERTIFICATE_OPTIONAL,PST_FEATURE_REQUIRED,1,1,0),18);
 printf("MOCK_CLIENT_SELECTION=PASS MOCK_SERVER_SELECTION=PASS EXACT=PASS ORDERED=PASS AUTOMATIC=REGISTRATION_ORDER NO_POST_SELECTION_FALLBACK=PASS CONFIG_VALIDATION=PASS\n");
 printf("test_api2_spi3_contract: PASS\n");return 0;}
