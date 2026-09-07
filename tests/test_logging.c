/* SPDX-License-Identifier: MPL-2.0 */
#include "papinho_secure_transport.h"
#include "pst_log.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x,n) if(!(x)){printf("test_logging: FAIL %d\n",n);return n;}
typedef struct sink { int count;PST_LOG_EVENT event; } sink;
static void PST_CALL capture(void *v,const PST_LOG_EVENT *e){sink*s=(sink*)v;s->count++;s->event=*e;}
int main(void){PST_LOG_CONFIG c;pst_log_state state;sink s;memset(&s,0,sizeof(s));CHECK(pst_log_config_init(&c)==PST_RESULT_OK,1);c.level=PST_LOG_LEVEL_INFO;c.callback=capture;c.user_context=&s;CHECK(pst_log_state_initialize(&state,&c)==PST_RESULT_OK,2);pst_log_emit_facts(&state,PST_LOG_LEVEL_INFO,PST_LOG_EVENT_CONNECTION_SECURE,PST_LOG_CATEGORY_TLS,PST_RESULT_OK,PST_DIAGNOSTIC_OPERATION_HANDSHAKE,"provider",PST_CONNECTION_ROLE_CLIENT,PST_KNOWN_TRUE,PST_KNOWN_TRUE);CHECK(s.count==1&&s.event.role==PST_CONNECTION_ROLE_CLIENT&&s.event.peer_auth_fact==PST_KNOWN_TRUE&&s.event.policy_fact==PST_KNOWN_TRUE&&!strcmp(s.event.backend_id,"provider"),3);pst_log_emit(&state,PST_LOG_LEVEL_TRACE,PST_LOG_EVENT_OPERATION_PROGRESS,PST_LOG_CATEGORY_IO,PST_RESULT_OK,PST_DIAGNOSTIC_OPERATION_READ,"secret-not-payload");CHECK(s.count==1,4);c.level=PST_LOG_LEVEL_OFF;CHECK(pst_log_state_initialize(&state,&c)==PST_RESULT_OK,5);pst_log_emit(&state,PST_LOG_LEVEL_ERROR,PST_LOG_EVENT_CONNECTION_FAILURE,PST_LOG_CATEGORY_TLS,PST_RESULT_PROTOCOL_FAILURE,PST_DIAGNOSTIC_OPERATION_HANDSHAKE,"provider");CHECK(s.count==1,6);printf("LOGGING=PASS ROLE_FACTS=PASS AUTH_POLICY_FACTS=PASS FILTERING=PASS\n");printf("test_logging: PASS\n");return 0;}
