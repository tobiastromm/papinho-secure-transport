/* SPDX-License-Identifier: MPL-2.0 */
#include "papinho_secure_transport.h"
#include "pst_backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x,n) if(!(x)){printf("test_multi_backend: FAIL %d\n",n);return n;}
typedef struct mock_global { const char *id; pst_u32 caps; int init_count,shutdown_count,runtime_count,destroy_count,connection_count,fail_init,fail_create; } mock_global;
typedef struct mock_connection { mock_global *global; } mock_connection;
static mock_global ga={"mock-a",PST_CAP_ROLE_CLIENT|PST_CAP_TLS_1_2|PST_CAP_NONBLOCKING|PST_CAP_BACKEND_WAIT,0,0,0,0,0,0,0};
static mock_global gb={"mock-b",PST_CAP_ROLE_CLIENT|PST_CAP_TLS_1_3|PST_CAP_NONBLOCKING|PST_CAP_BACKEND_WAIT,0,0,0,0,0,0,0};
static mock_global gs={"mock-server",PST_CAP_ROLE_SERVER|PST_CAP_TLS_1_2|PST_CAP_LOCAL_IDENTITY|PST_CAP_NONBLOCKING|PST_CAP_BACKEND_WAIT,0,0,0,0,0,0,0};
static mock_global *initializing;
static PST_RESULT initialize(void **out){if(!out)return PST_RESULT_INVALID_ARGUMENT;*out=NULL;if(initializing->fail_init)return PST_RESULT_BACKEND_FAILURE;initializing->init_count++;*out=initializing;return PST_RESULT_OK;}
static void shutdown_provider(void *v){mock_global *g=(mock_global*)v;if(g)g->shutdown_count++;}
static PST_RESULT runtime_create(void *v,void **out){mock_global*g=(mock_global*)v;if(!g||!out)return PST_RESULT_INVALID_ARGUMENT;g->runtime_count++;*out=g;return PST_RESULT_OK;}
static void runtime_destroy(void *v){mock_global*g=(mock_global*)v;if(g)g->destroy_count++;}
static PST_RESULT query(void *v,pst_u32 *caps){mock_global*g=(mock_global*)v;if(!g||!caps)return PST_RESULT_INVALID_ARGUMENT;*caps=g->caps;return PST_RESULT_OK;}
static PST_RESULT validate(void *v,pst_u32 required){mock_global*g=(mock_global*)v;return g&&!(required&~g->caps)?PST_RESULT_OK:PST_RESULT_UNSUPPORTED;}
static PST_RESULT connection_create(void *v,const PST_BACKEND_CONNECTION_OPTIONS *o,void **out){mock_global*g=(mock_global*)v;mock_connection*c;if(!g||!o||!out||o->spi_version!=PST_BACKEND_SPI_VERSION)return PST_RESULT_INVALID_ARGUMENT;if(g->fail_create)return PST_RESULT_BACKEND_FAILURE;c=(mock_connection*)calloc(1,sizeof(*c));if(!c)return PST_RESULT_OUT_OF_MEMORY;c->global=g;g->connection_count++;*out=c;return PST_RESULT_OK;}
static void connection_destroy(void *v){free(v);}
static PST_RESULT attach(void *v,void*t,pst_u32 own,pst_u32*accepted){(void)t;if(!v||!accepted)return PST_RESULT_INVALID_ARGUMENT;*accepted=own==PST_OWNERSHIP_TRANSFERRED;return *accepted?PST_RESULT_OK:PST_RESULT_UNSUPPORTED;}
static PST_RESULT step(void*v,pst_u32*op,PST_RESULT*e){if(!v||!op||!e)return PST_RESULT_INVALID_ARGUMENT;*op=PST_OPERATION_COMPLETE;*e=PST_RESULT_OK;return PST_RESULT_OK;}
static PST_RESULT interest(void*v,pst_u32*i){if(!v||!i)return PST_RESULT_INVALID_ARGUMENT;*i=PST_INTEREST_READ;return PST_RESULT_OK;}
static PST_RESULT wait_fn(void*v,pst_u32 i,pst_u32 t,PST_BACKEND_WAIT_RESULT*r){(void)i;(void)t;if(!v||!r)return PST_RESULT_INVALID_ARGUMENT;r->ready_interest=PST_INTEREST_READ;r->timed_out=0;return PST_RESULT_OK;}
static PST_RESULT read_fn(void*v,void*b,pst_size n,PST_BACKEND_IO_RESULT*r){(void)b;(void)n;if(!v||!r)return PST_RESULT_INVALID_ARGUMENT;memset(r,0,sizeof(*r));r->operation=PST_OPERATION_COMPLETE;return PST_RESULT_OK;}
static PST_RESULT write_fn(void*v,const void*b,pst_size n,PST_BACKEND_IO_RESULT*r){return read_fn(v,(void*)b,n,r);}
static const PST_BACKEND_VTABLE vtable={sizeof(PST_BACKEND_VTABLE),PST_BACKEND_SPI_VERSION,initialize,shutdown_provider,runtime_create,runtime_destroy,query,validate,connection_create,connection_destroy,attach,step,interest,wait_fn,read_fn,write_fn,step,NULL,NULL,NULL,NULL};
static const PST_BACKEND_DESCRIPTOR da={sizeof(PST_BACKEND_DESCRIPTOR),PST_BACKEND_SPI_VERSION,"mock-a","mock a",PST_CAP_ROLE_CLIENT|PST_CAP_TLS_1_2|PST_CAP_NONBLOCKING|PST_CAP_BACKEND_WAIT,&vtable,NULL};
static const PST_BACKEND_DESCRIPTOR db={sizeof(PST_BACKEND_DESCRIPTOR),PST_BACKEND_SPI_VERSION,"mock-b","mock b",PST_CAP_ROLE_CLIENT|PST_CAP_TLS_1_3|PST_CAP_NONBLOCKING|PST_CAP_BACKEND_WAIT,&vtable,NULL};
static const PST_BACKEND_DESCRIPTOR ds={sizeof(PST_BACKEND_DESCRIPTOR),PST_BACKEND_SPI_VERSION,"mock-server","mock server",PST_CAP_ROLE_SERVER|PST_CAP_TLS_1_2|PST_CAP_LOCAL_IDENTITY|PST_CAP_NONBLOCKING|PST_CAP_BACKEND_WAIT,&vtable,NULL};
static void runtime_options(PST_RUNTIME_OPTIONS *o){memset(o,0,sizeof(*o));o->struct_size=sizeof(*o);o->api_version=PST_API_VERSION;}
static void config(PST_CONNECTION_CONFIG*c,pst_u32 role,pst_u32 mode,const char*exact,const char*const*ordered,pst_size count,pst_u32 tls){memset(c,0,sizeof(*c));c->struct_size=sizeof(*c);c->api_version=PST_API_VERSION;c->role=role;c->provider_selection.struct_size=sizeof(c->provider_selection);c->provider_selection.api_version=PST_API_VERSION;c->provider_selection.mode=mode;c->provider_selection.exact_provider_id=exact;c->provider_selection.ordered_provider_ids=ordered;c->provider_selection.ordered_provider_count=count;c->local_identity.struct_size=sizeof(c->local_identity);c->local_identity.api_version=PST_API_VERSION;c->peer_authentication.struct_size=sizeof(c->peer_authentication);c->peer_authentication.api_version=PST_API_VERSION;c->peer_authentication.certificate_mode=PST_PEER_CERTIFICATE_DISABLED;c->tls.struct_size=sizeof(c->tls);c->tls.api_version=PST_API_VERSION;c->tls.minimum_version=tls;c->tls.maximum_version=tls;c->alpn.struct_size=sizeof(c->alpn);c->alpn.api_version=PST_API_VERSION;c->alpn.mode=PST_FEATURE_DISABLED;}
static int selected(pst_connection*c,const char*id){PST_PROVIDER_INFO i;memset(&i,0,sizeof(i));i.struct_size=sizeof(i);i.api_version=PST_API_VERSION;return pst_connection_get_provider_info(c,&i)==PST_RESULT_OK&&!strcmp(i.provider_id,id)&&i.initialized;}
int main(void){PST_RUNTIME_OPTIONS ro;PST_RUNTIME_INFO ri;PST_PROVIDER_INFO pi;PST_CONNECTION_CONFIG cc;pst_runtime*r;pst_connection*c;const char*order[2]={"mock-a","mock-b"};
 pst_backend_registry_reset();initializing=&ga;CHECK(pst_backend_register(&da)==PST_RESULT_OK,1);initializing=&gb;CHECK(pst_backend_register(&db)==PST_RESULT_OK,2);initializing=&gs;CHECK(pst_backend_register(&ds)==PST_RESULT_OK,3);runtime_options(&ro);CHECK(pst_runtime_create(&ro,&r)==PST_RESULT_OK,4);memset(&ri,0,sizeof(ri));ri.struct_size=sizeof(ri);ri.api_version=PST_API_VERSION;CHECK(pst_runtime_get_info(r,&ri)==PST_RESULT_OK&&ri.provider_count==3,5);memset(&pi,0,sizeof(pi));pi.struct_size=sizeof(pi);pi.api_version=PST_API_VERSION;CHECK(pst_runtime_get_provider_info(r,0,&pi)==PST_RESULT_OK&&!pi.initialized&&!strcmp(pi.provider_id,"mock-a"),6);
 initializing=&ga;config(&cc,PST_CONNECTION_ROLE_CLIENT,PST_BACKEND_SELECTION_EXACT,"mock-a",NULL,0,PST_TLS_VERSION_1_2);CHECK(pst_connection_create(r,&cc,&c)==PST_RESULT_OK&&selected(c,"mock-a"),7);pst_connection_release(c);CHECK(ga.init_count==1,8);
 initializing=&gb;config(&cc,PST_CONNECTION_ROLE_CLIENT,PST_BACKEND_SELECTION_ORDERED,NULL,order,2,PST_TLS_VERSION_1_3);CHECK(pst_connection_create(r,&cc,&c)==PST_RESULT_OK&&selected(c,"mock-b"),9);pst_connection_release(c);CHECK(ga.init_count==1&&gb.init_count==1,10);
 config(&cc,PST_CONNECTION_ROLE_CLIENT,PST_BACKEND_SELECTION_AUTOMATIC,NULL,NULL,0,PST_TLS_VERSION_1_2);CHECK(pst_connection_create(r,&cc,&c)==PST_RESULT_OK&&selected(c,"mock-a"),11);pst_connection_release(c);CHECK(ga.init_count==1,12);
 gb.fail_create=1;config(&cc,PST_CONNECTION_ROLE_CLIENT,PST_BACKEND_SELECTION_EXACT,"mock-b",NULL,0,PST_TLS_VERSION_1_3);CHECK(pst_connection_create(r,&cc,&c)==PST_RESULT_BACKEND_FAILURE,13);CHECK(ga.connection_count==2,14);gb.fail_create=0;
 config(&cc,PST_CONNECTION_ROLE_SERVER,PST_BACKEND_SELECTION_EXACT,"mock-server",NULL,0,PST_TLS_VERSION_1_2);CHECK(pst_connection_create(r,&cc,&c)==PST_RESULT_POLICY_VIOLATION,15);
 pst_runtime_release(r);CHECK(ga.shutdown_count==1&&gb.shutdown_count==1,16);pst_backend_registry_reset();printf("RUNTIME_REGISTRY=PASS LAZY_INIT=PASS EXACT=PASS ORDERED=PASS AUTOMATIC=REGISTRATION_ORDER PINNING=PASS QUERIES=PASS SERVER_CORE_VALIDATION=PASS\n");printf("test_multi_backend: PASS\n");return 0;}
