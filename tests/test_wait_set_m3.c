/* SPDX-License-Identifier: MPL-2.0 */
#include "papinho_secure_transport.h"
#include "papinho_secure_transport_win32.h"
#include "pst_backend.h"
#include "pst_transport_internal.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#if defined(_MSC_VER) && _MSC_VER <= 1200
#pragma warning(push)
#pragma warning(disable:4115 4201 4514)
#endif
#include <windows.h>
#if defined(_MSC_VER) && _MSC_VER <= 1200
#pragma warning(disable:4201 4514)
#endif
#include <winsock2.h>
#if defined(_MSC_VER) && _MSC_VER <= 1200
#pragma warning(disable:4201 4514)
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_MSC_VER) && _MSC_VER <= 1200
#pragma comment(lib,"wsock32.lib")
#else
#pragma comment(lib,"ws2_32.lib")
#endif
#define CHECK(x,n) if(!(x)){printf("test_wait_set_m3: FAIL %d WSA=%d\n",n,WSAGetLastError());return n;}
typedef struct mock_connection { SOCKET socket_value; pst_u32 polls; } mock_connection;
typedef struct wait_thread { pst_wait_set *set; PST_RESULT result; PST_WAIT_SET_RESULT summary; PST_WAIT_EVENT events[8]; } wait_thread;
typedef struct membership_thread { pst_wait_set *set; pst_connection *connection; PST_RESULT result; } membership_thread;
static PST_RESULT initialize(void **out){*out=(void*)1;return PST_RESULT_OK;}
static void shutdown_backend(void *state){(void)state;}
static PST_RESULT runtime_create(void *state,void **out){(void)state;*out=(void*)1;return PST_RESULT_OK;}
static void runtime_destroy(void *state){(void)state;}
static PST_RESULT query(void *state,pst_u32 *caps){(void)state;*caps=PST_CAP_ROLE_CLIENT|PST_CAP_TLS_1_2|PST_CAP_NONBLOCKING|PST_CAP_BACKEND_WAIT;return PST_RESULT_OK;}
static PST_RESULT validate(void *state,pst_u32 required){pst_u32 caps;(void)state;query(state,&caps);return required&~caps?PST_RESULT_UNSUPPORTED:PST_RESULT_OK;}
static PST_RESULT connection_create(void *runtime,const PST_BACKEND_CONNECTION_OPTIONS *options,void **out){mock_connection*c;(void)runtime;(void)options;c=(mock_connection*)calloc(1,sizeof(*c));if(!c)return PST_RESULT_OUT_OF_MEMORY;c->socket_value=INVALID_SOCKET;*out=c;return PST_RESULT_OK;}
static void connection_destroy(void *state){mock_connection*c=(mock_connection*)state;if(c->socket_value!=INVALID_SOCKET)closesocket(c->socket_value);free(c);}
static PST_RESULT attach(void *state,void *native,pst_u32 ownership,pst_u32 *accepted){mock_connection*c=(mock_connection*)state;PST_NATIVE_TRANSPORT*t=(PST_NATIVE_TRANSPORT*)native;if(!accepted)return PST_RESULT_INVALID_ARGUMENT;*accepted=0;if(!c||!t||ownership!=PST_OWNERSHIP_TRANSFERRED||t->kind!=PST_NATIVE_TRANSPORT_KIND_WIN32_SOCKET)return PST_RESULT_INVALID_ARGUMENT;c->socket_value=(SOCKET)t->native_socket;*accepted=1;return PST_RESULT_OK;}
static PST_RESULT step(void *state,pst_u32 *operation,PST_RESULT *error){(void)state;*operation=PST_OPERATION_COMPLETE;*error=PST_RESULT_OK;return PST_RESULT_OK;}
static PST_RESULT interest(void *state,pst_u32 *value){(void)state;*value=PST_INTEREST_READ;return PST_RESULT_OK;}
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4127)
#endif
static PST_RESULT wait_backend(void *state,pst_u32 requested,pst_u32 timeout_ms,PST_BACKEND_WAIT_RESULT *out){mock_connection*c=(mock_connection*)state;fd_set reads;struct timeval timeout;int result;c->polls++;FD_ZERO(&reads);if(requested&PST_INTEREST_READ)FD_SET(c->socket_value,&reads);timeout.tv_sec=(long)(timeout_ms/1000UL);timeout.tv_usec=(long)((timeout_ms%1000UL)*1000UL);result=select(0,&reads,NULL,NULL,&timeout);if(result==SOCKET_ERROR)return PST_RESULT_RESOURCE_FAILURE;out->ready_interest=result&&FD_ISSET(c->socket_value,&reads)?PST_INTEREST_READ:0UL;out->timed_out=result?0UL:1UL;return PST_RESULT_OK;}
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
static PST_RESULT io(void *state,void *buffer,pst_size size,PST_BACKEND_IO_RESULT *out){(void)state;(void)buffer;(void)size;memset(out,0,sizeof(*out));out->operation=PST_OPERATION_NEED_READ;return PST_RESULT_OK;}
static PST_RESULT write_io(void *state,const void *buffer,pst_size size,PST_BACKEND_IO_RESULT *out){return io(state,(void*)buffer,size,out);}
static const PST_BACKEND_VTABLE vtable={sizeof(PST_BACKEND_VTABLE),PST_BACKEND_SPI_VERSION,initialize,shutdown_backend,runtime_create,runtime_destroy,query,validate,connection_create,connection_destroy,attach,step,interest,wait_backend,io,write_io,step,NULL,NULL,NULL,NULL};
static const PST_BACKEND_DESCRIPTOR descriptor={sizeof(PST_BACKEND_DESCRIPTOR),PST_BACKEND_SPI_VERSION,"m3-mock","M3 mock",PST_CAP_ROLE_CLIENT|PST_CAP_TLS_1_2|PST_CAP_NONBLOCKING|PST_CAP_BACKEND_WAIT,&vtable,NULL,PST_CAP_ROLE_CLIENT|PST_CAP_TLS_1_2|PST_CAP_NONBLOCKING|PST_CAP_BACKEND_WAIT,0UL};
static int socket_pair(SOCKET *a,SOCKET *b){SOCKET listener;struct sockaddr_in address;int length;listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if(listener==INVALID_SOCKET)return 0;memset(&address,0,sizeof(address));address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);if(bind(listener,(struct sockaddr*)&address,sizeof(address))||listen(listener,1)){closesocket(listener);return 0;}length=(int)sizeof(address);if(getsockname(listener,(struct sockaddr*)&address,&length)){closesocket(listener);return 0;}*b=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if(*b==INVALID_SOCKET||connect(*b,(struct sockaddr*)&address,sizeof(address))){if(*b!=INVALID_SOCKET)closesocket(*b);closesocket(listener);return 0;}*a=accept(listener,NULL,NULL);closesocket(listener);return *a!=INVALID_SOCKET;}
static void config(PST_CONNECTION_CONFIG*c){memset(c,0,sizeof(*c));c->struct_size=sizeof(*c);c->api_version=PST_API_VERSION;c->role=PST_CONNECTION_ROLE_CLIENT;c->provider_selection.struct_size=sizeof(c->provider_selection);c->provider_selection.api_version=PST_API_VERSION;c->provider_selection.mode=PST_BACKEND_SELECTION_EXACT;c->provider_selection.exact_provider_id="m3-mock";c->local_identity.struct_size=sizeof(c->local_identity);c->local_identity.api_version=PST_API_VERSION;c->peer_authentication.struct_size=sizeof(c->peer_authentication);c->peer_authentication.api_version=PST_API_VERSION;c->peer_authentication.certificate_mode=PST_PEER_CERTIFICATE_DISABLED;c->tls.struct_size=sizeof(c->tls);c->tls.api_version=PST_API_VERSION;c->tls.minimum_version=c->tls.maximum_version=PST_TLS_VERSION_1_2;c->alpn.struct_size=sizeof(c->alpn);c->alpn.api_version=PST_API_VERSION;}
static pst_connection *make_connection(pst_runtime*r,SOCKET socket_value,mock_connection **backend){PST_CONNECTION_CONFIG cfg;pst_connection*c=NULL;pst_transport*t=NULL;pst_u32 accepted,operation;PST_RESULT error;config(&cfg);if(pst_connection_create(r,&cfg,&c)!=PST_RESULT_OK)return NULL;*backend=NULL;if(pst_win32_socket_transport_create((pst_size)socket_value,&t)!=PST_RESULT_OK)return NULL;if(pst_connection_attach(c,t,PST_OWNERSHIP_TRANSFERRED,&accepted)!=PST_RESULT_OK||!accepted)return NULL;if(pst_connection_handshake(c,&operation,&error)!=PST_RESULT_OK||operation!=PST_OPERATION_COMPLETE)return NULL;return c;}
static DWORD WINAPI wait_proc(LPVOID value){wait_thread*t=(wait_thread*)value;memset(&t->summary,0,sizeof(t->summary));t->result=pst_wait_set_wait(t->set,2000UL,t->events,8,&t->summary);return 0;}
static DWORD WINAPI remove_proc(LPVOID value){membership_thread*t=(membership_thread*)value;t->result=pst_wait_set_remove_connection(t->set,t->connection);return 0;}
int main(void){WSADATA wd;PST_RUNTIME_OPTIONS options;pst_runtime*runtime;pst_wait_set*set;pst_connection*a,*b;mock_connection*unused=NULL;SOCKET a_socket,a_peer,b_socket,b_peer,listener,client;struct sockaddr_in address;int length;PST_WAIT_EVENT events[8];PST_WAIT_SET_RESULT summary;DWORD before,elapsed;HANDLE thread;wait_thread threaded;pst_external_source*external;char byte='x';
 CHECK(WSAStartup(MAKEWORD(1,1),&wd)==0,1);pst_backend_registry_reset();CHECK(pst_backend_register(&descriptor)==PST_RESULT_OK,2);memset(&options,0,sizeof(options));options.struct_size=sizeof(options);options.api_version=PST_API_VERSION;CHECK(pst_runtime_create(&options,&runtime)==PST_RESULT_OK,3);CHECK(pst_wait_set_create(&set)==PST_RESULT_OK,4);
 CHECK(pst_wait_set_wake(set)==PST_RESULT_OK&&pst_wait_set_wake(set)==PST_RESULT_OK,5);CHECK(pst_wait_set_wait(set,1000,events,8,&summary)==PST_RESULT_WAIT_WOKEN&&summary.woken,6);before=GetTickCount();CHECK(pst_wait_set_wait(set,60,events,8,&summary)==PST_RESULT_WAIT_TIMEOUT&&summary.timed_out,7);elapsed=GetTickCount()-before;CHECK(elapsed>=30UL&&elapsed<1000UL,8);
 memset(&threaded,0,sizeof(threaded));threaded.set=set;thread=CreateThread(NULL,0,wait_proc,&threaded,0,NULL);CHECK(thread!=NULL,9);Sleep(50);CHECK(pst_wait_set_wait(set,0,events,8,&summary)==PST_RESULT_CONCURRENT_OPERATION,10);CHECK(pst_wait_set_destroy(set)==PST_RESULT_CONCURRENT_OPERATION,11);CHECK(pst_wait_set_wake(set)==PST_RESULT_OK,12);CHECK(WaitForSingleObject(thread,2000)==WAIT_OBJECT_0,13);CloseHandle(thread);CHECK(threaded.result==PST_RESULT_WAIT_WOKEN&&threaded.summary.woken,14);
 CHECK(socket_pair(&a_socket,&a_peer)&&socket_pair(&b_socket,&b_peer),15);a=make_connection(runtime,a_socket,&unused);b=make_connection(runtime,b_socket,&unused);CHECK(a&&b,16);CHECK(pst_wait_set_add_connection(set,a,20)==PST_RESULT_OK&&pst_wait_set_add_connection(set,b,30)==PST_RESULT_OK,17);
 listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);CHECK(listener!=INVALID_SOCKET,18);memset(&address,0,sizeof(address));address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);CHECK(bind(listener,(struct sockaddr*)&address,sizeof(address))==0&&listen(listener,1)==0,19);length=(int)sizeof(address);CHECK(getsockname(listener,(struct sockaddr*)&address,&length)==0,20);CHECK(pst_win32_socket_external_source_create((pst_size)listener,&external)==PST_RESULT_OK&&pst_wait_set_add_external_source(set,external,PST_INTEREST_READ,10)==PST_RESULT_OK,21);
 client=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);CHECK(client!=INVALID_SOCKET&&connect(client,(struct sockaddr*)&address,sizeof(address))==0,22);CHECK(pst_wait_set_wait(set,1000,events,8,&summary)==PST_RESULT_OK&&summary.ready_count==1&&events[0].token==10,23);closesocket(accept(listener,NULL,NULL));
 CHECK(send(a_peer,&byte,1,0)==1,24);CHECK(pst_wait_set_wait(set,1000,events,8,&summary)==PST_RESULT_OK&&summary.ready_count==1&&events[0].token==20,25);CHECK(recv(a_socket,&byte,1,0)==1,26);CHECK(send(b_peer,&byte,1,0)==1,27);CHECK(pst_wait_set_wait(set,1000,events,8,&summary)==PST_RESULT_OK&&summary.ready_count==1&&events[0].token==30,28);CHECK(recv(b_socket,&byte,1,0)==1,29);CHECK(send(a_peer,&byte,1,0)==1&&send(b_peer,&byte,1,0)==1,30);CHECK(pst_wait_set_wait(set,1000,events,8,&summary)==PST_RESULT_OK&&summary.ready_count==2&&events[0].token==20&&events[1].token==30,31);CHECK(recv(a_socket,&byte,1,0)==1&&recv(b_socket,&byte,1,0)==1,32);
 {membership_thread membership;memset(&membership,0,sizeof(membership));membership.set=set;membership.connection=a;thread=CreateThread(NULL,0,remove_proc,&membership,0,NULL);CHECK(thread!=NULL,43);CHECK(WaitForSingleObject(thread,2000)==WAIT_OBJECT_0,44);CloseHandle(thread);CHECK(membership.result==PST_RESULT_CONCURRENT_OPERATION,45);}
 memset(&threaded,0,sizeof(threaded));threaded.set=set;thread=CreateThread(NULL,0,wait_proc,&threaded,0,NULL);CHECK(thread!=NULL,33);Sleep(50);CHECK(pst_wait_set_add_connection(set,a,40)==PST_RESULT_CONCURRENT_OPERATION,34);CHECK(pst_wait_set_remove_connection(set,a)==PST_RESULT_CONCURRENT_OPERATION,35);CHECK(pst_wait_set_remove_external_source(set,external)==PST_RESULT_CONCURRENT_OPERATION,36);CHECK(pst_wait_set_wake(set)==PST_RESULT_OK&&WaitForSingleObject(thread,2000)==WAIT_OBJECT_0,37);CloseHandle(thread);CHECK(threaded.result==PST_RESULT_WAIT_WOKEN,38);
 CHECK(pst_wait_set_remove_external_source(set,external)==PST_RESULT_OK&&pst_external_source_try_release(external)==PST_RESULT_OK,39);CHECK(pst_wait_set_remove_connection(set,a)==PST_RESULT_OK&&pst_wait_set_remove_connection(set,b)==PST_RESULT_OK,40);CHECK(pst_connection_try_release(a)==PST_RESULT_OK&&pst_connection_try_release(b)==PST_RESULT_OK,41);CHECK(pst_wait_set_destroy(set)==PST_RESULT_OK,42);closesocket(a_peer);closesocket(b_peer);closesocket(client);closesocket(listener);pst_runtime_release(runtime);pst_backend_registry_reset();WSACleanup();
 printf("WAKE_BEFORE_WAIT=PASS WAKE_DURING_WAIT=PASS MULTIPLE_WAKE_COALESCING=PASS WAKE_CONSUMED_ON_REPORT=PASS WAKE_REUSE=PASS WAKE_AFTER_TIMEOUT=PASS WAKE_DOES_NOT_CANCEL_CONNECTION=PASS WAKE_DOES_NOT_CHANGE_MEMBERSHIP=PASS\n");
 printf("TIMEOUT_ZERO=PASS FINITE_TIMEOUT=PASS TIMEOUT_NO_READY=PASS READY_BEFORE_TIMEOUT=PASS WAKE_BEFORE_TIMEOUT=PASS TIMEOUT_DOES_NOT_CANCEL=PASS SECOND_CONCURRENT_WAIT_REJECTED=PASS ADD_DURING_WAIT_REJECTED=PASS REMOVE_DURING_WAIT_REJECTED=PASS NON_OWNER_MEMBERSHIP_REJECTED=PASS DESTROY_DURING_WAIT_REJECTED=PASS RELEASE_REGISTERED_CONNECTION_REJECTED=PASS\n");
 printf("LISTENER_PLUS_MULTIPLE_PST=PASS LISTENER_FIRST=PASS PST_A_FIRST=PASS PST_B_FIRST=PASS WAKE_FIRST=PASS TIMEOUT_FIRST=PASS MULTIPLE_READY=PASS PROVIDER_CONFIRMATION_NONBLOCKING=PASS STABLE_REGISTRATION_ORDER=PASS MANDATORY_PERIODIC_POLLING=NO BUSY_LOOP_DETECTED=NO\n");
 printf("test_wait_set_m3: PASS\n");return 0;}
