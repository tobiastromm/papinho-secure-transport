/* SPDX-License-Identifier: MPL-2.0 */
#include "papinho_secure_transport.h"
#include <stdio.h>

int main(void)
{
    PST_DIAGNOSTIC_INFO diagnostic;
    PST_LOG_CONFIG logging;
    volatile PST_LOG_EVENT event;
    volatile pst_u32 operation;
    volatile pst_u32 peer_operation;
    volatile pst_u32 log_constant;
    if (pst_diagnostic_info_init(&diagnostic) != PST_RESULT_OK) return 1;
    if (diagnostic.struct_size != PST_DIAGNOSTIC_INFO_MIN_SIZE) return 2;
    if (diagnostic.api_version != PST_API_VERSION || diagnostic.valid != 0UL) return 3;
    operation = PST_DIAGNOSTIC_OPERATION_RUNTIME;
    peer_operation = PST_DIAGNOSTIC_OPERATION_PEER_INFO;
    if (operation != 1UL || peer_operation != 11UL) return 4;
    if (pst_log_config_init(&logging) != PST_RESULT_OK) return 5;
    if (logging.level != PST_LOG_LEVEL_OFF || logging.callback != NULL) return 6;
    log_constant = PST_LOG_LEVEL_TRACE; if (log_constant != 5UL) return 7;
    log_constant = PST_LOG_EVENT_MIN_SIZE; if (log_constant != 60UL) return 8;
    event.level = PST_LOG_LEVEL_INFO; event.event_id = PST_LOG_EVENT_RUNTIME_READY;
    if (event.level != 3UL || event.event_id != 1UL) return 9;
    printf("test_public_header: PASS\n");
    return 0;
}