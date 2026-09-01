#include "pst_backend.h"
#include "pst_internal.h"
#include <stdio.h>
#include <string.h>
typedef struct creation_state { pst_internal_diagnostic diagnostic; int initialized; int shutdowns; } creation_state;
static creation_state g_a,g_b,g_c,g_oom;
static int g_connection_fail;
#define CHECK(c,n) if(!(c))return(n)
static PST_RESULT init_failure(creation_state *s,const char *id,PST_RESULT result,pst_u32 domain,pst_i32 code,void **out)
{
    pst_diagnostic_initialize(&s->diagnostic);
    pst_diagnostic_capture(&s->diagnostic,result,PST_DIAGNOSTIC_PHASE_BACKEND_INITIALIZE,id,domain,code,0,domain==PST_DIAGNOSTIC_DOMAIN_NONE?0:PST_DIAGNOSTIC_FLAG_NATIVE);
    *out=s; return result;
}
static PST_RESULT init_a(void **out){return init_failure(&g_a,"create-fail-a",PST_RESULT_UNAVAILABLE,PST_DIAGNOSTIC_DOMAIN_WIN32,126,out);}
static PST_RESULT init_c(void **out){return init_failure(&g_c,"create-fail-c",PST_RESULT_BACKEND_FAILURE,PST_DIAGNOSTIC_DOMAIN_NSS,-8179,out);}
static PST_RESULT init_oom(void **out){(void)out;return PST_RESULT_OUT_OF_MEMORY;}
static PST_RESULT init_b(void **out){pst_diagnostic_initialize(&g_b.diagnostic);g_b.initialized=1;*out=&g_b;return PST_RESULT_OK;}
static void mock_shutdown(void *state){creation_state *s=(creation_state*)state;if(s){s->initialized=0;++s->shutdowns;pst_diagnostic_clear(&s->diagnostic);}}
static PST_RESULT mock_runtime_create(void *state,void **out){*out=state;return PST_RESULT_OK;}
static void mock_runtime_destroy(void *state){(void)state;}
static PST_RESULT mock_query(void *state,pst_u32 *caps){if(!state||!caps)return PST_RESULT_INVALID_ARGUMENT;*caps=PST_BACKEND_CAP_TLS_1_2|PST_BACKEND_CAP_NONBLOCKING|PST_BACKEND_CAP_BACKEND_WAIT;return PST_RESULT_OK;}
static PST_RESULT mock_validate(void *state,pst_u32 required){pst_u32 caps;if(mock_query(state,&caps)!=PST_RESULT_OK)return PST_RESULT_INVALID_ARGUMENT;return (required&~caps)?PST_RESULT_UNSUPPORTED:PST_RESULT_OK;}
static PST_RESULT mock_connection_create(void *state,void **out){creation_state *s=(creation_state*)state;if(g_connection_fail){pst_diagnostic_capture(&s->diagnostic,PST_RESULT_BACKEND_FAILURE,PST_DIAGNOSTIC_PHASE_CONNECTION_CREATE,"create-ok-b",PST_DIAGNOSTIC_DOMAIN_NSS,-12286,0,PST_DIAGNOSTIC_FLAG_NATIVE);return PST_RESULT_BACKEND_FAILURE;}*out=state;return PST_RESULT_OK;}
static void mock_connection_destroy(void *state){(void)state;}
static PST_RESULT mock_attach(void *s,void *t,pst_u32 o,pst_u32 *a){(void)s;(void)t;(void)o;if(a)*a=0;return PST_RESULT_UNSUPPORTED;}
static PST_RESULT mock_step(void *s,pst_u32 *o,PST_RESULT *e){(void)s;*o=PST_BACKEND_OPERATION_COMPLETE;*e=PST_RESULT_OK;return PST_RESULT_OK;}
static PST_RESULT mock_interest(void *s,pst_u32 *i){(void)s;*i=0;return PST_RESULT_OK;}
static PST_RESULT mock_wait(void *s,pst_u32 i,pst_u32 t,PST_BACKEND_WAIT_RESULT *r){(void)s;(void)i;(void)t;r->ready_interest=0;r->timed_out=1;return PST_RESULT_OK;}
static PST_RESULT mock_read(void *s,void *b,pst_size n,PST_BACKEND_IO_RESULT *r){(void)s;(void)b;(void)n;memset(r,0,sizeof(*r));return PST_RESULT_OK;}
static PST_RESULT mock_write(void *s,const void *b,pst_size n,PST_BACKEND_IO_RESULT *r){return mock_read(s,(void*)b,n,r);}
static PST_RESULT mock_configure(void *state,const pst_config *config){return state&&config?PST_RESULT_OK:PST_RESULT_INVALID_ARGUMENT;}
static void mock_diagnostic_copy(const void *state,pst_internal_diagnostic *out){if(!out)return;if(!state){pst_diagnostic_initialize(out);return;}pst_diagnostic_copy(out,&((const creation_state*)state)->diagnostic);}
#define VTABLE(name,initfn) static const PST_BACKEND_VTABLE name={sizeof(PST_BACKEND_VTABLE),PST_BACKEND_SPI_VERSION,initfn,mock_shutdown,mock_runtime_create,mock_runtime_destroy,mock_query,mock_validate,mock_connection_create,mock_connection_destroy,mock_attach,mock_step,mock_interest,mock_wait,mock_read,mock_write,mock_step,NULL,NULL,mock_configure,NULL,mock_diagnostic_copy}
VTABLE(v_a,init_a); VTABLE(v_b,init_b); VTABLE(v_c,init_c); VTABLE(v_oom,init_oom);
static const PST_BACKEND_DESCRIPTOR d_a={sizeof(PST_BACKEND_DESCRIPTOR),PST_BACKEND_SPI_VERSION,"create-fail-a","fail A",PST_BACKEND_CAP_TLS_1_2|PST_BACKEND_CAP_NONBLOCKING|PST_BACKEND_CAP_BACKEND_WAIT,&v_a};
static const PST_BACKEND_DESCRIPTOR d_b={sizeof(PST_BACKEND_DESCRIPTOR),PST_BACKEND_SPI_VERSION,"create-ok-b","success B",PST_BACKEND_CAP_TLS_1_2|PST_BACKEND_CAP_NONBLOCKING|PST_BACKEND_CAP_BACKEND_WAIT,&v_b};
static const PST_BACKEND_DESCRIPTOR d_c={sizeof(PST_BACKEND_DESCRIPTOR),PST_BACKEND_SPI_VERSION,"create-fail-c","fail C",PST_BACKEND_CAP_TLS_1_2|PST_BACKEND_CAP_NONBLOCKING|PST_BACKEND_CAP_BACKEND_WAIT,&v_c};
static const PST_BACKEND_DESCRIPTOR d_oom={sizeof(PST_BACKEND_DESCRIPTOR),PST_BACKEND_SPI_VERSION,"create-oom","oom",PST_BACKEND_CAP_TLS_1_2|PST_BACKEND_CAP_NONBLOCKING|PST_BACKEND_CAP_BACKEND_WAIT,&v_oom};
static void options(PST_RUNTIME_OPTIONS *o,pst_u32 selection,const char *exact,const char **ordered,pst_size count){memset(o,0,sizeof(*o));o->struct_size=sizeof(*o);o->api_version=PST_API_VERSION;o->selection=selection;o->exact_backend_id=exact;o->preferred_backend_ids=ordered;o->preferred_backend_count=count;}
int main(void)
{
    PST_RUNTIME_OPTIONS o; pst_internal_operation_context a,c,success,all,oom,connection; pst_internal_diagnostic da,dc,saved; pst_runtime *runtime; pst_connection *conn; pst_config *config; const char *ordered[2]; pst_u32 g;
    pst_backend_registry_reset(); CHECK(pst_backend_register(&d_a)==PST_RESULT_OK,1);CHECK(pst_backend_register(&d_b)==PST_RESULT_OK,2);CHECK(pst_backend_register(&d_c)==PST_RESULT_OK,3);CHECK(pst_backend_register(&d_oom)==PST_RESULT_OK,4);
    options(&o,PST_BACKEND_SELECTION_EXACT,"missing-backend",NULL,0);runtime=NULL;CHECK(pst_runtime_create_internal(&o,&runtime,&a)==PST_RESULT_UNSUPPORTED,5);pst_internal_operation_context_diagnostic_copy(&a,&da);CHECK(da.valid&&da.result==PST_RESULT_UNSUPPORTED&&da.phase==PST_DIAGNOSTIC_PHASE_BACKEND_SELECT,6);CHECK(da.native_domain==PST_DIAGNOSTIC_DOMAIN_NONE&&da.native_code==0&&da.backend_id[0]=='\0',7);
    options(&o,PST_BACKEND_SELECTION_EXACT,"create-fail-a",NULL,0);CHECK(pst_runtime_create_internal(&o,&runtime,&a)==PST_RESULT_UNSUPPORTED,8);pst_internal_operation_context_diagnostic_copy(&a,&da);CHECK(da.valid&&da.result==PST_RESULT_UNAVAILABLE&&da.native_domain==PST_DIAGNOSTIC_DOMAIN_WIN32&&da.native_code==126,9);CHECK(!strcmp(da.backend_id,"create-fail-a")&&g_a.shutdowns==1&&!g_a.diagnostic.valid,10);pst_diagnostic_copy(&saved,&da);
    options(&o,PST_BACKEND_SELECTION_EXACT,"create-fail-c",NULL,0);CHECK(pst_runtime_create_internal(&o,&runtime,&c)==PST_RESULT_UNSUPPORTED,11);pst_internal_operation_context_diagnostic_copy(&c,&dc);CHECK(dc.valid&&dc.native_domain==PST_DIAGNOSTIC_DOMAIN_NSS&&dc.native_code==-8179,12);CHECK(saved.native_code==126&&!strcmp(saved.backend_id,"create-fail-a"),13);
    ordered[0]="create-fail-a";ordered[1]="create-ok-b";options(&o,PST_BACKEND_SELECTION_ORDERED,NULL,ordered,2);CHECK(pst_runtime_create_internal(&o,&runtime,&success)==PST_RESULT_OK,14);pst_internal_operation_context_diagnostic_copy(&success,&da);CHECK(!da.valid&&da.generation==2UL,15);pst_runtime_release(runtime);
    ordered[0]="create-fail-a";ordered[1]="create-fail-c";options(&o,PST_BACKEND_SELECTION_ORDERED,NULL,ordered,2);CHECK(pst_runtime_create_internal(&o,&runtime,&all)==PST_RESULT_UNSUPPORTED,16);pst_internal_operation_context_diagnostic_copy(&all,&da);CHECK(da.valid&&da.native_code==-8179&&!strcmp(da.backend_id,"create-fail-c")&&da.generation==2UL,17);
    options(&o,PST_BACKEND_SELECTION_EXACT,"create-oom",NULL,0);CHECK(pst_runtime_create_internal(&o,&runtime,&oom)==PST_RESULT_UNSUPPORTED,18);pst_internal_operation_context_diagnostic_copy(&oom,&da);CHECK(da.valid&&da.result==PST_RESULT_OUT_OF_MEMORY&&da.native_domain==PST_DIAGNOSTIC_DOMAIN_NONE&&da.native_code==0,19);
    options(&o,PST_BACKEND_SELECTION_EXACT,"create-ok-b",NULL,0);CHECK(pst_runtime_create_internal(&o,&runtime,&success)==PST_RESULT_OK,20);CHECK(pst_config_create(&config)==PST_RESULT_OK&&pst_config_freeze(config)==PST_RESULT_OK,21);g_connection_fail=1;conn=NULL;CHECK(pst_connection_create_internal(runtime,config,&conn,&connection)==PST_RESULT_BACKEND_FAILURE,22);pst_internal_operation_context_diagnostic_copy(&connection,&da);CHECK(da.valid&&da.phase==PST_DIAGNOSTIC_PHASE_CONNECTION_CREATE&&da.native_code==-12286,23);g=da.generation;pst_internal_operation_context_reset(&connection);pst_internal_operation_context_diagnostic_copy(&connection,&dc);CHECK(!dc.valid&&dc.generation==g+1UL&&g==1UL,24);pst_diagnostic_copy(&saved,&da);pst_config_release(config);pst_runtime_release(runtime);CHECK(saved.valid&&saved.native_code==-12286,25);
    CHECK(strstr(saved.backend_id,"create-ok-b")!=NULL,26);
    pst_backend_registry_reset();printf("test_diagnostic_creation: PASS\n");return 0;
}