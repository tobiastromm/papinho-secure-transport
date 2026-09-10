/* SPDX-License-Identifier: MPL-2.0 */
#include "papinho_secure_transport_win32.h"
#include "pst_internal.h"
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
#include <stdlib.h>
#include <string.h>
#if defined(_MSC_VER) && _MSC_VER <= 1200
#pragma comment(lib,"wsock32.lib")
#else
#pragma comment(lib,"ws2_32.lib")
#endif
typedef struct pst_win32_transport { pst_transport base; PST_NATIVE_TRANSPORT native; } pst_win32_transport;
typedef struct pst_win32_socket_source { pst_external_source base; SOCKET socket_value; } pst_win32_socket_source;
typedef struct pst_win32_wait_set { SOCKET wake_read,wake_write; DWORD owner_thread; LONG active; } pst_win32_wait_set;
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4127)
#endif
static PST_RESULT pst_win32_socket_source_poll(pst_external_source *base,pst_u32 interests,pst_u32 *ready){pst_win32_socket_source*s=(pst_win32_socket_source*)base;fd_set reads,writes;struct timeval zero;int result;if(!ready)return PST_RESULT_INVALID_ARGUMENT;*ready=0UL;if(s->socket_value==INVALID_SOCKET)return PST_RESULT_RESOURCE_FAILURE;FD_ZERO(&reads);FD_ZERO(&writes);if(interests&PST_INTEREST_READ)FD_SET(s->socket_value,&reads);if(interests&PST_INTEREST_WRITE)FD_SET(s->socket_value,&writes);zero.tv_sec=0;zero.tv_usec=0;result=select(0,(interests&PST_INTEREST_READ)?&reads:NULL,(interests&PST_INTEREST_WRITE)?&writes:NULL,NULL,&zero);if(result==SOCKET_ERROR)return PST_RESULT_RESOURCE_FAILURE;if((interests&PST_INTEREST_READ)&&FD_ISSET(s->socket_value,&reads))*ready|=PST_INTEREST_READ;if((interests&PST_INTEREST_WRITE)&&FD_ISSET(s->socket_value,&writes))*ready|=PST_INTEREST_WRITE;return PST_RESULT_OK;}
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
static void pst_win32_socket_source_destroy(pst_external_source *base){free(base);}
static PST_RESULT pst_win32_socket_source_native(pst_external_source *base,pst_size *native_source){pst_win32_socket_source*s=(pst_win32_socket_source*)base;if(!native_source)return PST_RESULT_INVALID_ARGUMENT;if(s->socket_value==INVALID_SOCKET)return PST_RESULT_RESOURCE_FAILURE;*native_source=(pst_size)s->socket_value;return PST_RESULT_OK;}
static PST_RESULT pst_win32_transport_wait_source(const pst_transport *base,pst_size *native_source){const pst_win32_transport*t=(const pst_win32_transport*)base;if(!native_source)return PST_RESULT_INVALID_ARGUMENT;if((SOCKET)t->native.native_socket==INVALID_SOCKET)return PST_RESULT_RESOURCE_FAILURE;*native_source=t->native.native_socket;return PST_RESULT_OK;}
static void pst_win32_transport_destroy(pst_transport *base,int consumed){pst_win32_transport *t=(pst_win32_transport*)base;if(!consumed)closesocket((SOCKET)t->native.native_socket);free(t);}
PST_RESULT PST_CALL pst_win32_socket_transport_create(pst_size socket_value,pst_transport **out){pst_win32_transport *t;if(!out)return PST_RESULT_INVALID_ARGUMENT;*out=NULL;t=(pst_win32_transport*)calloc(1,sizeof(*t));if(!t)return PST_RESULT_OUT_OF_MEMORY;t->base.backend_id=NULL;t->base.native=&t->native;t->base.destroy=pst_win32_transport_destroy;t->base.wait_source=pst_win32_transport_wait_source;t->native.struct_size=sizeof(t->native);t->native.version=PST_NATIVE_TRANSPORT_VERSION;t->native.kind=PST_NATIVE_TRANSPORT_KIND_WIN32_SOCKET;t->native.native_socket=socket_value;*out=&t->base;return PST_RESULT_OK;}
PST_RESULT PST_CALL pst_win32_socket_external_source_create(pst_size socket_value,pst_external_source **out){pst_win32_socket_source*s;if(!out)return PST_RESULT_INVALID_ARGUMENT;*out=NULL;if((SOCKET)socket_value==INVALID_SOCKET)return PST_RESULT_INVALID_ARGUMENT;s=(pst_win32_socket_source*)calloc(1,sizeof(*s));if(!s)return PST_RESULT_OUT_OF_MEMORY;s->base.poll=pst_win32_socket_source_poll;s->base.native_source=pst_win32_socket_source_native;s->base.destroy=pst_win32_socket_source_destroy;s->socket_value=(SOCKET)socket_value;*out=&s->base;return PST_RESULT_OK;}

