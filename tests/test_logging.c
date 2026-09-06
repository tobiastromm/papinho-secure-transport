/* SPDX-License-Identifier: MPL-2.0 */
#include "pst_backend.h"
#include "pst_internal.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
typedef struct test_sink { pst_u32 count;void *context;PST_LOG_EVENT last; } test_sink;
typedef struct larger_log_config { PST_LOG_CONFIG base;unsigned char tail[8]; } larger_log_config;
typedef struct mock_state { pst_internal_diagnostic diagnostic; } mock_state;
static mock_state g_state;
#define CHECK(c,n) if(!(c))return(n)
static void PST_CALL sink_callback(void *context,const PST_LOG_EVENT *event){test_sink *sink=(test_sink*)context;++sink->count;sink->context=context;sink->last=*event;}
static PST_RESULT mock_initialize(void **out){pst_diagnostic_initialize(&g_state.diagnostic);*out=&g_state;return PST_RESULT_OK;}
static void mock_shutdown(void *state){(void)state;}
static PST_RESULT mock_runtime_create(void *state,void **out){*out=state;return PST_RESULT_OK;}
static void mock_runtime_destroy(void *state){(void)state;}
static PST_RESULT mock_query(void *state,pst_u32 *caps){if(!state||!caps)return PST_RESULT_INVALID_ARGUMENT;*caps=PST_BACKEND_CAP_TLS_1_2|PST_BACKEND_CAP_NONBLOCKING|PST_BACKEND_CAP_BACKEND_WAIT;return PST_RESULT_OK;}
static PST_RESULT mock_validate(void *state,pst_u32 required){pst_u32 caps;if(mock_query(state,&caps)!=PST_RESULT_OK)return PST_RESULT_INVALID_ARGUMENT;return (required&~caps)?PST_RESULT_UNSUPPORTED:PST_RESULT_OK;}
static PST_RESULT mock_connection_create(void *state,void **out){*out=state;return PST_RESULT_OK;}
static void mock_connection_destroy(void *state){(void)state;}
static PST_RESULT mock_attach(void *s,void *t,pst_u32 o,pst_u32 *a){(void)s;(void)t;(void)o;if(a)*a=0;return PST_RESULT_UNSUPPORTED;}
static PST_RESULT mock_step(void *s,pst_u32 *o,PST_RESULT *e){(void)s;*o=PST_BACKEND_OPERATION_COMPLETE;*e=PST_RESULT_OK;return PST_RESULT_OK;}
static PST_RESULT mock_interest(void *s,pst_u32 *i){(void)s;*i=0;return PST_RESULT_OK;}
static PST_RESULT mock_wait(void *s,pst_u32 i,pst_u32 t,PST_BACKEND_WAIT_RESULT *r){(void)s;(void)i;(void)t;r->ready_interest=0;r->timed_out=1;return PST_RESULT_OK;}
static PST_RESULT mock_read(void *s,void *b,pst_size n,PST_BACKEND_IO_RESULT *r){(void)s;(void)b;(void)n;memset(r,0,sizeof(*r));return PST_RESULT_OK;}
static PST_RESULT mock_write(void *s,const void *b,pst_size n,PST_BACKEND_IO_RESULT *r){return mock_read(s,(void*)b,n,r);}
static PST_RESULT mock_configure(void *state,const pst_config *config){return state&&config?PST_RESULT_OK:PST_RESULT_INVALID_ARGUMENT;}
static void mock_diagnostic_copy(const void *state,pst_internal_diagnostic *out){if(!out)return;if(!state){pst_diagnostic_initialize(out);return;}pst_diagnostic_copy(out,&((const mock_state*)state)->diagnostic);}
static const PST_BACKEND_VTABLE vtable={sizeof(PST_BACKEND_VTABLE),PST_BACKEND_SPI_VERSION,mock_initialize,mock_shutdown,mock_runtime_create,mock_runtime_destroy,mock_query,mock_validate,mock_connection_create,mock_connection_destroy,mock_attach,mock_step,mock_interest,mock_wait,mock_read,mock_write,mock_step,NULL,NULL,mock_configure,NULL,mock_diagnostic_copy};
static const PST_BACKEND_DESCRIPTOR descriptor={sizeof(PST_BACKEND_DESCRIPTOR),PST_BACKEND_SPI_VERSION,"logging-mock","logging mock",PST_BACKEND_CAP_TLS_1_2|PST_BACKEND_CAP_NONBLOCKING|PST_BACKEND_CAP_BACKEND_WAIT,&vtable};
static void options(PST_RUNTIME_OPTIONS *o,const char *id){memset(o,0,sizeof(*o));o->struct_size=sizeof(*o);o->api_version=PST_API_VERSION;o->selection=PST_BACKEND_SELECTION_EXACT;o->exact_backend_id=id;}
static int check_abi(void)
{
    volatile pst_size value;volatile pst_u32 constant;
    constant=PST_LOG_LEVEL_OFF;CHECK(constant==0UL,1);constant=PST_LOG_LEVEL_ERROR;CHECK(constant==1UL,2);
    constant=PST_LOG_LEVEL_WARN;CHECK(constant==2UL,3);constant=PST_LOG_LEVEL_INFO;CHECK(constant==3UL,4);
    constant=PST_LOG_LEVEL_DEBUG;CHECK(constant==4UL,5);constant=PST_LOG_LEVEL_TRACE;CHECK(constant==5UL,6);
    value=sizeof(PST_LOG_EVENT);CHECK(value==60UL,3);
    value=offsetof(PST_LOG_EVENT,struct_size);CHECK(value==0UL,4);
    value=offsetof(PST_LOG_EVENT,api_version);CHECK(value==4UL,5);
    value=offsetof(PST_LOG_EVENT,level);CHECK(value==8UL,6);
    value=offsetof(PST_LOG_EVENT,event_id);CHECK(value==12UL,7);
    value=offsetof(PST_LOG_EVENT,category);CHECK(value==16UL,8);
    value=offsetof(PST_LOG_EVENT,normalized_result);CHECK(value==20UL,9);
    value=offsetof(PST_LOG_EVENT,operation);CHECK(value==24UL,10);
    value=offsetof(PST_LOG_EVENT,backend_id);CHECK(value==28UL,11);
    value=sizeof(PST_LOG_CONFIG);CHECK(value==20UL,12);
    value=offsetof(PST_LOG_CONFIG,struct_size);CHECK(value==0UL,13);
    value=offsetof(PST_LOG_CONFIG,api_version);CHECK(value==4UL,14);
    value=offsetof(PST_LOG_CONFIG,level);CHECK(value==8UL,15);
    value=offsetof(PST_LOG_CONFIG,callback);CHECK(value==12UL,16);
    value=offsetof(PST_LOG_CONFIG,user_context);CHECK(value==16UL,17);
    constant=PST_LOG_EVENT_RUNTIME_READY;CHECK(constant==1UL,18);constant=PST_LOG_EVENT_OPERATION_PROGRESS;CHECK(constant==8UL,19);
    constant=PST_LOG_CATEGORY_RUNTIME;CHECK(constant==1UL,20);constant=PST_LOG_CATEGORY_SHUTDOWN;CHECK(constant==8UL,21);
    return 0;
}
static int check_matrix(void)
{
    PST_LOG_LEVEL configured,event_level;int expected;
    for(configured=PST_LOG_LEVEL_OFF;configured<=PST_LOG_LEVEL_TRACE;++configured)
        for(event_level=PST_LOG_LEVEL_OFF;event_level<=PST_LOG_LEVEL_TRACE;++event_level){
            expected=configured!=PST_LOG_LEVEL_OFF&&event_level!=PST_LOG_LEVEL_OFF&&event_level<=configured;
            CHECK(pst_log_should_emit(configured,event_level)==expected,20);
        }
    CHECK(!pst_log_should_emit(99UL,PST_LOG_LEVEL_ERROR),21);
    CHECK(!pst_log_should_emit(PST_LOG_LEVEL_TRACE,99UL),22);
    return 0;
}
static int check_config(void)
{
    PST_LOG_CONFIG config;larger_log_config larger;pst_log_state state;test_sink sink;pst_size i;
    memset(&sink,0,sizeof(sink));CHECK(pst_log_config_init(NULL)==PST_RESULT_INVALID_ARGUMENT,30);
    memset(&config,0xa5,sizeof(config));CHECK(pst_log_config_init(&config)==PST_RESULT_OK,31);
    CHECK(config.struct_size==sizeof(config)&&config.api_version==PST_API_VERSION&&config.level==PST_LOG_LEVEL_OFF,32);
    CHECK(config.callback==NULL&&config.user_context==NULL,33);
    config.struct_size=PST_LOG_CONFIG_MIN_SIZE-1UL;CHECK(pst_log_state_initialize(&state,&config)==PST_RESULT_INVALID_ARGUMENT,34);
    config.struct_size=sizeof(config);config.api_version=0x00020000UL;CHECK(pst_log_state_initialize(&state,&config)==PST_RESULT_INCOMPATIBLE_API,35);
    config.api_version=PST_API_VERSION;config.level=99UL;CHECK(pst_log_state_initialize(&state,&config)==PST_RESULT_INVALID_ARGUMENT,36);
    config.level=PST_LOG_LEVEL_TRACE;config.callback=NULL;config.user_context=&sink;CHECK(pst_log_state_initialize(&state,&config)==PST_RESULT_OK,37);
    pst_log_emit(&state,PST_LOG_LEVEL_ERROR,PST_LOG_EVENT_RUNTIME_FAILURE,PST_LOG_CATEGORY_RUNTIME,PST_RESULT_UNAVAILABLE,PST_DIAGNOSTIC_OPERATION_RUNTIME,"logging-mock");CHECK(!sink.count,38);
    config.callback=sink_callback;CHECK(pst_log_state_initialize(&state,&config)==PST_RESULT_OK,39);
    memset(&larger,0xa5,sizeof(larger));larger.base.struct_size=sizeof(larger);larger.base.api_version=PST_API_VERSION;larger.base.level=PST_LOG_LEVEL_TRACE;larger.base.callback=sink_callback;larger.base.user_context=&sink;CHECK(pst_log_state_initialize(&state,&larger.base)==PST_RESULT_OK,44);for(i=0;i<sizeof(larger.tail);++i)CHECK(larger.tail[i]==0xa5,45);
    pst_log_emit(&state,PST_LOG_LEVEL_TRACE,PST_LOG_EVENT_OPERATION_PROGRESS,PST_LOG_CATEGORY_READINESS,PST_RESULT_OK,PST_DIAGNOSTIC_OPERATION_WAIT,"logging-mock");
    CHECK(sink.count==1UL&&sink.context==&sink,40);CHECK(sink.last.struct_size==60UL&&sink.last.api_version==PST_API_VERSION,41);
    CHECK(sink.last.level==PST_LOG_LEVEL_TRACE&&sink.last.operation==PST_DIAGNOSTIC_OPERATION_WAIT,42);
    CHECK(!strcmp(sink.last.backend_id,"logging-mock"),43);
    return 0;
}
static int event_contains(const PST_LOG_EVENT *event,const char *text)
{
    const unsigned char *bytes; pst_size i,length;
    bytes=(const unsigned char*)event;length=strlen(text);
    for(i=0;i+length<=sizeof(*event);++i)if(memcmp(bytes+i,text,length)==0)return 1;
    return 0;
}
static int check_redaction_abuse(void)
{
    struct fixture { char backend_id[32];char secret[96]; } fixture;
    pst_log_state state;test_sink sink;pst_size i;
    memset(&fixture,0,sizeof(fixture));strcpy(fixture.backend_id,"safe-backend");
    strcpy(fixture.secret,"%s%s%s password token C:\\private\\key.der peer.example payload");
    memset(&sink,0,sizeof(sink));state.level=PST_LOG_LEVEL_TRACE;state.callback=sink_callback;state.user_context=&sink;
    pst_log_emit(&state,PST_LOG_LEVEL_TRACE,PST_LOG_EVENT_OPERATION_PROGRESS,PST_LOG_CATEGORY_IO,PST_RESULT_OK,PST_DIAGNOSTIC_OPERATION_READ,fixture.backend_id);
    CHECK(sink.count==1UL&&!strcmp(sink.last.backend_id,"safe-backend"),46);
    CHECK(!event_contains(&sink.last,"%s%s%s")&&!event_contains(&sink.last,"password")&&!event_contains(&sink.last,"token"),47);
    CHECK(!event_contains(&sink.last,"private")&&!event_contains(&sink.last,"example")&&!event_contains(&sink.last,"payload"),48);
    for(i=strlen(sink.last.backend_id)+1UL;i<sizeof(sink.last.backend_id);++i)CHECK(sink.last.backend_id[i]=='\0',49);
    return 0;
}
static int check_runtime(void)
{
    PST_RUNTIME_OPTIONS o;PST_LOG_CONFIG log_a,log_b,log_off,log_null;PST_DIAGNOSTIC_INFO d_off,d_error;
    test_sink a,b,off; pst_runtime *ra,*rb,*rn; pst_config *config; pst_connection *ca,*cb,*cc; PST_RESULT result;
    memset(&a,0,sizeof(a));memset(&b,0,sizeof(b));memset(&off,0,sizeof(off));
    CHECK(pst_log_config_init(&log_a)==PST_RESULT_OK,50);log_a.level=PST_LOG_LEVEL_INFO;log_a.callback=sink_callback;log_a.user_context=&a;
    CHECK(pst_log_config_init(&log_b)==PST_RESULT_OK,51);log_b.level=PST_LOG_LEVEL_TRACE;log_b.callback=sink_callback;log_b.user_context=&b;
    CHECK(pst_log_config_init(&log_off)==PST_RESULT_OK,52);log_off.level=PST_LOG_LEVEL_OFF;log_off.callback=sink_callback;log_off.user_context=&off;
    CHECK(pst_log_config_init(&log_null)==PST_RESULT_OK,53);log_null.level=PST_LOG_LEVEL_TRACE;log_null.callback=NULL;log_null.user_context=&off;
    options(&o,"logging-mock");ra=rb=rn=NULL;
    CHECK(pst_runtime_create_with_logging(&o,&log_a,&ra,NULL)==PST_RESULT_OK&&ra!=NULL,54);
    CHECK(a.count==1UL&&a.last.event_id==PST_LOG_EVENT_RUNTIME_READY&&a.last.level==PST_LOG_LEVEL_INFO,55);
    CHECK(pst_runtime_create_with_logging(&o,&log_b,&rb,NULL)==PST_RESULT_OK&&rb!=NULL,56);
    CHECK(b.count==1UL&&b.last.event_id==PST_LOG_EVENT_RUNTIME_READY&&a.count==1UL,57);
    CHECK(pst_runtime_create_with_logging(&o,&log_null,&rn,NULL)==PST_RESULT_OK&&rn!=NULL&&!off.count,58);
    CHECK(pst_config_create(&config)==PST_RESULT_OK&&pst_config_freeze(config)==PST_RESULT_OK,59);
    ca=cb=cc=NULL;CHECK(pst_connection_create(ra,config,&ca)==PST_RESULT_OK,60);
    CHECK(a.count==1UL,61);
    CHECK(pst_connection_create(rb,config,&cb)==PST_RESULT_OK,62);
    CHECK(pst_connection_create(rb,config,&cc)==PST_RESULT_OK,63);
    CHECK(b.count==3UL&&b.last.event_id==PST_LOG_EVENT_STATE_TRANSITION&&b.last.level==PST_LOG_LEVEL_DEBUG,64);
    pst_connection_release(ca);pst_connection_release(cb);pst_connection_release(cc);pst_config_release(config);
    pst_runtime_release(ra);pst_runtime_release(rb);pst_runtime_release(rn);
    options(&o,"missing-logging-backend");CHECK(pst_diagnostic_info_init(&d_off)==PST_RESULT_OK,65);
    result=pst_runtime_create_with_logging(&o,&log_off,&rn,&d_off);CHECK(result==PST_RESULT_UNSUPPORTED&&rn==NULL&&!off.count,66);
    CHECK(d_off.valid&&d_off.normalized_result==result&&d_off.operation==PST_DIAGNOSTIC_OPERATION_RUNTIME,67);
    CHECK(pst_diagnostic_info_init(&d_error)==PST_RESULT_OK,68);log_a.level=PST_LOG_LEVEL_ERROR;
    result=pst_runtime_create_with_logging(&o,&log_a,&rn,&d_error);CHECK(result==PST_RESULT_UNSUPPORTED&&rn==NULL,69);
    CHECK(a.count==2UL&&a.last.event_id==PST_LOG_EVENT_RUNTIME_FAILURE&&a.last.normalized_result==result,70);
    CHECK(d_error.valid&&d_error.normalized_result==d_off.normalized_result&&d_error.operation==d_off.operation,71);
    log_a.level=99UL;CHECK(pst_runtime_create_with_logging(&o,&log_a,&rn,NULL)==PST_RESULT_INVALID_ARGUMENT&&rn==NULL,72);
    return 0;
}
int main(void)
{
    int result;pst_backend_registry_reset();CHECK(pst_backend_register(&descriptor)==PST_RESULT_OK,80);
    result=check_abi();if(result)return result;result=check_matrix();if(result)return result;
    result=check_config();if(result)return result;result=check_redaction_abuse();if(result)return result;
    result=check_runtime();if(result)return result;
    pst_backend_registry_reset();printf("test_logging: PASS\n");return 0;
}