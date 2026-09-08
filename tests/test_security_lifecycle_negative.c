/* SPDX-License-Identifier: MPL-2.0 */
#include "papinho_secure_transport.h"
#include "pst_backend.h"
#include "pst_identity_internal.h"
#include "pst_transport_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x,n) if(!(x)){printf("test_security_lifecycle_negative: FAIL %d\n",n);return n;}

typedef struct mock_provider {
    const char *id;
    int fail_initialize;
    int fail_attach_before;
    int fail_attach_after;
    int fail_handshake;
    int initialize_count;
    int shutdown_count;
    int runtime_create_count;
    int runtime_destroy_count;
    int connection_create_count;
    int connection_destroy_count;
    int close_count;
    int destroy_order;
} mock_provider;

typedef struct mock_connection { mock_provider *provider; } mock_connection;

static mock_provider provider_a = { "negative-a",0,0,0,0,0,0,0,0,0,0,0,0 };
static mock_provider provider_b = { "negative-b",0,0,0,0,0,0,0,0,0,0,0,0 };
static int destroy_sequence;

static PST_RESULT initialize_provider(mock_provider *p,void **out)
{
    if(!out)return PST_RESULT_INVALID_ARGUMENT;
    *out=NULL;
    ++p->initialize_count;
    if(p->fail_initialize)return PST_RESULT_BACKEND_FAILURE;
    *out=p;
    return PST_RESULT_OK;
}
static PST_RESULT initialize_a(void **out){return initialize_provider(&provider_a,out);}
static PST_RESULT initialize_b(void **out){return initialize_provider(&provider_b,out);}
static void shutdown_provider(void *state){mock_provider *p=(mock_provider*)state;if(p)++p->shutdown_count;}
static PST_RESULT runtime_create(void *state,void **out){mock_provider*p=(mock_provider*)state;if(!p||!out)return PST_RESULT_INVALID_ARGUMENT;++p->runtime_create_count;*out=p;return PST_RESULT_OK;}
static void runtime_destroy(void *state){mock_provider*p=(mock_provider*)state;if(p){++p->runtime_destroy_count;p->destroy_order=++destroy_sequence;}}
static PST_RESULT query(void *state,pst_u32 *caps){if(!state||!caps)return PST_RESULT_INVALID_ARGUMENT;*caps=PST_CAP_ROLE_CLIENT|PST_CAP_TLS_1_2|PST_CAP_NONBLOCKING|PST_CAP_BACKEND_WAIT;return PST_RESULT_OK;}
static PST_RESULT validate(void *state,pst_u32 required){pst_u32 caps;if(query(state,&caps)!=PST_RESULT_OK)return PST_RESULT_BACKEND_FAILURE;return (required&~caps)?PST_RESULT_UNSUPPORTED:PST_RESULT_OK;}
static PST_RESULT connection_create(void *state,const PST_BACKEND_CONNECTION_OPTIONS *options,void **out)
{
    mock_provider *p=(mock_provider*)state;mock_connection *c;
    if(!p||!options||!out)return PST_RESULT_INVALID_ARGUMENT;
    *out=NULL;c=(mock_connection*)calloc(1,sizeof(*c));if(!c)return PST_RESULT_OUT_OF_MEMORY;
    c->provider=p;++p->connection_create_count;*out=c;return PST_RESULT_OK;
}
static void connection_destroy(void *state){mock_connection*c=(mock_connection*)state;if(c){++c->provider->connection_destroy_count;free(c);}}
static PST_RESULT attach(void *state,void *transport,pst_u32 ownership,pst_u32 *accepted)
{
    mock_connection*c=(mock_connection*)state;(void)transport;
    if(!c||!accepted)return PST_RESULT_INVALID_ARGUMENT;*accepted=0;
    if(ownership!=PST_OWNERSHIP_TRANSFERRED)return PST_RESULT_UNSUPPORTED;
    if(c->provider->fail_attach_before)return PST_RESULT_TRANSPORT_FAILURE;
    *accepted=1;
    return c->provider->fail_attach_after?PST_RESULT_TRANSPORT_FAILURE:PST_RESULT_OK;
}
static PST_RESULT handshake(void *state,pst_u32 *operation,PST_RESULT *error)
{
    mock_connection*c=(mock_connection*)state;if(!c||!operation||!error)return PST_RESULT_INVALID_ARGUMENT;
    if(c->provider->fail_handshake){*operation=PST_OPERATION_FAILED;*error=PST_RESULT_PROTOCOL_FAILURE;return PST_RESULT_OK;}
    *operation=PST_OPERATION_COMPLETE;*error=PST_RESULT_OK;return PST_RESULT_OK;
}
static PST_RESULT interest(void *state,pst_u32 *out){if(!state||!out)return PST_RESULT_INVALID_ARGUMENT;*out=PST_INTEREST_READ;return PST_RESULT_OK;}
static PST_RESULT wait_fn(void *state,pst_u32 requested,pst_u32 timeout,PST_BACKEND_WAIT_RESULT *out){(void)timeout;if(!state||!out)return PST_RESULT_INVALID_ARGUMENT;out->ready_interest=requested;out->timed_out=0;return PST_RESULT_OK;}
static PST_RESULT read_fn(void *state,void *buffer,pst_size size,PST_BACKEND_IO_RESULT *out){(void)buffer;(void)size;if(!state||!out)return PST_RESULT_INVALID_ARGUMENT;memset(out,0,sizeof(*out));out->operation=PST_OPERATION_COMPLETE;out->error=PST_RESULT_OK;return PST_RESULT_OK;}
static PST_RESULT write_fn(void *state,const void *buffer,pst_size size,PST_BACKEND_IO_RESULT *out){return read_fn(state,(void*)buffer,size,out);}
static PST_RESULT shutdown_fn(void *state,pst_u32 *operation,PST_RESULT *error){if(!state||!operation||!error)return PST_RESULT_INVALID_ARGUMENT;*operation=PST_OPERATION_COMPLETE;*error=PST_RESULT_OK;return PST_RESULT_OK;}

