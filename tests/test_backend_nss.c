#include "backends/nss/pst_backend_nss.h"
#include "prerr.h"
#include "sslerr.h"
#include "secerr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(condition, code) if (!(condition)) return (code)
int main(void)
{
    const PST_BACKEND_DESCRIPTOR *descriptor;
    const PST_BACKEND_DESCRIPTOR *found;
    pst_u32 expected;
    pst_u32 capabilities;
    void *backend_state;
    void *second_state;
    void *runtime_state;
    void *connection_state;
    pst_u32 accepted;
    pst_u32 ready_interest;
    pst_u32 close_notify_observed;
    PST_BACKEND_IO_RESULT io_result;
    PST_RUNTIME_OPTIONS options;
    PST_RUNTIME_INFO runtime_info;
    pst_runtime *public_runtime;
    const char *preferences[2];
    pst_backend_registry_reset();
    descriptor = pst_backend_nss_descriptor();
    CHECK(descriptor != NULL, 1);
    CHECK(strcmp(descriptor->id, "retrozilla-nss") == 0, 2);
    CHECK(pst_backend_validate(descriptor) == PST_RESULT_OK, 3);
    expected = PST_BACKEND_CAP_TLS_1_2 | PST_BACKEND_CAP_TLS_1_3 |
        PST_BACKEND_CAP_HOSTNAME_VERIFY | PST_BACKEND_CAP_NONBLOCKING |
        PST_BACKEND_CAP_BACKEND_WAIT | PST_BACKEND_CAP_CLIENT_AUTH |
        PST_BACKEND_CAP_CUSTOM_TRUST | PST_BACKEND_CAP_PEER_INFO |
        PST_BACKEND_CAP_ALPN;
    CHECK(descriptor->capabilities == expected, 4);
    CHECK(descriptor->vtable->wait != NULL, 5);
    CHECK(descriptor->vtable->peer_info_create != NULL, 6);
    CHECK(descriptor->vtable->connection_configure_identity != NULL, 31);
    CHECK(pst_backend_nss_is_would_block(PR_WOULD_BLOCK_ERROR), 7);
    CHECK(!pst_backend_nss_is_would_block(PR_CONNECT_RESET_ERROR), 8);
    CHECK(pst_backend_nss_classify_poll_flags(1, 0, 0, 0, 0,
        &ready_interest) == PST_RESULT_OK, 48);
    CHECK(ready_interest == PST_BACKEND_INTEREST_READ, 49);
    CHECK(pst_backend_nss_classify_poll_flags(0, 1, 0, 0, 0,
        &ready_interest) == PST_RESULT_OK, 50);
    CHECK(ready_interest == PST_BACKEND_INTEREST_WRITE, 51);
    CHECK(pst_backend_nss_classify_poll_flags(1, 1, 0, 0, 0,
        &ready_interest) == PST_RESULT_OK, 52);
    CHECK(ready_interest == (PST_BACKEND_INTEREST_READ |
        PST_BACKEND_INTEREST_WRITE), 53);
    CHECK(pst_backend_nss_classify_poll_flags(1, 0, 0, 1, 0,
        &ready_interest) == PST_RESULT_OK, 41);
    CHECK(ready_interest == PST_BACKEND_INTEREST_READ, 42);
    CHECK(pst_backend_nss_classify_poll_flags(0, 0, 0, 1, 0,
        &ready_interest) == PST_RESULT_OK, 43);
    CHECK(ready_interest == PST_BACKEND_INTEREST_READ, 44);
    CHECK(pst_backend_nss_classify_poll_flags(0, 1, 0, 1, 0,
        &ready_interest) == PST_RESULT_OK, 54);
    CHECK(ready_interest == (PST_BACKEND_INTEREST_READ |
        PST_BACKEND_INTEREST_WRITE), 55);
    CHECK(pst_backend_nss_classify_poll_flags(1, 1, 1, 0, 0,
        &ready_interest) == PST_RESULT_TRANSPORT_FAILURE, 45);
    CHECK(pst_backend_nss_classify_poll_flags(0, 0, 0, 0, 1,
        &ready_interest) == PST_RESULT_TRANSPORT_FAILURE, 46);
    CHECK(pst_backend_nss_classify_poll_flags(0, 0, 0, 0, 0,
        NULL) == PST_RESULT_INVALID_ARGUMENT, 47);
    close_notify_observed = 0UL;
    CHECK(!pst_backend_nss_is_close_notify_alert(10UL), 56);
    pst_backend_nss_observe_alert(10UL, &close_notify_observed);
    CHECK(close_notify_observed == 0UL, 57);
    CHECK(pst_backend_nss_is_close_notify_alert(0UL), 58);
    pst_backend_nss_observe_alert(0UL, &close_notify_observed);
    CHECK(close_notify_observed == 1UL, 59);
    memset(&io_result, 0xa5, sizeof(io_result));
    CHECK(pst_backend_nss_classify_eof(1UL, &io_result) == PST_RESULT_OK, 60);
    CHECK(io_result.operation == PST_BACKEND_OPERATION_CLOSED &&
        io_result.close_kind == PST_BACKEND_CLOSE_CLEAN &&
        io_result.error == PST_RESULT_OK, 61);
    memset(&io_result, 0xa5, sizeof(io_result));
    CHECK(pst_backend_nss_classify_eof(0UL, &io_result) == PST_RESULT_OK, 62);
    CHECK(io_result.operation == PST_BACKEND_OPERATION_FAILED &&
        io_result.close_kind == PST_BACKEND_CLOSE_TRUNCATED &&
        io_result.error == PST_RESULT_TRUNCATED, 63);
    CHECK(pst_backend_nss_classify_eof(0UL, NULL) ==
        PST_RESULT_INVALID_ARGUMENT, 64);
    CHECK(pst_backend_nss_alert_registration_result(1) == PST_RESULT_OK, 65);
    CHECK(pst_backend_nss_alert_registration_result(0) ==
        PST_RESULT_BACKEND_FAILURE, 66);
    CHECK(pst_backend_nss_normalize_error(PR_CONNECT_RESET_ERROR) == PST_RESULT_TRUNCATED, 9);
    CHECK(pst_backend_nss_normalize_error(PR_IO_ERROR) == PST_RESULT_TRANSPORT_FAILURE, 10);
    CHECK(pst_backend_nss_normalize_error(SSL_ERROR_BAD_CERT_DOMAIN) == PST_RESULT_HOSTNAME_MISMATCH, 11);
    CHECK(pst_backend_nss_normalize_error(SEC_ERROR_UNKNOWN_ISSUER) == PST_RESULT_AUTH_FAILURE, 12);
    CHECK(pst_backend_nss_normalize_error(SSL_ERROR_NO_CYPHER_OVERLAP) == PST_RESULT_PROTOCOL_FAILURE, 13);
    CHECK(pst_backend_nss_normalize_error(SEC_ERROR_LIBRARY_FAILURE) == PST_RESULT_BACKEND_FAILURE, 14);
    CHECK(pst_backend_nss_last_error(NULL) == 0, 15);
    CHECK(pst_backend_nss_register() == PST_RESULT_OK, 16);
    found = pst_backend_find("retrozilla-nss"); CHECK(found == descriptor, 17);
    CHECK(pst_backend_nss_register() == PST_RESULT_INVALID_STATE, 18);
    CHECK(pst_backend_unregister("retrozilla-nss") == PST_RESULT_OK, 19);
    if (getenv("PST_NSS_RUN_LIFECYCLE") != NULL) {
        backend_state = NULL; second_state = NULL; runtime_state = NULL;
        connection_state = NULL; accepted = 9UL;
        CHECK(descriptor->vtable->initialize(&backend_state) == PST_RESULT_OK, 20);
        CHECK(backend_state != NULL, 21);
        CHECK(descriptor->vtable->query_capabilities(backend_state, &capabilities) == PST_RESULT_OK, 28);
        CHECK((capabilities & PST_BACKEND_CAP_HOSTNAME_VERIFY) != 0UL, 29);
        CHECK((capabilities & PST_BACKEND_CAP_TLS_1_3) != 0UL, 30);
        CHECK(descriptor->vtable->initialize(&second_state) == PST_RESULT_INVALID_STATE, 22);
        CHECK(second_state == NULL, 23);
        CHECK(descriptor->vtable->runtime_create(backend_state, &runtime_state) == PST_RESULT_OK, 24);
        CHECK(descriptor->vtable->connection_create(runtime_state, &connection_state) == PST_RESULT_OK, 25);
        CHECK(descriptor->vtable->attach_transport(connection_state, NULL,
            PST_BACKEND_OWNERSHIP_TRANSFERRED, &accepted) == PST_RESULT_INVALID_ARGUMENT, 26);
        CHECK(accepted == 0UL, 27);
        descriptor->vtable->connection_destroy(connection_state);
        descriptor->vtable->runtime_destroy(runtime_state);
        descriptor->vtable->shutdown(backend_state);
        CHECK(pst_backend_nss_register() == PST_RESULT_OK, 32);
        memset(&options,0,sizeof(options));options.struct_size=sizeof(options);
        options.api_version=PST_API_VERSION;options.selection=PST_BACKEND_SELECTION_EXACT;
        options.exact_backend_id="missing";public_runtime=NULL;
        CHECK(pst_runtime_create(&options,&public_runtime)==PST_RESULT_UNSUPPORTED,33);
        options.exact_backend_id="retrozilla-nss";
        CHECK(pst_runtime_create(&options,&public_runtime)==PST_RESULT_OK,34);
        memset(&runtime_info,0,sizeof(runtime_info));runtime_info.struct_size=sizeof(runtime_info);runtime_info.api_version=PST_API_VERSION;
        CHECK(pst_runtime_get_info(public_runtime,&runtime_info)==PST_RESULT_OK,35);
        CHECK(strcmp(runtime_info.backend_id,"retrozilla-nss")==0,36);pst_runtime_release(public_runtime);
        preferences[0]="missing";preferences[1]="retrozilla-nss";options.selection=PST_BACKEND_SELECTION_ORDERED;options.preferred_backend_ids=preferences;options.preferred_backend_count=2;
        CHECK(pst_runtime_create(&options,&public_runtime)==PST_RESULT_OK,37);pst_runtime_release(public_runtime);
        options.selection=PST_BACKEND_SELECTION_AUTOMATIC;options.required_capabilities=0;
        CHECK(pst_runtime_create(&options,&public_runtime)==PST_RESULT_OK,38);pst_runtime_release(public_runtime);
        options.selection=PST_BACKEND_SELECTION_EXACT;options.exact_backend_id="retrozilla-nss";options.required_capabilities=PST_CAP_SYSTEM_TRUST;
        CHECK(pst_runtime_create(&options,&public_runtime)==PST_RESULT_UNSUPPORTED,39);
        CHECK(pst_backend_unregister("retrozilla-nss")==PST_RESULT_OK,40);
    }
    printf("test_backend_nss: PASS\n"); return 0;
}
