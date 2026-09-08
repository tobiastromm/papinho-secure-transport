/* SPDX-License-Identifier: MPL-2.0 */
#include "papinho_secure_transport.h"
#include "pst_backend.h"
#include "pst_identity_internal.h"
#include "backends/nss/pst_backend_nss.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x,n) if(!(x)){fprintf(stderr,"NSS_SERVER_VALIDATION=FAIL STEP=%d\n",n);goto done;}

static unsigned char *load_file(const char *path,pst_size *size)
{
 FILE *file;long length;unsigned char *data;*size=0;file=fopen(path,"rb");
 if(!file)return NULL;if(fseek(file,0,SEEK_END)||(length=ftell(file))<=0||
    fseek(file,0,SEEK_SET)){fclose(file);return NULL;}
 data=(unsigned char*)malloc((size_t)length);
 if(!data||fread(data,1,(size_t)length,file)!=(size_t)length){free(data);fclose(file);return NULL;}
 fclose(file);*size=(pst_size)length;return data;
}

static PST_RESULT attempt(const PST_BACKEND_DESCRIPTOR *descriptor,void *runtime,
 const unsigned char *leaf,pst_size leaf_size,const unsigned char *intermediate,
 pst_size intermediate_size,const unsigned char *key,pst_size key_size)
{
 PST_CREDENTIAL_SOURCE source;PST_DER_ITEM chain[2];pst_credentials *credentials=NULL;
 PST_CONNECTION_CONFIG config;pst_connection_config_snapshot *snapshot=NULL;
 PST_BACKEND_CONNECTION_OPTIONS options;void *connection=NULL;PST_RESULT result;
 memset(&source,0,sizeof(source));source.struct_size=sizeof(source);
 source.api_version=PST_API_VERSION;source.kind=PST_CREDENTIAL_SOURCE_CERT_CHAIN_DER_PKCS8_DER;
 chain[0].data=leaf;chain[0].size=leaf_size;chain[1].data=intermediate;
 chain[1].size=intermediate_size;source.certificate_chain=chain;
 source.certificate_count=2;source.private_key_der=key;source.private_key_der_size=key_size;
 result=pst_credentials_create(&source,&credentials);if(result!=PST_RESULT_OK)return result;
 memset(&config,0,sizeof(config));config.struct_size=sizeof(config);config.api_version=PST_API_VERSION;
 config.role=PST_CONNECTION_ROLE_SERVER;config.provider_selection.struct_size=sizeof(config.provider_selection);
 config.provider_selection.api_version=PST_API_VERSION;config.provider_selection.mode=PST_BACKEND_SELECTION_EXACT;
 config.provider_selection.exact_provider_id="retrozilla-nss";config.local_identity.struct_size=sizeof(config.local_identity);
 config.local_identity.api_version=PST_API_VERSION;config.local_identity.credentials=credentials;
 config.peer_authentication.struct_size=sizeof(config.peer_authentication);
 config.peer_authentication.api_version=PST_API_VERSION;
 config.peer_authentication.certificate_mode=PST_PEER_CERTIFICATE_DISABLED;
 config.tls.struct_size=sizeof(config.tls);config.tls.api_version=PST_API_VERSION;
 config.tls.minimum_version=PST_TLS_VERSION_1_2;config.tls.maximum_version=PST_TLS_VERSION_1_2;
 config.alpn.struct_size=sizeof(config.alpn);config.alpn.api_version=PST_API_VERSION;
 config.alpn.mode=PST_FEATURE_DISABLED;
 result=pst_connection_config_snapshot_create(&config,&snapshot);
 pst_credentials_release(credentials);if(result!=PST_RESULT_OK)return result;
 memset(&options,0,sizeof(options));options.struct_size=sizeof(options);
 options.spi_version=PST_BACKEND_SPI_VERSION;options.role=PST_CONNECTION_ROLE_SERVER;
 options.configuration=pst_connection_config_snapshot_public(snapshot);
 options.required_capabilities=pst_connection_config_required_capabilities(snapshot);
 result=descriptor->vtable->connection_create(runtime,&options,&connection);
 if(connection)descriptor->vtable->connection_destroy(connection);
 pst_connection_config_snapshot_release(snapshot);return result;
}

