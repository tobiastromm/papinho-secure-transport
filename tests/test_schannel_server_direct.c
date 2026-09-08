/* SPDX-License-Identifier: MPL-2.0 */
#include "papinho_secure_transport.h"
#include "pst_backend.h"
#include "pst_identity_internal.h"
#include "pst_transport_internal.h"
#include "backends/schannel/pst_backend_schannel.h"
#include <winsock2.h>
#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *stage="start";
#define CHECK(x) do{if(!(x)){fprintf(stderr,"SCHANNEL_SERVER_DIRECT=FAIL LINE=%d STAGE=%s WIN32=%lu\n",__LINE__,stage,(unsigned long)GetLastError());goto done;}}while(0)

static unsigned char *load(const char *path,pst_size *size)
{
    FILE *file;long length;unsigned char *data;
    *size=0;file=fopen(path,"rb");if(!file)return NULL;
    fseek(file,0,SEEK_END);length=ftell(file);fseek(file,0,SEEK_SET);
    data=(unsigned char*)malloc((size_t)length);
    if(!data||fread(data,1,(size_t)length,file)!=(size_t)length){free(data);fclose(file);return NULL;}
    fclose(file);*size=(pst_size)length;return data;
}
static PCCERT_CONTEXT find_intermediate(HCERTSTORE store,const unsigned char *der,pst_size size)
{
    PCCERT_CONTEXT certificate=NULL;
    while((certificate=CertEnumCertificatesInStore(store,certificate))!=NULL)if(certificate->cbCertEncoded==size&&!memcmp(certificate->pbCertEncoded,der,size))return certificate;
    return NULL;
}
static int intermediate_present(const unsigned char *der,pst_size size)
{
    HCERTSTORE store;PCCERT_CONTEXT certificate;int present;
    store=CertOpenStore(CERT_STORE_PROV_SYSTEM_A,0,0,CERT_SYSTEM_STORE_CURRENT_USER|CERT_STORE_OPEN_EXISTING_FLAG,"CA");if(!store)return 0;
    certificate=find_intermediate(store,der,size);present=certificate?1:0;if(certificate)CertFreeCertificateContext(certificate);CertCloseStore(store,0);return present;
}
static int intermediate_add(const unsigned char *der,pst_size size)
{
    HCERTSTORE store;PCCERT_CONTEXT certificate;int result=0;
    certificate=CertCreateCertificateContext(X509_ASN_ENCODING|PKCS_7_ASN_ENCODING,der,(DWORD)size);if(!certificate)return 0;
    store=CertOpenStore(CERT_STORE_PROV_SYSTEM_A,0,0,CERT_SYSTEM_STORE_CURRENT_USER|CERT_STORE_OPEN_EXISTING_FLAG,"CA");
    if(store){result=CertAddCertificateContextToStore(store,certificate,CERT_STORE_ADD_ALWAYS,NULL)?1:0;CertCloseStore(store,0);}CertFreeCertificateContext(certificate);return result;
}
static void intermediate_delete(const unsigned char *der,pst_size size)
{
    HCERTSTORE store;PCCERT_CONTEXT certificate;
    store=CertOpenStore(CERT_STORE_PROV_SYSTEM_A,0,0,CERT_SYSTEM_STORE_CURRENT_USER|CERT_STORE_OPEN_EXISTING_FLAG,"CA");if(!store)return;
    certificate=find_intermediate(store,der,size);if(certificate)CertDeleteCertificateFromStore(certificate);CertCloseStore(store,0);
}
static int wait_step(const PST_BACKEND_DESCRIPTOR *d,void *c)
{
    pst_u32 interest=0;PST_BACKEND_WAIT_RESULT wait_result;
    if(d->vtable->get_interest(c,&interest)!=PST_RESULT_OK||!interest)return 0;
    memset(&wait_result,0,sizeof(wait_result));
    return d->vtable->wait(c,interest,2000,&wait_result)==PST_RESULT_OK&&!wait_result.timed_out&&wait_result.ready_interest;
}
static int apply_mode(const char *value,int *preexisting,int *abrupt,int *use_alpn,int *use_system,int *expect_auth_failure,int *expect_identity_failure,int *missing_identity,int *expect_alpn_failure,int *alpn_optional,int *expect_no_alpn,int *peer_mode)
{
    if(!strcmp(value,"preexisting"))*preexisting=1;else if(!strcmp(value,"abrupt"))*abrupt=1;else if(!strcmp(value,"alpn"))*use_alpn=1;else if(!strcmp(value,"alpn-required-fail")){*use_alpn=1;*expect_alpn_failure=1;}else if(!strcmp(value,"alpn-optional-none")){*use_alpn=1;*alpn_optional=1;*expect_no_alpn=1;}else if(!strcmp(value,"system"))*use_system=1;else if(!strcmp(value,"disabled"))*peer_mode=PST_PEER_CERTIFICATE_DISABLED;else if(!strcmp(value,"optional"))*peer_mode=PST_PEER_CERTIFICATE_OPTIONAL;else if(!strcmp(value,"optional-fail")){*peer_mode=PST_PEER_CERTIFICATE_OPTIONAL;*expect_auth_failure=1;}else if(!strcmp(value,"required-fail"))*expect_auth_failure=1;else if(!strcmp(value,"identity-fail"))*expect_identity_failure=1;else if(!strcmp(value,"missing-identity"))*missing_identity=1;else return 0;return 1;
}
int main(int argc,char **argv)
{
    const PST_BACKEND_DESCRIPTOR *d=pst_backend_schannel_descriptor();
    void *backend=NULL,*runtime=NULL,*connection=NULL,*shared_connection=NULL,*peer_info=NULL;
    PST_CREDENTIAL_SOURCE credential_source;PST_TRUST_SOURCE trust_source;
    PST_DER_ITEM chain[2],anchor;PST_ALPN_PROTOCOL alpn[2];PST_CONNECTION_CONFIG config;
    pst_connection_config_snapshot *snapshot=NULL;PST_BACKEND_CONNECTION_OPTIONS options;
    PST_NATIVE_TRANSPORT native;pst_credentials *credentials=NULL;pst_trust *trust=NULL;
    unsigned char *leaf=NULL,*intermediate=NULL,*key=NULL,*root=NULL,buffer[25];
    pst_size leaf_size,intermediate_size,key_size,root_size,total=0;
    WSADATA winsock_data;SOCKET listener=INVALID_SOCKET,socket_value=INVALID_SOCKET;
    struct sockaddr_in address;pst_u32 accepted=0,operation=0,steps=0;
    PST_RESULT error=PST_RESULT_OK,result;PST_BACKEND_IO_RESULT io;PST_PEER_INFO_SUMMARY peer_summary;unsigned char alpn_value[32];pst_size alpn_size=0;int ok=0,winsock_started=0,preexisting=0,abrupt=0,use_alpn=0,use_system=0,expect_auth_failure=0,expect_identity_failure=0,missing_identity=0,expect_alpn_failure=0,alpn_optional=0,expect_no_alpn=0;int peer_mode=PST_PEER_CERTIFICATE_REQUIRED;pst_internal_diagnostic diagnostic;

    if(argc<7||argc>9||(strcmp(argv[2],"12")&&strcmp(argv[2],"13")))return 2;
    {int argument;for(argument=7;argument<argc;argument++)if(!apply_mode(argv[argument],&preexisting,&abrupt,&use_alpn,&use_system,&expect_auth_failure,&expect_identity_failure,&missing_identity,&expect_alpn_failure,&alpn_optional,&expect_no_alpn,&peer_mode))return 2;}
    stage="load";leaf=load(argv[3],&leaf_size);intermediate=load(argv[4],&intermediate_size);
    key=load(argv[5],&key_size);root=load(argv[6],&root_size);CHECK(leaf&&intermediate&&key&&root);
    memset(&credential_source,0,sizeof(credential_source));
    credential_source.struct_size=sizeof(credential_source);credential_source.api_version=PST_API_VERSION;
    credential_source.kind=PST_CREDENTIAL_SOURCE_CERT_CHAIN_DER_PKCS8_DER;
    chain[0].data=leaf;chain[0].size=leaf_size;chain[1].data=intermediate;chain[1].size=intermediate_size;
    credential_source.certificate_chain=chain;credential_source.certificate_count=2;
    credential_source.private_key_der=key;credential_source.private_key_der_size=key_size;
    stage="credentials";CHECK(pst_credentials_create(&credential_source,&credentials)==PST_RESULT_OK);
    memset(&trust_source,0,sizeof(trust_source));trust_source.struct_size=sizeof(trust_source);
    trust_source.api_version=PST_API_VERSION;trust_source.kind=use_system?PST_TRUST_SOURCE_SYSTEM:PST_TRUST_SOURCE_CUSTOM_CA_DER;
    anchor.data=root;anchor.size=root_size;if(!use_system){trust_source.anchors=&anchor;trust_source.anchor_count=1;}
    stage="trust";if(peer_mode!=PST_PEER_CERTIFICATE_DISABLED)CHECK(pst_trust_create(&trust_source,&trust)==PST_RESULT_OK);
    memset(&config,0,sizeof(config));config.struct_size=sizeof(config);config.api_version=PST_API_VERSION;
    config.role=PST_CONNECTION_ROLE_SERVER;
    config.provider_selection.struct_size=sizeof(config.provider_selection);config.provider_selection.api_version=PST_API_VERSION;
    config.provider_selection.mode=PST_BACKEND_SELECTION_EXACT;config.provider_selection.exact_provider_id="schannel";
    config.local_identity.struct_size=sizeof(config.local_identity);config.local_identity.api_version=PST_API_VERSION;
    config.local_identity.credentials=missing_identity?NULL:credentials;
    config.peer_authentication.struct_size=sizeof(config.peer_authentication);config.peer_authentication.api_version=PST_API_VERSION;
    config.peer_authentication.certificate_mode=(pst_u32)peer_mode;config.peer_authentication.trust=trust;
    config.tls.struct_size=sizeof(config.tls);config.tls.api_version=PST_API_VERSION;
    config.tls.minimum_version=!strcmp(argv[2],"12")?PST_TLS_VERSION_1_2:PST_TLS_VERSION_1_3;config.tls.maximum_version=config.tls.minimum_version;
    config.alpn.struct_size=sizeof(config.alpn);config.alpn.api_version=PST_API_VERSION;config.alpn.mode=PST_FEATURE_DISABLED;
    if(use_alpn){alpn[0].data=(const pst_u8*)"http/1.1";alpn[0].size=8;alpn[1].data=(const pst_u8*)"h2";alpn[1].size=2;config.alpn.mode=alpn_optional?PST_FEATURE_OPTIONAL:PST_FEATURE_REQUIRED;config.alpn.protocols=alpn;config.alpn.protocol_count=2;}
    stage="snapshot";result=pst_connection_config_snapshot_create(&config,&snapshot);if(missing_identity){CHECK(result==PST_RESULT_POLICY_VIOLATION&&snapshot==NULL);printf("SCHANNEL_SERVER_IDENTITY_NEGATIVE CASE=MISSING RESULT=POLICY_VIOLATION NORMAL_RESIDUE=0 PASS=1\n");ok=1;goto done;}CHECK(result==PST_RESULT_OK);
    if(preexisting){stage="preexisting-add";CHECK(intermediate_add(intermediate,intermediate_size));}
    stage="backend";CHECK(d->vtable->initialize(&backend)==PST_RESULT_OK);
    CHECK(d->vtable->runtime_create(backend,&runtime)==PST_RESULT_OK);
    memset(&options,0,sizeof(options));options.struct_size=sizeof(options);options.spi_version=PST_BACKEND_SPI_VERSION;
    options.role=PST_CONNECTION_ROLE_SERVER;options.configuration=pst_connection_config_snapshot_public(snapshot);
    options.required_capabilities=pst_connection_config_required_capabilities(snapshot);
    stage="connection-create";result=d->vtable->connection_create(runtime,&options,&connection);if(expect_identity_failure){CHECK(result==PST_RESULT_AUTH_FAILURE&&connection==NULL&&!intermediate_present(intermediate,intermediate_size));printf("SCHANNEL_SERVER_IDENTITY_NEGATIVE CASE=INVALID RESULT=AUTH_FAILURE NORMAL_RESIDUE=0 PASS=1\n");ok=1;goto done;}CHECK(result==PST_RESULT_OK);
    CHECK(intermediate_present(intermediate,intermediate_size));
    stage="shared-create";CHECK(d->vtable->connection_create(runtime,&options,&shared_connection)==PST_RESULT_OK);
    d->vtable->connection_destroy(shared_connection);shared_connection=NULL;
    stage="shared-release";CHECK(intermediate_present(intermediate,intermediate_size));
    stage="listen";CHECK(!WSAStartup(MAKEWORD(2,2),&winsock_data));winsock_started=1;
    listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);CHECK(listener!=INVALID_SOCKET);
    memset(&address,0,sizeof(address));address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    address.sin_port=htons((u_short)atoi(argv[1]));CHECK(!bind(listener,(struct sockaddr*)&address,sizeof(address))&&!listen(listener,1));
    printf("READY PORT=%s TLS=%s ROLE=SERVER CAPABILITY_PUBLISHED=0\n",argv[1],argv[2]);fflush(stdout);
    socket_value=accept(listener,NULL,NULL);CHECK(socket_value!=INVALID_SOCKET);
    memset(&native,0,sizeof(native));native.struct_size=sizeof(native);native.version=PST_NATIVE_TRANSPORT_VERSION;
    native.kind=PST_NATIVE_TRANSPORT_KIND_WIN32_SOCKET;native.native_socket=(pst_size)socket_value;
    stage="attach";CHECK(d->vtable->attach_transport(connection,&native,PST_BACKEND_OWNERSHIP_TRANSFERRED,&accepted)==PST_RESULT_OK&&accepted);
    socket_value=INVALID_SOCKET;stage="handshake";
    for(steps=0;steps<200;steps++){CHECK(d->vtable->handshake_step(connection,&operation,&error)==PST_RESULT_OK);if(operation==PST_BACKEND_OPERATION_COMPLETE||operation==PST_BACKEND_OPERATION_FAILED)break;CHECK(wait_step(d,connection));}
    if(expect_auth_failure){stage="expected-auth-failure";CHECK(operation==PST_BACKEND_OPERATION_FAILED&&error==PST_RESULT_AUTH_FAILURE);memset(&diagnostic,0,sizeof(diagnostic));d->vtable->diagnostic_copy(connection,&diagnostic);CHECK(diagnostic.valid&&diagnostic.result==PST_RESULT_AUTH_FAILURE&&(diagnostic.reason==PST_DIAGNOSTIC_REASON_PEER_CERT_ABSENT||diagnostic.reason==PST_DIAGNOSTIC_REASON_PEER_CERT_INVALID||diagnostic.reason==PST_DIAGNOSTIC_REASON_PEER_CERT_UNTRUSTED));CHECK(d->vtable->handshake_step(connection,&operation,&error)==PST_RESULT_OK&&operation==PST_BACKEND_OPERATION_FAILED);d->vtable->connection_destroy(connection);connection=NULL;stage="failure-cleanup";CHECK(intermediate_present(intermediate,intermediate_size)==preexisting);printf("SCHANNEL_SERVER_EXPECTED_AUTH_FAILURE PEER_MODE=%d REASON=%lu TERMINAL_NO_RESURRECTION=1 NORMAL_RESIDUE=0 PASS=1\n",peer_mode,(unsigned long)diagnostic.reason);ok=1;goto done;}
    if(expect_alpn_failure){stage="expected-alpn-failure";memset(&diagnostic,0,sizeof(diagnostic));d->vtable->diagnostic_copy(connection,&diagnostic);fprintf(stderr,"ALPN_FAILURE_OBSERVED OP=%lu ERROR=%ld PHASE=%lu RESULT=%ld NATIVE=%ld\n",(unsigned long)operation,(long)error,(unsigned long)diagnostic.phase,(long)diagnostic.result,(long)diagnostic.native_code);CHECK(operation==PST_BACKEND_OPERATION_FAILED&&(error==PST_RESULT_POLICY_VIOLATION||error==PST_RESULT_PROTOCOL_FAILURE));CHECK(diagnostic.valid&&diagnostic.phase==PST_DIAGNOSTIC_PHASE_ALPN);d->vtable->connection_destroy(connection);connection=NULL;CHECK(!intermediate_present(intermediate,intermediate_size));printf("SCHANNEL_SERVER_ALPN_NEGATIVE MODE=REQUIRED REASON=ALPN_MISMATCH TERMINAL=FAILED NORMAL_RESIDUE=0 PASS=1\n");ok=1;goto done;}
    CHECK(operation==PST_BACKEND_OPERATION_COMPLETE);if(peer_mode==PST_PEER_CERTIFICATE_REQUIRED){stage="peer-info";CHECK(d->vtable->peer_info_create(connection,&peer_info)==PST_RESULT_OK);memset(&peer_summary,0,sizeof(peer_summary));peer_summary.struct_size=sizeof(peer_summary);peer_summary.api_version=PST_API_VERSION;CHECK(pst_peer_info_get_summary((pst_peer_info*)peer_info,&peer_summary)==PST_RESULT_OK&&peer_summary.local_role==PST_CONNECTION_ROLE_SERVER&&peer_summary.peer_name_validated==PST_KNOWN_NOT_APPLICABLE&&peer_summary.peer_authenticated==PST_KNOWN_TRUE);d->vtable->peer_info_destroy(peer_info);peer_info=NULL;}if(use_alpn){stage="alpn";result=d->vtable->connection_get_alpn(connection,alpn_value,sizeof(alpn_value),&alpn_size);if(expect_no_alpn)CHECK(result==PST_RESULT_UNAVAILABLE);else CHECK(result==PST_RESULT_OK&&alpn_size==8&&!memcmp(alpn_value,"http/1.1",8));}stage="read";
    while(total<sizeof(buffer)){CHECK(d->vtable->read(connection,buffer+total,sizeof(buffer)-total,&io)==PST_RESULT_OK);CHECK(io.operation!=PST_BACKEND_OPERATION_FAILED&&io.operation!=PST_BACKEND_OPERATION_CLOSED);total+=io.bytes_transferred;if(total<sizeof(buffer))CHECK(wait_step(d,connection));}
    stage="write";do{CHECK(d->vtable->write(connection,buffer,sizeof(buffer),&io)==PST_RESULT_OK);CHECK(io.operation!=PST_BACKEND_OPERATION_FAILED);if(io.operation!=PST_BACKEND_OPERATION_COMPLETE)CHECK(wait_step(d,connection));}while(io.operation!=PST_BACKEND_OPERATION_COMPLETE);
    stage="peer-close";do{CHECK(d->vtable->read(connection,buffer,sizeof(buffer),&io)==PST_RESULT_OK);if(abrupt){CHECK(io.operation!=PST_BACKEND_OPERATION_CLOSED);if(io.operation==PST_BACKEND_OPERATION_FAILED)break;}else CHECK(io.operation!=PST_BACKEND_OPERATION_FAILED);if(io.operation!=PST_BACKEND_OPERATION_CLOSED&&io.operation!=PST_BACKEND_OPERATION_FAILED)CHECK(wait_step(d,connection));}while(io.operation!=PST_BACKEND_OPERATION_CLOSED&&io.operation!=PST_BACKEND_OPERATION_FAILED);
    if(abrupt){CHECK(io.operation==PST_BACKEND_OPERATION_FAILED&&io.close_kind==PST_BACKEND_CLOSE_TRUNCATED&&io.error==PST_RESULT_TRUNCATED);}
    else{CHECK(io.close_kind==PST_BACKEND_CLOSE_CLEAN);stage="reciprocal-close";do{CHECK(d->vtable->shutdown_step(connection,&operation,&error)==PST_RESULT_OK);CHECK(operation!=PST_BACKEND_OPERATION_FAILED);if(operation!=PST_BACKEND_OPERATION_COMPLETE)CHECK(wait_step(d,connection));}while(operation!=PST_BACKEND_OPERATION_COMPLETE);}
    d->vtable->connection_destroy(connection);connection=NULL;
    stage="last-reference";CHECK(intermediate_present(intermediate,intermediate_size)==preexisting);
    if(preexisting){intermediate_delete(intermediate,intermediate_size);CHECK(!intermediate_present(intermediate,intermediate_size));}
    printf("SCHANNEL_SERVER_DIRECT TLS=%s TRUST=%s PEER_MODE=%d READ=25 WRITE=25 CONTENT_MATCH=1 PEER_CLOSE=%s RECIPROCAL_CLOSE=%d ALPN=%s SHARED_REFCOUNT=1 PREEXISTING_PRESERVED=%d NORMAL_RESIDUE=0 HANDSHAKE_STEPS=%lu PASS=1\n",!strcmp(argv[2],"12")?"0x0303":"0x0304",peer_mode==PST_PEER_CERTIFICATE_DISABLED?"NONE":(use_system?"SYSTEM":"CUSTOM"),peer_mode,abrupt?"TRUNCATED":"CLEAN",abrupt?0:1,use_alpn?"http/1.1":"NONE",preexisting,(unsigned long)steps+1);ok=1;
done:
    if(peer_info)d->vtable->peer_info_destroy(peer_info);
    if(shared_connection)d->vtable->connection_destroy(shared_connection);
    if(connection)d->vtable->connection_destroy(connection);if(runtime)d->vtable->runtime_destroy(runtime);if(backend)d->vtable->shutdown(backend);
    if(preexisting&&intermediate_present(intermediate,intermediate_size))intermediate_delete(intermediate,intermediate_size);
    pst_connection_config_snapshot_release(snapshot);pst_trust_release(trust);pst_credentials_release(credentials);
    if(socket_value!=INVALID_SOCKET)closesocket(socket_value);if(listener!=INVALID_SOCKET)closesocket(listener);if(winsock_started)WSACleanup();
    free(leaf);free(intermediate);free(key);free(root);return ok?0:20;
}
