/* SPDX-License-Identifier: MPL-2.0 */
#include "pst_diagnostic.h"
#include <string.h>
void pst_diagnostic_initialize(pst_internal_diagnostic *d){if(d)memset(d,0,sizeof(*d));}
void pst_diagnostic_clear(pst_internal_diagnostic *d){pst_u32 g;pst_internal_transport_termination t;if(!d)return;g=d->generation+1UL;t=d->transport_termination;memset(d,0,sizeof(*d));d->generation=g;d->transport_termination=t;}
void pst_diagnostic_copy(pst_internal_diagnostic *d,const pst_internal_diagnostic *s){if(!d||!s||d==s)return;memcpy(d,s,sizeof(*d));}
void pst_diagnostic_capture(pst_internal_diagnostic *d,PST_RESULT r,pst_u32 p,const char *id,pst_u32 domain,pst_i32 code,pst_i32 secondary,pst_u32 flags){pst_u32 g;pst_size n;pst_internal_transport_termination t;if(!d)return;g=d->generation+1UL;t=d->transport_termination;memset(d,0,sizeof(*d));d->generation=g;d->transport_termination=t;d->valid=1UL;d->result=r;d->phase=p;d->native_domain=domain;d->native_code=code;d->secondary_native_code=secondary;d->flags=flags;if(id){n=strlen(id);if(n>=PST_DIAGNOSTIC_BACKEND_ID_CAPACITY)n=PST_DIAGNOSTIC_BACKEND_ID_CAPACITY-1;memcpy(d->backend_id,id,n);d->backend_id[n]='\0';}}
static int pst_diagnostic_version_ok(pst_u32 version){return ((version>>16)&0xffffUL)==PST_API_VERSION_MAJOR;}
static pst_u32 pst_diagnostic_public_operation(pst_u32 phase)
{
    if(phase==PST_DIAGNOSTIC_PHASE_BACKEND_INITIALIZE||phase==PST_DIAGNOSTIC_PHASE_RUNTIME_CREATE||phase==PST_DIAGNOSTIC_PHASE_CAPABILITY_VALIDATE||phase==PST_DIAGNOSTIC_PHASE_BACKEND_SELECT)return PST_DIAGNOSTIC_OPERATION_RUNTIME;
    if(phase==PST_DIAGNOSTIC_PHASE_TLS_CONFIGURE||phase==PST_DIAGNOSTIC_PHASE_ALPN||phase==PST_DIAGNOSTIC_PHASE_IDENTITY_SETUP)return PST_DIAGNOSTIC_OPERATION_CONFIGURATION;
    if(phase==PST_DIAGNOSTIC_PHASE_TRANSPORT_ATTACH)return PST_DIAGNOSTIC_OPERATION_TRANSPORT;
    if(phase==PST_DIAGNOSTIC_PHASE_CONNECTION_CREATE)return PST_DIAGNOSTIC_OPERATION_CONNECTION;
    if(phase==PST_DIAGNOSTIC_PHASE_HANDSHAKE)return PST_DIAGNOSTIC_OPERATION_HANDSHAKE;
    if(phase==PST_DIAGNOSTIC_PHASE_PEER_AUTHENTICATE||phase==PST_DIAGNOSTIC_PHASE_HOSTNAME_VERIFY)return PST_DIAGNOSTIC_OPERATION_AUTHENTICATION;
    if(phase==PST_DIAGNOSTIC_PHASE_READ)return PST_DIAGNOSTIC_OPERATION_READ;
    if(phase==PST_DIAGNOSTIC_PHASE_WRITE)return PST_DIAGNOSTIC_OPERATION_WRITE;
    if(phase==PST_DIAGNOSTIC_PHASE_WAIT)return PST_DIAGNOSTIC_OPERATION_WAIT;
    if(phase==PST_DIAGNOSTIC_PHASE_SHUTDOWN)return PST_DIAGNOSTIC_OPERATION_SHUTDOWN;
    if(phase==PST_DIAGNOSTIC_PHASE_PEER_INFO)return PST_DIAGNOSTIC_OPERATION_PEER_INFO;
    return PST_DIAGNOSTIC_OPERATION_NONE;
}
void pst_transport_termination_capture(pst_internal_diagnostic *d,pst_u32 termination,pst_u32 phase,pst_u32 domain,pst_u32 code,pst_u32 code_valid)
{
    pst_internal_transport_termination *t;if(!d||d->transport_termination.valid)return;t=&d->transport_termination;t->valid=1UL;t->termination=termination;t->operation=pst_diagnostic_public_operation(phase);t->native_domain=code_valid?domain:PST_NATIVE_ERROR_DOMAIN_NONE;t->native_code=code_valid?code:0UL;t->native_code_valid=code_valid?1UL:0UL;
}
PST_RESULT PST_CALL pst_transport_termination_info_init(PST_TRANSPORT_TERMINATION_INFO *out){if(!out)return PST_RESULT_INVALID_ARGUMENT;memset(out,0,sizeof(*out));out->struct_size=sizeof(*out);out->api_version=PST_API_VERSION;return PST_RESULT_OK;}
PST_RESULT pst_transport_termination_export_public(const pst_internal_diagnostic *source,PST_TRANSPORT_TERMINATION_INFO *out)
{
    pst_u32 size;if(!source||!out)return PST_RESULT_INVALID_ARGUMENT;if(out->struct_size<PST_TRANSPORT_TERMINATION_INFO_MIN_SIZE)return PST_RESULT_INVALID_ARGUMENT;if(!pst_diagnostic_version_ok(out->api_version))return PST_RESULT_INCOMPATIBLE_API;size=out->struct_size;memset(out,0,PST_TRANSPORT_TERMINATION_INFO_MIN_SIZE);out->struct_size=size;out->api_version=PST_API_VERSION;out->valid=source->transport_termination.valid;out->termination=source->transport_termination.termination;out->operation=source->transport_termination.operation;out->native_domain=source->transport_termination.native_domain;out->native_code=source->transport_termination.native_code;out->native_code_valid=source->transport_termination.native_code_valid;return PST_RESULT_OK;
}
static pst_u32 pst_diagnostic_public_reason(const pst_internal_diagnostic *source)
{
    if(source->reason)return source->reason;
    if(source->result==PST_RESULT_PEER_NAME_MISMATCH)return PST_DIAGNOSTIC_REASON_PEER_NAME_MISMATCH;
    if(source->phase==PST_DIAGNOSTIC_PHASE_ALPN)return PST_DIAGNOSTIC_REASON_ALPN_MISMATCH;
    if(source->phase==PST_DIAGNOSTIC_PHASE_CAPABILITY_VALIDATE||source->phase==PST_DIAGNOSTIC_PHASE_TLS_CONFIGURE)return PST_DIAGNOSTIC_REASON_TLS_POLICY_MISMATCH;
    if(source->result==PST_RESULT_AUTH_FAILURE)return PST_DIAGNOSTIC_REASON_PEER_CERT_INVALID;
    return PST_DIAGNOSTIC_REASON_NONE;
}
PST_RESULT PST_CALL pst_diagnostic_info_init(PST_DIAGNOSTIC_INFO *out)
{
    if(!out)return PST_RESULT_INVALID_ARGUMENT;
    memset(out,0,sizeof(*out));out->struct_size=sizeof(*out);out->api_version=PST_API_VERSION;return PST_RESULT_OK;
}
PST_RESULT pst_diagnostic_validate_public(const PST_DIAGNOSTIC_INFO *out)
{
    if(!out)return PST_RESULT_INVALID_ARGUMENT;
    if(out->struct_size<PST_DIAGNOSTIC_INFO_MIN_SIZE)return PST_RESULT_INVALID_ARGUMENT;
    if(!pst_diagnostic_version_ok(out->api_version))return PST_RESULT_INCOMPATIBLE_API;
    return PST_RESULT_OK;
}
PST_RESULT pst_diagnostic_export_public(const pst_internal_diagnostic *source,PST_DIAGNOSTIC_INFO *out)
{
    PST_RESULT result; pst_u32 size;
    if(!source||!out)return PST_RESULT_INVALID_ARGUMENT;
    result=pst_diagnostic_validate_public(out);if(result!=PST_RESULT_OK)return result;
    size=out->struct_size;memset(out,0,sizeof(*out));out->struct_size=size;out->api_version=PST_API_VERSION;
    out->valid=source->valid;out->generation=source->generation;
    if(source->valid){out->normalized_result=source->result;out->operation=pst_diagnostic_public_operation(source->phase);out->role=source->role;out->reason=pst_diagnostic_public_reason(source);memcpy(out->backend_id,source->backend_id,sizeof(out->backend_id));out->backend_id[sizeof(out->backend_id)-1]='\0';}
    return PST_RESULT_OK;
}
