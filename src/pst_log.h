#ifndef PST_LOG_H
#define PST_LOG_H
#include "papinho_secure_transport.h"
typedef struct pst_log_state {
    PST_LOG_LEVEL level;
    PST_LOG_CALLBACK callback;
    void *user_context;
} pst_log_state;
PST_RESULT pst_log_state_initialize(pst_log_state *state,const PST_LOG_CONFIG *config);
int pst_log_should_emit(PST_LOG_LEVEL configured,PST_LOG_LEVEL event_level);
void pst_log_emit(const pst_log_state *state,PST_LOG_LEVEL level,pst_u32 event_id,pst_u32 category,PST_RESULT normalized_result,pst_u32 operation,const char *backend_id);
#endif