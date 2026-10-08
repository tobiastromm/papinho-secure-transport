/* SPDX-License-Identifier: MPL-2.0 */
#include "papinho_secure_transport.h"
#include <stddef.h>
#include <stdio.h>

typedef struct candidate_transport_termination_info {
    pst_u32 struct_size;
    pst_u32 api_version;
    pst_u32 valid;
    pst_u32 termination;
    pst_u32 operation;
    pst_u32 native_domain;
    pst_u32 native_code;
    pst_u32 native_code_valid;
} candidate_transport_termination_info;

typedef struct candidate_alignment_probe {
    char prefix;
    candidate_transport_termination_info value;
} candidate_alignment_probe;

#define ABI_ASSERT(name,expression) typedef char abi_assert_##name[(expression)?1:-1]
ABI_ASSERT(size,sizeof(candidate_transport_termination_info)==32UL);
ABI_ASSERT(alignment,offsetof(candidate_alignment_probe,value)==4UL);
ABI_ASSERT(offset_struct_size,offsetof(candidate_transport_termination_info,struct_size)==0UL);
ABI_ASSERT(offset_api_version,offsetof(candidate_transport_termination_info,api_version)==4UL);
ABI_ASSERT(offset_valid,offsetof(candidate_transport_termination_info,valid)==8UL);
ABI_ASSERT(offset_termination,offsetof(candidate_transport_termination_info,termination)==12UL);
ABI_ASSERT(offset_operation,offsetof(candidate_transport_termination_info,operation)==16UL);
ABI_ASSERT(offset_native_domain,offsetof(candidate_transport_termination_info,native_domain)==20UL);
ABI_ASSERT(offset_native_code,offsetof(candidate_transport_termination_info,native_code)==24UL);
ABI_ASSERT(offset_native_code_valid,offsetof(candidate_transport_termination_info,native_code_valid)==28UL);

int main(void)
{
    printf("TRANSPORT_TERMINATION_ABI SIZE=32 ALIGNMENT=4 OFFSETS=0,4,8,12,16,20,24,28 PASS\n");
    return 0;
}
