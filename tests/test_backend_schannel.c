/* SPDX-License-Identifier: MPL-2.0 */
#include "papinho_secure_transport.h"
#include "backends/schannel/pst_backend_schannel.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x,n) if(!(x)){printf("test_backend_schannel: FAIL %d\n",n);return n;}
static void config_init(PST_CONNECTION_CONFIG *config,const char *provider,pst_trust *trust)
{
 memset(config,0,sizeof(*config));config->struct_size=sizeof(*config);config->api_version=PST_API_VERSION;config->role=PST_CONNECTION_ROLE_CLIENT;
 config->provider_selection.struct_size=sizeof(config->provider_selection);config->provider_selection.api_version=PST_API_VERSION;config->provider_selection.mode=PST_BACKEND_SELECTION_EXACT;config->provider_selection.exact_provider_id=provider;
 config->local_identity.struct_size=sizeof(config->local_identity);config->local_identity.api_version=PST_API_VERSION;
 config->peer_authentication.struct_size=sizeof(config->peer_authentication);config->peer_authentication.api_version=PST_API_VERSION;config->peer_authentication.certificate_mode=PST_PEER_CERTIFICATE_REQUIRED;config->peer_authentication.trust=trust;config->peer_authentication.expected_peer_name="localhost";config->peer_authentication.expected_peer_name_size=9;
 config->tls.struct_size=sizeof(config->tls);config->tls.api_version=PST_API_VERSION;config->tls.minimum_version=PST_TLS_VERSION_1_2;config->tls.maximum_version=PST_TLS_VERSION_1_2;
 config->alpn.struct_size=sizeof(config->alpn);config->alpn.api_version=PST_API_VERSION;config->alpn.mode=PST_FEATURE_DISABLED;
}
int main(void)
{
 const PST_BACKEND_DESCRIPTOR *descriptor;void *state;int i;PST_RUNTIME_OPTIONS options;PST_RUNTIME_INFO runtime_info;PST_PROVIDER_INFO provider_info;PST_CONNECTION_CONFIG config;PST_TRUST_SOURCE trust_source;
 pst_runtime *runtime;pst_connection *connection;pst_trust *trust;PST_RESULT result;
 descriptor=pst_backend_schannel_descriptor();CHECK(descriptor&&!strcmp(descriptor->id,"schannel"),1);
 CHECK(descriptor->spi_version==PST_BACKEND_SPI_VERSION&&descriptor->metadata&&descriptor->metadata->implementation.major==1UL,2);
 CHECK((descriptor->server_capabilities&(PST_BACKEND_CAP_TLS_1_2|PST_BACKEND_CAP_ROLE_SERVER|PST_BACKEND_CAP_NONBLOCKING|PST_BACKEND_CAP_BACKEND_WAIT|PST_BACKEND_CAP_LOCAL_IDENTITY|PST_BACKEND_CAP_PEER_CERT_AUTH|PST_BACKEND_CAP_PEER_CERT_OPTIONAL|PST_BACKEND_CAP_CUSTOM_TRUST|PST_BACKEND_CAP_SYSTEM_TRUST|PST_BACKEND_CAP_PEER_INFO))==(PST_BACKEND_CAP_TLS_1_2|PST_BACKEND_CAP_ROLE_SERVER|PST_BACKEND_CAP_NONBLOCKING|PST_BACKEND_CAP_BACKEND_WAIT|PST_BACKEND_CAP_LOCAL_IDENTITY|PST_BACKEND_CAP_PEER_CERT_AUTH|PST_BACKEND_CAP_PEER_CERT_OPTIONAL|PST_BACKEND_CAP_CUSTOM_TRUST|PST_BACKEND_CAP_SYSTEM_TRUST|PST_BACKEND_CAP_PEER_INFO)&&!(descriptor->server_capabilities&(PST_BACKEND_CAP_TLS_1_3|PST_BACKEND_CAP_ALPN_SERVER|PST_BACKEND_CAP_PEER_NAME_VERIFY)),3);
 for(i=0;i<100;i++){state=NULL;CHECK(descriptor->vtable->initialize(&state)==PST_RESULT_OK&&state,4);descriptor->vtable->shutdown(state);}
 pst_backend_registry_reset();CHECK(pst_backend_schannel_register()==PST_RESULT_OK,5);
 memset(&options,0,sizeof(options));options.struct_size=sizeof(options);options.api_version=PST_API_VERSION;runtime=NULL;CHECK(pst_runtime_create(&options,&runtime)==PST_RESULT_OK,6);
 memset(&runtime_info,0,sizeof(runtime_info));runtime_info.struct_size=sizeof(runtime_info);runtime_info.api_version=PST_API_VERSION;CHECK(pst_runtime_get_info(runtime,&runtime_info)==PST_RESULT_OK&&runtime_info.provider_count==1,7);
 memset(&trust_source,0,sizeof(trust_source));trust_source.struct_size=sizeof(trust_source);trust_source.api_version=PST_API_VERSION;trust_source.kind=PST_TRUST_SOURCE_SYSTEM;trust=NULL;CHECK(pst_trust_create(&trust_source,&trust)==PST_RESULT_OK,8);
 config_init(&config,"missing",trust);connection=NULL;CHECK(pst_connection_create(runtime,&config,&connection)==PST_RESULT_UNSUPPORTED&&!connection,9);
 config.provider_selection.exact_provider_id="schannel";result=pst_connection_create(runtime,&config,&connection);CHECK(result==PST_RESULT_OK&&connection,10);pst_connection_release(connection);connection=NULL;
 memset(&provider_info,0,sizeof(provider_info));provider_info.struct_size=sizeof(provider_info);provider_info.api_version=PST_API_VERSION;CHECK(pst_runtime_get_provider_info(runtime,0,&provider_info)==PST_RESULT_OK&&provider_info.initialized&&!strcmp(provider_info.provider_id,"schannel")&&(provider_info.server_capabilities&PST_CAP_ROLE_SERVER)&&!(provider_info.server_capabilities&(PST_CAP_TLS_1_3|PST_CAP_ALPN_SERVER|PST_CAP_PEER_NAME_VERIFY)),11);
 pst_trust_release(trust);pst_runtime_release(runtime);pst_backend_registry_reset();
 printf("SCHANNEL_BACKEND=PASS SPI3=PASS ROLE_CLIENT=ADVERTISED ROLE_SERVER=ADVERTISED TLS12_SERVER=ADVERTISED TLS13_SERVER=UNAVAILABLE_NOT_ADVERTISED ALPN_SERVER=NOT_ADVERTISED INIT_CYCLES=100 CLIENT_NO_LOCAL_IDENTITY=PASS\n");
 printf("test_backend_schannel: PASS\n");return 0;
}
