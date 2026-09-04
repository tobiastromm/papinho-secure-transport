#include "papinho_secure_transport.h"
#include "papinho_secure_transport_win32.h"

#include <stdio.h>
#include <string.h>

typedef struct example_log {
    pst_u32 count;
} example_log;

static void PST_CALL on_log(void *context, const PST_LOG_EVENT *event)
{
    example_log *log = (example_log *)context;
    ++log->count;
    printf("level=%lu event=%lu result=%ld operation=%lu backend=%s\n",
           (unsigned long)event->level, (unsigned long)event->event_id,
           (long)event->normalized_result, (unsigned long)event->operation,
           event->backend_id);
}

int main(void)
{
    PST_RUNTIME_OPTIONS options;
    PST_LOG_CONFIG logging;
    PST_DIAGNOSTIC_INFO diagnostic;
    PST_RESULT result;
    example_log sink;
    pst_runtime *runtime = NULL;

    memset(&sink, 0, sizeof(sink));
    pst_log_config_init(&logging);
    logging.level = PST_LOG_LEVEL_INFO;
    logging.callback = on_log;
    logging.user_context = &sink;

    memset(&options, 0, sizeof(options));
    options.struct_size = sizeof(options);
    options.api_version = PST_API_VERSION;
    options.selection = PST_BACKEND_SELECTION_AUTOMATIC;
    options.required_capabilities = PST_CAP_TLS_1_2;

    if (pst_win32_register_builtin_providers() != PST_RESULT_OK) return 1;
    if (pst_diagnostic_info_init(&diagnostic) != PST_RESULT_OK) return 2;
    result = pst_runtime_create_with_logging(&options, &logging, &runtime,
                                             &diagnostic);
    if (result != PST_RESULT_OK) {
        printf("result=%s diagnostic_valid=%lu\n", pst_result_string(result),
               (unsigned long)diagnostic.valid);
        return 3;
    }
    printf("Callbacks are synchronous; event pointers are ephemeral and the caller owns this context. events=%lu\n",
           (unsigned long)sink.count);
    pst_runtime_release(runtime);
    return 0;
}