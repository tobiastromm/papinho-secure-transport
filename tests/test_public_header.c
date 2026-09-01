#include "papinho_secure_transport.h"
#include <stdio.h>

int main(void)
{
    PST_DIAGNOSTIC_INFO diagnostic;
    volatile pst_u32 operation;
    volatile pst_u32 peer_operation;
    if (pst_diagnostic_info_init(&diagnostic) != PST_RESULT_OK) return 1;
    if (diagnostic.struct_size != PST_DIAGNOSTIC_INFO_MIN_SIZE) return 2;
    if (diagnostic.api_version != PST_API_VERSION || diagnostic.valid != 0UL) return 3;
    operation = PST_DIAGNOSTIC_OPERATION_RUNTIME;
    peer_operation = PST_DIAGNOSTIC_OPERATION_PEER_INFO;
    if (operation != 1UL || peer_operation != 11UL) return 4;
    printf("test_public_header: PASS\n");
    return 0;
}