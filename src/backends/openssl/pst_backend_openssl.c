#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/provider.h>
#include "backends/openssl/pst_backend_openssl.h"
#include "pst_transport_internal.h"
#include <stdlib.h>
#include <string.h>

typedef struct ossl_base { pst_internal_diagnostic diagnostic; } ossl_base;
typedef struct ossl_backend { pst_internal_diagnostic diagnostic; } ossl_backend;
typedef struct ossl_runtime { pst_internal_diagnostic diagnostic; OSSL_LIB_CTX *library_context; OSSL_PROVIDER *default_provider; } ossl_runtime;
typedef struct ossl_connection { pst_internal_diagnostic diagnostic; ossl_runtime *runtime; SOCKET socket_value; int owns_socket; } ossl_connection;

static void ossl_clear_errors(void){while(ERR_get_error()!=0UL){} }
static void ossl_capture(ossl_base *s,PST_RESULT result,pst_u32 phase){pst_diagnostic_capture(&s->diagnostic,result,phase,"openssl",PST_DIAGNOSTIC_DOMAIN_NONE,0,0,0);}
static PST_RESULT ossl_initialize(void **out){ossl_backend *s;if(!out)return PST_RESULT_INVALID_ARGUMENT;*out=NULL;ossl_clear_errors();if(OPENSSL_version_major()!=3||OPENSSL_version_minor()!=5||OPENSSL_version_patch()!=8)return PST_RESULT_INCOMPATIBLE_API;if(!OPENSSL_init_crypto(OPENSSL_INIT_NO_LOAD_CONFIG,NULL)){ossl_clear_errors();return PST_RESULT_BACKEND_FAILURE;}s=(ossl_backend*)calloc(1,sizeof(*s));if(!s){ossl_clear_errors();return PST_RESULT_OUT_OF_MEMORY;}pst_diagnostic_initialize(&s->diagnostic);ossl_clear_errors();*out=s;return PST_RESULT_OK;}
static void ossl_shutdown(void *v){ossl_clear_errors();free(v);}
static PST_RESULT ossl_runtime_create(void *backend,void **out){ossl_runtime *r;if(!backend||!out)return PST_RESULT_INVALID_ARGUMENT;*out=NULL;ossl_clear_errors();r=(ossl_runtime*)calloc(1,sizeof(*r));if(!r)return PST_RESULT_OUT_OF_MEMORY;pst_diagnostic_initialize(&r->diagnostic);r->library_context=OSSL_LIB_CTX_new();if(!r->library_context){ossl_capture((ossl_base*)r,PST_RESULT_BACKEND_FAILURE,PST_DIAGNOSTIC_PHASE_RUNTIME_CREATE);ossl_clear_errors();free(r);return PST_RESULT_BACKEND_FAILURE;}r->default_provider=OSSL_PROVIDER_load(r->library_context,"default");if(!r->default_provider){ossl_capture((ossl_base*)r,PST_RESULT_BACKEND_FAILURE,PST_DIAGNOSTIC_PHASE_RUNTIME_CREATE);ossl_clear_errors();OSSL_LIB_CTX_free(r->library_context);free(r);return PST_RESULT_BACKEND_FAILURE;}ossl_clear_errors();*out=r;return PST_RESULT_OK;}
static void ossl_runtime_destroy(void *v){ossl_runtime *r=(ossl_runtime*)v;if(!r)return;ossl_clear_errors();if(r->default_provider)OSSL_PROVIDER_unload(r->default_provider);OSSL_LIB_CTX_free(r->library_context);ossl_clear_errors();free(r);}
static PST_RESULT ossl_query(void *v,pst_u32 *caps){if(!v||!caps)return PST_RESULT_INVALID_ARGUMENT;*caps=PST_BACKEND_CAP_NONBLOCKING;return PST_RESULT_OK;}
static PST_RESULT ossl_validate(void *v,pst_u32 required){return v&&!(required&~PST_BACKEND_CAP_NONBLOCKING)?PST_RESULT_OK:PST_RESULT_UNSUPPORTED;}
static PST_RESULT ossl_connection_create(void *v,void **out){ossl_connection *c;if(!v||!out)return PST_RESULT_INVALID_ARGUMENT;*out=NULL;c=(ossl_connection*)calloc(1,sizeof(*c));if(!c)return PST_RESULT_OUT_OF_MEMORY;pst_diagnostic_initialize(&c->diagnostic);c->runtime=(ossl_runtime*)v;c->socket_value=INVALID_SOCKET;*out=c;return PST_RESULT_OK;}
static void ossl_connection_destroy(void *v){ossl_connection *c=(ossl_connection*)v;if(!c)return;if(c->owns_socket&&c->socket_value!=INVALID_SOCKET)closesocket(c->socket_value);ossl_clear_errors();free(c);}
static PST_RESULT ossl_attach(void *v,void *native,pst_u32 ownership,pst_u32 *accepted){ossl_connection *c=(ossl_connection*)v;PST_NATIVE_TRANSPORT *t=(PST_NATIVE_TRANSPORT*)native;u_long nonblocking=1UL;if(!accepted)return PST_RESULT_INVALID_ARGUMENT;*accepted=0;if(!c||!t||c->owns_socket)return PST_RESULT_INVALID_ARGUMENT;if(ownership!=PST_BACKEND_OWNERSHIP_TRANSFERRED)return PST_RESULT_UNSUPPORTED;if(t->struct_size<PST_NATIVE_TRANSPORT_MIN_SIZE||t->version!=PST_NATIVE_TRANSPORT_VERSION||t->kind!=PST_NATIVE_TRANSPORT_KIND_WIN32_SOCKET)return PST_RESULT_INVALID_ARGUMENT;if(ioctlsocket((SOCKET)t->native_socket,FIONBIO,&nonblocking)!=0){ossl_capture((ossl_base*)c,PST_RESULT_TRANSPORT_FAILURE,PST_DIAGNOSTIC_PHASE_TRANSPORT_ATTACH);return PST_RESULT_TRANSPORT_FAILURE;}c->socket_value=(SOCKET)t->native_socket;c->owns_socket=1;*accepted=1;return PST_RESULT_OK;}
static PST_RESULT ossl_unsupported_op(void *v,pst_u32 *operation,PST_RESULT *error){if(!v||!operation||!error)return PST_RESULT_INVALID_ARGUMENT;*operation=PST_BACKEND_OPERATION_FAILED;*error=PST_RESULT_UNSUPPORTED;return PST_RESULT_UNSUPPORTED;}
static PST_RESULT ossl_interest(void *v,pst_u32 *interest){if(!v||!interest)return PST_RESULT_INVALID_ARGUMENT;*interest=PST_BACKEND_INTEREST_NONE;return PST_RESULT_UNSUPPORTED;}
static PST_RESULT ossl_read(void *v,void *buffer,pst_size capacity,PST_BACKEND_IO_RESULT *result){(void)buffer;(void)capacity;if(!v||!result)return PST_RESULT_INVALID_ARGUMENT;memset(result,0,sizeof(*result));result->operation=PST_BACKEND_OPERATION_FAILED;result->error=PST_RESULT_UNSUPPORTED;return PST_RESULT_UNSUPPORTED;}
static PST_RESULT ossl_write(void *v,const void *buffer,pst_size length,PST_BACKEND_IO_RESULT *result){return ossl_read(v,(void*)buffer,length,result);}
static PST_RESULT ossl_peer(void *v,void **out){if(out)*out=NULL;return v&&out?PST_RESULT_UNSUPPORTED:PST_RESULT_INVALID_ARGUMENT;}
static void ossl_peer_destroy(void *v){(void)v;}
static PST_RESULT ossl_configure(void *v,const pst_config *config){return v&&config?PST_RESULT_OK:PST_RESULT_INVALID_ARGUMENT;}
static PST_RESULT ossl_alpn(void *v,pst_u8 *buffer,pst_size capacity,pst_size *size){(void)buffer;(void)capacity;if(size)*size=0;return v&&size?PST_RESULT_UNSUPPORTED:PST_RESULT_INVALID_ARGUMENT;}
static void ossl_diagnostic(const void *v,pst_internal_diagnostic *out){if(!out)return;if(v)pst_diagnostic_copy(out,&((const ossl_base*)v)->diagnostic);else pst_diagnostic_initialize(out);}
static const PST_BACKEND_VTABLE ossl_vtable={sizeof(PST_BACKEND_VTABLE),PST_BACKEND_SPI_VERSION,ossl_initialize,ossl_shutdown,ossl_runtime_create,ossl_runtime_destroy,ossl_query,ossl_validate,ossl_connection_create,ossl_connection_destroy,ossl_attach,ossl_unsupported_op,ossl_interest,NULL,ossl_read,ossl_write,ossl_unsupported_op,ossl_peer,ossl_peer_destroy,ossl_configure,ossl_alpn,ossl_diagnostic};
static const PST_BACKEND_METADATA ossl_metadata={sizeof(PST_BACKEND_METADATA),PST_BACKEND_METADATA_VERSION,{PST_BACKEND_VERSION_AVAILABLE,0UL,1UL,0UL,"pst-openssl","skeleton"},1UL,{{PST_BACKEND_VERSION_AVAILABLE,3UL,5UL,8UL,"OpenSSL","LTS"},{0}}};
static const PST_BACKEND_DESCRIPTOR ossl_descriptor={sizeof(PST_BACKEND_DESCRIPTOR),PST_BACKEND_SPI_VERSION,"openssl","OpenSSL 3.5.8",PST_BACKEND_CAP_NONBLOCKING,&ossl_vtable,&ossl_metadata};
const PST_BACKEND_DESCRIPTOR *pst_backend_openssl_descriptor(void){return &ossl_descriptor;}
PST_RESULT pst_backend_openssl_register(void){return pst_backend_register(&ossl_descriptor);}
