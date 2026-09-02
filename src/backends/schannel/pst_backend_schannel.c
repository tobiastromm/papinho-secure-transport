#define SECURITY_WIN32
#include <winsock2.h>
#include <windows.h>
#include <security.h>
#include <sspi.h>
#include <schannel.h>
#include "backends/schannel/pst_backend_schannel.h"
#include "pst_transport_internal.h"
#include <stdlib.h>
#include <string.h>
typedef struct sch_base { pst_internal_diagnostic diagnostic; } sch_base;
typedef struct sch_backend { pst_internal_diagnostic diagnostic; PSecurityFunctionTableA table; } sch_backend;
typedef struct sch_runtime { pst_internal_diagnostic diagnostic; sch_backend *backend; } sch_runtime;
typedef struct sch_connection { pst_internal_diagnostic diagnostic; sch_runtime *runtime; SOCKET socket_value; int owns_socket; CredHandle credentials; CtxtHandle context; int credentials_valid; int context_valid; } sch_connection;
static void capture(sch_base *s,PST_RESULT r,pst_u32 phase,pst_i32 code){pst_diagnostic_capture(&s->diagnostic,r,phase,"schannel",code?PST_DIAGNOSTIC_DOMAIN_WIN32:PST_DIAGNOSTIC_DOMAIN_NONE,code,0,code?PST_DIAGNOSTIC_FLAG_NATIVE:0);}
static PST_RESULT sch_initialize(void **out){sch_backend *s;if(!out)return PST_RESULT_INVALID_ARGUMENT;*out=NULL;s=(sch_backend*)calloc(1,sizeof(*s));if(!s)return PST_RESULT_OUT_OF_MEMORY;pst_diagnostic_initialize(&s->diagnostic);s->table=InitSecurityInterfaceA();if(!s->table){capture((sch_base*)s,PST_RESULT_BACKEND_FAILURE,PST_DIAGNOSTIC_PHASE_BACKEND_INITIALIZE,(pst_i32)GetLastError());free(s);return PST_RESULT_BACKEND_FAILURE;}*out=s;return PST_RESULT_OK;}
static void sch_shutdown(void *v){free(v);}
static PST_RESULT sch_runtime_create(void *v,void **out){sch_runtime *r;if(!v||!out)return PST_RESULT_INVALID_ARGUMENT;*out=NULL;r=(sch_runtime*)calloc(1,sizeof(*r));if(!r)return PST_RESULT_OUT_OF_MEMORY;pst_diagnostic_initialize(&r->diagnostic);r->backend=(sch_backend*)v;*out=r;return PST_RESULT_OK;}
static void sch_runtime_destroy(void *v){free(v);}
static PST_RESULT sch_query(void *v,pst_u32 *caps){if(!v||!caps)return PST_RESULT_INVALID_ARGUMENT;*caps=PST_BACKEND_CAP_NONBLOCKING;return PST_RESULT_OK;}
static PST_RESULT sch_validate(void *v,pst_u32 required){return v&&!(required&~PST_BACKEND_CAP_NONBLOCKING)?PST_RESULT_OK:PST_RESULT_UNSUPPORTED;}
static PST_RESULT sch_connection_create(void *v,void **out){sch_connection *c;if(!v||!out)return PST_RESULT_INVALID_ARGUMENT;*out=NULL;c=(sch_connection*)calloc(1,sizeof(*c));if(!c)return PST_RESULT_OUT_OF_MEMORY;pst_diagnostic_initialize(&c->diagnostic);c->runtime=(sch_runtime*)v;c->socket_value=INVALID_SOCKET;*out=c;return PST_RESULT_OK;}
static void sch_connection_destroy(void *v){sch_connection *c=(sch_connection*)v;if(!c)return;if(c->context_valid)DeleteSecurityContext(&c->context);if(c->credentials_valid)FreeCredentialsHandle(&c->credentials);if(c->owns_socket&&c->socket_value!=INVALID_SOCKET)closesocket(c->socket_value);free(c);}
static PST_RESULT sch_attach(void *v,void *native,pst_u32 ownership,pst_u32 *accepted){sch_connection *c=(sch_connection*)v;PST_NATIVE_TRANSPORT *t=(PST_NATIVE_TRANSPORT*)native;u_long nonblocking=1UL;if(!accepted)return PST_RESULT_INVALID_ARGUMENT;*accepted=0;if(!c||!t||c->owns_socket)return PST_RESULT_INVALID_ARGUMENT;if(ownership!=PST_BACKEND_OWNERSHIP_TRANSFERRED)return PST_RESULT_UNSUPPORTED;if(t->struct_size<PST_NATIVE_TRANSPORT_MIN_SIZE||t->version!=PST_NATIVE_TRANSPORT_VERSION||t->kind!=PST_NATIVE_TRANSPORT_KIND_WIN32_SOCKET)return PST_RESULT_INVALID_ARGUMENT;if(ioctlsocket((SOCKET)t->native_socket,FIONBIO,&nonblocking)!=0){capture((sch_base*)c,PST_RESULT_TRANSPORT_FAILURE,PST_DIAGNOSTIC_PHASE_TRANSPORT_ATTACH,(pst_i32)WSAGetLastError());return PST_RESULT_TRANSPORT_FAILURE;}c->socket_value=(SOCKET)t->native_socket;c->owns_socket=1;*accepted=1;return PST_RESULT_OK;}
static PST_RESULT sch_handshake(void *v,pst_u32 *op,PST_RESULT *error){sch_connection *c=(sch_connection*)v;if(!c||!op||!error)return PST_RESULT_INVALID_ARGUMENT;capture((sch_base*)c,PST_RESULT_UNSUPPORTED,PST_DIAGNOSTIC_PHASE_HANDSHAKE,0);*op=PST_BACKEND_OPERATION_FAILED;*error=PST_RESULT_UNSUPPORTED;return PST_RESULT_OK;}
static PST_RESULT sch_interest(void *v,pst_u32 *interest){if(!v||!interest)return PST_RESULT_INVALID_ARGUMENT;*interest=PST_BACKEND_INTEREST_NONE;return PST_RESULT_UNSUPPORTED;}
static PST_RESULT sch_read(void *v,void *b,pst_size n,PST_BACKEND_IO_RESULT *r){(void)v;(void)b;(void)n;if(!r)return PST_RESULT_INVALID_ARGUMENT;memset(r,0,sizeof(*r));r->operation=PST_BACKEND_OPERATION_FAILED;r->error=PST_RESULT_UNSUPPORTED;return PST_RESULT_UNSUPPORTED;}
static PST_RESULT sch_write(void *v,const void *b,pst_size n,PST_BACKEND_IO_RESULT *r){return sch_read(v,(void*)b,n,r);}
static PST_RESULT sch_close(void *v,pst_u32 *op,PST_RESULT *error){if(!v||!op||!error)return PST_RESULT_INVALID_ARGUMENT;*op=PST_BACKEND_OPERATION_FAILED;*error=PST_RESULT_UNSUPPORTED;return PST_RESULT_UNSUPPORTED;}
static void sch_diag(const void *v,pst_internal_diagnostic *out){if(!out)return;if(v)pst_diagnostic_copy(out,&((const sch_base*)v)->diagnostic);else pst_diagnostic_initialize(out);}
static const PST_BACKEND_VTABLE sch_vtable={sizeof(PST_BACKEND_VTABLE),PST_BACKEND_SPI_VERSION,sch_initialize,sch_shutdown,sch_runtime_create,sch_runtime_destroy,sch_query,sch_validate,sch_connection_create,sch_connection_destroy,sch_attach,sch_handshake,sch_interest,NULL,sch_read,sch_write,sch_close,NULL,NULL,NULL,NULL,sch_diag};
static const PST_BACKEND_METADATA sch_metadata={sizeof(PST_BACKEND_METADATA),PST_BACKEND_METADATA_VERSION,{PST_BACKEND_VERSION_AVAILABLE,1UL,0UL,0UL,"pst-schannel","skeleton"},1UL,{{0UL,0UL,0UL,0UL,"Schannel-SSPI","OS-provided"},{0}}};
static const PST_BACKEND_DESCRIPTOR sch_descriptor={sizeof(PST_BACKEND_DESCRIPTOR),PST_BACKEND_SPI_VERSION,"schannel","Windows Schannel/SSPI",PST_BACKEND_CAP_NONBLOCKING,&sch_vtable,&sch_metadata};
const PST_BACKEND_DESCRIPTOR *pst_backend_schannel_descriptor(void){return &sch_descriptor;}
PST_RESULT pst_backend_schannel_register(void){return pst_backend_register(&sch_descriptor);}