int main(int argc,char **argv)
{
 const PST_BACKEND_DESCRIPTOR *descriptor=pst_backend_nss_descriptor();void *backend=NULL,*runtime=NULL;
 unsigned char *leaf=NULL,*intermediate=NULL,*key=NULL,*wrong_key=NULL;
 unsigned char malformed[3]={0x01,0x02,0x03};pst_size leaf_size,intermediate_size,key_size,wrong_key_size;
 PST_CONNECTION_CONFIG missing;pst_connection_config_snapshot *missing_snapshot=NULL;PST_RESULT result;int ok=0;
 if(argc!=5)return 2;leaf=load_file(argv[1],&leaf_size);intermediate=load_file(argv[2],&intermediate_size);
 key=load_file(argv[3],&key_size);wrong_key=load_file(argv[4],&wrong_key_size);
 CHECK(leaf&&intermediate&&key&&wrong_key,1);
 memset(&missing,0,sizeof(missing));missing.struct_size=sizeof(missing);missing.api_version=PST_API_VERSION;
 missing.role=PST_CONNECTION_ROLE_SERVER;missing.local_identity.struct_size=sizeof(missing.local_identity);missing.local_identity.api_version=PST_API_VERSION;missing.provider_selection.struct_size=sizeof(missing.provider_selection);
 missing.provider_selection.api_version=PST_API_VERSION;missing.provider_selection.mode=PST_BACKEND_SELECTION_EXACT;
 missing.provider_selection.exact_provider_id="retrozilla-nss";missing.peer_authentication.struct_size=sizeof(missing.peer_authentication);
 missing.peer_authentication.api_version=PST_API_VERSION;missing.tls.struct_size=sizeof(missing.tls);
 missing.tls.api_version=PST_API_VERSION;missing.tls.minimum_version=PST_TLS_VERSION_1_2;
 missing.tls.maximum_version=PST_TLS_VERSION_1_2;missing.alpn.struct_size=sizeof(missing.alpn);
 missing.alpn.api_version=PST_API_VERSION;CHECK(pst_connection_config_snapshot_create(&missing,&missing_snapshot)==PST_RESULT_POLICY_VIOLATION,2);
 CHECK(descriptor->vtable->initialize(&backend)==PST_RESULT_OK,3);
 CHECK(descriptor->vtable->runtime_create(backend,&runtime)==PST_RESULT_OK,4);
 result=attempt(descriptor,runtime,leaf,leaf_size,intermediate,intermediate_size,key,key_size);
 CHECK(result==PST_RESULT_OK,5);
 result=attempt(descriptor,runtime,malformed,sizeof(malformed),intermediate,intermediate_size,key,key_size);
 CHECK(result==PST_RESULT_AUTH_FAILURE,6);
 result=attempt(descriptor,runtime,leaf,leaf_size,intermediate,intermediate_size,malformed,sizeof(malformed));
 CHECK(result==PST_RESULT_AUTH_FAILURE,7);
 result=attempt(descriptor,runtime,leaf,leaf_size,intermediate,intermediate_size,wrong_key,wrong_key_size);
 CHECK(result==PST_RESULT_AUTH_FAILURE,8);
 result=attempt(descriptor,runtime,leaf,leaf_size,malformed,sizeof(malformed),key,key_size);
 CHECK(result==PST_RESULT_AUTH_FAILURE,9);
 printf("NSS_SERVER_LOCAL_IDENTITY=PASS MISSING=PASS MALFORMED_LEAF=PASS MALFORMED_PKCS8=PASS KEY_MISMATCH=PASS MALFORMED_CHAIN=PASS KEY_MATCH_VALIDATION=PASS\n");
 ok=1;
done:
 pst_connection_config_snapshot_release(missing_snapshot);
 if(runtime)descriptor->vtable->runtime_destroy(runtime);
 if(backend)descriptor->vtable->shutdown(backend);
 free(leaf);free(intermediate);free(key);free(wrong_key);return ok?0:20;
}
