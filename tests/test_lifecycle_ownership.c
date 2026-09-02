#include "pst_backend.h"
#include "pst_internal.h"
#include "pst_identity_internal.h"
#include "pst_transport_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) if (!(x)) { printf("test_lifecycle_ownership: FAIL line %d\n", __LINE__); return 1; }
#define CAPS (PST_BACKEND_CAP_TLS_1_2 | PST_BACKEND_CAP_NONBLOCKING | PST_BACKEND_CAP_BACKEND_WAIT)
#define ATTACH_OK 0
#define ATTACH_FAIL_BEFORE 1
#define ATTACH_FAIL_AFTER 2
#define STEP_COMPLETE 0
#define STEP_PENDING 1
#define STEP_FAILED 2

typedef struct lifecycle_counts {
    int backend_initialize, backend_shutdown;
    int runtime_create, runtime_destroy, runtime_failure_cleanup;
    int connection_create, connection_destroy, connection_failure_cleanup;
    int transport_attach, ownership_accept;
    int provider_transport_close, caller_transport_close, wrapper_destroy;
    int diagnostic_copy, log_callback, late_callback;
    int handshake, wait, read, write, shutdown_step;
    int sequence, diagnostic_order, destroy_order;
} lifecycle_counts;

typedef struct lifecycle_modes {
    int initialize_fail, runtime_fail, connection_fail, configure_fail;
    int attach_mode, handshake_mode, shutdown_mode, read_closed;
} lifecycle_modes;

typedef struct lifecycle_prefix { pst_internal_diagnostic diagnostic; int alive; } lifecycle_prefix;
typedef struct lifecycle_backend { lifecycle_prefix p; } lifecycle_backend;
typedef struct lifecycle_runtime { lifecycle_prefix p; lifecycle_backend *backend; } lifecycle_runtime;
typedef struct lifecycle_connection { lifecycle_prefix p; lifecycle_runtime *runtime; int owns_transport; } lifecycle_connection;
typedef struct lifecycle_transport { pst_transport base; int token; } lifecycle_transport;
typedef struct lifecycle_sink { int alive; } lifecycle_sink;

static lifecycle_counts g;
static lifecycle_modes m;

