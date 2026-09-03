#include "pst_backend.h"
#include "pst_internal.h"
#include "pst_identity_internal.h"
#include "pst_transport_internal.h"
#include <stdlib.h>
#include <string.h>
struct pst_runtime { const PST_BACKEND_DESCRIPTOR *backend;void *backend_state,*runtime_state;pst_u32 capabilities,connections; pst_internal_diagnostic diagnostic; pst_log_state log; };
struct pst_connection { pst_runtime *runtime;pst_config *config;void *backend_state;pst_transport *transport;pst_u32 state,pending_io,pending_interest,last_wait_ready,suppressed_interest;int wait_observed; pst_internal_diagnostic diagnostic; };
static void diagnostic_pull(const PST_BACKEND_DESCRIPTOR *d,const void *state,pst_internal_diagnostic *out){if(d->vtable->struct_size>=PST_BACKEND_VTABLE_FIELD_SIZE(diagnostic_copy)&&d->vtable->diagnostic_copy)d->vtable->diagnostic_copy(state,out);}
static void diagnostic_core(pst_connection *c,PST_RESULT result,pst_u32 phase){pst_diagnostic_capture(&c->diagnostic,result,phase,c->runtime->backend->id,PST_DIAGNOSTIC_DOMAIN_NONE,0,0,0);}
static void log_connection_failure(pst_connection *c,PST_RESULT result,pst_u32 operation,pst_u32 category){pst_u32 event_id;if(!c)return;event_id=(result==PST_RESULT_AUTH_FAILURE||result==PST_RESULT_HOSTNAME_MISMATCH)?PST_LOG_EVENT_AUTHENTICATION_FAILURE:PST_LOG_EVENT_CONNECTION_FAILURE;if(event_id==PST_LOG_EVENT_AUTHENTICATION_FAILURE)category=PST_LOG_CATEGORY_AUTHENTICATION;pst_log_emit(&c->runtime->log,PST_LOG_LEVEL_ERROR,event_id,category,result,operation,c->runtime->backend->id);}
void pst_internal_operation_context_initialize(pst_internal_operation_context *context){if(context)pst_diagnostic_initialize(&context->diagnostic);}
void pst_internal_operation_context_reset(pst_internal_operation_context *context){if(context)pst_diagnostic_clear(&context->diagnostic);}
void pst_internal_operation_context_diagnostic_copy(const pst_internal_operation_context *context,pst_internal_diagnostic *out){if(!out)return;if(!context){pst_diagnostic_initialize(out);return;}pst_diagnostic_copy(out,&context->diagnostic);}
static void operation_capture(pst_internal_operation_context *context,PST_RESULT result,pst_u32 phase,const char *backend_id){if(context)pst_diagnostic_capture(&context->diagnostic,result,phase,backend_id,PST_DIAGNOSTIC_DOMAIN_NONE,0,0,0);}
static void operation_backend(pst_internal_operation_context *context,const PST_BACKEND_DESCRIPTOR *descriptor,const void *state,PST_RESULT result,pst_u32 phase){pst_internal_diagnostic source;if(!context)return;pst_diagnostic_initialize(&source);if(state)diagnostic_pull(descriptor,state,&source);if(source.valid)pst_diagnostic_capture(&context->diagnostic,source.result,source.phase,source.backend_id,source.native_domain,source.native_code,source.secondary_native_code,source.flags);else operation_capture(context,result,phase,descriptor?descriptor->id:NULL);}
#define C_CREATED 1UL
#define C_ATTACHED 2UL
#define C_HANDSHAKING 3UL
#define C_ESTABLISHED 4UL
#define C_SHUTTING 5UL
#define C_CLOSED 6UL
#define C_FAILED 7UL
#define IO_NONE 0UL
#define IO_READ 1UL
#define IO_WRITE 2UL
static void progress_reset(pst_connection *c){c->pending_io=IO_NONE;c->pending_interest=0UL;c->last_wait_ready=0UL;c->suppressed_interest=0UL;c->wait_observed=0;}
static pst_u32 operation_interest(pst_u32 op){if(op==PST_OPERATION_NEED_READ)return PST_BACKEND_INTEREST_READ;if(op==PST_OPERATION_NEED_WRITE)return PST_BACKEND_INTEREST_WRITE;if(op==PST_OPERATION_NEED_READ_WRITE)return PST_BACKEND_INTEREST_READ|PST_BACKEND_INTEREST_WRITE;return PST_BACKEND_INTEREST_NONE;}
static void progress_after_io(pst_connection *c,pst_u32 kind,const PST_BACKEND_IO_RESULT *b,PST_RESULT result){pst_u32 interest,primary;if(result!=PST_RESULT_OK||b->bytes_transferred!=0||b->operation==PST_OPERATION_COMPLETE||b->operation==PST_OPERATION_CLOSED||b->operation==PST_OPERATION_FAILED){progress_reset(c);return;}interest=operation_interest(b->operation);primary=kind==IO_READ?PST_BACKEND_INTEREST_READ:PST_BACKEND_INTEREST_WRITE;c->suppressed_interest=0UL;if(c->pending_io==kind&&c->pending_interest==interest&&c->wait_observed&&(interest&primary)!=0UL&&(c->last_wait_ready&primary)==0UL)c->suppressed_interest=c->last_wait_ready&interest&~primary;c->pending_io=kind;c->pending_interest=interest;c->last_wait_ready=0UL;c->wait_observed=0;}
static int version_ok(pst_u32 v){return ((v>>16)&0xffffUL)==PST_API_VERSION_MAJOR;}
static const PST_BACKEND_DESCRIPTOR *select_backend(const PST_RUNTIME_OPTIONS *o,pst_size i){if(o->selection==PST_BACKEND_SELECTION_EXACT)return i?NULL:pst_backend_find(o->exact_backend_id);if(o->selection==PST_BACKEND_SELECTION_ORDERED)return i<o->preferred_backend_count?pst_backend_find(o->preferred_backend_ids[i]):NULL;return i<pst_backend_count()?pst_backend_find_by_index(i):NULL;}
PST_RESULT pst_runtime_create_internal(const PST_RUNTIME_OPTIONS *o,pst_runtime **out,pst_internal_operation_context *context)
{
    pst_runtime *r; const PST_BACKEND_DESCRIPTOR *d; PST_RESULT x; pst_size i,limit;
    pst_internal_operation_context_initialize(context);
    if(!out){operation_capture(context,PST_RESULT_INVALID_ARGUMENT,PST_DIAGNOSTIC_PHASE_RUNTIME_CREATE,NULL);return PST_RESULT_INVALID_ARGUMENT;}
    *out=NULL;
    if(!o||o->struct_size<PST_RUNTIME_OPTIONS_MIN_SIZE){operation_capture(context,PST_RESULT_INVALID_ARGUMENT,PST_DIAGNOSTIC_PHASE_RUNTIME_CREATE,NULL);return PST_RESULT_INVALID_ARGUMENT;}
    if(!version_ok(o->api_version)){operation_capture(context,PST_RESULT_INCOMPATIBLE_API,PST_DIAGNOSTIC_PHASE_RUNTIME_CREATE,NULL);return PST_RESULT_INCOMPATIBLE_API;}
    if(o->selection<1||o->selection>3){operation_capture(context,PST_RESULT_INVALID_ARGUMENT,PST_DIAGNOSTIC_PHASE_RUNTIME_CREATE,NULL);return PST_RESULT_INVALID_ARGUMENT;}
    if(o->selection==PST_BACKEND_SELECTION_EXACT&&(!o->exact_backend_id||!o->exact_backend_id[0])){operation_capture(context,PST_RESULT_INVALID_ARGUMENT,PST_DIAGNOSTIC_PHASE_BACKEND_SELECT,NULL);return PST_RESULT_INVALID_ARGUMENT;}
    if(o->selection==PST_BACKEND_SELECTION_ORDERED&&(!o->preferred_backend_ids||!o->preferred_backend_count)){operation_capture(context,PST_RESULT_INVALID_ARGUMENT,PST_DIAGNOSTIC_PHASE_BACKEND_SELECT,NULL);return PST_RESULT_INVALID_ARGUMENT;}
    limit=o->selection==PST_BACKEND_SELECTION_ORDERED?o->preferred_backend_count:(o->selection==PST_BACKEND_SELECTION_EXACT?1:pst_backend_count());
    if(limit==0){operation_capture(context,PST_RESULT_UNSUPPORTED,PST_DIAGNOSTIC_PHASE_BACKEND_SELECT,NULL);return PST_RESULT_UNSUPPORTED;}
    for(i=0;i<limit;i++){
        d=select_backend(o,i);
        if(!d){operation_capture(context,PST_RESULT_UNSUPPORTED,PST_DIAGNOSTIC_PHASE_BACKEND_SELECT,NULL);continue;}
        r=(pst_runtime*)calloc(1,sizeof(*r));
        if(!r){operation_capture(context,PST_RESULT_OUT_OF_MEMORY,PST_DIAGNOSTIC_PHASE_RUNTIME_CREATE,d->id);return PST_RESULT_OUT_OF_MEMORY;}
        r->backend=d; pst_diagnostic_initialize(&r->diagnostic);
        x=d->vtable->initialize(&r->backend_state);
        if(x!=PST_RESULT_OK){operation_backend(context,d,r->backend_state,x,PST_DIAGNOSTIC_PHASE_BACKEND_INITIALIZE);if(r->backend_state)d->vtable->shutdown(r->backend_state);free(r);continue;}
        x=d->vtable->query_capabilities(r->backend_state,&r->capabilities);
        if(x!=PST_RESULT_OK){operation_backend(context,d,r->backend_state,x,PST_DIAGNOSTIC_PHASE_CAPABILITY_VALIDATE);d->vtable->shutdown(r->backend_state);free(r);continue;}
        if((o->required_capabilities&~r->capabilities)!=0){operation_capture(context,PST_RESULT_UNSUPPORTED,PST_DIAGNOSTIC_PHASE_CAPABILITY_VALIDATE,d->id);d->vtable->shutdown(r->backend_state);free(r);continue;}
        x=d->vtable->runtime_create(r->backend_state,&r->runtime_state);
        if(x!=PST_RESULT_OK){operation_backend(context,d,r->backend_state,x,PST_DIAGNOSTIC_PHASE_RUNTIME_CREATE);d->vtable->shutdown(r->backend_state);free(r);continue;}
        diagnostic_pull(d,r->runtime_state,&r->diagnostic);
        if(context)pst_diagnostic_clear(&context->diagnostic);
        *out=r; return PST_RESULT_OK;
    }
    return PST_RESULT_UNSUPPORTED;
}
PST_RESULT PST_CALL pst_runtime_create_ex(const PST_RUNTIME_OPTIONS *o,pst_runtime **out,PST_DIAGNOSTIC_INFO *diagnostic)
{
    pst_internal_operation_context context;PST_RESULT result,export_result;
    if(out)*out=NULL;
    if(diagnostic){result=pst_diagnostic_validate_public(diagnostic);if(result!=PST_RESULT_OK)return result;}
    result=pst_runtime_create_internal(o,out,&context);
    if(diagnostic){export_result=pst_diagnostic_export_public(&context.diagnostic,diagnostic);if(export_result!=PST_RESULT_OK)return export_result;if(diagnostic->valid)diagnostic->normalized_result=result;}
    return result;
}
PST_RESULT PST_CALL pst_runtime_create_with_logging(const PST_RUNTIME_OPTIONS *o,const PST_LOG_CONFIG *logging,pst_runtime **out,PST_DIAGNOSTIC_INFO *diagnostic)
{
    pst_log_state log;pst_internal_operation_context context;PST_DIAGNOSTIC_INFO public_diagnostic;PST_RESULT result,export_result;pst_u32 operation;const char *backend_id;
    if(out)*out=NULL;
    if(diagnostic){result=pst_diagnostic_validate_public(diagnostic);if(result!=PST_RESULT_OK)return result;}
    result=pst_log_state_initialize(&log,logging);if(result!=PST_RESULT_OK)return result;
    result=pst_runtime_create_internal(o,out,&context);
    public_diagnostic.struct_size=sizeof(public_diagnostic);public_diagnostic.api_version=PST_API_VERSION;
    export_result=pst_diagnostic_export_public(&context.diagnostic,&public_diagnostic);
    operation=export_result==PST_RESULT_OK?public_diagnostic.operation:PST_DIAGNOSTIC_OPERATION_RUNTIME;
    backend_id=(export_result==PST_RESULT_OK&&public_diagnostic.backend_id[0])?public_diagnostic.backend_id:NULL;
    if(result==PST_RESULT_OK){(*out)->log=log;pst_log_emit(&log,PST_LOG_LEVEL_INFO,PST_LOG_EVENT_RUNTIME_READY,PST_LOG_CATEGORY_RUNTIME,PST_RESULT_OK,PST_DIAGNOSTIC_OPERATION_RUNTIME,(*out)->backend->id);}
    else pst_log_emit(&log,PST_LOG_LEVEL_ERROR,PST_LOG_EVENT_RUNTIME_FAILURE,PST_LOG_CATEGORY_RUNTIME,result,operation,backend_id);
    if(diagnostic){export_result=pst_diagnostic_export_public(&context.diagnostic,diagnostic);if(export_result!=PST_RESULT_OK)return export_result;if(diagnostic->valid)diagnostic->normalized_result=result;}
    return result;
}
PST_RESULT PST_CALL pst_runtime_create(const PST_RUNTIME_OPTIONS *o,pst_runtime **out){return pst_runtime_create_ex(o,out,NULL);}
PST_RESULT PST_CALL pst_runtime_copy_diagnostic(const pst_runtime *runtime,PST_DIAGNOSTIC_INFO *diagnostic){pst_internal_diagnostic empty;PST_RESULT result;if(!diagnostic)return PST_RESULT_INVALID_ARGUMENT;result=pst_diagnostic_validate_public(diagnostic);if(result!=PST_RESULT_OK)return result;if(!runtime){pst_diagnostic_initialize(&empty);pst_diagnostic_export_public(&empty,diagnostic);return PST_RESULT_INVALID_ARGUMENT;}return pst_diagnostic_export_public(&runtime->diagnostic,diagnostic);}
void PST_CALL pst_runtime_release(pst_runtime *r){if(!r)return;if(r->connections)return;r->backend->vtable->runtime_destroy(r->runtime_state);r->backend->vtable->shutdown(r->backend_state);free(r);}
PST_RESULT PST_CALL pst_runtime_get_info(const pst_runtime *r,PST_RUNTIME_INFO *i){if(!i||i->struct_size<PST_RUNTIME_INFO_MIN_SIZE)return PST_RESULT_INVALID_ARGUMENT;if(!version_ok(i->api_version))return PST_RESULT_INCOMPATIBLE_API;i->backend_id=NULL;i->capabilities=0UL;if(!r)return PST_RESULT_INVALID_ARGUMENT;i->backend_id=r->backend->id;i->capabilities=r->capabilities;return PST_RESULT_OK;}
PST_RESULT pst_connection_create_internal(pst_runtime *r,pst_config *cfg,pst_connection **out,pst_internal_operation_context *context)
{
    pst_connection *c; PST_RESULT x; pst_u32 req;
    pst_internal_operation_context_initialize(context);
    if(!out){operation_capture(context,PST_RESULT_INVALID_ARGUMENT,PST_DIAGNOSTIC_PHASE_CONNECTION_CREATE,NULL);return PST_RESULT_INVALID_ARGUMENT;}
    *out=NULL;
    if(!r||!cfg||!pst_config_is_frozen(cfg)){operation_capture(context,PST_RESULT_INVALID_STATE,PST_DIAGNOSTIC_PHASE_CONNECTION_CREATE,r?r->backend->id:NULL);return PST_RESULT_INVALID_STATE;}
    req=pst_config_required_capabilities(cfg);
    if(req&~r->capabilities){operation_capture(context,PST_RESULT_UNSUPPORTED,PST_DIAGNOSTIC_PHASE_CAPABILITY_VALIDATE,r->backend->id);return PST_RESULT_UNSUPPORTED;}
    x=r->backend->vtable->validate_requirements(r->runtime_state,req);
    if(x!=PST_RESULT_OK){operation_backend(context,r->backend,r->runtime_state,x,PST_DIAGNOSTIC_PHASE_CAPABILITY_VALIDATE);return x;}
    c=(pst_connection*)calloc(1,sizeof(*c));
    if(!c){operation_capture(context,PST_RESULT_OUT_OF_MEMORY,PST_DIAGNOSTIC_PHASE_CONNECTION_CREATE,r->backend->id);return PST_RESULT_OUT_OF_MEMORY;}
    x=r->backend->vtable->connection_create(r->runtime_state,&c->backend_state);
    if(x!=PST_RESULT_OK){operation_backend(context,r->backend,r->runtime_state,x,PST_DIAGNOSTIC_PHASE_CONNECTION_CREATE);free(c);return x;}
    x=PST_RESULT_OK;
    if(r->backend->vtable->struct_size>=(pst_u32)(offsetof(PST_BACKEND_VTABLE,connection_configure_identity)+sizeof(r->backend->vtable->connection_configure_identity))&&r->backend->vtable->connection_configure_identity)
        x=r->backend->vtable->connection_configure_identity(c->backend_state,cfg);
    if(x!=PST_RESULT_OK){operation_backend(context,r->backend,c->backend_state,x,PST_DIAGNOSTIC_PHASE_IDENTITY_SETUP);r->backend->vtable->connection_destroy(c->backend_state);free(c);return x;}
    c->runtime=r;c->config=cfg;c->state=C_CREATED;pst_diagnostic_initialize(&c->diagnostic);diagnostic_pull(r->backend,c->backend_state,&c->diagnostic);pst_config_retain(cfg);++r->connections;
    if(context)pst_diagnostic_clear(&context->diagnostic);
    *out=c;return PST_RESULT_OK;
}
PST_RESULT PST_CALL pst_connection_create_ex(pst_runtime *r,pst_config *cfg,pst_connection **out,PST_DIAGNOSTIC_INFO *diagnostic)
{
    pst_internal_operation_context context;PST_RESULT result,export_result;
    if(out)*out=NULL;
    if(diagnostic){result=pst_diagnostic_validate_public(diagnostic);if(result!=PST_RESULT_OK)return result;}
    result=pst_connection_create_internal(r,cfg,out,&context);
    if(r){if(result==PST_RESULT_OK)pst_log_emit(&r->log,PST_LOG_LEVEL_DEBUG,PST_LOG_EVENT_STATE_TRANSITION,PST_LOG_CATEGORY_CONNECTION,PST_RESULT_OK,PST_DIAGNOSTIC_OPERATION_CONNECTION,r->backend->id);else pst_log_emit(&r->log,PST_LOG_LEVEL_ERROR,PST_LOG_EVENT_CONNECTION_FAILURE,PST_LOG_CATEGORY_CONNECTION,result,PST_DIAGNOSTIC_OPERATION_CONNECTION,r->backend->id);}
    if(diagnostic){export_result=pst_diagnostic_export_public(&context.diagnostic,diagnostic);if(export_result!=PST_RESULT_OK)return export_result;if(diagnostic->valid)diagnostic->normalized_result=result;}
    return result;
}
PST_RESULT PST_CALL pst_connection_create(pst_runtime *r,pst_config *cfg,pst_connection **out){return pst_connection_create_ex(r,cfg,out,NULL);}
PST_RESULT PST_CALL pst_connection_copy_diagnostic(const pst_connection *connection,PST_DIAGNOSTIC_INFO *diagnostic){pst_internal_diagnostic empty;PST_RESULT result;if(!diagnostic)return PST_RESULT_INVALID_ARGUMENT;result=pst_diagnostic_validate_public(diagnostic);if(result!=PST_RESULT_OK)return result;if(!connection){pst_diagnostic_initialize(&empty);pst_diagnostic_export_public(&empty,diagnostic);return PST_RESULT_INVALID_ARGUMENT;}return pst_diagnostic_export_public(&connection->diagnostic,diagnostic);}
PST_RESULT PST_CALL pst_connection_attach(pst_connection *c,pst_transport *t,pst_u32 own,pst_u32 *accepted){PST_RESULT x;if(!accepted)return PST_RESULT_INVALID_ARGUMENT;*accepted=0;if(!c||!t||c->state!=C_CREATED)return PST_RESULT_INVALID_STATE;if(own!=PST_OWNERSHIP_TRANSFERRED)return PST_RESULT_UNSUPPORTED;x=c->runtime->backend->vtable->attach_transport(c->backend_state,t->native,own,accepted);diagnostic_pull(c->runtime->backend,c->backend_state,&c->diagnostic);if(*accepted){c->transport=t;c->state=x==PST_RESULT_OK?C_ATTACHED:C_FAILED;}if(x==PST_RESULT_OK)pst_log_emit(&c->runtime->log,PST_LOG_LEVEL_DEBUG,PST_LOG_EVENT_STATE_TRANSITION,PST_LOG_CATEGORY_CONNECTION,PST_RESULT_OK,PST_DIAGNOSTIC_OPERATION_TRANSPORT,c->runtime->backend->id);else log_connection_failure(c,x,PST_DIAGNOSTIC_OPERATION_TRANSPORT,PST_LOG_CATEGORY_CONNECTION);return x;}
PST_RESULT PST_CALL pst_connection_handshake(pst_connection *c,pst_u32 *op,PST_RESULT *error){PST_RESULT x,reported;if(!c||!op||!error||(c->state!=C_ATTACHED&&c->state!=C_HANDSHAKING)){if(op)*op=PST_OPERATION_FAILED;if(error)*error=PST_RESULT_INVALID_STATE;return PST_RESULT_INVALID_STATE;}progress_reset(c);c->state=C_HANDSHAKING;x=c->runtime->backend->vtable->handshake_step(c->backend_state,op,error);diagnostic_pull(c->runtime->backend,c->backend_state,&c->diagnostic);reported=x!=PST_RESULT_OK?x:*error;if(x!=PST_RESULT_OK||*op==PST_OPERATION_FAILED){c->state=C_FAILED;log_connection_failure(c,reported,PST_DIAGNOSTIC_OPERATION_HANDSHAKE,PST_LOG_CATEGORY_TLS);}else if(*op==PST_OPERATION_COMPLETE){c->state=C_ESTABLISHED;pst_log_emit(&c->runtime->log,PST_LOG_LEVEL_INFO,PST_LOG_EVENT_CONNECTION_SECURE,PST_LOG_CATEGORY_TLS,PST_RESULT_OK,PST_DIAGNOSTIC_OPERATION_HANDSHAKE,c->runtime->backend->id);}else pst_log_emit(&c->runtime->log,PST_LOG_LEVEL_TRACE,PST_LOG_EVENT_OPERATION_PROGRESS,PST_LOG_CATEGORY_TLS,PST_RESULT_OK,PST_DIAGNOSTIC_OPERATION_HANDSHAKE,c->runtime->backend->id);return x;}
PST_RESULT PST_CALL pst_connection_get_interest(pst_connection *c,pst_u32 *i){if(i)*i=PST_INTEREST_NONE;if(!c||!i||(c->state!=C_HANDSHAKING&&c->state!=C_SHUTTING&&c->state!=C_ESTABLISHED))return PST_RESULT_INVALID_STATE;return c->runtime->backend->vtable->get_interest(c->backend_state,i);}
PST_RESULT PST_CALL pst_connection_wait(pst_connection *c,pst_u32 timeout,PST_WAIT_RESULT *out){PST_BACKEND_WAIT_RESULT b;pst_u32 i,requested;PST_RESULT x;if(!out)return PST_RESULT_INVALID_ARGUMENT;out->ready_interest=PST_INTEREST_NONE;out->timed_out=0UL;x=pst_connection_get_interest(c,&i);if(x!=PST_RESULT_OK)return x;requested=i&~c->suppressed_interest;if(requested==0UL)requested=i;memset(&b,0,sizeof(b));x=c->runtime->backend->vtable->wait(c->backend_state,requested,timeout,&b);diagnostic_pull(c->runtime->backend,c->backend_state,&c->diagnostic);out->ready_interest=b.ready_interest;out->timed_out=b.timed_out;if(x==PST_RESULT_OK){if(b.timed_out){c->suppressed_interest=0UL;c->last_wait_ready=0UL;c->wait_observed=0;}else{c->last_wait_ready=b.ready_interest;c->wait_observed=b.ready_interest!=0UL;}pst_log_emit(&c->runtime->log,PST_LOG_LEVEL_TRACE,PST_LOG_EVENT_OPERATION_PROGRESS,PST_LOG_CATEGORY_READINESS,PST_RESULT_OK,PST_DIAGNOSTIC_OPERATION_WAIT,c->runtime->backend->id);}else{c->state=x==PST_RESULT_CLOSED?C_CLOSED:C_FAILED;log_connection_failure(c,x,PST_DIAGNOSTIC_OPERATION_WAIT,PST_LOG_CATEGORY_READINESS);}return x;}
static void copy_io(PST_IO_RESULT *o,const PST_BACKEND_IO_RESULT *b){o->bytes_transferred=b->bytes_transferred;o->operation=b->operation;o->close_kind=b->close_kind;o->error=b->error;}
PST_RESULT PST_CALL pst_connection_read(pst_connection *c,void *buf,pst_size n,PST_IO_RESULT *o){PST_BACKEND_IO_RESULT b;PST_RESULT x,reported;if(o){o->bytes_transferred=0;o->operation=PST_OPERATION_FAILED;o->close_kind=PST_CLOSE_NONE;o->error=PST_RESULT_INVALID_STATE;}if(!c)return PST_RESULT_INVALID_STATE;if(c->state!=C_ESTABLISHED){diagnostic_core(c,PST_RESULT_INVALID_STATE,PST_DIAGNOSTIC_PHASE_READ);return PST_RESULT_INVALID_STATE;}if(!o)return PST_RESULT_INVALID_ARGUMENT;memset(&b,0,sizeof(b));x=c->runtime->backend->vtable->read(c->backend_state,buf,n,&b);diagnostic_pull(c->runtime->backend,c->backend_state,&c->diagnostic);copy_io(o,&b);progress_after_io(c,IO_READ,&b,x);reported=x!=PST_RESULT_OK?x:b.error;if(b.operation==PST_OPERATION_CLOSED)c->state=C_CLOSED;else if(b.operation==PST_OPERATION_FAILED)c->state=C_FAILED;if(x!=PST_RESULT_OK||b.operation==PST_OPERATION_FAILED)log_connection_failure(c,reported,PST_DIAGNOSTIC_OPERATION_READ,PST_LOG_CATEGORY_IO);else pst_log_emit(&c->runtime->log,PST_LOG_LEVEL_TRACE,PST_LOG_EVENT_OPERATION_PROGRESS,PST_LOG_CATEGORY_IO,PST_RESULT_OK,PST_DIAGNOSTIC_OPERATION_READ,c->runtime->backend->id);return x;}
PST_RESULT PST_CALL pst_connection_write(pst_connection *c,const void *buf,pst_size n,PST_IO_RESULT *o){PST_BACKEND_IO_RESULT b;PST_RESULT x,reported;if(o){o->bytes_transferred=0;o->operation=PST_OPERATION_FAILED;o->close_kind=PST_CLOSE_NONE;o->error=PST_RESULT_INVALID_STATE;}if(!c||c->state!=C_ESTABLISHED)return PST_RESULT_INVALID_STATE;if(!o)return PST_RESULT_INVALID_ARGUMENT;memset(&b,0,sizeof(b));x=c->runtime->backend->vtable->write(c->backend_state,buf,n,&b);diagnostic_pull(c->runtime->backend,c->backend_state,&c->diagnostic);copy_io(o,&b);progress_after_io(c,IO_WRITE,&b,x);reported=x!=PST_RESULT_OK?x:b.error;if(b.operation==PST_OPERATION_FAILED)c->state=C_FAILED;if(x!=PST_RESULT_OK||b.operation==PST_OPERATION_FAILED)log_connection_failure(c,reported,PST_DIAGNOSTIC_OPERATION_WRITE,PST_LOG_CATEGORY_IO);else pst_log_emit(&c->runtime->log,PST_LOG_LEVEL_TRACE,PST_LOG_EVENT_OPERATION_PROGRESS,PST_LOG_CATEGORY_IO,PST_RESULT_OK,PST_DIAGNOSTIC_OPERATION_WRITE,c->runtime->backend->id);return x;}
PST_RESULT PST_CALL pst_connection_get_peer_info(pst_connection *c,pst_peer_info **out){if(out)*out=NULL;if(!c||c->state!=C_ESTABLISHED)return PST_RESULT_INVALID_STATE;if(!out)return PST_RESULT_INVALID_ARGUMENT;if(!(c->runtime->capabilities&PST_BACKEND_CAP_PEER_INFO)||!c->runtime->backend->vtable->peer_info_create)return PST_RESULT_UNSUPPORTED;return c->runtime->backend->vtable->peer_info_create(c->backend_state,(void**)out);}
PST_RESULT PST_CALL pst_connection_get_negotiated_alpn(pst_connection *c,pst_u8 *b,pst_size cap,pst_size *n){const PST_BACKEND_VTABLE *v;if(n)*n=0;if(!c||c->state!=C_ESTABLISHED)return PST_RESULT_INVALID_STATE;if(!n)return PST_RESULT_INVALID_ARGUMENT;v=c->runtime->backend->vtable;if(!(c->runtime->capabilities&PST_BACKEND_CAP_ALPN)||v->struct_size<(pst_u32)(offsetof(PST_BACKEND_VTABLE,connection_get_alpn)+sizeof(v->connection_get_alpn))||!v->connection_get_alpn)return PST_RESULT_UNSUPPORTED;return v->connection_get_alpn(c->backend_state,b,cap,n);}
PST_RESULT PST_CALL pst_connection_shutdown(pst_connection *c,pst_u32 *op,PST_RESULT *e){PST_RESULT x,reported;if(!c||!op||!e||(c->state!=C_ESTABLISHED&&c->state!=C_SHUTTING)){if(op)*op=PST_OPERATION_FAILED;if(e)*e=PST_RESULT_INVALID_STATE;return PST_RESULT_INVALID_STATE;}progress_reset(c);c->state=C_SHUTTING;x=c->runtime->backend->vtable->shutdown_step(c->backend_state,op,e);diagnostic_pull(c->runtime->backend,c->backend_state,&c->diagnostic);reported=x!=PST_RESULT_OK?x:*e;if(*op==PST_OPERATION_COMPLETE){c->state=C_CLOSED;pst_log_emit(&c->runtime->log,PST_LOG_LEVEL_INFO,PST_LOG_EVENT_CONNECTION_CLOSED,PST_LOG_CATEGORY_SHUTDOWN,PST_RESULT_OK,PST_DIAGNOSTIC_OPERATION_SHUTDOWN,c->runtime->backend->id);}else if(*op==PST_OPERATION_FAILED){c->state=C_FAILED;log_connection_failure(c,reported,PST_DIAGNOSTIC_OPERATION_SHUTDOWN,PST_LOG_CATEGORY_SHUTDOWN);}else if(x!=PST_RESULT_OK)log_connection_failure(c,x,PST_DIAGNOSTIC_OPERATION_SHUTDOWN,PST_LOG_CATEGORY_SHUTDOWN);else pst_log_emit(&c->runtime->log,PST_LOG_LEVEL_TRACE,PST_LOG_EVENT_OPERATION_PROGRESS,PST_LOG_CATEGORY_SHUTDOWN,PST_RESULT_OK,PST_DIAGNOSTIC_OPERATION_SHUTDOWN,c->runtime->backend->id);return x;}
void PST_CALL pst_connection_release(pst_connection *c){if(!c)return;c->runtime->backend->vtable->connection_destroy(c->backend_state);if(c->transport)c->transport->destroy(c->transport,1);pst_config_release(c->config);--c->runtime->connections;free(c);}
void PST_CALL pst_transport_release(pst_transport *t){if(t)t->destroy(t,0);}
void pst_runtime_diagnostic_copy(const pst_runtime *r,pst_internal_diagnostic *out){if(!out)return;if(!r){pst_diagnostic_initialize(out);return;}pst_diagnostic_copy(out,&r->diagnostic);}
void pst_connection_diagnostic_copy(const pst_connection *c,pst_internal_diagnostic *out){if(!out)return;if(!c){pst_diagnostic_initialize(out);return;}pst_diagnostic_copy(out,&c->diagnostic);}
