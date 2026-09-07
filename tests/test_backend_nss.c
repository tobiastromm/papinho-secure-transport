/* SPDX-License-Identifier: MPL-2.0 */
#include "backends/nss/pst_backend_nss.h"
#include "papinho_secure_transport_win32.h"
#include "prerr.h"
#include "sslerr.h"
#include "secerr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x,n) if(!(x)){printf("test_backend_nss: FAIL %d\n",n);return n;}
int main(void){const PST_BACKEND_DESCRIPTOR*d;PST_BACKEND_IO_RESULT io;pst_u32 ready,observed=0,expected,caps;void*backend=NULL,*runtime=NULL;
 pst_backend_registry_reset();d=pst_backend_nss_descriptor();CHECK(d&&!strcmp(d->id,"retrozilla-nss")&&pst_backend_validate(d)==PST_RESULT_OK,1);expected=PST_CAP_TLS_1_2|PST_CAP_TLS_1_3|PST_CAP_ROLE_CLIENT|PST_CAP_LOCAL_IDENTITY|PST_CAP_PEER_CERT_AUTH|PST_CAP_PEER_NAME_VERIFY|PST_CAP_CUSTOM_TRUST|PST_CAP_PEER_INFO|PST_CAP_ALPN_CLIENT|PST_CAP_NONBLOCKING|PST_CAP_BACKEND_WAIT;CHECK(d->capabilities==expected&&!(d->capabilities&PST_CAP_ROLE_SERVER),2);CHECK(d->vtable->wait&&d->vtable->peer_info_create&&d->metadata&&d->metadata->component_count==2,3);
 CHECK(pst_backend_nss_is_would_block(PR_WOULD_BLOCK_ERROR)&&!pst_backend_nss_is_would_block(PR_CONNECT_RESET_ERROR),4);CHECK(pst_backend_nss_classify_poll_flags(1,1,0,0,0,&ready)==PST_RESULT_OK&&ready==(PST_INTEREST_READ|PST_INTEREST_WRITE),5);CHECK(pst_backend_nss_classify_poll_flags(0,0,1,0,0,&ready)==PST_RESULT_TRANSPORT_FAILURE,6);CHECK(pst_backend_nss_classify_poll_flags(0,0,0,1,0,&ready)==PST_RESULT_OK&&ready==PST_INTEREST_READ,7);
 pst_backend_nss_observe_alert(10,&observed);CHECK(!observed,8);pst_backend_nss_observe_alert(0,&observed);CHECK(observed,9);memset(&io,0,sizeof(io));CHECK(pst_backend_nss_classify_eof(1,&io)==PST_RESULT_OK&&io.operation==PST_OPERATION_CLOSED&&io.close_kind==PST_CLOSE_CLEAN,10);CHECK(pst_backend_nss_classify_eof(0,&io)==PST_RESULT_OK&&io.operation==PST_OPERATION_FAILED&&io.close_kind==PST_CLOSE_TRUNCATED,11);
 CHECK(pst_backend_nss_normalize_error(PR_CONNECT_RESET_ERROR)==PST_RESULT_TRUNCATED,12);CHECK(pst_backend_nss_normalize_error(PR_IO_ERROR)==PST_RESULT_TRANSPORT_FAILURE,13);CHECK(pst_backend_nss_normalize_error(SSL_ERROR_BAD_CERT_DOMAIN)==PST_RESULT_PEER_NAME_MISMATCH,14);CHECK(pst_backend_nss_normalize_error(SEC_ERROR_UNKNOWN_ISSUER)==PST_RESULT_AUTH_FAILURE,15);
 if(getenv("PST_NSS_RUN_LIFECYCLE")){CHECK(d->vtable->initialize(&backend)==PST_RESULT_OK&&backend,16);CHECK(d->vtable->query_capabilities(backend,&caps)==PST_RESULT_OK&&caps==expected,17);CHECK(d->vtable->runtime_create(backend,&runtime)==PST_RESULT_OK&&runtime,18);d->vtable->runtime_destroy(runtime);d->vtable->shutdown(backend);}
 pst_backend_registry_reset();CHECK(pst_win32_register_retrozilla_nss()==PST_RESULT_OK,19);printf("NSS_SPI3_CLIENT=PASS ROLE_SERVER=NOT_ADVERTISED READINESS=PASS CLOSE_CLASSIFICATION=PASS\n");printf("test_backend_nss: PASS\n");return 0;}
