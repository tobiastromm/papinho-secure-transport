/* SPDX-License-Identifier: MPL-2.0 */
#include "papinho_secure_transport.h"
#include "backends/openssl/pst_backend_openssl.h"
#include <windows.h>
#include <openssl/err.h>
#include <openssl/provider.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x,n) if(!(x)){printf("test_backend_openssl: FAIL %d\n",n);return n;}
static void config_init(PST_CONNECTION_CONFIG *config,const char *provider,pst_trust *trust)
{
 memset(config,0,sizeof(*config));config->struct_size=sizeof(*config);config->api_version=PST_API_VERSION;config->role=PST_CONNECTION_ROLE_CLIENT;
 config->provider_selection.struct_size=sizeof(config->provider_selection);config->provider_selection.api_version=PST_API_VERSION;config->provider_selection.mode=PST_BACKEND_SELECTION_EXACT;config->provider_selection.exact_provider_id=provider;
 config->local_identity.struct_size=sizeof(config->local_identity);config->local_identity.api_version=PST_API_VERSION;
 config->peer_authentication.struct_size=sizeof(config->peer_authentication);config->peer_authentication.api_version=PST_API_VERSION;config->peer_authentication.certificate_mode=PST_PEER_CERTIFICATE_REQUIRED;config->peer_authentication.trust=trust;config->peer_authentication.expected_peer_name="localhost";config->peer_authentication.expected_peer_name_size=9;
 config->tls.struct_size=sizeof(config->tls);config->tls.api_version=PST_API_VERSION;config->tls.minimum_version=PST_TLS_VERSION_1_2;config->tls.maximum_version=PST_TLS_VERSION_1_3;
 config->alpn.struct_size=sizeof(config->alpn);config->alpn.api_version=PST_API_VERSION;config->alpn.mode=PST_FEATURE_DISABLED;
}
int main(void)
{
 const PST_BACKEND_DESCRIPTOR *descriptor;void *state;int i;PST_RUNTIME_OPTIONS options;PST_RUNTIME_INFO runtime_info;PST_PROVIDER_INFO provider_info;PST_CONNECTION_CONFIG config;PST_TRUST_SOURCE trust_source;
 pst_runtime *runtime_a,*runtime_b;pst_connection *connection;pst_trust *trust;OSSL_LIB_CTX *audit_context;OSSL_PROVIDER *audit_default;
 CHECK(SetEnvironmentVariableA("OPENSSL_CONF","pst-missing-openssl.cnf")&&SetEnvironmentVariableA("OPENSSL_MODULES","pst-missing-openssl-modules"),1);
 audit_context=OSSL_LIB_CTX_new();CHECK(audit_context!=NULL,2);audit_default=OSSL_PROVIDER_load(audit_context,"default");
 CHECK(audit_default!=NULL&&OSSL_PROVIDER_available(audit_context,"default")==1&&OSSL_PROVIDER_available(audit_context,"legacy")==0&&OSSL_PROVIDER_available(audit_context,"fips")==0,3);
 OSSL_PROVIDER_unload(audit_default);OSSL_LIB_CTX_free(audit_context);ERR_clear_error();descriptor=pst_backend_openssl_descriptor();
 CHECK(descriptor&&strcmp(descriptor->id,"openssl")==0,4);CHECK(descriptor->spi_version==PST_BACKEND_SPI_VERSION&&descriptor->metadata&&descriptor->metadata->components[0].major==3UL&&descriptor->metadata->components[0].minor==5UL&&descriptor->metadata->components[0].patch==8UL,5);
 CHECK((descriptor->capabilities&(PST_BACKEND_CAP_TLS_1_2|PST_BACKEND_CAP_TLS_1_3|PST_BACKEND_CAP_ROLE_CLIENT|PST_BACKEND_CAP_LOCAL_IDENTITY|PST_BACKEND_CAP_PEER_CERT_AUTH|PST_BACKEND_CAP_CUSTOM_TRUST|PST_BACKEND_CAP_SYSTEM_TRUST|PST_BACKEND_CAP_PEER_NAME_VERIFY|PST_BACKEND_CAP_ALPN_CLIENT|PST_BACKEND_CAP_PEER_INFO|PST_BACKEND_CAP_NONBLOCKING|PST_BACKEND_CAP_BACKEND_WAIT))==(PST_BACKEND_CAP_TLS_1_2|PST_BACKEND_CAP_TLS_1_3|PST_BACKEND_CAP_ROLE_CLIENT|PST_BACKEND_CAP_LOCAL_IDENTITY|PST_BACKEND_CAP_PEER_CERT_AUTH|PST_BACKEND_CAP_CUSTOM_TRUST|PST_BACKEND_CAP_SYSTEM_TRUST|PST_BACKEND_CAP_PEER_NAME_VERIFY|PST_BACKEND_CAP_ALPN_CLIENT|PST_BACKEND_CAP_PEER_INFO|PST_BACKEND_CAP_NONBLOCKING|PST_BACKEND_CAP_BACKEND_WAIT)&&!(descriptor->capabilities&PST_BACKEND_CAP_ROLE_SERVER),6);
 for(i=0;i<100;i++){state=NULL;CHECK(descriptor->vtable->initialize(&state)==PST_RESULT_OK&&state,7);descriptor->vtable->shutdown(state);}
 ERR_raise(ERR_LIB_USER,1);CHECK(ERR_peek_error()!=0UL,8);state=NULL;CHECK(descriptor->vtable->initialize(&state)==PST_RESULT_OK&&ERR_peek_error()==0UL,9);descriptor->vtable->shutdown(state);
 pst_backend_registry_reset();CHECK(pst_backend_openssl_register()==PST_RESULT_OK&&pst_backend_count()==1&&pst_backend_openssl_register()==PST_RESULT_INVALID_STATE,10);
 memset(&options,0,sizeof(options));options.struct_size=sizeof(options);options.api_version=PST_API_VERSION;runtime_a=runtime_b=NULL;
 CHECK(pst_runtime_create(&options,&runtime_a)==PST_RESULT_OK&&pst_runtime_create(&options,&runtime_b)==PST_RESULT_OK,11);
 memset(&runtime_info,0,sizeof(runtime_info));runtime_info.struct_size=sizeof(runtime_info);runtime_info.api_version=PST_API_VERSION;CHECK(pst_runtime_get_info(runtime_b,&runtime_info)==PST_RESULT_OK&&runtime_info.provider_count==1,12);
 memset(&trust_source,0,sizeof(trust_source));trust_source.struct_size=sizeof(trust_source);trust_source.api_version=PST_API_VERSION;trust_source.kind=PST_TRUST_SOURCE_SYSTEM;trust=NULL;CHECK(pst_trust_create(&trust_source,&trust)==PST_RESULT_OK,13);
 config_init(&config,"missing",trust);connection=NULL;CHECK(pst_connection_create(runtime_b,&config,&connection)==PST_RESULT_UNSUPPORTED&&!connection,14);
 config.provider_selection.exact_provider_id="openssl";CHECK(pst_connection_create(runtime_b,&config,&connection)==PST_RESULT_OK&&connection,15);
 memset(&provider_info,0,sizeof(provider_info));provider_info.struct_size=sizeof(provider_info);provider_info.api_version=PST_API_VERSION;CHECK(pst_connection_get_provider_info(connection,&provider_info)==PST_RESULT_OK&&!strcmp(provider_info.provider_id,"openssl")&&(provider_info.capabilities&PST_CAP_TLS_1_3)&&!(provider_info.capabilities&PST_CAP_ROLE_SERVER),16);
 pst_connection_release(connection);pst_trust_release(trust);pst_runtime_release(runtime_a);pst_runtime_release(runtime_b);pst_backend_registry_reset();CHECK(ERR_peek_error()==0UL,17);
 printf("OPENSSL_BACKEND=PASS VERSION=3.5.8 SPI3=PASS ROLE_CLIENT=ADVERTISED ROLE_SERVER=NOT_ADVERTISED TLS12=ADVERTISED TLS13=ADVERTISED INIT_CYCLES=100 MULTI_RUNTIME=PASS ERROR_QUEUE=ISOLATED\n");
 printf("test_backend_openssl: PASS\n");return 0;
}
