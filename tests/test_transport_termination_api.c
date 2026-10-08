/* SPDX-License-Identifier: MPL-2.0 */
#include "pst_diagnostic.h"
#include <string.h>
#define CHECK(c,n) if(!(c)) return (n)
typedef struct larger_termination_info { PST_TRANSPORT_TERMINATION_INFO base; pst_u32 tail; } larger_termination_info;
int main(void)
{
    pst_internal_diagnostic diagnostic;PST_TRANSPORT_TERMINATION_INFO info;larger_termination_info larger;
    pst_diagnostic_initialize(&diagnostic);
    CHECK(pst_transport_termination_info_init(NULL)==PST_RESULT_INVALID_ARGUMENT,1);
    CHECK(pst_transport_termination_info_init(&info)==PST_RESULT_OK,2);
    CHECK(info.struct_size==32UL&&info.api_version==PST_API_VERSION&&!info.valid,3);
    CHECK(pst_transport_termination_export_public(&diagnostic,&info)==PST_RESULT_OK&&!info.valid,4);
    info.struct_size=31UL;CHECK(pst_transport_termination_export_public(&diagnostic,&info)==PST_RESULT_INVALID_ARGUMENT,5);
    info.struct_size=32UL;info.api_version=0x00030000UL;CHECK(pst_transport_termination_export_public(&diagnostic,&info)==PST_RESULT_INCOMPATIBLE_API,6);
    memset(&larger,0,sizeof(larger));larger.base.struct_size=sizeof(larger);larger.base.api_version=PST_API_VERSION;larger.tail=0xa5a5a5a5UL;
    CHECK(pst_transport_termination_export_public(&diagnostic,&larger.base)==PST_RESULT_OK,7);
    CHECK(larger.base.struct_size==sizeof(larger)&&larger.tail==0xa5a5a5a5UL,8);
    pst_transport_termination_capture(&diagnostic,PST_TRANSPORT_TERMINATION_TCP_FIN,PST_DIAGNOSTIC_PHASE_READ,PST_NATIVE_ERROR_DOMAIN_NONE,0UL,0UL);
    CHECK(pst_transport_termination_export_public(&diagnostic,&larger.base)==PST_RESULT_OK,9);
    CHECK(larger.base.valid&&larger.base.termination==PST_TRANSPORT_TERMINATION_TCP_FIN,10);
    CHECK(larger.base.operation==PST_DIAGNOSTIC_OPERATION_READ&&!larger.base.native_code_valid,11);
    return 0;
}
