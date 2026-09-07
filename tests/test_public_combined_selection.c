/* SPDX-License-Identifier: MPL-2.0 */
#include "papinho_secure_transport.h"
#include "papinho_secure_transport_win32.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) if(!(x)){printf("test_public_combined_selection: FAIL line %d\n",__LINE__);return 1;}
static void config_init(PST_CONNECTION_CONFIG *c,pst_trust *trust,pst_u32 mode,const char *exact,const char *const *ordered,pst_size count,pst_u32 tls)
{
 memset(c,0,sizeof(*c));c->struct_size=sizeof(*c);c->api_version=PST_API_VERSION;c->role=PST_CONNECTION_ROLE_CLIENT;
 c->provider_selection.struct_size=sizeof(c->provider_selection);c->provider_selection.api_version=PST_API_VERSION;c->provider_selection.mode=mode;c->provider_selection.exact_provider_id=exact;c->provider_selection.ordered_provider_ids=ordered;c->provider_selection.ordered_provider_count=count;
 c->local_identity.struct_size=sizeof(c->local_identity);c->local_identity.api_version=PST_API_VERSION;
 c->peer_authentication.struct_size=sizeof(c->peer_authentication);c->peer_authentication.api_version=PST_API_VERSION;c->peer_authentication.certificate_mode=PST_PEER_CERTIFICATE_REQUIRED;c->peer_authentication.trust=trust;c->peer_authentication.expected_peer_name="localhost";c->peer_authentication.expected_peer_name_size=9;
 c->tls.struct_size=sizeof(c->tls);c->tls.api_version=PST_API_VERSION;c->tls.minimum_version=tls;c->tls.maximum_version=tls;
 c->alpn.struct_size=sizeof(c->alpn);c->alpn.api_version=PST_API_VERSION;c->alpn.mode=PST_FEATURE_DISABLED;
}
static PST_RESULT select_provider(pst_runtime *runtime,pst_trust *trust,pst_u32 mode,const char *exact,const char *const *ordered,pst_size count,pst_u32 tls,const char *expected)
{
 PST_CONNECTION_CONFIG config;PST_PROVIDER_INFO info;pst_connection *connection=NULL;PST_RESULT result;
 config_init(&config,trust,mode,exact,ordered,count,tls);result=pst_connection_create(runtime,&config,&connection);if(result!=PST_RESULT_OK){printf("SELECTION_CREATE MODE=%lu TLS=0x%04lx RESULT=%s\n",(unsigned long)mode,(unsigned long)tls,pst_result_string(result));return result;}
 memset(&info,0,sizeof(info));info.struct_size=sizeof(info);info.api_version=PST_API_VERSION;result=pst_connection_get_provider_info(connection,&info);if(result==PST_RESULT_OK&&strcmp(info.provider_id,expected)!=0)result=PST_RESULT_BACKEND_FAILURE;pst_connection_release(connection);return result;
}
int main(void)
{
 PST_RUNTIME_OPTIONS options;PST_TRUST_SOURCE source;const char *order[2];pst_runtime *runtime=NULL;pst_trust *trust=NULL;
 memset(&options,0,sizeof(options));options.struct_size=sizeof(options);options.api_version=PST_API_VERSION;
 memset(&source,0,sizeof(source));source.struct_size=sizeof(source);source.api_version=PST_API_VERSION;source.kind=PST_TRUST_SOURCE_SYSTEM;
 CHECK(pst_win32_register_builtin_providers()==PST_RESULT_OK);CHECK(pst_runtime_create(&options,&runtime)==PST_RESULT_OK);CHECK(pst_trust_create(&source,&trust)==PST_RESULT_OK);
 CHECK(select_provider(runtime,trust,PST_BACKEND_SELECTION_AUTOMATIC,NULL,NULL,0,PST_TLS_VERSION_1_2,"schannel")==PST_RESULT_OK);
 CHECK(select_provider(runtime,trust,PST_BACKEND_SELECTION_AUTOMATIC,NULL,NULL,0,PST_TLS_VERSION_1_3,"openssl")==PST_RESULT_OK);
 CHECK(select_provider(runtime,trust,PST_BACKEND_SELECTION_EXACT,"openssl",NULL,0,PST_TLS_VERSION_1_3,"openssl")==PST_RESULT_OK);
 CHECK(select_provider(runtime,trust,PST_BACKEND_SELECTION_EXACT,"schannel",NULL,0,PST_TLS_VERSION_1_3,"schannel")==PST_RESULT_UNSUPPORTED);
 order[0]="openssl";order[1]="schannel";CHECK(select_provider(runtime,trust,PST_BACKEND_SELECTION_ORDERED,NULL,order,2,PST_TLS_VERSION_1_2,"openssl")==PST_RESULT_OK);
 pst_trust_release(trust);pst_runtime_release(runtime);
 printf("PUBLIC_COMBINED_SELECTION TLS12_SYSTEM_AUTO=schannel TLS13_SYSTEM_AUTO=openssl EXACT_OPENSSL_TLS13=PASS EXACT_SCHANNEL_TLS13=UNSUPPORTED ORDERED_OPENSSL_FIRST_TLS12=openssl CONNECTION_LEVEL_PINNING=PASS PASS=1\n");return 0;
}
