#include "papinho_secure_transport_win32.h"

typedef char pst_win32_header_result_is_4[(sizeof(PST_RESULT) == 4) ? 1 : -1];
typedef char pst_win32_header_size_matches_pointer[(sizeof(pst_size) == sizeof(void *)) ? 1 : -1];
