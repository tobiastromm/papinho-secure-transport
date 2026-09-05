/* SPDX-License-Identifier: MPL-2.0 */
#include "papinho_secure_transport_win32.h"
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
static void pst_win32_transport_destroy(pst_transport *base,int consumed){pst_win32_transport *t=(pst_win32_transport*)base;if(!consumed)closesocket((SOCKET)t->native.native_socket);free(t);}
PST_RESULT PST_CALL pst_win32_socket_transport_create(pst_size socket_value,pst_transport **out){pst_win32_transport *t;if(!out)return PST_RESULT_INVALID_ARGUMENT;*out=NULL;t=(pst_win32_transport*)calloc(1,sizeof(*t));if(!t)return PST_RESULT_OUT_OF_MEMORY;t->base.backend_id=NULL;t->base.native=&t->native;t->base.destroy=pst_win32_transport_destroy;t->native.struct_size=sizeof(t->native);t->native.version=PST_NATIVE_TRANSPORT_VERSION;t->native.kind=PST_NATIVE_TRANSPORT_KIND_WIN32_SOCKET;t->native.native_socket=socket_value;*out=&t->base;return PST_RESULT_OK;}