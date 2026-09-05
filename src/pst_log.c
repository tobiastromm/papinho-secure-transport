/* SPDX-License-Identifier: MPL-2.0 */
#include "pst_log.h"
#include "pst_internal.h"
#include <string.h>
static int pst_log_level_valid(PST_LOG_LEVEL level){return level<=PST_LOG_LEVEL_TRACE;}
PST_RESULT PST_CALL pst_log_config_init(PST_LOG_CONFIG *config)
{
    if(!config)return PST_RESULT_INVALID_ARGUMENT;
    memset(config,0,sizeof(*config));config->struct_size=(pst_u32)sizeof(*config);
    config->api_version=PST_API_VERSION;config->level=PST_LOG_LEVEL_OFF;return PST_RESULT_OK;
}
PST_RESULT pst_log_state_initialize(pst_log_state *state,const PST_LOG_CONFIG *config)
{
    PST_RESULT result;if(!state)return PST_RESULT_INVALID_ARGUMENT;
    state->level=PST_LOG_LEVEL_OFF;state->callback=NULL;state->user_context=NULL;
    if(!config)return PST_RESULT_OK;
    result=pst_validate_public_struct(config,PST_LOG_CONFIG_MIN_SIZE);if(result!=PST_RESULT_OK)return result;
    if(!pst_log_level_valid(config->level))return PST_RESULT_INVALID_ARGUMENT;
    if(!config->callback)return PST_RESULT_OK;
    state->level=config->level;state->callback=config->callback;state->user_context=config->user_context;return PST_RESULT_OK;
}
int pst_log_should_emit(PST_LOG_LEVEL configured,PST_LOG_LEVEL event_level)
{
    if(configured>PST_LOG_LEVEL_TRACE||event_level>PST_LOG_LEVEL_TRACE||configured==PST_LOG_LEVEL_OFF||event_level==PST_LOG_LEVEL_OFF)return 0;
    return event_level<=configured;
}
void pst_log_emit(const pst_log_state *state,PST_LOG_LEVEL level,pst_u32 event_id,pst_u32 category,PST_RESULT normalized_result,pst_u32 operation,const char *backend_id)
{
    PST_LOG_EVENT event;pst_size i;if(!state||!state->callback||!pst_log_should_emit(state->level,level))return;
    memset(&event,0,sizeof(event));event.struct_size=(pst_u32)sizeof(event);event.api_version=PST_API_VERSION;
    event.level=level;event.event_id=event_id;event.category=category;event.normalized_result=normalized_result;event.operation=operation;
    if(backend_id){for(i=0;i+1UL<PST_DIAGNOSTIC_BACKEND_ID_CAPACITY&&backend_id[i]!='\0';++i)event.backend_id[i]=backend_id[i];event.backend_id[i]='\0';}
    state->callback(state->user_context,&event);
}