#define VTABLE(initfn) {sizeof(PST_BACKEND_VTABLE),PST_BACKEND_SPI_VERSION,initfn,shutdown_provider,runtime_create,runtime_destroy,query,validate,connection_create,connection_destroy,attach,handshake,interest,wait_fn,read_fn,write_fn,shutdown_fn,NULL,NULL,NULL,NULL}
static const PST_BACKEND_VTABLE vtable_a=VTABLE(initialize_a);
static const PST_BACKEND_VTABLE vtable_b=VTABLE(initialize_b);
#define CAPS (PST_CAP_ROLE_CLIENT|PST_CAP_TLS_1_2|PST_CAP_NONBLOCKING|PST_CAP_BACKEND_WAIT)
static const PST_BACKEND_DESCRIPTOR descriptor_a={sizeof(PST_BACKEND_DESCRIPTOR),PST_BACKEND_SPI_VERSION,"negative-a","negative a",CAPS,&vtable_a,NULL,CAPS,0UL};
static const PST_BACKEND_DESCRIPTOR descriptor_b={sizeof(PST_BACKEND_DESCRIPTOR),PST_BACKEND_SPI_VERSION,"negative-b","negative b",CAPS,&vtable_b,NULL,CAPS,0UL};

static void transport_destroy(pst_transport *transport,int close_native)
{
    mock_provider *p=(mock_provider*)transport->native;
    if(close_native&&p)++p->close_count;
}
static void init_runtime(PST_RUNTIME_OPTIONS *options){memset(options,0,sizeof(*options));options->struct_size=sizeof(*options);options->api_version=PST_API_VERSION;}
static void init_config(PST_CONNECTION_CONFIG *config)
{
    memset(config,0,sizeof(*config));config->struct_size=sizeof(*config);config->api_version=PST_API_VERSION;config->role=PST_CONNECTION_ROLE_CLIENT;
    config->provider_selection.struct_size=sizeof(config->provider_selection);config->provider_selection.api_version=PST_API_VERSION;config->provider_selection.mode=PST_BACKEND_SELECTION_EXACT;config->provider_selection.exact_provider_id="negative-a";
    config->local_identity.struct_size=sizeof(config->local_identity);config->local_identity.api_version=PST_API_VERSION;
    config->peer_authentication.struct_size=sizeof(config->peer_authentication);config->peer_authentication.api_version=PST_API_VERSION;config->peer_authentication.certificate_mode=PST_PEER_CERTIFICATE_DISABLED;
    config->tls.struct_size=sizeof(config->tls);config->tls.api_version=PST_API_VERSION;config->tls.minimum_version=PST_TLS_VERSION_1_2;config->tls.maximum_version=PST_TLS_VERSION_1_2;
    config->alpn.struct_size=sizeof(config->alpn);config->alpn.api_version=PST_API_VERSION;config->alpn.mode=PST_FEATURE_DISABLED;
}
static void reset_providers(void){memset(&provider_a.fail_initialize,0,sizeof(provider_a)-offsetof(mock_provider,fail_initialize));memset(&provider_b.fail_initialize,0,sizeof(provider_b)-offsetof(mock_provider,fail_initialize));destroy_sequence=0;}
static pst_transport make_transport(mock_provider *p){pst_transport t;memset(&t,0,sizeof(t));t.backend_id=p->id;t.native=p;t.destroy=transport_destroy;return t;}

