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
    pst_backend_registry_reset();
    descriptor = pst_backend_nss_descriptor();
    CHECK(descriptor != NULL, 1);
    CHECK(strcmp(descriptor->id, "retrozilla-nss") == 0, 2);
    CHECK(pst_backend_validate(descriptor) == PST_RESULT_OK, 3);
    expected = PST_BACKEND_CAP_TLS_1_2 | PST_BACKEND_CAP_TLS_1_3 |
        PST_BACKEND_CAP_HOSTNAME_VERIFY | PST_BACKEND_CAP_NONBLOCKING |
        PST_BACKEND_CAP_BACKEND_WAIT;
    CHECK(descriptor->capabilities == expected, 4);
    CHECK(descriptor->vtable->wait != NULL, 5);
    CHECK(descriptor->vtable->peer_info_create == NULL, 6);
    CHECK(pst_backend_nss_is_would_block(PR_WOULD_BLOCK_ERROR), 7);
    CHECK(!pst_backend_nss_is_would_block(PR_CONNECT_RESET_ERROR), 8);
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
        CHECK((capabilities & PST_BACKEND_CAP_HOSTNAME_VERIFY) == 0UL, 29);
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
    }
    printf("test_backend_nss: PASS\n"); return 0;
}