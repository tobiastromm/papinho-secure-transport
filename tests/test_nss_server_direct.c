/* SPDX-License-Identifier: MPL-2.0 */
#include "papinho_secure_transport.h"
#include "pst_backend.h"
#include "pst_identity_internal.h"
#include "pst_diagnostic.h"
#include "pst_transport_internal.h"
#include "backends/nss/pst_backend_nss.h"
#if defined(_MSC_VER) && _MSC_VER == 1200
#pragma warning(push)
#pragma warning(disable:4115)
#endif
#include <winsock.h>
#if defined(_MSC_VER) && _MSC_VER == 1200
#pragma warning(pop)
#pragma warning(disable:4514)
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) if (!(x)) { fprintf(stderr,"NSS_SERVER_DIRECT=FAIL LINE=%d\n",__LINE__); fflush(stderr); goto done; }

static void trace(const char *s) { printf("NSS_SERVER_TRACE=%s\n",s); fflush(stdout); }
static LONG WINAPI crash_report(EXCEPTION_POINTERS *exception)
{
 MEMORY_BASIC_INFORMATION memory;
 char module[MAX_PATH];
 void *address=exception->ExceptionRecord->ExceptionAddress;
 memset(&memory,0,sizeof(memory));memset(module,0,sizeof(module));
 if(VirtualQuery(address,&memory,sizeof(memory))!=0)
  GetModuleFileNameA((HMODULE)memory.AllocationBase,module,sizeof(module));
 fprintf(stderr,"NSS_SERVER_EXCEPTION CODE=0x%08lx ADDRESS=0x%08lx BASE=0x%08lx MODULE=%s\n",
  (unsigned long)exception->ExceptionRecord->ExceptionCode,
  (unsigned long)address,(unsigned long)memory.AllocationBase,module);
 fflush(stderr);return EXCEPTION_EXECUTE_HANDLER;
}
static void trace_module(const char *name)
{
 HMODULE module=GetModuleHandleA(name);
 char path[MAX_PATH];path[0]='\0';
 if(module)GetModuleFileNameA(module,path,sizeof(path));
 printf("NSS_SERVER_MODULE=%s BASE=0x%08lx PATH=%s\n",name,
  (unsigned long)module,path[0]?path:"NOT_LOADED");
 fflush(stdout);
}
static unsigned char *load_file(const char *p,pst_size *n)
{
 FILE *f;long z;unsigned char *d;*n=0;f=fopen(p,"rb");if(!f)return NULL;
 if(fseek(f,0,SEEK_END)||((z=ftell(f))<=0)||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}
 d=(unsigned char*)malloc((size_t)z);if(!d||fread(d,1,(size_t)z,f)!=(size_t)z){free(d);fclose(f);return NULL;}
 fclose(f);*n=(pst_size)z;return d;
}
static int split_alpn(char *text,PST_ALPN_PROTOCOL *protocols,pst_size *count)
{
 char *cursor=text,*comma;*count=0;if(!strcmp(text,"-"))return 1;
 while(*cursor&&*count<4){comma=strchr(cursor,',');if(comma)*comma='\0';
  protocols[*count].data=(const pst_u8*)cursor;protocols[*count].size=strlen(cursor);
  if(!protocols[*count].size||protocols[*count].size>255)return 0;(*count)++;
  if(!comma)return 1;cursor=comma+1;}return *cursor=='\0';
}
static int wait_for(const PST_BACKEND_DESCRIPTOR *d,void *c)
{
 pst_u32 interest=0;PST_BACKEND_WAIT_RESULT w;PST_RESULT r;
 r=d->vtable->get_interest(c,&interest);printf("NSS_SERVER_WAIT INTEREST=%lu GET_RESULT=%ld\n",(unsigned long)interest,(long)r);fflush(stdout);
 if(r!=PST_RESULT_OK||!interest)return 0;memset(&w,0,sizeof(w));r=d->vtable->wait(c,interest,2000,&w);
 printf("NSS_SERVER_WAIT RESULT=%ld READY=%lu TIMEOUT=%lu\n",(long)r,(unsigned long)w.ready_interest,(unsigned long)w.timed_out);fflush(stdout);
 return r==PST_RESULT_OK&&!w.timed_out&&w.ready_interest;
}
static int wait_for_read(const PST_BACKEND_DESCRIPTOR *d,void *c)
{
 PST_BACKEND_WAIT_RESULT w;PST_RESULT r;memset(&w,0,sizeof(w));
 r=d->vtable->wait(c,PST_BACKEND_INTEREST_READ,2000,&w);
 printf("NSS_SERVER_WAIT_PRIMARY_READ RESULT=%ld READY=%lu TIMEOUT=%lu\n",(long)r,(unsigned long)w.ready_interest,(unsigned long)w.timed_out);fflush(stdout);
 return r==PST_RESULT_OK&&!w.timed_out&&(w.ready_interest&PST_BACKEND_INTEREST_READ)!=0;
}
int main(int ac,char **av)
{
 const PST_BACKEND_DESCRIPTOR *d=pst_backend_nss_descriptor();void *b=NULL,*r=NULL,*c=NULL,*peer=NULL;
 PST_CREDENTIAL_SOURCE cs;PST_TRUST_SOURCE ts;PST_CONNECTION_CONFIG cfg;PST_DER_ITEM chain[2],anchor;PST_ALPN_PROTOCOL protocols[4];pst_connection_config_snapshot *snap=NULL;
 PST_BACKEND_CONNECTION_OPTIONS o;PST_NATIVE_TRANSPORT native;PST_BACKEND_IO_RESULT io;PST_PEER_INFO_SUMMARY summary;pst_internal_diagnostic diagnostic;PST_RESULT result,error=PST_RESULT_OK,expected=PST_RESULT_OK;
 unsigned char *leaf=NULL,*intermediate=NULL,*key=NULL,*ca=NULL,buffer[25],negotiated[256];pst_size ln,in,kn,can,total=0,protocol_count=0,negotiated_size=0;
 WSADATA wd;SOCKET listener=INVALID_SOCKET,accepted=INVALID_SOCKET;struct sockaddr_in a;
 pst_u32 owned=0,op=0,i,peer_mode,expected_reason=0,alpn_mode=PST_FEATURE_DISABLED;int ok=0,wsa=0,tls13;char alpn_text[512];const char *expected_alpn="-",*close_mode="clean";
 SetErrorMode(SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(crash_report);
 memset(&io,0,sizeof(io));
 if(ac!=8&&ac!=10&&ac!=13&&ac!=14)return 2;if(ac>=10){expected=(PST_RESULT)atoi(av[8]);expected_reason=(pst_u32)atoi(av[9]);}if(ac>=13){alpn_mode=(pst_u32)atoi(av[10]);CHECK(strlen(av[11])<sizeof(alpn_text));strcpy(alpn_text,av[11]);expected_alpn=av[12];CHECK(split_alpn(alpn_text,protocols,&protocol_count));}if(ac==14)close_mode=av[13];CHECK(!strcmp(close_mode,"clean")||!strcmp(close_mode,"raw-abrupt")||!strcmp(close_mode,"data-abrupt"));tls13=atoi(av[2])==13;peer_mode=(pst_u32)atoi(av[3]);leaf=load_file(av[4],&ln);intermediate=load_file(av[5],&in);key=load_file(av[6],&kn);ca=load_file(av[7],&can);CHECK(leaf&&intermediate&&key&&ca);CHECK(peer_mode<=PST_PEER_CERTIFICATE_REQUIRED&&alpn_mode<=PST_FEATURE_REQUIRED);
 memset(&cs,0,sizeof(cs));cs.struct_size=sizeof(cs);cs.api_version=PST_API_VERSION;cs.kind=PST_CREDENTIAL_SOURCE_CERT_CHAIN_DER_PKCS8_DER;
 chain[0].data=leaf;chain[0].size=ln;chain[1].data=intermediate;chain[1].size=in;cs.certificate_chain=chain;cs.certificate_count=2;cs.private_key_der=key;cs.private_key_der_size=kn;
 {pst_credentials *cred=NULL;pst_trust *trust=NULL;CHECK(pst_credentials_create(&cs,&cred)==PST_RESULT_OK);if(peer_mode!=PST_PEER_CERTIFICATE_DISABLED){memset(&ts,0,sizeof(ts));ts.struct_size=sizeof(ts);ts.api_version=PST_API_VERSION;ts.kind=PST_TRUST_SOURCE_CUSTOM_CA_DER;anchor.data=ca;anchor.size=can;ts.anchors=&anchor;ts.anchor_count=1;CHECK(pst_trust_create(&ts,&trust)==PST_RESULT_OK);}memset(&cfg,0,sizeof(cfg));cfg.struct_size=sizeof(cfg);cfg.api_version=PST_API_VERSION;cfg.role=PST_CONNECTION_ROLE_SERVER;
 cfg.provider_selection.struct_size=sizeof(cfg.provider_selection);cfg.provider_selection.api_version=PST_API_VERSION;cfg.provider_selection.mode=PST_BACKEND_SELECTION_EXACT;cfg.provider_selection.exact_provider_id="retrozilla-nss";
 cfg.local_identity.struct_size=sizeof(cfg.local_identity);cfg.local_identity.api_version=PST_API_VERSION;cfg.local_identity.credentials=cred;
 cfg.peer_authentication.struct_size=sizeof(cfg.peer_authentication);cfg.peer_authentication.api_version=PST_API_VERSION;cfg.peer_authentication.certificate_mode=peer_mode;cfg.peer_authentication.trust=trust;
 cfg.tls.struct_size=sizeof(cfg.tls);cfg.tls.api_version=PST_API_VERSION;cfg.tls.minimum_version=tls13?PST_TLS_VERSION_1_3:PST_TLS_VERSION_1_2;cfg.tls.maximum_version=cfg.tls.minimum_version;
 cfg.alpn.struct_size=sizeof(cfg.alpn);cfg.alpn.api_version=PST_API_VERSION;cfg.alpn.mode=alpn_mode;cfg.alpn.protocols=protocol_count?protocols:NULL;cfg.alpn.protocol_count=protocol_count;
 CHECK(pst_connection_config_snapshot_create(&cfg,&snap)==PST_RESULT_OK);pst_credentials_release(cred);pst_trust_release(trust);}
 trace("BACKEND_INITIALIZE_BEGIN");CHECK(d->vtable->initialize(&b)==PST_RESULT_OK);CHECK(d->vtable->runtime_create(b,&r)==PST_RESULT_OK);
 memset(&o,0,sizeof(o));o.struct_size=sizeof(o);o.spi_version=PST_BACKEND_SPI_VERSION;o.role=PST_CONNECTION_ROLE_SERVER;o.configuration=pst_connection_config_snapshot_public(snap);o.required_capabilities=pst_connection_config_required_capabilities(snap);
 CHECK(d->vtable->connection_create(r,&o,&c)==PST_RESULT_OK);trace("BACKEND_INITIALIZE_END");CHECK(WSAStartup(MAKEWORD(2,0),&wd)==0);wsa=1;
 listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);CHECK(listener!=INVALID_SOCKET);memset(&a,0,sizeof(a));a.sin_family=AF_INET;a.sin_addr.s_addr=htonl(INADDR_ANY);a.sin_port=htons((u_short)atoi(av[1]));
 CHECK(bind(listener,(struct sockaddr*)&a,sizeof(a))==0);CHECK(listen(listener,1)==0);printf("READY ADDRESS=0.0.0.0 PORT=%s TLS=%s ROLE=SERVER CAPABILITY_PUBLISHED=1 DIRECT_PROVIDER_TEST=1\n",av[1],av[2]);fflush(stdout);
 accepted=accept(listener,NULL,NULL);CHECK(accepted!=INVALID_SOCKET);trace("ACCEPTED");memset(&native,0,sizeof(native));native.struct_size=sizeof(native);native.version=PST_NATIVE_TRANSPORT_VERSION;native.kind=PST_NATIVE_TRANSPORT_KIND_WIN32_SOCKET;native.native_socket=(pst_size)accepted;
 trace("ATTACH_BEGIN");result=d->vtable->attach_transport(c,&native,PST_BACKEND_OWNERSHIP_TRANSFERRED,&owned);printf("NSS_SERVER_ATTACH RESULT=%ld OWNERSHIP=%lu\n",(long)result,(unsigned long)owned);fflush(stdout);CHECK(result==PST_RESULT_OK&&owned);accepted=INVALID_SOCKET;trace("ATTACH_END");trace_module("ssl3.dll");trace_module("nss3.dll");trace_module("nssutil3.dll");trace_module("nspr4.dll");trace_module("plc4.dll");trace_module("plds4.dll");trace_module("softokn3.dll");trace_module("freebl3.dll");
 for(i=0;i<200;i++){result=d->vtable->handshake_step(c,&op,&error);printf("NSS_SERVER_HANDSHAKE STEP=%lu RESULT=%ld OP=%lu ERROR=%ld\n",(unsigned long)(i+1),(long)result,(unsigned long)op,(long)error);fflush(stdout);CHECK(result==PST_RESULT_OK);if(op==PST_BACKEND_OPERATION_COMPLETE)break;if(op==PST_BACKEND_OPERATION_FAILED){memset(&diagnostic,0,sizeof(diagnostic));d->vtable->diagnostic_copy(c,&diagnostic);printf("NSS_SERVER_FAILURE_DIAGNOSTIC RESULT=%ld REASON=%lu PHASE=%lu ROLE=%lu\n",(long)diagnostic.result,(unsigned long)diagnostic.reason,(unsigned long)diagnostic.phase,(unsigned long)diagnostic.role);CHECK(error==expected&&expected!=PST_RESULT_OK);CHECK(!expected_reason||diagnostic.reason==expected_reason);printf("NSS_SERVER_EXPECTED_FAILURE RESULT=%ld REASON=%lu PASS=1\n",(long)expected,(unsigned long)diagnostic.reason);ok=1;goto done;}CHECK(wait_for(d,c));}CHECK(op==PST_BACKEND_OPERATION_COMPLETE&&expected==PST_RESULT_OK);
 CHECK(d->vtable->peer_info_create(c,&peer)==PST_RESULT_OK);memset(&summary,0,sizeof(summary));summary.struct_size=sizeof(summary);summary.api_version=PST_API_VERSION;CHECK(pst_peer_info_get_summary((pst_peer_info*)peer,&summary)==PST_RESULT_OK);CHECK(summary.local_role==PST_CONNECTION_ROLE_SERVER&&summary.peer_name_validated==PST_KNOWN_NOT_APPLICABLE);if(peer_mode==PST_PEER_CERTIFICATE_DISABLED){CHECK(summary.certificate_present==PST_KNOWN_FALSE&&summary.peer_authenticated==PST_KNOWN_FALSE);}else if(peer_mode==PST_PEER_CERTIFICATE_REQUIRED){CHECK(summary.certificate_present==PST_KNOWN_TRUE&&summary.peer_authenticated==PST_KNOWN_TRUE&&summary.chain_validated==PST_KNOWN_TRUE);}else{CHECK((summary.certificate_present==PST_KNOWN_FALSE&&summary.peer_authenticated==PST_KNOWN_FALSE)||(summary.certificate_present==PST_KNOWN_TRUE&&summary.peer_authenticated==PST_KNOWN_TRUE&&summary.chain_validated==PST_KNOWN_TRUE));}d->vtable->peer_info_destroy(peer);peer=NULL;if(strcmp(expected_alpn,"-")){CHECK(d->vtable->connection_get_alpn(c,negotiated,sizeof(negotiated),&negotiated_size)==PST_RESULT_OK);CHECK(negotiated_size==strlen(expected_alpn)&&!memcmp(negotiated,expected_alpn,negotiated_size));}else{CHECK(d->vtable->connection_get_alpn(c,negotiated,sizeof(negotiated),&negotiated_size)==PST_RESULT_UNAVAILABLE);}
 if(strcmp(close_mode,"raw-abrupt")){while(total<sizeof(buffer)){result=d->vtable->read(c,buffer+total,sizeof(buffer)-total,&io);printf("NSS_SERVER_READ RESULT=%ld OP=%lu BYTES=%lu\n",(long)result,(unsigned long)io.operation,(unsigned long)io.bytes_transferred);fflush(stdout);CHECK(result==PST_RESULT_OK&&io.operation!=PST_BACKEND_OPERATION_FAILED);total+=io.bytes_transferred;if(total<sizeof(buffer))CHECK(wait_for(d,c));}}
 if(strcmp(close_mode,"clean")){for(i=0;i<100;i++){result=d->vtable->read(c,buffer,1,&io);printf("NSS_SERVER_TRUNCATION STEP=%lu RESULT=%ld OP=%lu ERROR=%ld CLOSE=%lu\n",(unsigned long)(i+1),(long)result,(unsigned long)io.operation,(long)io.error,(unsigned long)io.close_kind);fflush(stdout);CHECK(result==PST_RESULT_OK);if(io.operation==PST_BACKEND_OPERATION_FAILED)break;CHECK(io.operation!=PST_BACKEND_OPERATION_CLOSED);CHECK(wait_for_read(d,c));}CHECK(io.operation==PST_BACKEND_OPERATION_FAILED&&io.error==PST_RESULT_TRUNCATED&&io.close_kind==PST_BACKEND_CLOSE_TRUNCATED);printf("NSS_SERVER_TRUNCATION MODE=%s READ=%lu RESULT=TRUNCATED CLOSE_KIND=TRUNCATED PASS=1\n",close_mode,(unsigned long)total);ok=1;goto done;}
 CHECK(d->vtable->write(c,buffer,sizeof(buffer),&io)==PST_RESULT_OK);CHECK(io.bytes_transferred==sizeof(buffer));
 for(i=0;i<100;i++){result=d->vtable->shutdown_step(c,&op,&error);printf("NSS_SERVER_SHUTDOWN STEP=%lu RESULT=%ld OP=%lu ERROR=%ld\n",(unsigned long)(i+1),(long)result,(unsigned long)op,(long)error);fflush(stdout);CHECK(result==PST_RESULT_OK&&op!=PST_BACKEND_OPERATION_FAILED);if(op==PST_BACKEND_OPERATION_COMPLETE)break;CHECK(wait_for(d,c));}CHECK(op==PST_BACKEND_OPERATION_COMPLETE);
 printf("NSS_SERVER_DIRECT TLS=0x%04x READ=25 WRITE=25 CONTENT_MATCH=1 ALPN=%s SHUTDOWN=RECIPROCAL PASS=1\n",tls13?0x0304:0x0303,expected_alpn);ok=1;
done: if(peer)d->vtable->peer_info_destroy(peer);if(c)d->vtable->connection_destroy(c);if(r)d->vtable->runtime_destroy(r);if(b)d->vtable->shutdown(b);pst_connection_config_snapshot_release(snap);if(accepted!=INVALID_SOCKET)closesocket(accepted);if(listener!=INVALID_SOCKET)closesocket(listener);if(wsa)WSACleanup();free(leaf);free(intermediate);free(key);free(ca);return ok?0:20;
}