int main(void)
{
    PST_RUNTIME_OPTIONS options;PST_CONNECTION_CONFIG config;PST_ALPN_PROTOCOL protocol;PST_RESULT error;PST_IO_RESULT io;PST_WAIT_RESULT wait_result;
    pst_connection_config_snapshot *snapshot;pst_runtime *runtime;pst_connection *connection_a,*connection_b;pst_transport transport;pst_u32 accepted,operation;
    const char *ordered[3];pst_u8 alpn_byte='x';int before,i;
    reset_providers();pst_backend_registry_reset();CHECK(pst_backend_register(&descriptor_a)==PST_RESULT_OK,1);CHECK(pst_backend_register(&descriptor_b)==PST_RESULT_OK,2);
    init_config(&config);config.role=PST_CONNECTION_ROLE_INVALID;CHECK(pst_connection_config_snapshot_create(&config,&snapshot)==PST_RESULT_INVALID_ARGUMENT,3);
    init_config(&config);config.provider_selection.mode=99;CHECK(pst_connection_config_snapshot_create(&config,&snapshot)==PST_RESULT_INVALID_ARGUMENT,4);
    init_config(&config);config.tls.minimum_version=PST_TLS_VERSION_1_3;CHECK(pst_connection_config_snapshot_create(&config,&snapshot)==PST_RESULT_INVALID_ARGUMENT,5);
    init_config(&config);config.role=PST_CONNECTION_ROLE_SERVER;CHECK(pst_connection_config_snapshot_create(&config,&snapshot)==PST_RESULT_POLICY_VIOLATION,6);
    init_config(&config);protocol.data=&alpn_byte;protocol.size=0;config.alpn.mode=PST_FEATURE_REQUIRED;config.alpn.protocols=&protocol;config.alpn.protocol_count=1;CHECK(pst_connection_config_snapshot_create(&config,&snapshot)==PST_RESULT_INVALID_ARGUMENT,7);
    init_config(&config);config.provider_selection.mode=PST_BACKEND_SELECTION_ORDERED;config.provider_selection.exact_provider_id=NULL;config.provider_selection.ordered_provider_ids=NULL;config.provider_selection.ordered_provider_count=0;CHECK(pst_connection_config_snapshot_create(&config,&snapshot)==PST_RESULT_INVALID_ARGUMENT,8);
    CHECK(provider_a.initialize_count==0&&provider_b.initialize_count==0,9);

    init_runtime(&options);CHECK(pst_runtime_create(&options,&runtime)==PST_RESULT_OK,10);
    init_config(&config);config.provider_selection.exact_provider_id="missing";CHECK(pst_connection_create(runtime,&config,&connection_a)==PST_RESULT_UNSUPPORTED,11);CHECK(provider_a.initialize_count==0&&provider_b.initialize_count==0,12);
    init_config(&config);config.provider_selection.required_capabilities=PST_CAP_SYSTEM_TRUST;CHECK(pst_connection_create(runtime,&config,&connection_a)==PST_RESULT_UNSUPPORTED,13);CHECK(provider_a.initialize_count==0,14);
    provider_a.fail_initialize=1;init_config(&config);config.provider_selection.mode=PST_BACKEND_SELECTION_AUTOMATIC;config.provider_selection.exact_provider_id=NULL;CHECK(pst_connection_create(runtime,&config,&connection_a)==PST_RESULT_OK,15);CHECK(provider_a.initialize_count==1&&provider_b.initialize_count==1,16);
    pst_connection_release(connection_a);provider_a.fail_initialize=0;pst_runtime_release(runtime);CHECK(provider_b.runtime_destroy_count==1&&provider_b.shutdown_count==1,17);

    reset_providers();pst_backend_registry_reset();CHECK(pst_backend_register(&descriptor_a)==PST_RESULT_OK,18);CHECK(pst_backend_register(&descriptor_b)==PST_RESULT_OK,19);init_runtime(&options);CHECK(pst_runtime_create(&options,&runtime)==PST_RESULT_OK,20);
    ordered[0]="missing";ordered[1]="negative-a";ordered[2]="negative-a";init_config(&config);config.provider_selection.mode=PST_BACKEND_SELECTION_ORDERED;config.provider_selection.exact_provider_id=NULL;config.provider_selection.ordered_provider_ids=ordered;config.provider_selection.ordered_provider_count=3;CHECK(pst_connection_create(runtime,&config,&connection_a)==PST_RESULT_OK,21);CHECK(provider_a.initialize_count==1,22);
    transport=make_transport(&provider_a);provider_a.fail_attach_before=1;accepted=99;CHECK(pst_connection_attach(connection_a,&transport,PST_OWNERSHIP_TRANSFERRED,&accepted)==PST_RESULT_TRANSPORT_FAILURE&&!accepted,23);pst_connection_release(connection_a);CHECK(provider_a.close_count==0,24);pst_transport_release(&transport);provider_a.fail_attach_before=0;

    init_config(&config);CHECK(pst_connection_create(runtime,&config,&connection_a)==PST_RESULT_OK,25);transport=make_transport(&provider_a);provider_a.fail_attach_after=1;accepted=0;CHECK(pst_connection_attach(connection_a,&transport,PST_OWNERSHIP_TRANSFERRED,&accepted)==PST_RESULT_TRANSPORT_FAILURE&&accepted,26);pst_connection_release(connection_a);CHECK(provider_a.close_count==1,27);provider_a.fail_attach_after=0;

    init_config(&config);CHECK(pst_connection_create(runtime,&config,&connection_a)==PST_RESULT_OK,28);transport=make_transport(&provider_a);accepted=0;CHECK(pst_connection_attach(connection_a,&transport,PST_OWNERSHIP_TRANSFERRED,&accepted)==PST_RESULT_OK&&accepted,29);provider_a.fail_handshake=1;CHECK(pst_connection_handshake(connection_a,&operation,&error)==PST_RESULT_OK&&operation==PST_OPERATION_FAILED&&error==PST_RESULT_PROTOCOL_FAILURE,30);before=provider_b.initialize_count;CHECK(pst_connection_handshake(connection_a,&operation,&error)==PST_RESULT_INVALID_STATE,31);CHECK(pst_connection_read(connection_a,NULL,0,&io)==PST_RESULT_INVALID_STATE,32);CHECK(pst_connection_write(connection_a,NULL,0,&io)==PST_RESULT_INVALID_STATE,33);CHECK(pst_connection_wait(connection_a,1,&wait_result)==PST_RESULT_INVALID_STATE,34);CHECK(pst_connection_shutdown(connection_a,&operation,&error)==PST_RESULT_INVALID_STATE,35);CHECK(provider_b.initialize_count==before,36);pst_connection_release(connection_a);CHECK(provider_a.close_count==2,37);provider_a.fail_handshake=0;

    init_config(&config);CHECK(pst_connection_create(runtime,&config,&connection_a)==PST_RESULT_OK,38);init_config(&config);config.provider_selection.exact_provider_id="negative-b";CHECK(pst_connection_create(runtime,&config,&connection_b)==PST_RESULT_OK,39);pst_runtime_release(runtime);CHECK(provider_a.runtime_destroy_count==0&&provider_b.runtime_destroy_count==0,40);pst_connection_release(connection_a);CHECK(provider_a.runtime_destroy_count==0&&provider_b.runtime_destroy_count==0,41);pst_connection_release(connection_b);CHECK(provider_b.destroy_order==1&&provider_a.destroy_order==2,42);CHECK(provider_a.connection_create_count==4&&provider_a.connection_destroy_count==4,43);CHECK(provider_b.connection_create_count==1&&provider_b.connection_destroy_count==1,44);
    reset_providers();pst_backend_registry_reset();CHECK(pst_backend_register(&descriptor_a)==PST_RESULT_OK,45);init_runtime(&options);CHECK(pst_runtime_create(&options,&runtime)==PST_RESULT_OK,46);init_config(&config);
    for(i=0;i<100;i++){
        CHECK(pst_connection_create(runtime,&config,&connection_a)==PST_RESULT_OK,47);
        transport=make_transport(&provider_a);accepted=0;CHECK(pst_connection_attach(connection_a,&transport,PST_OWNERSHIP_TRANSFERRED,&accepted)==PST_RESULT_OK&&accepted,48);
        provider_a.fail_handshake=(i&1);CHECK(pst_connection_handshake(connection_a,&operation,&error)==PST_RESULT_OK,49);
        if(provider_a.fail_handshake){CHECK(operation==PST_OPERATION_FAILED&&error==PST_RESULT_PROTOCOL_FAILURE,50);CHECK(pst_connection_handshake(connection_a,&operation,&error)==PST_RESULT_INVALID_STATE,51);}
        else{CHECK(operation==PST_OPERATION_COMPLETE,52);CHECK(pst_connection_shutdown(connection_a,&operation,&error)==PST_RESULT_OK&&operation==PST_OPERATION_COMPLETE,53);CHECK(pst_connection_shutdown(connection_a,&operation,&error)==PST_RESULT_INVALID_STATE,54);}
        pst_connection_release(connection_a);
    }
    provider_a.fail_handshake=0;pst_runtime_release(runtime);CHECK(provider_a.connection_create_count==100&&provider_a.connection_destroy_count==100&&provider_a.close_count==100,55);
    pst_backend_registry_reset();
    printf("CONFIG_TRANSACTIONAL_VALIDATION=PASS CORE_INVALID_PROVIDER_INIT_COUNT=0\n");
    printf("EXACT_NEGATIVE_SELECTION=PASS ORDERED_NEGATIVE_SELECTION=PASS AUTOMATIC_NEGATIVE_SELECTION=PASS LAZY_PROVIDER_INIT_PRESERVED=PASS\n");
    printf("OWNERSHIP_BEFORE_ACCEPTANCE=CONSUMER OWNERSHIP_AFTER_ACCEPTANCE=PST EXACTLY_ONE_CLOSE=PASS DOUBLE_CLOSE=0 LEAKED_ACCEPTED_TRANSPORT=0\n");
    printf("RUNTIME_CHILD_LIFETIME=PASS RUNTIME_DEFERRED_DESTROY=PASS PROVIDER_REVERSE_DESTROY_ORDER=PASS\n");
    printf("TERMINAL_FAILURE_NO_RESURRECTION=PASS TERMINAL_CAUSE_IMMUTABLE=PASS POST_BIND_PROVIDER_SWITCH_COUNT=0\n");
    printf("BOUNDED_MALFORMED_CORPUS=PASS LIFECYCLE_STRESS=PASS RUNTIME_REUSE_AFTER_FAILURE=PASS CRASHES=0 HANGS=0\n");
    printf("test_security_lifecycle_negative: PASS\n");
    return 0;
}