static void reset_test(void) { memset(&g, 0, sizeof(g)); memset(&m, 0, sizeof(m)); }
static void capture(lifecycle_prefix *p, PST_RESULT result, pst_u32 phase)
{
    pst_diagnostic_capture(&p->diagnostic, result, phase, "lifecycle-mock",
        PST_DIAGNOSTIC_DOMAIN_NONE, 0, 0, 0);
}
static PST_RESULT mock_initialize(void **out)
{
    lifecycle_backend *s;
    ++g.backend_initialize;
    if (!out) return PST_RESULT_INVALID_ARGUMENT;
    *out = NULL;
    s = (lifecycle_backend *)calloc(1, sizeof(*s));
    if (!s) return PST_RESULT_OUT_OF_MEMORY;
    pst_diagnostic_initialize(&s->p.diagnostic); s->p.alive = 1; *out = s;
    if (m.initialize_fail) { capture(&s->p, PST_RESULT_BACKEND_FAILURE, PST_DIAGNOSTIC_PHASE_BACKEND_INITIALIZE); return PST_RESULT_BACKEND_FAILURE; }
    return PST_RESULT_OK;
}
static void mock_backend_shutdown(void *state)
{
    lifecycle_backend *s = (lifecycle_backend *)state;
    ++g.backend_shutdown; if (s) { s->p.alive = 0; free(s); }
}
static PST_RESULT mock_runtime_create(void *state, void **out)
{
    lifecycle_runtime *r;
    ++g.runtime_create; if (!state || !out) return PST_RESULT_INVALID_ARGUMENT; *out = NULL;
    if (m.runtime_fail) { capture(&((lifecycle_backend *)state)->p, PST_RESULT_BACKEND_FAILURE, PST_DIAGNOSTIC_PHASE_RUNTIME_CREATE); ++g.runtime_failure_cleanup; return PST_RESULT_BACKEND_FAILURE; }
    r = (lifecycle_runtime *)calloc(1, sizeof(*r)); if (!r) return PST_RESULT_OUT_OF_MEMORY;
    pst_diagnostic_initialize(&r->p.diagnostic); r->p.alive = 1; r->backend = (lifecycle_backend *)state; *out = r; return PST_RESULT_OK;
}
static void mock_runtime_destroy(void *state)
{
    lifecycle_runtime *r = (lifecycle_runtime *)state;
    ++g.runtime_destroy; if (r) { r->p.alive = 0; free(r); }
}
static PST_RESULT mock_query(void *state, pst_u32 *caps) { if (!state || !caps) return PST_RESULT_INVALID_ARGUMENT; *caps = CAPS; return PST_RESULT_OK; }
static PST_RESULT mock_validate(void *state, pst_u32 required) { return state && !(required & ~CAPS) ? PST_RESULT_OK : PST_RESULT_UNSUPPORTED; }
static PST_RESULT mock_connection_create(void *state, void **out)
{
    lifecycle_connection *c;
    ++g.connection_create; if (!state || !out) return PST_RESULT_INVALID_ARGUMENT; *out = NULL;
    if (m.connection_fail) { capture(&((lifecycle_runtime *)state)->p, PST_RESULT_BACKEND_FAILURE, PST_DIAGNOSTIC_PHASE_CONNECTION_CREATE); ++g.connection_failure_cleanup; return PST_RESULT_BACKEND_FAILURE; }
    c = (lifecycle_connection *)calloc(1, sizeof(*c)); if (!c) return PST_RESULT_OUT_OF_MEMORY;
    pst_diagnostic_initialize(&c->p.diagnostic); c->p.alive = 1; c->runtime = (lifecycle_runtime *)state; *out = c; return PST_RESULT_OK;
}
static void mock_connection_destroy(void *state)
{
    lifecycle_connection *c = (lifecycle_connection *)state;
    ++g.connection_destroy; g.destroy_order = ++g.sequence;
    if (c) { if (c->owns_transport) { ++g.provider_transport_close; c->owns_transport = 0; } c->p.alive = 0; free(c); }
}
static PST_RESULT mock_configure(void *state, const pst_config *config)
{
    lifecycle_connection *c = (lifecycle_connection *)state;
    if (!c || !config) return PST_RESULT_INVALID_ARGUMENT;
    if (m.configure_fail) { capture(&c->p, PST_RESULT_POLICY_VIOLATION, PST_DIAGNOSTIC_PHASE_IDENTITY_SETUP); return PST_RESULT_POLICY_VIOLATION; }
    return PST_RESULT_OK;
}
static PST_RESULT mock_attach(void *state, void *transport, pst_u32 ownership, pst_u32 *accepted)
{
    lifecycle_connection *c = (lifecycle_connection *)state;
    ++g.transport_attach; if (!accepted) return PST_RESULT_INVALID_ARGUMENT; *accepted = 0;
    if (!c || !transport || ownership != PST_BACKEND_OWNERSHIP_TRANSFERRED) return PST_RESULT_INVALID_ARGUMENT;
    if (m.attach_mode == ATTACH_FAIL_BEFORE) return PST_RESULT_TRANSPORT_FAILURE;
    *accepted = 1; ++g.ownership_accept;
    if (m.attach_mode == ATTACH_FAIL_AFTER) { ++g.provider_transport_close; return PST_RESULT_BACKEND_FAILURE; }
    c->owns_transport = 1; return PST_RESULT_OK;
}
static PST_RESULT mock_handshake(void *state, pst_u32 *operation, PST_RESULT *error)
{
    lifecycle_connection *c = (lifecycle_connection *)state;
    ++g.handshake; if (!c || !operation || !error) return PST_RESULT_INVALID_ARGUMENT;
    if (m.handshake_mode == STEP_PENDING) { *operation = PST_BACKEND_OPERATION_NEED_READ; *error = PST_RESULT_OK; return PST_RESULT_OK; }
    if (m.handshake_mode == STEP_FAILED) { capture(&c->p, PST_RESULT_AUTH_FAILURE, PST_DIAGNOSTIC_PHASE_PEER_AUTHENTICATE); *operation = PST_BACKEND_OPERATION_FAILED; *error = PST_RESULT_AUTH_FAILURE; return PST_RESULT_OK; }
    *operation = PST_BACKEND_OPERATION_COMPLETE; *error = PST_RESULT_OK; return PST_RESULT_OK;
}
static PST_RESULT mock_interest(void *state, pst_u32 *interest) { if (!state || !interest) return PST_RESULT_INVALID_ARGUMENT; *interest = PST_BACKEND_INTEREST_READ; return PST_RESULT_OK; }
static PST_RESULT mock_wait(void *state, pst_u32 interest, pst_u32 timeout, PST_BACKEND_WAIT_RESULT *out)
{
    (void)interest; (void)timeout; ++g.wait; if (!state || !out) return PST_RESULT_INVALID_ARGUMENT; out->ready_interest = PST_BACKEND_INTEREST_READ; out->timed_out = 0; return PST_RESULT_OK;
}
static PST_RESULT mock_read(void *state, void *buffer, pst_size size, PST_BACKEND_IO_RESULT *out)
{
    (void)buffer; (void)size; ++g.read; if (!state || !out) return PST_RESULT_INVALID_ARGUMENT; memset(out, 0, sizeof(*out));
    if (m.read_closed) { out->operation = PST_BACKEND_OPERATION_CLOSED; out->close_kind = PST_BACKEND_CLOSE_CLEAN; }
    else out->operation = PST_BACKEND_OPERATION_COMPLETE;
    out->error = PST_RESULT_OK; return PST_RESULT_OK;
}
static PST_RESULT mock_write(void *state, const void *buffer, pst_size size, PST_BACKEND_IO_RESULT *out)
{
    (void)buffer; ++g.write; if (!state || !out) return PST_RESULT_INVALID_ARGUMENT; memset(out, 0, sizeof(*out)); out->bytes_transferred = size; out->operation = PST_BACKEND_OPERATION_COMPLETE; return PST_RESULT_OK;
}
static PST_RESULT mock_shutdown_step(void *state, pst_u32 *operation, PST_RESULT *error)
{
    ++g.shutdown_step; if (!state || !operation || !error) return PST_RESULT_INVALID_ARGUMENT;
    *operation = m.shutdown_mode == STEP_PENDING ? PST_BACKEND_OPERATION_NEED_READ : PST_BACKEND_OPERATION_COMPLETE; *error = PST_RESULT_OK; return PST_RESULT_OK;
}
static void mock_diagnostic_copy(const void *state, pst_internal_diagnostic *out)
{
    const lifecycle_prefix *p = (const lifecycle_prefix *)state;
    ++g.diagnostic_copy; g.diagnostic_order = ++g.sequence;
    if (!out) return; if (!p || !p->alive) { pst_diagnostic_initialize(out); return; } pst_diagnostic_copy(out, &p->diagnostic);
}
static void transport_destroy(pst_transport *base, int consumed)
{
    ++g.wrapper_destroy; if (!consumed) ++g.caller_transport_close; free(base);
}
static lifecycle_transport *new_transport(void)
{
    lifecycle_transport *t = (lifecycle_transport *)calloc(1, sizeof(*t));
    if (!t) return NULL; t->base.backend_id = "lifecycle-mock"; t->base.native = t; t->base.destroy = transport_destroy; t->token = 1; return t;
}
static void PST_CALL log_sink(void *context, const PST_LOG_EVENT *event)
{
    lifecycle_sink *sink = (lifecycle_sink *)context; (void)event; ++g.log_callback; if (!sink || !sink->alive) ++g.late_callback;
}