static void pst_win32_wait_close(pst_win32_wait_set *s){if(s->wake_read!=INVALID_SOCKET)closesocket(s->wake_read);if(s->wake_write!=INVALID_SOCKET)closesocket(s->wake_write);WSACleanup();free(s);}
PST_RESULT pst_platform_wait_set_create(void **out){WSADATA data;pst_win32_wait_set*s;SOCKET listener=INVALID_SOCKET;struct sockaddr_in address;int length;u_long nonblocking=1UL;if(!out)return PST_RESULT_INVALID_ARGUMENT;*out=NULL;if(WSAStartup(MAKEWORD(1,1),&data)!=0)return PST_RESULT_RESOURCE_FAILURE;s=(pst_win32_wait_set*)calloc(1,sizeof(*s));if(!s){WSACleanup();return PST_RESULT_OUT_OF_MEMORY;}s->wake_read=INVALID_SOCKET;s->wake_write=INVALID_SOCKET;s->owner_thread=GetCurrentThreadId();listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if(listener==INVALID_SOCKET)goto fail;memset(&address,0,sizeof(address));address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);address.sin_port=0;if(bind(listener,(struct sockaddr*)&address,sizeof(address))==SOCKET_ERROR||listen(listener,1)==SOCKET_ERROR)goto fail;length=(int)sizeof(address);if(getsockname(listener,(struct sockaddr*)&address,&length)==SOCKET_ERROR)goto fail;s->wake_write=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if(s->wake_write==INVALID_SOCKET||connect(s->wake_write,(struct sockaddr*)&address,sizeof(address))==SOCKET_ERROR)goto fail;s->wake_read=accept(listener,NULL,NULL);if(s->wake_read==INVALID_SOCKET)goto fail;closesocket(listener);listener=INVALID_SOCKET;if(ioctlsocket(s->wake_read,FIONBIO,&nonblocking)==SOCKET_ERROR||ioctlsocket(s->wake_write,FIONBIO,&nonblocking)==SOCKET_ERROR)goto fail;*out=s;return PST_RESULT_OK;fail:if(listener!=INVALID_SOCKET)closesocket(listener);pst_win32_wait_close(s);return PST_RESULT_RESOURCE_FAILURE;}
void pst_platform_wait_set_destroy(void *state){if(state)pst_win32_wait_close((pst_win32_wait_set*)state);}
PST_RESULT pst_platform_wait_set_wake(void *state){pst_win32_wait_set*s=(pst_win32_wait_set*)state;char byte=1;int result,error;if(!s)return PST_RESULT_INVALID_ARGUMENT;result=send(s->wake_write,&byte,1,0);if(result==1)return PST_RESULT_OK;if(result==SOCKET_ERROR){error=WSAGetLastError();if(error==WSAEWOULDBLOCK)return PST_RESULT_OK;}return PST_RESULT_RESOURCE_FAILURE;}
PST_RESULT pst_platform_wait_set_begin(void *state){pst_win32_wait_set*s=(pst_win32_wait_set*)state;if(!s)return PST_RESULT_INVALID_ARGUMENT;return InterlockedExchange(&s->active,1L)==0L?PST_RESULT_OK:PST_RESULT_CONCURRENT_OPERATION;}
void pst_platform_wait_set_end(void *state){pst_win32_wait_set*s=(pst_win32_wait_set*)state;if(s)InterlockedExchange(&s->active,0L);}
int pst_platform_wait_set_is_owner(void *state){pst_win32_wait_set*s=(pst_win32_wait_set*)state;return s&&s->owner_thread==GetCurrentThreadId();}
int pst_platform_wait_set_is_active(void *state){pst_win32_wait_set*s=(pst_win32_wait_set*)state;return s&&InterlockedExchangeAdd(&s->active,0L)!=0L;}
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4127)
#endif
PST_RESULT pst_platform_wait_set_wait(void *state,pst_platform_wait_entry *entries,pst_size count,pst_u32 timeout_ms,pst_u32 *woken){pst_win32_wait_set*s=(pst_win32_wait_set*)state;fd_set reads,writes;struct timeval timeout;SOCKET value;int result;char buffer[64];pst_size i;if(!s||!woken||(count&&!entries))return PST_RESULT_INVALID_ARGUMENT;*woken=0UL;if(count+1UL>(pst_size)FD_SETSIZE)return PST_RESULT_INSUFFICIENT_CAPACITY;FD_ZERO(&reads);FD_ZERO(&writes);FD_SET(s->wake_read,&reads);for(i=0;i<count;i++){entries[i].ready_interest=0UL;value=(SOCKET)entries[i].native_source;if(value==INVALID_SOCKET)return PST_RESULT_RESOURCE_FAILURE;if(entries[i].interests&PST_INTEREST_READ)FD_SET(value,&reads);if(entries[i].interests&PST_INTEREST_WRITE)FD_SET(value,&writes);}timeout.tv_sec=(long)(timeout_ms/1000UL);timeout.tv_usec=(long)((timeout_ms%1000UL)*1000UL);result=select(0,&reads,&writes,NULL,&timeout);if(result==SOCKET_ERROR)return PST_RESULT_RESOURCE_FAILURE;if(result==0)return PST_RESULT_WAIT_TIMEOUT;if(FD_ISSET(s->wake_read,&reads)){*woken=1UL;while(recv(s->wake_read,buffer,sizeof(buffer),0)>0){}}for(i=0;i<count;i++){value=(SOCKET)entries[i].native_source;if((entries[i].interests&PST_INTEREST_READ)&&FD_ISSET(value,&reads))entries[i].ready_interest|=PST_INTEREST_READ;if((entries[i].interests&PST_INTEREST_WRITE)&&FD_ISSET(value,&writes))entries[i].ready_interest|=PST_INTEREST_WRITE;}return PST_RESULT_OK;}
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
pst_u32 pst_platform_monotonic_ms(void){return (pst_u32)GetTickCount();}
