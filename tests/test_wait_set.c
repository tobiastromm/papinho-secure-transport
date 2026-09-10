/* SPDX-License-Identifier: MPL-2.0 */
#include "papinho_secure_transport.h"
#include "pst_backend.h"
#include "pst_internal.h"
#include "pst_transport_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x,n) if(!(x)){printf("test_wait_set: FAIL %d\n",n);return n;}
typedef struct mock_state { pst_u32 ready; int fail_read; } mock_state;
static mock_state *states[8];static int state_count,destroy_count,transport_close_count;
static PST_RESULT init(void**o){*o=(void*)1;return PST_RESULT_OK;}static void shut(void*v){(void)v;}
static PST_RESULT rt_create(void*v,void**o){(void)v;*o=(void*)1;return PST_RESULT_OK;}static void rt_destroy(void*v){(void)v;}
static PST_RESULT query(void*v,pst_u32*c){(void)v;*c=PST_CAP_ROLE_CLIENT|PST_CAP_TLS_1_2|PST_CAP_NONBLOCKING|PST_CAP_BACKEND_WAIT;return PST_RESULT_OK;}
static PST_RESULT validate(void*v,pst_u32 c){pst_u32 have;(void)v;query(v,&have);return c&~have?PST_RESULT_UNSUPPORTED:PST_RESULT_OK;}
static PST_RESULT conn_create(void*v,const PST_BACKEND_CONNECTION_OPTIONS*o,void**out){mock_state*s;(void)v;if(!o||!out)return PST_RESULT_INVALID_ARGUMENT;s=(mock_state*)calloc(1,sizeof(*s));if(!s)return PST_RESULT_OUT_OF_MEMORY;states[state_count++]=s;*out=s;return PST_RESULT_OK;}
static void conn_destroy(void*v){destroy_count++;free(v);}static PST_RESULT attach(void*v,void*t,pst_u32 own,pst_u32*a){(void)v;(void)t;*a=own==PST_OWNERSHIP_TRANSFERRED;return *a?PST_RESULT_OK:PST_RESULT_UNSUPPORTED;}
static PST_RESULT step(void*v,pst_u32*op,PST_RESULT*e){(void)v;*op=PST_OPERATION_COMPLETE;*e=PST_RESULT_OK;return PST_RESULT_OK;}static PST_RESULT interest(void*v,pst_u32*i){(void)v;*i=PST_INTEREST_READ;return PST_RESULT_OK;}
static PST_RESULT wait_fn(void*v,pst_u32 i,pst_u32 timeout,PST_BACKEND_WAIT_RESULT*r){mock_state*s=(mock_state*)v;(void)i;if(timeout)return PST_RESULT_BACKEND_FAILURE;r->ready_interest=s->ready;r->timed_out=s->ready?0UL:1UL;return PST_RESULT_OK;}
static PST_RESULT read_fn(void*v,void*b,pst_size n,PST_BACKEND_IO_RESULT*r){mock_state*s=(mock_state*)v;(void)b;(void)n;memset(r,0,sizeof(*r));if(s->fail_read){r->operation=PST_OPERATION_FAILED;r->close_kind=PST_CLOSE_TRUNCATED;r->error=PST_RESULT_TRUNCATED;}else r->operation=PST_OPERATION_NEED_READ;return PST_RESULT_OK;}
static PST_RESULT write_fn(void*v,const void*b,pst_size n,PST_BACKEND_IO_RESULT*r){return read_fn(v,(void*)b,n,r);}
static const PST_BACKEND_VTABLE vt={sizeof(vt),PST_BACKEND_SPI_VERSION,init,shut,rt_create,rt_destroy,query,validate,conn_create,conn_destroy,attach,step,interest,wait_fn,read_fn,write_fn,step,NULL,NULL,NULL,NULL};
static const PST_BACKEND_DESCRIPTOR desc={sizeof(desc),PST_BACKEND_SPI_VERSION,"wait-mock","wait mock",PST_CAP_ROLE_CLIENT|PST_CAP_TLS_1_2|PST_CAP_NONBLOCKING|PST_CAP_BACKEND_WAIT,&vt,NULL,PST_CAP_ROLE_CLIENT|PST_CAP_TLS_1_2|PST_CAP_NONBLOCKING|PST_CAP_BACKEND_WAIT,0UL};
static const PST_BACKEND_DESCRIPTOR desc2={sizeof(desc2),PST_BACKEND_SPI_VERSION,"wait-mock-2","wait mock 2",PST_CAP_ROLE_CLIENT|PST_CAP_TLS_1_2|PST_CAP_NONBLOCKING|PST_CAP_BACKEND_WAIT,&vt,NULL,PST_CAP_ROLE_CLIENT|PST_CAP_TLS_1_2|PST_CAP_NONBLOCKING|PST_CAP_BACKEND_WAIT,0UL};
static pst_connection*make_created(pst_runtime*r,const char*provider){PST_CONNECTION_CONFIG c;pst_connection*x=NULL;memset(&c,0,sizeof(c));c.struct_size=sizeof(c);c.api_version=PST_API_VERSION;c.role=PST_CONNECTION_ROLE_CLIENT;c.provider_selection.struct_size=sizeof(c.provider_selection);c.provider_selection.api_version=PST_API_VERSION;c.provider_selection.mode=PST_BACKEND_SELECTION_EXACT;c.provider_selection.exact_provider_id=provider;c.local_identity.struct_size=sizeof(c.local_identity);c.local_identity.api_version=PST_API_VERSION;c.peer_authentication.struct_size=sizeof(c.peer_authentication);c.peer_authentication.api_version=PST_API_VERSION;c.peer_authentication.certificate_mode=PST_PEER_CERTIFICATE_DISABLED;c.tls.struct_size=sizeof(c.tls);c.tls.api_version=PST_API_VERSION;c.tls.minimum_version=c.tls.maximum_version=PST_TLS_VERSION_1_2;c.alpn.struct_size=sizeof(c.alpn);c.alpn.api_version=PST_API_VERSION;return pst_connection_create(r,&c,&x)==PST_RESULT_OK?x:NULL;}
static void td(pst_transport*t,int consumed){if(!consumed)transport_close_count++;free(t);}static pst_connection*make(pst_runtime*r,const char*provider){PST_CONNECTION_CONFIG c;pst_connection*x=NULL;pst_transport*t;pst_u32 accepted,op;PST_RESULT e;memset(&c,0,sizeof(c));c.struct_size=sizeof(c);c.api_version=PST_API_VERSION;c.role=PST_CONNECTION_ROLE_CLIENT;c.provider_selection.struct_size=sizeof(c.provider_selection);c.provider_selection.api_version=PST_API_VERSION;c.provider_selection.mode=PST_BACKEND_SELECTION_EXACT;c.provider_selection.exact_provider_id=provider;c.local_identity.struct_size=sizeof(c.local_identity);c.local_identity.api_version=PST_API_VERSION;c.peer_authentication.struct_size=sizeof(c.peer_authentication);c.peer_authentication.api_version=PST_API_VERSION;c.peer_authentication.certificate_mode=PST_PEER_CERTIFICATE_DISABLED;c.tls.struct_size=sizeof(c.tls);c.tls.api_version=PST_API_VERSION;c.tls.minimum_version=c.tls.maximum_version=PST_TLS_VERSION_1_2;c.alpn.struct_size=sizeof(c.alpn);c.alpn.api_version=PST_API_VERSION;if(pst_connection_create(r,&c,&x)!=PST_RESULT_OK)return NULL;t=(pst_transport*)calloc(1,sizeof(*t));if(!t)return NULL;t->backend_id=provider;t->native=t;t->destroy=td;if(pst_connection_attach(x,t,PST_OWNERSHIP_TRANSFERRED,&accepted)!=PST_RESULT_OK||!accepted)return NULL;if(pst_connection_handshake(x,&op,&e)!=PST_RESULT_OK||op!=PST_OPERATION_COMPLETE)return NULL;return x;}
int main(void)
{
 PST_RUNTIME_OPTIONS o;pst_runtime*r;pst_connection*a,*b,*c,*d,*e;pst_wait_set*s,*s2;PST_WAIT_EVENT ev[3];PST_WAIT_SET_RESULT wr;PST_IO_RESULT io;char byte;
 memset(&o,0,sizeof(o));o.struct_size=sizeof(o);o.api_version=PST_API_VERSION;pst_backend_registry_reset();
 CHECK(pst_backend_register(&desc)==PST_RESULT_OK,1);CHECK(pst_backend_register(&desc2)==PST_RESULT_OK,2);CHECK(pst_runtime_create(&o,&r)==PST_RESULT_OK,3);
 CHECK(pst_wait_set_create(NULL)==PST_RESULT_INVALID_ARGUMENT,4);CHECK(pst_wait_set_create(&s)==PST_RESULT_OK&&pst_wait_set_destroy(s)==PST_RESULT_OK,5);
 CHECK(pst_wait_set_create(&s)==PST_RESULT_OK&&pst_wait_set_create(&s2)==PST_RESULT_OK,6);
 a=make(r,"wait-mock");b=make(r,"wait-mock");c=make(r,"wait-mock-2");d=make(r,"wait-mock-2");e=make_created(r,"wait-mock");CHECK(a&&b&&c&&d&&e,7);
 CHECK(pst_wait_set_add_connection(NULL,a,1)==PST_RESULT_INVALID_ARGUMENT&&pst_wait_set_add_connection(s,NULL,1)==PST_RESULT_INVALID_ARGUMENT,8);
 CHECK(pst_wait_set_add_connection(s,a,30)==PST_RESULT_OK,9);CHECK(pst_wait_set_add_connection(s,a,31)==PST_RESULT_ALREADY_REGISTERED,10);CHECK(pst_wait_set_add_connection(s2,a,32)==PST_RESULT_ALREADY_REGISTERED,11);CHECK(pst_wait_set_add_connection(s,b,30)==PST_RESULT_ALREADY_REGISTERED,12);
 CHECK(pst_wait_set_remove_connection(s,c)==PST_RESULT_NOT_REGISTERED&&pst_wait_set_remove_connection(NULL,c)==PST_RESULT_INVALID_ARGUMENT,13);CHECK(pst_wait_set_destroy(s)==PST_RESULT_INVALID_STATE,14);
 CHECK(pst_connection_try_release(a)==PST_RESULT_INVALID_STATE&&destroy_count==0&&transport_close_count==0,15);pst_connection_release(a);CHECK(destroy_count==0,16);
 CHECK(pst_wait_set_add_connection(s,b,10)==PST_RESULT_OK&&pst_wait_set_add_connection(s,c,20)==PST_RESULT_OK,17);
 states[0]->ready=states[1]->ready=states[2]->ready=PST_INTEREST_READ;memset(&wr,0,sizeof(wr));
 CHECK(pst_wait_set_wait(NULL,0,ev,3,&wr)==PST_RESULT_INVALID_ARGUMENT&&pst_wait_set_wait(s,0,ev,3,NULL)==PST_RESULT_INVALID_ARGUMENT&&pst_wait_set_wait(s,0,NULL,1,&wr)==PST_RESULT_INVALID_ARGUMENT,18);
 CHECK(pst_wait_set_wait(s,0,ev,2,&wr)==PST_RESULT_INSUFFICIENT_CAPACITY&&wr.ready_count==3&&wr.event_count==2&&ev[0].token==30&&ev[1].token==10,19);
 CHECK(pst_wait_set_wait(s,0,NULL,0,&wr)==PST_RESULT_INSUFFICIENT_CAPACITY&&wr.ready_count==3&&wr.event_count==0,20);CHECK(pst_wait_set_wait(s,0,ev,3,&wr)==PST_RESULT_OK&&ev[2].token==20,21);
 states[0]->ready=states[1]->ready=states[2]->ready=0;states[2]->ready=PST_INTEREST_WRITE;CHECK(pst_wait_set_wait(s,0,ev,3,&wr)==PST_RESULT_OK&&wr.ready_count==1&&ev[0].token==20&&ev[0].ready_interest==PST_INTEREST_WRITE,22);
 states[0]->ready=states[1]->ready=states[2]->ready=0;CHECK(pst_wait_set_wait(s,0,ev,3,&wr)==PST_RESULT_WAIT_TIMEOUT&&wr.timed_out&&wr.ready_count==0,23);CHECK(pst_wait_set_wait(s,1,ev,3,&wr)==PST_RESULT_UNSUPPORTED,24);
 states[1]->fail_read=1;states[2]->ready=PST_INTEREST_READ;CHECK(pst_connection_read(b,&byte,1,&io)==PST_RESULT_OK&&io.operation==PST_OPERATION_FAILED,25);
 CHECK(pst_wait_set_wait(s,0,ev,3,&wr)==PST_RESULT_OK&&wr.ready_count==2&&ev[0].token==10&&(ev[0].flags&PST_WAIT_READY_TERMINAL)&&ev[0].result==PST_RESULT_TRUNCATED&&ev[1].token==20,26);
 CHECK(pst_wait_set_wait(s,0,ev,3,&wr)==PST_RESULT_OK&&ev[0].result==PST_RESULT_TRUNCATED,27);
 CHECK(pst_wait_set_remove_connection(s,a)==PST_RESULT_OK&&pst_wait_set_add_connection(s,a,40)==PST_RESULT_OK,28);CHECK(pst_wait_set_wait(s,0,ev,3,&wr)==PST_RESULT_OK&&ev[0].token==10&&ev[1].token==20,29);
 CHECK(pst_wait_set_remove_connection(s,a)==PST_RESULT_OK&&pst_connection_try_release(a)==PST_RESULT_OK,30);CHECK(pst_wait_set_remove_connection(s,b)==PST_RESULT_OK&&pst_wait_set_remove_connection(s,c)==PST_RESULT_OK,31);
 CHECK(pst_wait_set_destroy(s2)==PST_RESULT_OK&&pst_wait_set_create(&s2)==PST_RESULT_OK,32);pst_wait_set_test_fail_next_growth();CHECK(pst_wait_set_add_connection(s2,d,50)==PST_RESULT_OUT_OF_MEMORY,33);
 CHECK(pst_wait_set_add_connection(s2,d,50)==PST_RESULT_OK,34);CHECK(pst_wait_set_remove_connection(s2,d)==PST_RESULT_OK&&pst_connection_try_release(d)==PST_RESULT_OK,35);
 CHECK(pst_wait_set_add_connection(s2,e,60)==PST_RESULT_OK&&pst_wait_set_wait(s2,0,ev,3,&wr)==PST_RESULT_INVALID_STATE,36);CHECK(pst_wait_set_remove_connection(s2,e)==PST_RESULT_OK&&pst_connection_try_release(e)==PST_RESULT_OK,37);
 CHECK(pst_connection_try_release(b)==PST_RESULT_OK&&pst_connection_try_release(c)==PST_RESULT_OK,38);CHECK(pst_wait_set_destroy(s)==PST_RESULT_OK&&pst_wait_set_destroy(s2)==PST_RESULT_OK,39);
 pst_runtime_release(r);pst_backend_registry_reset();CHECK(destroy_count==5&&transport_close_count==0,40);
 printf("WAIT_SET_CREATE_DESTROY_EMPTY=PASS WAIT_SET_ADD_ONE=PASS WAIT_SET_REMOVE_ONE=PASS DUPLICATE_CONNECTION_REJECTED=PASS DUPLICATE_TOKEN_REJECTED=PASS REMOVE_ABSENT_REJECTED=PASS ONE_WAIT_SET_PER_CONNECTION=PASS REMOVE_THEN_READD=PASS REMOVE_THEN_RELEASE=PASS RELEASE_REGISTERED_CONNECTION_REJECTED=PASS REGISTERED_RELEASE_DOES_NOT_CLOSE_TRANSPORT=PASS DESTROY_WITH_MEMBERS_REJECTED=PASS\n");
 printf("MULTIPLE_MEMBERS=PASS MIXED_PROVIDER_MEMBERSHIP=PASS ONE_READY_AMONG_MANY=PASS MULTIPLE_READY=PASS TERMINAL_PLUS_READY=PASS BOUNDED_ENUMERATION=PASS INSUFFICIENT_CAPACITY=PASS STABLE_REGISTRATION_ORDER=PASS READINESS_NOT_CONSUMED_BY_ENUMERATION=PASS TIMEOUT_ZERO_NONBLOCKING=PASS TIMEOUT_ZERO_NO_READY=PASS TIMEOUT_ZERO_READY=PASS\n");
 printf("TERMINAL_MEMBER_VISIBLE=PASS TERMINAL_MEMBER_REMAINS_VISIBLE=PASS TERMINAL_CAUSE_IMMUTABLE=PASS PARTIAL_ADD_FAILURE_CLEANUP=PASS WAIT_SET_REUSABLE_AFTER_NONTERMINAL_ERROR=PASS NO_CORRUPT_MEMBERSHIP_AFTER_FAILURE=PASS NO_LEAK_AFTER_FAILURE=PASS NO_DOUBLE_CLOSE=PASS NO_MEMBER_LEAK=PASS\n");
 printf("test_wait_set: PASS\n");return 0;
}
