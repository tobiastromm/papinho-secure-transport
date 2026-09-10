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
typedef struct pst_win32_transport { pst_transport base; PST_NATIVE_TRANSPORT native; } pst_win32_transport;
typedef struct pst_win32_socket_source { pst_external_source base; SOCKET socket_value; } pst_win32_socket_source;
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4127)
#endif
static PST_RESULT pst_win32_socket_source_poll(pst_external_source *base,pst_u32 interests,pst_u32 *ready){pst_win32_socket_source*s=(pst_win32_socket_source*)base;fd_set reads,writes;struct timeval zero;int result;if(!ready)return PST_RESULT_INVALID_ARGUMENT;*ready=0UL;if(s->socket_value==INVALID_SOCKET)return PST_RESULT_RESOURCE_FAILURE;FD_ZERO(&reads);FD_ZERO(&writes);if(interests&PST_INTEREST_READ)FD_SET(s->socket_value,&reads);if(interests&PST_INTEREST_WRITE)FD_SET(s->socket_value,&writes);zero.tv_sec=0;zero.tv_usec=0;result=select(0,(interests&PST_INTEREST_READ)?&reads:NULL,(interests&PST_INTEREST_WRITE)?&writes:NULL,NULL,&zero);if(result==SOCKET_ERROR)return PST_RESULT_RESOURCE_FAILURE;if((interests&PST_INTEREST_READ)&&FD_ISSET(s->socket_value,&reads))*ready|=PST_INTEREST_READ;if((interests&PST_INTEREST_WRITE)&&FD_ISSET(s->socket_value,&writes))*ready|=PST_INTEREST_WRITE;return PST_RESULT_OK;}
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
static void pst_win32_socket_source_destroy(pst_external_source *base){free(base);}
static void pst_win32_transport_destroy(pst_transport *base,int consumed){pst_win32_transport *t=(pst_win32_transport*)base;if(!consumed)closesocket((SOCKET)t->native.native_socket);free(t);}
PST_RESULT PST_CALL pst_win32_socket_transport_create(pst_size socket_value,pst_transport **out){pst_win32_transport *t;if(!out)return PST_RESULT_INVALID_ARGUMENT;*out=NULL;t=(pst_win32_transport*)calloc(1,sizeof(*t));if(!t)return PST_RESULT_OUT_OF_MEMORY;t->base.backend_id=NULL;t->base.native=&t->native;t->base.destroy=pst_win32_transport_destroy;t->native.struct_size=sizeof(t->native);t->native.version=PST_NATIVE_TRANSPORT_VERSION;t->native.kind=PST_NATIVE_TRANSPORT_KIND_WIN32_SOCKET;t->native.native_socket=socket_value;*out=&t->base;return PST_RESULT_OK;}
PST_RESULT PST_CALL pst_win32_socket_external_source_create(pst_size socket_value,pst_external_source **out){pst_win32_socket_source*s;if(!out)return PST_RESULT_INVALID_ARGUMENT;*out=NULL;if((SOCKET)socket_value==INVALID_SOCKET)return PST_RESULT_INVALID_ARGUMENT;s=(pst_win32_socket_source*)calloc(1,sizeof(*s));if(!s)return PST_RESULT_OUT_OF_MEMORY;s->base.poll=pst_win32_socket_source_poll;s->base.destroy=pst_win32_socket_source_destroy;s->socket_value=(SOCKET)socket_value;*out=&s->base;return PST_RESULT_OK;}
