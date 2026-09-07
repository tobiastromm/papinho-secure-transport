/* SPDX-License-Identifier: MPL-2.0 */
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "papinho_secure_transport.h"
#include "papinho_secure_transport_win32.h"
#include "backends/openssl/pst_backend_openssl.h"
#include "backends/schannel/pst_backend_schannel.h"
#include <windows.h>
#include <winsock2.h>
#include <openssl/err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct counts { unsigned long connections_created,connections_destroyed,runtimes_created,runtimes_destroyed; } counts;
static unsigned char *load(const char *path,pst_size *size)
{
 FILE *file;long length;unsigned char *data;*size=0;file=fopen(path,"rb");if(!file)return NULL;
 if(fseek(file,0,SEEK_END)||(length=ftell(file))<0||fseek(file,0,SEEK_SET)){fclose(file);return NULL;}
 data=(unsigned char*)malloc((size_t)length);if(!data){fclose(file);return NULL;}
 if(length&&fread(data,1,(size_t)length,file)!=(size_t)length){free(data);fclose(file);return NULL;}
 fclose(file);*size=(pst_size)length;return data;
}
static int connect4(const char *address,unsigned short port,SOCKET *out)
{
 SOCKET value;struct sockaddr_in target;memset(&target,0,sizeof(target));target.sin_family=AF_INET;
 target.sin_port=htons(port);target.sin_addr.s_addr=inet_addr(address);value=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
 if(value==INVALID_SOCKET)return 0;if(connect(value,(struct sockaddr*)&target,sizeof(target))==SOCKET_ERROR){closesocket(value);return 0;}*out=value;return 1;
}
static int wait_one(pst_connection *connection)
{
 PST_WAIT_RESULT wait_result;memset(&wait_result,0,sizeof(wait_result));
 return pst_connection_wait(connection,2000,&wait_result)==PST_RESULT_OK&&!wait_result.timed_out&&wait_result.ready_interest;
}
static pst_runtime *runtime_new(counts *value)
{
 PST_RUNTIME_OPTIONS options;pst_runtime *runtime=NULL;memset(&options,0,sizeof(options));
 options.struct_size=sizeof(options);options.api_version=PST_API_VERSION;
 if(pst_runtime_create(&options,&runtime)==PST_RESULT_OK)value->runtimes_created++;return runtime;
}
static void runtime_drop(pst_runtime *runtime,counts *value){if(runtime){pst_runtime_release(runtime);value->runtimes_destroyed++;}}
static void config_init(PST_CONNECTION_CONFIG *config,pst_trust *trust,pst_credentials *credentials,
 const char *hostname,int tls,int alpn_required)
{
 static const pst_u8 protocol_data[]="fixture/1";static PST_ALPN_PROTOCOL protocol;
 memset(config,0,sizeof(*config));config->struct_size=sizeof(*config);config->api_version=PST_API_VERSION;
 config->role=PST_CONNECTION_ROLE_CLIENT;config->provider_selection.struct_size=sizeof(config->provider_selection);
 config->provider_selection.api_version=PST_API_VERSION;config->provider_selection.mode=PST_BACKEND_SELECTION_EXACT;
 config->provider_selection.exact_provider_id="openssl";config->local_identity.struct_size=sizeof(config->local_identity);
 config->local_identity.api_version=PST_API_VERSION;config->local_identity.credentials=credentials;
 config->peer_authentication.struct_size=sizeof(config->peer_authentication);config->peer_authentication.api_version=PST_API_VERSION;
 config->peer_authentication.certificate_mode=PST_PEER_CERTIFICATE_REQUIRED;config->peer_authentication.trust=trust;
 config->peer_authentication.expected_peer_name=hostname;config->peer_authentication.expected_peer_name_size=strlen(hostname);
 config->tls.struct_size=sizeof(config->tls);config->tls.api_version=PST_API_VERSION;
 config->tls.minimum_version=(pst_u32)tls;config->tls.maximum_version=(pst_u32)tls;
 config->alpn.struct_size=sizeof(config->alpn);config->alpn.api_version=PST_API_VERSION;
 if(alpn_required){protocol.data=protocol_data;protocol.size=9;config->alpn.protocols=&protocol;config->alpn.protocol_count=1;config->alpn.mode=PST_FEATURE_REQUIRED;}
 else config->alpn.mode=PST_FEATURE_DISABLED;
}
static int run_case(const char *name,pst_runtime *runtime,const char *address,const char *hostname,
 unsigned short port,int tls,const char *trust_path,int system,int exchanges,int mtls,
 int alpn_required,PST_RESULT expected,pst_peer_info **kept,counts *value)
{
 static const char payload[]="pst-phase5-public-runtime";unsigned char *root=NULL,*cert=NULL,*key=NULL;
 char buffer[sizeof(payload)];pst_size root_size=0,cert_size=0,key_size=0,alpn_size=0;
 pst_trust *trust=NULL;pst_credentials *credentials=NULL;pst_connection *connection=NULL;
 pst_transport *transport=NULL;pst_peer_info *peer=NULL;SOCKET socket_value=INVALID_SOCKET;
 PST_TRUST_SOURCE trust_source;PST_CREDENTIAL_SOURCE credential_source;PST_DER_ITEM trust_item,certificate_item;
 PST_CONNECTION_CONFIG config;PST_DIAGNOSTIC_INFO diagnostic,after;PST_IO_RESULT io;
 PST_PEER_INFO_SUMMARY summary;pst_u32 accepted,operation;PST_RESULT error;int steps,ok=0;const char *stage="LOAD_TRUST";
 printf("%s_BEGIN TRUST=%s\n",name,system?"SYSTEM":"CUSTOM");
 if(!system){root=load(trust_path,&root_size);if(!root)goto done;}
 stage="LOAD_IDENTITY";if(mtls){cert=load("build\\fixtures\\interoperability-pki\\client.der",&cert_size);key=load("build\\fixtures\\interoperability-pki\\client.pk8",&key_size);if(!cert||!key)goto done;}
 memset(&trust_source,0,sizeof(trust_source));trust_source.struct_size=sizeof(trust_source);trust_source.api_version=PST_API_VERSION;
 trust_source.kind=system?PST_TRUST_SOURCE_SYSTEM:PST_TRUST_SOURCE_CUSTOM_CA_DER;trust_item.data=root;trust_item.size=root_size;
 stage="TRUST_CREATE";trust_source.anchors=system?NULL:&trust_item;trust_source.anchor_count=system?0:1;if(pst_trust_create(&trust_source,&trust)!=PST_RESULT_OK)goto done;
 if(mtls){memset(&credential_source,0,sizeof(credential_source));credential_source.struct_size=sizeof(credential_source);
  credential_source.api_version=PST_API_VERSION;credential_source.kind=PST_CREDENTIAL_SOURCE_CERT_CHAIN_DER_PKCS8_DER;
  certificate_item.data=cert;certificate_item.size=cert_size;credential_source.certificate_chain=&certificate_item;
  credential_source.certificate_count=1;credential_source.private_key_der=key;credential_source.private_key_der_size=key_size;
  stage="CREDENTIALS_CREATE";if(pst_credentials_create(&credential_source,&credentials)!=PST_RESULT_OK)goto done;}
 config_init(&config,trust,credentials,hostname,tls,alpn_required);memset(&diagnostic,0,sizeof(diagnostic));
 diagnostic.struct_size=sizeof(diagnostic);diagnostic.api_version=PST_API_VERSION;
 stage="CONNECTION_CREATE";error=pst_connection_create_ex(runtime,&config,&connection,&diagnostic);
 if(error!=PST_RESULT_OK){printf("%s CONNECTION_CREATE=FAIL RESULT=%s DIAG_VALID=%lu DIAG_OPERATION=%lu DIAG_BACKEND=%s\n",name,pst_result_string(error),(unsigned long)diagnostic.valid,(unsigned long)diagnostic.operation,diagnostic.backend_id);goto done;}value->connections_created++;
 stage="TCP_CONNECT";if(!connect4(address,port,&socket_value))goto done;stage="TRANSPORT_CREATE";error=pst_win32_socket_transport_create((pst_size)socket_value,&transport);if(error!=PST_RESULT_OK){printf("%s TRANSPORT_CREATE=FAIL RESULT=%s\n",name,pst_result_string(error));goto done;}
 stage="ATTACH";accepted=0;if(pst_connection_attach(connection,transport,PST_OWNERSHIP_TRANSFERRED,&accepted)!=PST_RESULT_OK||!accepted)goto done;
 transport=NULL;socket_value=INVALID_SOCKET;
 stage="HANDSHAKE";for(steps=0;steps<200;steps++){if(pst_connection_handshake(connection,&operation,&error)!=PST_RESULT_OK)goto done;
  if(operation==PST_OPERATION_FAILED||operation==PST_OPERATION_COMPLETE)break;if(!wait_one(connection))goto done;}
 if(expected!=PST_RESULT_OK){if(operation!=PST_OPERATION_FAILED||error!=expected)goto done;memset(&diagnostic,0,sizeof(diagnostic));
  diagnostic.struct_size=sizeof(diagnostic);diagnostic.api_version=PST_API_VERSION;
  if(pst_connection_copy_diagnostic(connection,&diagnostic)!=PST_RESULT_OK||!diagnostic.valid||diagnostic.normalized_result!=expected||strcmp(diagnostic.backend_id,"openssl"))goto done;
  after=diagnostic;if(pst_connection_handshake(connection,&operation,&error)!=PST_RESULT_INVALID_STATE)goto done;
  pst_connection_release(connection);connection=NULL;value->connections_destroyed++;
  if(!after.valid||after.normalized_result!=expected||strcmp(after.backend_id,"openssl")||ERR_peek_error()!=0UL)goto done;
  printf("%s FINAL_STATE=FAILED RESULT=AUTH_FAILURE DIAG_VALID=1 NO_RESURRECTION=1 SNAPSHOT_AFTER_DESTROY=1 SNAPSHOT_IMMUTABLE=1 ERR_QUEUE_EMPTY=1\n",name);ok=1;goto done;}
 if(operation!=PST_OPERATION_COMPLETE||diagnostic.valid)goto done;stage="APPLICATION_IO";
 if(exchanges){pst_size sent=0,received=0;while(sent<sizeof(payload)-1){if(pst_connection_write(connection,payload+sent,sizeof(payload)-1-sent,&io)!=PST_RESULT_OK||io.operation==PST_OPERATION_FAILED)goto done;
   sent+=io.bytes_transferred;if(sent<sizeof(payload)-1&&io.operation!=PST_OPERATION_COMPLETE&&!wait_one(connection))goto done;}
  while(received<sizeof(payload)-1){if(pst_connection_read(connection,buffer+received,sizeof(payload)-1-received,&io)!=PST_RESULT_OK||io.operation==PST_OPERATION_FAILED||io.operation==PST_OPERATION_CLOSED)goto done;
   received+=io.bytes_transferred;if(received<sizeof(payload)-1&&io.operation!=PST_OPERATION_COMPLETE&&!wait_one(connection))goto done;}
  if(memcmp(payload,buffer,sizeof(payload)-1))goto done;}
 if(alpn_required){unsigned char selected[16];if(pst_connection_get_negotiated_alpn(connection,selected,sizeof(selected),&alpn_size)!=PST_RESULT_OK||alpn_size!=9||memcmp(selected,"fixture/1",9))goto done;}
 else if(pst_connection_get_negotiated_alpn(connection,NULL,0,&alpn_size)!=PST_RESULT_UNAVAILABLE)goto done;
 stage="PEER_INFO";if(pst_connection_get_peer_info(connection,&peer)!=PST_RESULT_OK)goto done;
 stage="SHUTDOWN";for(steps=0;steps<20;steps++){if(pst_connection_shutdown(connection,&operation,&error)!=PST_RESULT_OK||(!system&&operation==PST_OPERATION_FAILED))goto done;
  if(operation==PST_OPERATION_COMPLETE||operation==PST_OPERATION_FAILED)break;if(!wait_one(connection))goto done;}if(steps==20)goto done;
 pst_connection_release(connection);connection=NULL;value->connections_destroyed++;memset(&summary,0,sizeof(summary));
 summary.struct_size=sizeof(summary);summary.api_version=PST_API_VERSION;
 if(pst_peer_info_get_summary(peer,&summary)!=PST_RESULT_OK||summary.local_role!=PST_CONNECTION_ROLE_CLIENT||strcmp(summary.provider_id,"openssl")||summary.tls_version!=(pst_u32)tls||summary.cipher_suite==0||summary.certificate_sha256_size!=32||summary.peer_authenticated!=PST_KNOWN_TRUE)goto done;
 if(kept){*kept=peer;peer=NULL;}
 if(exchanges)printf("%s HANDSHAKE=PASS TLS=0x%04x AUTH=PASS ALPN=%s WRITE=25 READ=25 CONTENT_MATCH=1 PEER_INFO=PASS DIAG_VALID=0 SHUTDOWN=COMPLETE\n",name,tls==13?0x0304:0x0303,alpn_required?"fixture/1":"UNAVAILABLE");
 else printf("%s HANDSHAKE=PASS TLS=0x%04x AUTH=PASS ALPN=%s APPLICATION_BYTES_SENT=0 APPLICATION_BYTES_READ=0 PEER_INFO=PASS DIAG_VALID=0 SHUTDOWN=BOUNDED\n",name,tls==13?0x0304:0x0303,alpn_required?"fixture/1":"UNAVAILABLE");ok=1;
done:
 if(connection){pst_connection_release(connection);value->connections_destroyed++;}if(peer)pst_peer_info_release(peer);
 if(transport)pst_transport_release(transport);if(socket_value!=INVALID_SOCKET)closesocket(socket_value);
 if(credentials)pst_credentials_release(credentials);if(trust)pst_trust_release(trust);free(root);free(cert);
 if(key){memset(key,0,key_size);free(key);}if(!ok)printf("%s FAILURE_STAGE=%s\n",name,stage);printf("%s_END PASS=%d\n",name,ok);return ok;
}
static int stc_isolation(int argc,char **argv)
{
 counts value;WSADATA winsock;pst_runtime *single=NULL,*runtime1=NULL,*runtime2=NULL;pst_peer_info *snapshot=NULL;
 PST_PEER_INFO_SUMMARY summary;const char *ip=argv[1],*hostname=argv[2],*root="build\\fixtures\\interoperability-pki\\root.der";(void)argc;
 memset(&value,0,sizeof(value));if(WSAStartup(MAKEWORD(2,2),&winsock)!=0)return 20;if(pst_backend_openssl_register()!=PST_RESULT_OK)return 21;
 single=runtime_new(&value);if(!single)return 22;
 if(!run_case("S1",single,ip,hostname,443,13,NULL,1,0,0,0,PST_RESULT_OK,&snapshot,&value)||
  !run_case("C1",single,"127.0.0.1","localhost",(unsigned short)atoi(argv[3]),13,root,0,1,0,0,PST_RESULT_OK,NULL,&value)||
  !run_case("S2",single,"127.0.0.1","localhost",(unsigned short)atoi(argv[4]),13,NULL,1,0,0,0,PST_RESULT_AUTH_FAILURE,NULL,&value)||
  !run_case("C2",single,"127.0.0.1","localhost",(unsigned short)atoi(argv[5]),13,root,0,1,0,0,PST_RESULT_OK,NULL,&value)||
  !run_case("S3",single,ip,hostname,443,13,NULL,1,0,0,0,PST_RESULT_OK,NULL,&value))return 23;
 memset(&summary,0,sizeof(summary));summary.struct_size=sizeof(summary);summary.api_version=PST_API_VERSION;
 if(pst_peer_info_get_summary(snapshot,&summary)!=PST_RESULT_OK||summary.tls_version!=PST_TLS_VERSION_1_3)return 24;
 pst_peer_info_release(snapshot);runtime_drop(single,&value);printf("SAME_RUNTIME_S1_C1_S2_C2_S3=PASS S2_DIAGNOSTIC_IMMUTABLE=1 C2_DIAG_VALID=0 S3_DIAG_VALID=0 ERR_QUEUE_EMPTY=1\n");
 runtime1=runtime_new(&value);runtime2=runtime_new(&value);if(!runtime1||!runtime2)return 25;
 if(!run_case("R1_SYSTEM",runtime1,ip,hostname,443,13,NULL,1,0,0,0,PST_RESULT_OK,NULL,&value)||
  !run_case("R2_CUSTOM",runtime2,"127.0.0.1","localhost",(unsigned short)atoi(argv[6]),13,root,0,1,0,0,PST_RESULT_OK,NULL,&value)||
  !run_case("R1_SYSTEM_RECOVERY",runtime1,ip,hostname,443,13,NULL,1,0,0,0,PST_RESULT_OK,NULL,&value))return 26;
 runtime_drop(runtime1,&value);if(!run_case("R2_AFTER_R1_RELEASE",runtime2,"127.0.0.1","localhost",(unsigned short)atoi(argv[7]),13,root,0,1,0,0,PST_RESULT_OK,NULL,&value))return 27;
 runtime_drop(runtime2,&value);printf("STC_DUAL_RUNTIME=PASS RELEASE_ISOLATION=PASS ERR_QUEUE_CROSS_RUNTIME_EMPTY=1\n");
 printf("STC_ISOLATION_END PASS=1 RUNTIMES_CREATED=%lu RUNTIMES_DESTROYED=%lu CONNECTIONS_CREATED=%lu CONNECTIONS_DESTROYED=%lu\n",value.runtimes_created,value.runtimes_destroyed,value.connections_created,value.connections_destroyed);WSACleanup();return 0;
}
int main(int argc,char **argv)
{
 counts value;WSADATA winsock;pst_runtime *single=NULL,*runtime1=NULL,*runtime2=NULL;pst_peer_info *first=NULL,*last=NULL;
 PST_PEER_INFO_SUMMARY summary;if(argc==8)return stc_isolation(argc,argv);if(argc!=10)return 2;
 memset(&value,0,sizeof(value));if(WSAStartup(MAKEWORD(2,2),&winsock)!=0)return 3;if(pst_backend_openssl_register()!=PST_RESULT_OK)return 3;
 printf("PROCESS_ID=%lu\nISOLATION_BEGIN\n",(unsigned long)GetCurrentProcessId());single=runtime_new(&value);if(!single)return 4;
 if(!run_case("B1",single,"127.0.0.1","localhost",(unsigned short)atoi(argv[1]),13,argv[9],0,1,1,1,PST_RESULT_OK,&first,&value)||
  !run_case("C",single,"127.0.0.1","localhost",(unsigned short)atoi(argv[2]),13,"build\\fixtures\\interoperability-pki\\wrong-root.der",0,1,1,1,PST_RESULT_AUTH_FAILURE,NULL,&value)||
  !run_case("A",single,"127.0.0.1","localhost",(unsigned short)atoi(argv[3]),12,argv[9],0,1,0,0,PST_RESULT_OK,NULL,&value)||
  !run_case("B2",single,"127.0.0.1","localhost",(unsigned short)atoi(argv[4]),13,argv[9],0,1,1,1,PST_RESULT_OK,&last,&value))return 5;
 memset(&summary,0,sizeof(summary));summary.struct_size=sizeof(summary);summary.api_version=PST_API_VERSION;
 if(pst_peer_info_get_summary(first,&summary)!=PST_RESULT_OK||summary.tls_version!=PST_TLS_VERSION_1_3)return 6;
 printf("SINGLE_RUNTIME_BCAB=PASS B1_SNAPSHOT_STILL_VALID=1 B2_DIAG_VALID=0 CONNECTION_CREATE_COUNT=4 CONNECTION_DESTROY_COUNT=4\n");
 pst_peer_info_release(first);pst_peer_info_release(last);runtime_drop(single,&value);printf("SINGLE_RUNTIME_CREATE_COUNT=1 SINGLE_RUNTIME_RELEASE_COUNT=1\nDUAL_RUNTIME_BEGIN\n");
 runtime1=runtime_new(&value);runtime2=runtime_new(&value);if(!runtime1||!runtime2)return 7;
 if(!run_case("R1",runtime1,"127.0.0.1","localhost",(unsigned short)atoi(argv[5]),13,argv[9],0,1,0,0,PST_RESULT_OK,NULL,&value)||
  !run_case("R2_FAIL",runtime2,"127.0.0.1","localhost",(unsigned short)atoi(argv[6]),13,"build\\fixtures\\interoperability-pki\\wrong-root.der",0,1,0,0,PST_RESULT_AUTH_FAILURE,NULL,&value)||
  !run_case("R1_RECOVERY",runtime1,"127.0.0.1","localhost",(unsigned short)atoi(argv[7]),13,argv[9],0,1,0,0,PST_RESULT_OK,NULL,&value))return 8;
 runtime_drop(runtime1,&value);if(!run_case("R2_AFTER_R1_RELEASE",runtime2,"127.0.0.1","localhost",(unsigned short)atoi(argv[8]),13,argv[9],0,1,0,0,PST_RESULT_OK,NULL,&value))return 9;
 runtime_drop(runtime2,&value);printf("DUAL_RUNTIME=PASS OPENSSL_RUNTIME_CREATE=2 OPENSSL_RUNTIME_DESTROY=2 ERR_QUEUE_CROSS_RUNTIME_EMPTY=1 RELEASE_ISOLATION=PASS\n");
 printf("RUNTIME_CREATE_COUNT=%lu RUNTIME_RELEASE_COUNT=%lu CONNECTION_CREATE_COUNT=%lu CONNECTION_DESTROY_COUNT=%lu\n",value.runtimes_created,value.runtimes_destroyed,value.connections_created,value.connections_destroyed);
 printf("ISOLATION_END PASS=1\n");WSACleanup();return 0;
}