static const PST_BACKEND_VTABLE vtable = {
    sizeof(PST_BACKEND_VTABLE), PST_BACKEND_SPI_VERSION,
    mock_initialize, mock_backend_shutdown, mock_runtime_create, mock_runtime_destroy,
    mock_query, mock_validate, mock_connection_create, mock_connection_destroy,
    mock_attach, mock_handshake, mock_interest, mock_wait, mock_read, mock_write,
    mock_shutdown_step, NULL, NULL, mock_configure, NULL, mock_diagnostic_copy
};
static const PST_BACKEND_DESCRIPTOR descriptor = {
    sizeof(PST_BACKEND_DESCRIPTOR), PST_BACKEND_SPI_VERSION,
    "lifecycle-mock", "Lifecycle ownership mock", CAPS, &vtable
};

static void init_options(PST_RUNTIME_OPTIONS *o)
{
    memset(o, 0, sizeof(*o)); o->struct_size = sizeof(*o); o->api_version = PST_API_VERSION; o->selection = PST_BACKEND_SELECTION_EXACT; o->exact_backend_id = "lifecycle-mock";
}
static int create_runtime_config(pst_runtime **runtime, pst_config **config)
{
    PST_RUNTIME_OPTIONS o; init_options(&o); *runtime = NULL; *config = NULL;
    if (pst_runtime_create(&o, runtime) != PST_RESULT_OK) return 0;
    if (pst_config_create(config) != PST_RESULT_OK || pst_config_freeze(*config) != PST_RESULT_OK) return 0;
    return 1;
}
static int create_connection(pst_runtime *runtime, pst_config *config, pst_connection **connection)
{
    *connection = NULL; return pst_connection_create(runtime, config, connection) == PST_RESULT_OK;
}
static int attach_connection(pst_connection *connection, lifecycle_transport **transport)
{
    pst_u32 accepted = 0; PST_RESULT r = pst_connection_attach(connection, &(*transport)->base, PST_OWNERSHIP_TRANSFERRED, &accepted);
    if (accepted) *transport = NULL; return r == PST_RESULT_OK && accepted == 1UL;
}
static int test_config_copy_lifetimes(void)
{
    char hostname[] = "localhost";
    pst_u8 alpn_data[] = {'o','n','e'};
    pst_u8 cert[] = {1,2}, key[] = {3,4}, ca[] = {5,6};
    PST_CREDENTIAL_SOURCE cs; PST_TRUST_SOURCE ts; PST_IDENTITY_CONFIG id;
    PST_TLS_POLICY policy; PST_ALPN_PROTOCOL protocol;
    pst_credentials *credentials; pst_trust *trust; pst_config *config;
    const pst_u8 *value; pst_size size;
    memset(&cs, 0, sizeof(cs)); cs.struct_size = sizeof(cs); cs.api_version = PST_API_VERSION; cs.kind = PST_CREDENTIAL_SOURCE_CERT_DER_PKCS8_DER; cs.certificate_der = cert; cs.certificate_der_size = sizeof(cert); cs.private_key_der = key; cs.private_key_der_size = sizeof(key);
    CHECK(pst_credentials_create(&cs, &credentials) == PST_RESULT_OK);
    memset(&ts, 0, sizeof(ts)); ts.struct_size = sizeof(ts); ts.api_version = PST_API_VERSION; ts.kind = PST_TRUST_SOURCE_CUSTOM_CA_DER; ts.data = ca; ts.data_size = sizeof(ca);
    CHECK(pst_trust_create(&ts, &trust) == PST_RESULT_OK); CHECK(pst_config_create(&config) == PST_RESULT_OK);
    memset(&id, 0, sizeof(id)); id.struct_size = sizeof(id); id.api_version = PST_API_VERSION; id.credentials = credentials; id.trust = trust; id.expected_hostname = hostname; id.expected_hostname_size = 9; id.require_peer_authentication = 1; id.require_client_authentication = 1;
    CHECK(pst_config_set_identity(config, &id) == PST_RESULT_OK); pst_credentials_release(credentials); pst_trust_release(trust);
    hostname[0] = 'X'; cert[0] = 9; key[0] = 9; ca[0] = 9;
    memset(&policy, 0, sizeof(policy)); policy.struct_size = sizeof(policy); policy.api_version = PST_API_VERSION; policy.minimum_version = PST_TLS_VERSION_1_2; policy.maximum_version = PST_TLS_VERSION_1_3; policy.early_data = PST_FEATURE_DISABLED; protocol.data = alpn_data; protocol.size = sizeof(alpn_data); policy.alpn_protocols = &protocol; policy.alpn_protocol_count = 1; policy.alpn_requirement = PST_FEATURE_REQUIRED;
    CHECK(pst_config_set_tls_policy(config, &policy) == PST_RESULT_OK); alpn_data[0] = 'X'; CHECK(pst_config_freeze(config) == PST_RESULT_OK);
    CHECK(!strcmp(pst_config_expected_hostname(config), "localhost")); value = pst_config_alpn_wire(config, &size); CHECK(size == 4 && value[0] == 3 && !memcmp(value + 1, "one", 3));
    value = pst_credentials_certificate_der(pst_config_credentials(config), &size); CHECK(size == 2 && value[0] == 1); value = pst_credentials_private_key_der(pst_config_credentials(config), &size); CHECK(size == 2 && value[0] == 3); value = pst_trust_data(pst_config_trust(config), &size); CHECK(size == 2 && value[0] == 5);
    pst_config_release(config); return 0;
}
static int test_release_state(int which)
{
    pst_runtime *r; pst_config *cfg; pst_connection *c; lifecycle_transport *t = NULL;
    PST_DIAGNOSTIC_INFO saved = {0}; PST_IO_RESULT io; pst_u32 op; PST_RESULT error; int calls_before;
    reset_test(); CHECK(create_runtime_config(&r, &cfg)); CHECK(create_connection(r, cfg, &c)); pst_config_release(cfg);
    if (which != 0) { t = new_transport(); CHECK(t != NULL); CHECK(attach_connection(c, &t)); }
    if (which == 2) { m.handshake_mode = STEP_PENDING; CHECK(pst_connection_handshake(c, &op, &error) == PST_RESULT_OK && op == PST_OPERATION_NEED_READ); }
    if (which >= 3) { CHECK(pst_connection_handshake(c, &op, &error) == PST_RESULT_OK && op == PST_OPERATION_COMPLETE); }
    if (which == 4) { m.shutdown_mode = STEP_PENDING; CHECK(pst_connection_shutdown(c, &op, &error) == PST_RESULT_OK && op == PST_OPERATION_NEED_READ); }
    if (which == 5) { m.read_closed = 1; CHECK(pst_connection_read(c, NULL, 0, &io) == PST_RESULT_OK && io.operation == PST_OPERATION_CLOSED); }
    if (which == 6) { /* recreate failure from attached instead of established */
        pst_connection_release(c); pst_runtime_release(r); reset_test(); CHECK(create_runtime_config(&r, &cfg)); CHECK(create_connection(r, cfg, &c)); pst_config_release(cfg); t = new_transport(); CHECK(t != NULL); CHECK(attach_connection(c, &t)); m.handshake_mode = STEP_FAILED; CHECK(pst_connection_handshake(c, &op, &error) == PST_RESULT_OK && op == PST_OPERATION_FAILED);
        CHECK(pst_diagnostic_info_init(&saved) == PST_RESULT_OK); CHECK(pst_connection_copy_diagnostic(c, &saved) == PST_RESULT_OK && saved.valid);
    }
    calls_before = g.handshake + g.wait + g.read + g.write + g.shutdown_step;
    pst_connection_release(c);
    CHECK(g.connection_destroy == 1); CHECK(g.handshake + g.wait + g.read + g.write + g.shutdown_step == calls_before);
    if (which == 0) {
        CHECK(g.provider_transport_close == 0 && g.wrapper_destroy == 0);
    } else {
        CHECK(g.provider_transport_close == 1 && g.caller_transport_close == 0 && g.wrapper_destroy == 1 && g.ownership_accept == 1);
    }
    pst_runtime_release(r); CHECK(g.backend_initialize == 1 && g.backend_shutdown == 1); CHECK(g.runtime_create == 1 && g.runtime_destroy == 1); CHECK(g.connection_create == 1 && g.connection_destroy == 1);
    if (which == 6) CHECK(saved.valid && saved.normalized_result == PST_RESULT_AUTH_FAILURE);
    return 0;
}
static int test_attach_failures(void)
{
    pst_runtime *r; pst_config *cfg; pst_connection *c; lifecycle_transport *t; pst_u32 accepted; PST_RESULT x;
    reset_test(); CHECK(create_runtime_config(&r, &cfg)); CHECK(create_connection(r, cfg, &c)); pst_config_release(cfg);
    m.attach_mode = ATTACH_FAIL_BEFORE; t = new_transport(); CHECK(t != NULL); accepted = 9; x = pst_connection_attach(c, &t->base, PST_OWNERSHIP_TRANSFERRED, &accepted);
    CHECK(x == PST_RESULT_TRANSPORT_FAILURE && accepted == 0 && g.provider_transport_close == 0); pst_transport_release(&t->base); CHECK(g.caller_transport_close == 1); pst_connection_release(c); pst_runtime_release(r); CHECK(g.backend_initialize == 1 && g.backend_shutdown == 1 && g.runtime_create == 1 && g.runtime_destroy == 1); CHECK(g.connection_create == 1 && g.connection_destroy == 1 && g.transport_attach == 1 && g.ownership_accept == 0);
    reset_test(); CHECK(create_runtime_config(&r, &cfg)); CHECK(create_connection(r, cfg, &c)); pst_config_release(cfg);
    m.attach_mode = ATTACH_FAIL_AFTER; t = new_transport(); CHECK(t != NULL); accepted = 0; x = pst_connection_attach(c, &t->base, PST_OWNERSHIP_TRANSFERRED, &accepted); CHECK(x == PST_RESULT_BACKEND_FAILURE && accepted == 1); t = NULL;
    CHECK(g.ownership_accept == 1 && g.provider_transport_close == 1 && g.caller_transport_close == 0); pst_connection_release(c); CHECK(g.provider_transport_close == 1 && g.wrapper_destroy == 1); pst_runtime_release(r); CHECK(g.backend_initialize == 1 && g.backend_shutdown == 1 && g.runtime_create == 1 && g.runtime_destroy == 1); CHECK(g.connection_create == 1 && g.connection_destroy == 1 && g.transport_attach == 1 && g.ownership_accept == 1); return 0;
}
static int test_creation_failures(void)
{
    PST_RUNTIME_OPTIONS o; pst_runtime *r; pst_config *cfg; pst_connection *c; PST_DIAGNOSTIC_INFO d;
    reset_test(); init_options(&o); m.initialize_fail = 1; r = (pst_runtime *)1; CHECK(pst_diagnostic_info_init(&d) == PST_RESULT_OK); CHECK(pst_runtime_create_ex(&o, &r, &d) == PST_RESULT_UNSUPPORTED && r == NULL); CHECK(g.backend_initialize == 1 && g.backend_shutdown == 1 && d.valid); m.initialize_fail = 0; CHECK(pst_runtime_create(&o, &r) == PST_RESULT_OK); pst_runtime_release(r); CHECK(g.backend_shutdown == 2);
    reset_test(); init_options(&o); m.runtime_fail = 1; r = (pst_runtime *)1; CHECK(pst_runtime_create_ex(&o, &r, &d) == PST_RESULT_UNSUPPORTED && r == NULL); CHECK(g.runtime_create == 1 && g.runtime_failure_cleanup == 1 && g.backend_shutdown == 1); m.runtime_fail = 0; CHECK(pst_runtime_create(&o, &r) == PST_RESULT_OK); pst_runtime_release(r);
    reset_test(); CHECK(create_runtime_config(&r, &cfg)); m.connection_fail = 1; c = (pst_connection *)1; CHECK(pst_connection_create(r, cfg, &c) == PST_RESULT_BACKEND_FAILURE && c == NULL); CHECK(g.connection_create == 1 && g.connection_failure_cleanup == 1 && g.connection_destroy == 0); m.connection_fail = 0; CHECK(create_connection(r, cfg, &c)); pst_connection_release(c); pst_config_release(cfg); pst_runtime_release(r);
    reset_test(); CHECK(create_runtime_config(&r, &cfg)); m.configure_fail = 1; c = (pst_connection *)1; CHECK(pst_diagnostic_info_init(&d) == PST_RESULT_OK); CHECK(pst_connection_create_ex(r, cfg, &c, &d) == PST_RESULT_POLICY_VIOLATION && c == NULL); CHECK(d.valid && g.diagnostic_copy == 2 && g.connection_destroy == 1); CHECK(g.diagnostic_order > 0 && g.destroy_order > g.diagnostic_order); pst_config_release(cfg); pst_runtime_release(r); return 0;
}
static int test_early_runtime_release(void)
{
    pst_runtime *r; pst_config *cfg; pst_connection *c; lifecycle_transport *t; pst_u32 op; PST_RESULT error;
    reset_test(); CHECK(create_runtime_config(&r, &cfg)); CHECK(create_connection(r, cfg, &c)); pst_config_release(cfg); pst_runtime_release(r); CHECK(g.runtime_destroy == 0 && g.backend_shutdown == 0);
    t = new_transport(); CHECK(t && attach_connection(c, &t)); CHECK(pst_connection_handshake(c, &op, &error) == PST_RESULT_OK && op == PST_OPERATION_COMPLETE); pst_connection_release(c); CHECK(g.connection_destroy == 1 && g.runtime_destroy == 0); pst_runtime_release(r); CHECK(g.backend_initialize == 1 && g.backend_shutdown == 1); CHECK(g.runtime_create == 1 && g.runtime_destroy == 1); CHECK(g.connection_create == 1 && g.connection_destroy == 1); return 0;
}
static int test_two_connections_logging(void)
{
    PST_RUNTIME_OPTIONS o; PST_LOG_CONFIG log; lifecycle_sink sink; pst_runtime *r; pst_config *cfg; pst_connection *a, *b; lifecycle_transport *ta, *tb; pst_u32 op; PST_RESULT error; PST_DIAGNOSTIC_INFO diagnostic; int after_a, before_release;
    reset_test(); init_options(&o); memset(&sink, 0, sizeof(sink)); sink.alive = 1; CHECK(pst_log_config_init(&log) == PST_RESULT_OK); log.level = PST_LOG_LEVEL_DEBUG; log.callback = log_sink; log.user_context = &sink;
    CHECK(pst_runtime_create_with_logging(&o, &log, &r, NULL) == PST_RESULT_OK); CHECK(pst_config_create(&cfg) == PST_RESULT_OK && pst_config_freeze(cfg) == PST_RESULT_OK); CHECK(create_connection(r, cfg, &a)); CHECK(create_connection(r, cfg, &b)); pst_config_release(cfg);
    ta = new_transport(); tb = new_transport(); CHECK(ta && tb && attach_connection(a, &ta) && attach_connection(b, &tb)); CHECK(pst_connection_handshake(a, &op, &error) == PST_RESULT_OK); m.handshake_mode = STEP_FAILED; CHECK(pst_connection_handshake(b, &op, &error) == PST_RESULT_OK && op == PST_OPERATION_FAILED); pst_connection_release(a); after_a = g.log_callback; CHECK(pst_diagnostic_info_init(&diagnostic) == PST_RESULT_OK); CHECK(pst_connection_copy_diagnostic(b, &diagnostic) == PST_RESULT_OK && diagnostic.valid); CHECK(g.log_callback == after_a); pst_connection_release(b); CHECK(g.connection_destroy == 2 && g.provider_transport_close == 2); before_release = g.log_callback; pst_runtime_release(r); CHECK(g.backend_initialize == 1 && g.backend_shutdown == 1 && g.runtime_create == 1 && g.runtime_destroy == 1); CHECK(g.connection_create == 2 && g.connection_destroy == 2 && g.ownership_accept == 2 && g.provider_transport_close == 2 && g.caller_transport_close == 0); CHECK(g.log_callback == before_release && g.late_callback == 0); sink.alive = 0; return 0;
}
int main(void)
{
    int i;
    pst_backend_registry_reset(); CHECK(pst_backend_register(&descriptor) == PST_RESULT_OK);
    CHECK(test_config_copy_lifetimes() == 0);
    for (i = 0; i < 7; ++i) CHECK(test_release_state(i) == 0);
    CHECK(test_attach_failures() == 0); CHECK(test_creation_failures() == 0); CHECK(test_early_runtime_release() == 0); CHECK(test_two_connections_logging() == 0);
    for (i = 0; i < 500; ++i) CHECK(test_release_state(3) == 0);
    printf("STRESS_MOCK_CYCLES=500 BALANCED_CYCLES=500 PASS=1\n");
    CHECK(pst_backend_unregister("lifecycle-mock") == PST_RESULT_OK);
    printf("test_lifecycle_ownership: PASS\n"); return 0;
}