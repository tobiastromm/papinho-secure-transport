#define SECURITY_WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define SCHANNEL_USE_BLACKLISTS
#include <winsock2.h>
#include <windows.h>
#include <security.h>
#include <winternl.h>
#include <sspi.h>
#include <schannel.h>
#include "backends/schannel/pst_backend_schannel.h"
#include "pst_transport_internal.h"
#include "pst_identity_internal.h"
#include <stdlib.h>
#include <string.h>

#define SCH_INPUT_CAPACITY 131072UL
#define SCH_CONTEXT_FLAGS (ISC_REQ_SEQUENCE_DETECT|ISC_REQ_REPLAY_DETECT|ISC_REQ_CONFIDENTIALITY|ISC_REQ_ALLOCATE_MEMORY|ISC_REQ_STREAM)
#define SCH_OUT_NONE 0
#define SCH_OUT_HANDSHAKE 1
#define SCH_OUT_APPLICATION 2
#define SCH_OUT_SHUTDOWN 3

typedef struct sch_base { pst_internal_diagnostic diagnostic; } sch_base;
typedef struct sch_backend { pst_internal_diagnostic diagnostic; PSecurityFunctionTableA table; pst_u32 capabilities; } sch_backend;
typedef struct sch_runtime { pst_internal_diagnostic diagnostic; sch_backend *backend; } sch_runtime;
typedef struct sch_connection {
    pst_internal_diagnostic diagnostic;
    sch_runtime *runtime;
    SOCKET socket_value;
    int owns_socket;
    CredHandle credentials;
    CtxtHandle context;
    int credentials_valid;
    int context_valid;
    char *server_name;
    pst_u32 minimum_version;
    pst_u32 maximum_version;
    int first_handshake;
    int need_more_input;
    int handshake_complete;
    int handshake_complete_pending;
    int failed;
    int clean_close;
    int shutdown_started;
    int shutdown_complete_pending;
    SecPkgContext_StreamSizes stream_sizes;
    pst_u32 negotiated_version;
    ALG_ID negotiated_cipher;
    unsigned char encrypted_input[SCH_INPUT_CAPACITY];
    unsigned long encrypted_input_size;
    unsigned char *output;
    unsigned long output_size;
    unsigned long output_offset;
    pst_size output_application_size;
    int output_kind;
    int output_sspi_owned;
    unsigned char *plaintext;
    pst_size plaintext_size;
    pst_size plaintext_offset;
    pst_u32 interest;
    int (WSAAPI *send_call)(SOCKET,const char*,int,int);
} sch_connection;

static void capture(sch_base *s,PST_RESULT r,pst_u32 phase,pst_i32 code){pst_diagnostic_capture(&s->diagnostic,r,phase,"schannel",code?PST_DIAGNOSTIC_DOMAIN_WIN32:PST_DIAGNOSTIC_DOMAIN_NONE,code,0,code?PST_DIAGNOSTIC_FLAG_NATIVE:0);}
static PST_RESULT map_security(SECURITY_STATUS status){
    if(status==SEC_E_WRONG_PRINCIPAL)return PST_RESULT_HOSTNAME_MISMATCH;
    if(status==SEC_E_UNTRUSTED_ROOT||status==SEC_E_CERT_UNKNOWN||status==SEC_E_CERT_EXPIRED)return PST_RESULT_AUTH_FAILURE;
    if(status==SEC_E_INSUFFICIENT_MEMORY)return PST_RESULT_OUT_OF_MEMORY;
    if(status==SEC_E_ALGORITHM_MISMATCH||status==SEC_E_UNSUPPORTED_FUNCTION)return PST_RESULT_UNSUPPORTED;
    if(status==SEC_E_ILLEGAL_MESSAGE||status==SEC_E_MESSAGE_ALTERED||status==SEC_E_OUT_OF_SEQUENCE)return PST_RESULT_PROTOCOL_FAILURE;
    return PST_RESULT_BACKEND_FAILURE;
}
static void output_clear(sch_connection *c){if(c->output){if(c->output_sspi_owned)FreeContextBuffer(c->output);else free(c->output);}c->output=NULL;c->output_size=0;c->output_offset=0;c->output_application_size=0;c->output_kind=SCH_OUT_NONE;c->output_sspi_owned=0;}
static int acquire_credentials(pst_u32 minimum,pst_u32 maximum,CredHandle *handle){SCH_CREDENTIALS credentials;TLS_PARAMETERS parameters;TimeStamp expiry;SECURITY_STATUS status;DWORD disabled;
    memset(&credentials,0,sizeof(credentials));memset(&parameters,0,sizeof(parameters));
    disabled=SP_PROT_SSL2_CLIENT|SP_PROT_SSL3_CLIENT|SP_PROT_TLS1_0_CLIENT|SP_PROT_TLS1_1_CLIENT;
    if(minimum>=PST_TLS_VERSION_1_3)disabled|=SP_PROT_TLS1_2_CLIENT;
#ifdef SP_PROT_TLS1_3_CLIENT
    if(maximum<PST_TLS_VERSION_1_3)disabled|=SP_PROT_TLS1_3_CLIENT;
#else
    if(minimum>=PST_TLS_VERSION_1_3)return 0;
#endif
    parameters.grbitDisabledProtocols=disabled;
    credentials.dwVersion=SCH_CREDENTIALS_VERSION;
    credentials.dwFlags=SCH_CRED_AUTO_CRED_VALIDATION|SCH_CRED_NO_DEFAULT_CREDS|SCH_USE_STRONG_CRYPTO;
    credentials.cTlsParameters=1;credentials.pTlsParameters=&parameters;
    status=AcquireCredentialsHandleA(NULL,UNISP_NAME_A,SECPKG_CRED_OUTBOUND,NULL,&credentials,NULL,NULL,handle,&expiry);
    return status==SEC_E_OK;
}
static int probe_protocol(pst_u32 version){CredHandle handle;if(!acquire_credentials(version,version,&handle))return 0;FreeCredentialsHandle(&handle);return 1;}
static PST_RESULT sch_initialize(void **out){sch_backend *s;if(!out)return PST_RESULT_INVALID_ARGUMENT;*out=NULL;s=(sch_backend*)calloc(1,sizeof(*s));if(!s)return PST_RESULT_OUT_OF_MEMORY;pst_diagnostic_initialize(&s->diagnostic);s->table=InitSecurityInterfaceA();if(!s->table){capture((sch_base*)s,PST_RESULT_BACKEND_FAILURE,PST_DIAGNOSTIC_PHASE_BACKEND_INITIALIZE,(pst_i32)GetLastError());free(s);return PST_RESULT_BACKEND_FAILURE;}s->capabilities=PST_BACKEND_CAP_NONBLOCKING|PST_BACKEND_CAP_BACKEND_WAIT|PST_BACKEND_CAP_SYSTEM_TRUST|PST_BACKEND_CAP_HOSTNAME_VERIFY;if(probe_protocol(PST_TLS_VERSION_1_2))s->capabilities|=PST_BACKEND_CAP_TLS_1_2;*out=s;return PST_RESULT_OK;}
static void sch_shutdown(void *v){free(v);}
static PST_RESULT sch_runtime_create(void *v,void **out){sch_runtime *r;if(!v||!out)return PST_RESULT_INVALID_ARGUMENT;*out=NULL;r=(sch_runtime*)calloc(1,sizeof(*r));if(!r)return PST_RESULT_OUT_OF_MEMORY;pst_diagnostic_initialize(&r->diagnostic);r->backend=(sch_backend*)v;*out=r;return PST_RESULT_OK;}
static void sch_runtime_destroy(void *v){free(v);}
static PST_RESULT sch_query(void *v,pst_u32 *caps){sch_backend*s=(sch_backend*)v;if(!s||!caps)return PST_RESULT_INVALID_ARGUMENT;*caps=s->capabilities;return PST_RESULT_OK;}
static PST_RESULT sch_validate(void *v,pst_u32 required){sch_runtime*r=(sch_runtime*)v;return r&&!(required&~r->backend->capabilities)?PST_RESULT_OK:PST_RESULT_UNSUPPORTED;}
static PST_RESULT sch_connection_create(void *v,void **out){sch_connection *c;if(!v||!out)return PST_RESULT_INVALID_ARGUMENT;*out=NULL;c=(sch_connection*)calloc(1,sizeof(*c));if(!c)return PST_RESULT_OUT_OF_MEMORY;pst_diagnostic_initialize(&c->diagnostic);c->runtime=(sch_runtime*)v;c->socket_value=INVALID_SOCKET;c->first_handshake=1;c->interest=PST_BACKEND_INTEREST_READ;c->send_call=send;*out=c;return PST_RESULT_OK;}
static void sch_connection_destroy(void *v){sch_connection *c=(sch_connection*)v;if(!c)return;output_clear(c);free(c->plaintext);free(c->server_name);if(c->context_valid)DeleteSecurityContext(&c->context);if(c->credentials_valid)FreeCredentialsHandle(&c->credentials);if(c->owns_socket&&c->socket_value!=INVALID_SOCKET)closesocket(c->socket_value);free(c);}
static PST_RESULT sch_configure(void *v,const pst_config *config){sch_connection*c=(sch_connection*)v;const char*name;const pst_trust*trust;pst_size n;if(!c||!config)return PST_RESULT_INVALID_ARGUMENT;if(pst_config_require_client_authentication(config)||pst_config_alpn_wire(config,&n)!=NULL)return PST_RESULT_UNSUPPORTED;if(!pst_config_require_peer_authentication(config))return PST_RESULT_POLICY_VIOLATION;trust=pst_config_trust(config);if(!trust||pst_trust_kind(trust)!=PST_TRUST_SOURCE_SYSTEM)return PST_RESULT_UNSUPPORTED;name=pst_config_expected_hostname(config);if(!name||!name[0])return PST_RESULT_POLICY_VIOLATION;c->server_name=(char*)malloc(strlen(name)+1);if(!c->server_name)return PST_RESULT_OUT_OF_MEMORY;strcpy(c->server_name,name);c->minimum_version=pst_config_minimum_version(config);c->maximum_version=pst_config_maximum_version(config);if(!acquire_credentials(c->minimum_version,c->maximum_version,&c->credentials)){capture((sch_base*)c,PST_RESULT_UNSUPPORTED,PST_DIAGNOSTIC_PHASE_TLS_CONFIGURE,(pst_i32)GetLastError());return PST_RESULT_UNSUPPORTED;}c->credentials_valid=1;return PST_RESULT_OK;}
static PST_RESULT sch_attach(void *v,void *native,pst_u32 ownership,pst_u32 *accepted){sch_connection *c=(sch_connection*)v;PST_NATIVE_TRANSPORT *t=(PST_NATIVE_TRANSPORT*)native;u_long nonblocking=1UL;if(!accepted)return PST_RESULT_INVALID_ARGUMENT;*accepted=0;if(!c||!t||c->owns_socket)return PST_RESULT_INVALID_ARGUMENT;if(ownership!=PST_BACKEND_OWNERSHIP_TRANSFERRED)return PST_RESULT_UNSUPPORTED;if(t->struct_size<PST_NATIVE_TRANSPORT_MIN_SIZE||t->version!=PST_NATIVE_TRANSPORT_VERSION||t->kind!=PST_NATIVE_TRANSPORT_KIND_WIN32_SOCKET)return PST_RESULT_INVALID_ARGUMENT;if(ioctlsocket((SOCKET)t->native_socket,FIONBIO,&nonblocking)!=0){capture((sch_base*)c,PST_RESULT_TRANSPORT_FAILURE,PST_DIAGNOSTIC_PHASE_TRANSPORT_ATTACH,(pst_i32)WSAGetLastError());return PST_RESULT_TRANSPORT_FAILURE;}c->socket_value=(SOCKET)t->native_socket;c->owns_socket=1;*accepted=1;return PST_RESULT_OK;}
static int recv_input(sch_connection*c,pst_u32 phase){int got,code;if(c->encrypted_input_size>=SCH_INPUT_CAPACITY){capture((sch_base*)c,PST_RESULT_PROTOCOL_FAILURE,phase,SEC_E_INCOMPLETE_MESSAGE);return -1;}got=recv(c->socket_value,(char*)c->encrypted_input+c->encrypted_input_size,(int)(SCH_INPUT_CAPACITY-c->encrypted_input_size),0);if(got>0){c->encrypted_input_size+=(unsigned long)got;return 1;}if(got==0)return -2;code=WSAGetLastError();if(code==WSAEWOULDBLOCK)return 0;capture((sch_base*)c,PST_RESULT_TRANSPORT_FAILURE,phase,(pst_i32)code);return -1;}
static int drain_output(sch_connection*c,pst_u32 phase){int sent,code;while(c->output_offset<c->output_size){sent=c->send_call(c->socket_value,(const char*)c->output+c->output_offset,(int)(c->output_size-c->output_offset),0);if(sent>0){c->output_offset+=(unsigned long)sent;continue;}code=WSAGetLastError();if(sent==SOCKET_ERROR&&code==WSAEWOULDBLOCK){c->interest=PST_BACKEND_INTEREST_WRITE;return 0;}capture((sch_base*)c,PST_RESULT_TRANSPORT_FAILURE,phase,(pst_i32)code);return -1;}return 1;}
static void preserve_extra(sch_connection*c,SecBuffer*buffers,unsigned long count){unsigned long i,extra=0;for(i=0;i<count;i++)if(buffers[i].BufferType==SECBUFFER_EXTRA)extra=buffers[i].cbBuffer;if(extra&&extra<=c->encrypted_input_size)memmove(c->encrypted_input,c->encrypted_input+c->encrypted_input_size-extra,extra);c->encrypted_input_size=extra;}
static PST_RESULT fail_operation(sch_connection*c,pst_u32 phase,SECURITY_STATUS status,pst_u32*op,PST_RESULT*error){*error=map_security(status);*op=PST_BACKEND_OPERATION_FAILED;c->failed=1;c->interest=PST_BACKEND_INTEREST_NONE;capture((sch_base*)c,*error,phase,(pst_i32)status);return PST_RESULT_OK;}
static PST_RESULT query_established(sch_connection*c,pst_u32*op,PST_RESULT*error){SecPkgContext_ConnectionInfo info;SECURITY_STATUS status;status=QueryContextAttributesA(&c->context,SECPKG_ATTR_STREAM_SIZES,&c->stream_sizes);if(status!=SEC_E_OK)return fail_operation(c,PST_DIAGNOSTIC_PHASE_HANDSHAKE,status,op,error);memset(&info,0,sizeof(info));status=QueryContextAttributesA(&c->context,SECPKG_ATTR_CONNECTION_INFO,&info);if(status!=SEC_E_OK)return fail_operation(c,PST_DIAGNOSTIC_PHASE_HANDSHAKE,status,op,error);if(info.dwProtocol==SP_PROT_TLS1_2_CLIENT)c->negotiated_version=PST_TLS_VERSION_1_2;
#ifdef SP_PROT_TLS1_3_CLIENT
    else if(info.dwProtocol==SP_PROT_TLS1_3_CLIENT)c->negotiated_version=PST_TLS_VERSION_1_3;
#endif
    else return fail_operation(c,PST_DIAGNOSTIC_PHASE_HANDSHAKE,SEC_E_ALGORITHM_MISMATCH,op,error);c->negotiated_cipher=info.aiCipher;c->handshake_complete=1;c->interest=PST_BACKEND_INTEREST_READ;*op=PST_BACKEND_OPERATION_COMPLETE;*error=PST_RESULT_OK;return PST_RESULT_OK;}
static PST_RESULT sch_handshake(void *v,pst_u32 *op,PST_RESULT *error){sch_connection*c=(sch_connection*)v;SecBuffer in_buffers[2],out_buffer;SecBufferDesc in_desc,out_desc;ULONG attrs=0;TimeStamp expiry;SECURITY_STATUS status;int rr;if(!c||!op||!error||!c->credentials_valid||!c->owns_socket)return PST_RESULT_INVALID_ARGUMENT;*error=PST_RESULT_OK;if(c->failed)return fail_operation(c,PST_DIAGNOSTIC_PHASE_HANDSHAKE,SEC_E_INTERNAL_ERROR,op,error);if(c->output){rr=drain_output(c,PST_DIAGNOSTIC_PHASE_HANDSHAKE);if(rr<0){*op=PST_BACKEND_OPERATION_FAILED;*error=PST_RESULT_TRANSPORT_FAILURE;c->failed=1;return PST_RESULT_OK;}if(rr==0){*op=PST_BACKEND_OPERATION_NEED_WRITE;return PST_RESULT_OK;}output_clear(c);if(c->handshake_complete_pending){c->handshake_complete_pending=0;return query_established(c,op,error);}}
    if(!c->first_handshake&&(c->encrypted_input_size==0||c->need_more_input)){rr=recv_input(c,PST_DIAGNOSTIC_PHASE_HANDSHAKE);if(rr==0){c->interest=PST_BACKEND_INTEREST_READ;*op=PST_BACKEND_OPERATION_NEED_READ;return PST_RESULT_OK;}if(rr==-2)return fail_operation(c,PST_DIAGNOSTIC_PHASE_HANDSHAKE,SEC_E_ILLEGAL_MESSAGE,op,error);if(rr<0){*op=PST_BACKEND_OPERATION_FAILED;*error=PST_RESULT_TRANSPORT_FAILURE;c->failed=1;return PST_RESULT_OK;}c->need_more_input=0;}
    memset(&out_buffer,0,sizeof(out_buffer));out_buffer.BufferType=SECBUFFER_TOKEN;out_desc.ulVersion=SECBUFFER_VERSION;out_desc.cBuffers=1;out_desc.pBuffers=&out_buffer;
    if(c->first_handshake){status=InitializeSecurityContextA(&c->credentials,NULL,c->server_name,SCH_CONTEXT_FLAGS,0,SECURITY_NATIVE_DREP,NULL,0,&c->context,&out_desc,&attrs,&expiry);if(status>=0)c->context_valid=1;c->first_handshake=0;}else{memset(in_buffers,0,sizeof(in_buffers));in_buffers[0].BufferType=SECBUFFER_TOKEN;in_buffers[0].pvBuffer=c->encrypted_input;in_buffers[0].cbBuffer=c->encrypted_input_size;in_buffers[1].BufferType=SECBUFFER_EMPTY;in_desc.ulVersion=SECBUFFER_VERSION;in_desc.cBuffers=2;in_desc.pBuffers=in_buffers;status=InitializeSecurityContextA(&c->credentials,&c->context,c->server_name,SCH_CONTEXT_FLAGS,0,SECURITY_NATIVE_DREP,&in_desc,0,&c->context,&out_desc,&attrs,&expiry);if(status!=SEC_E_INCOMPLETE_MESSAGE)preserve_extra(c,in_buffers,2);}
    if(status==SEC_I_COMPLETE_NEEDED||status==SEC_I_COMPLETE_AND_CONTINUE){SECURITY_STATUS complete_status=CompleteAuthToken(&c->context,&out_desc);if(complete_status!=SEC_E_OK){if(out_buffer.pvBuffer)FreeContextBuffer(out_buffer.pvBuffer);return fail_operation(c,PST_DIAGNOSTIC_PHASE_HANDSHAKE,complete_status,op,error);}}
    if(out_buffer.pvBuffer&&out_buffer.cbBuffer){c->output=(unsigned char*)out_buffer.pvBuffer;c->output_size=out_buffer.cbBuffer;c->output_kind=SCH_OUT_HANDSHAKE;c->output_sspi_owned=1;}
    if(status==SEC_E_INCOMPLETE_MESSAGE){c->need_more_input=1;c->interest=PST_BACKEND_INTEREST_READ;*op=PST_BACKEND_OPERATION_NEED_READ;return PST_RESULT_OK;}
    if(status==SEC_E_OK){if(c->output){c->handshake_complete_pending=1;c->interest=PST_BACKEND_INTEREST_WRITE;*op=PST_BACKEND_OPERATION_NEED_WRITE;return PST_RESULT_OK;}return query_established(c,op,error);}
    if(status==SEC_I_CONTINUE_NEEDED||status==SEC_I_COMPLETE_AND_CONTINUE){if(c->output){c->interest=PST_BACKEND_INTEREST_WRITE;*op=PST_BACKEND_OPERATION_NEED_WRITE;}else{c->need_more_input=c->encrypted_input_size==0;c->interest=PST_BACKEND_INTEREST_READ;*op=PST_BACKEND_OPERATION_NEED_READ;}return PST_RESULT_OK;}
    if(out_buffer.pvBuffer&&!c->output)FreeContextBuffer(out_buffer.pvBuffer);return fail_operation(c,PST_DIAGNOSTIC_PHASE_HANDSHAKE,status,op,error);}
static PST_RESULT sch_interest(void *v,pst_u32 *interest){sch_connection*c=(sch_connection*)v;if(!c||!interest)return PST_RESULT_INVALID_ARGUMENT;*interest=c->interest;return PST_RESULT_OK;}
static PST_RESULT sch_wait(void *v,pst_u32 interest,pst_u32 timeout_ms,PST_BACKEND_WAIT_RESULT*result){sch_connection*c=(sch_connection*)v;fd_set reads,writes,errors;struct timeval timeout;int ready,code;if(!c||!result||!c->owns_socket)return PST_RESULT_INVALID_ARGUMENT;memset(result,0,sizeof(*result));FD_ZERO(&reads);FD_ZERO(&writes);FD_ZERO(&errors);if(interest&PST_BACKEND_INTEREST_READ)FD_SET(c->socket_value,&reads);if(interest&PST_BACKEND_INTEREST_WRITE)FD_SET(c->socket_value,&writes);FD_SET(c->socket_value,&errors);timeout.tv_sec=(long)(timeout_ms/1000UL);timeout.tv_usec=(long)((timeout_ms%1000UL)*1000UL);ready=select(0,&reads,&writes,&errors,&timeout);if(ready==SOCKET_ERROR){code=WSAGetLastError();capture((sch_base*)c,PST_RESULT_TRANSPORT_FAILURE,PST_DIAGNOSTIC_PHASE_WAIT,(pst_i32)code);c->failed=1;return PST_RESULT_TRANSPORT_FAILURE;}if(ready==0){result->timed_out=1;return PST_RESULT_OK;}if(FD_ISSET(c->socket_value,&errors)){capture((sch_base*)c,PST_RESULT_TRANSPORT_FAILURE,PST_DIAGNOSTIC_PHASE_WAIT,(pst_i32)WSAECONNABORTED);c->failed=1;return PST_RESULT_TRANSPORT_FAILURE;}if(FD_ISSET(c->socket_value,&reads))result->ready_interest|=PST_BACKEND_INTEREST_READ;if(FD_ISSET(c->socket_value,&writes))result->ready_interest|=PST_BACKEND_INTEREST_WRITE;return PST_RESULT_OK;}
static void io_reset(PST_BACKEND_IO_RESULT*r){memset(r,0,sizeof(*r));r->operation=PST_BACKEND_OPERATION_FAILED;r->error=PST_RESULT_OK;}
static PST_RESULT sch_write(void *v,const void *buffer,pst_size length,PST_BACKEND_IO_RESULT*r){sch_connection*c=(sch_connection*)v;SecBuffer buffers[4];SecBufferDesc desc;unsigned long amount,total;SECURITY_STATUS status;int drained;if(!c||!r||(!buffer&&length))return PST_RESULT_INVALID_ARGUMENT;io_reset(r);if(!c->handshake_complete||c->failed)return PST_RESULT_INVALID_STATE;if(c->output){drained=drain_output(c,PST_DIAGNOSTIC_PHASE_WRITE);if(drained<0){r->operation=PST_BACKEND_OPERATION_FAILED;r->error=PST_RESULT_TRANSPORT_FAILURE;c->failed=1;return PST_RESULT_OK;}if(drained==0){r->operation=PST_BACKEND_OPERATION_NEED_WRITE;return PST_RESULT_OK;}r->bytes_transferred=c->output_application_size;output_clear(c);r->operation=PST_BACKEND_OPERATION_COMPLETE;c->interest=PST_BACKEND_INTEREST_READ;return PST_RESULT_OK;}if(!length){r->operation=PST_BACKEND_OPERATION_COMPLETE;return PST_RESULT_OK;}amount=(unsigned long)(length<c->stream_sizes.cbMaximumMessage?length:c->stream_sizes.cbMaximumMessage);total=c->stream_sizes.cbHeader+amount+c->stream_sizes.cbTrailer;c->output=(unsigned char*)malloc(total);if(!c->output)return PST_RESULT_OUT_OF_MEMORY;c->output_size=total;c->output_kind=SCH_OUT_APPLICATION;c->output_application_size=amount;memset(buffers,0,sizeof(buffers));buffers[0].BufferType=SECBUFFER_STREAM_HEADER;buffers[0].pvBuffer=c->output;buffers[0].cbBuffer=c->stream_sizes.cbHeader;buffers[1].BufferType=SECBUFFER_DATA;buffers[1].pvBuffer=c->output+c->stream_sizes.cbHeader;buffers[1].cbBuffer=amount;memcpy(buffers[1].pvBuffer,buffer,amount);buffers[2].BufferType=SECBUFFER_STREAM_TRAILER;buffers[2].pvBuffer=c->output+c->stream_sizes.cbHeader+amount;buffers[2].cbBuffer=c->stream_sizes.cbTrailer;buffers[3].BufferType=SECBUFFER_EMPTY;desc.ulVersion=SECBUFFER_VERSION;desc.cBuffers=4;desc.pBuffers=buffers;status=EncryptMessage(&c->context,0,&desc,0);if(status!=SEC_E_OK){output_clear(c);r->operation=PST_BACKEND_OPERATION_FAILED;r->error=map_security(status);capture((sch_base*)c,r->error,PST_DIAGNOSTIC_PHASE_WRITE,(pst_i32)status);c->failed=1;return PST_RESULT_OK;}c->output_size=buffers[0].cbBuffer+buffers[1].cbBuffer+buffers[2].cbBuffer;drained=drain_output(c,PST_DIAGNOSTIC_PHASE_WRITE);if(drained<0){r->operation=PST_BACKEND_OPERATION_FAILED;r->error=PST_RESULT_TRANSPORT_FAILURE;c->failed=1;return PST_RESULT_OK;}if(drained==0){r->operation=PST_BACKEND_OPERATION_NEED_WRITE;return PST_RESULT_OK;}r->bytes_transferred=amount;output_clear(c);r->operation=PST_BACKEND_OPERATION_COMPLETE;c->interest=PST_BACKEND_INTEREST_READ;return PST_RESULT_OK;}
static PST_RESULT deliver_plaintext(sch_connection*c,void*buffer,pst_size capacity,PST_BACKEND_IO_RESULT*r){pst_size available,take;available=c->plaintext_size-c->plaintext_offset;take=available<capacity?available:capacity;if(take)memcpy(buffer,c->plaintext+c->plaintext_offset,take);c->plaintext_offset+=take;r->bytes_transferred=take;r->operation=PST_BACKEND_OPERATION_COMPLETE;if(c->plaintext_offset==c->plaintext_size){free(c->plaintext);c->plaintext=NULL;c->plaintext_size=c->plaintext_offset=0;}c->interest=PST_BACKEND_INTEREST_READ;return PST_RESULT_OK;}
static PST_RESULT sch_read(void *v,void *buffer,pst_size capacity,PST_BACKEND_IO_RESULT*r){sch_connection*c=(sch_connection*)v;SecBuffer buffers[4];SecBufferDesc desc;SECURITY_STATUS status;unsigned long i,plain=0;unsigned char*plain_ptr=NULL;int rr;if(!c||!r||(!buffer&&capacity))return PST_RESULT_INVALID_ARGUMENT;io_reset(r);if(!c->handshake_complete||c->failed)return PST_RESULT_INVALID_STATE;if(c->plaintext)return deliver_plaintext(c,buffer,capacity,r);if(c->encrypted_input_size==0||c->need_more_input){rr=recv_input(c,PST_DIAGNOSTIC_PHASE_READ);if(rr==0){r->operation=PST_BACKEND_OPERATION_NEED_READ;c->interest=PST_BACKEND_INTEREST_READ;return PST_RESULT_OK;}if(rr==-2){r->operation=PST_BACKEND_OPERATION_FAILED;r->close_kind=PST_BACKEND_CLOSE_TRUNCATED;r->error=PST_RESULT_TRUNCATED;c->failed=1;capture((sch_base*)c,PST_RESULT_TRUNCATED,PST_DIAGNOSTIC_PHASE_READ,0);return PST_RESULT_OK;}if(rr<0){r->operation=PST_BACKEND_OPERATION_FAILED;r->error=PST_RESULT_TRANSPORT_FAILURE;c->failed=1;return PST_RESULT_OK;}c->need_more_input=0;}memset(buffers,0,sizeof(buffers));buffers[0].BufferType=SECBUFFER_DATA;buffers[0].pvBuffer=c->encrypted_input;buffers[0].cbBuffer=c->encrypted_input_size;for(i=1;i<4;i++)buffers[i].BufferType=SECBUFFER_EMPTY;desc.ulVersion=SECBUFFER_VERSION;desc.cBuffers=4;desc.pBuffers=buffers;status=DecryptMessage(&c->context,&desc,0,NULL);if(status==SEC_E_INCOMPLETE_MESSAGE){c->need_more_input=1;r->operation=PST_BACKEND_OPERATION_NEED_READ;c->interest=PST_BACKEND_INTEREST_READ;return PST_RESULT_OK;}if(status==SEC_I_CONTEXT_EXPIRED){c->clean_close=1;c->encrypted_input_size=0;r->operation=PST_BACKEND_OPERATION_CLOSED;r->close_kind=PST_BACKEND_CLOSE_CLEAN;r->error=PST_RESULT_OK;c->interest=PST_BACKEND_INTEREST_NONE;return PST_RESULT_OK;}if(status!=SEC_E_OK){r->operation=PST_BACKEND_OPERATION_FAILED;r->error=map_security(status);capture((sch_base*)c,r->error,PST_DIAGNOSTIC_PHASE_READ,(pst_i32)status);c->failed=1;return PST_RESULT_OK;}for(i=0;i<4;i++)if(buffers[i].BufferType==SECBUFFER_DATA){plain=buffers[i].cbBuffer;plain_ptr=(unsigned char*)buffers[i].pvBuffer;}if(plain){c->plaintext=(unsigned char*)malloc(plain);if(!c->plaintext)return PST_RESULT_OUT_OF_MEMORY;memcpy(c->plaintext,plain_ptr,plain);c->plaintext_size=plain;c->plaintext_offset=0;}preserve_extra(c,buffers,4);if(c->plaintext)return deliver_plaintext(c,buffer,capacity,r);r->operation=PST_BACKEND_OPERATION_NEED_READ;c->interest=PST_BACKEND_INTEREST_READ;return PST_RESULT_OK;}
static PST_RESULT sch_close(void *v,pst_u32 *op,PST_RESULT *error){sch_connection*c=(sch_connection*)v;SecBuffer token,out_buffer;SecBufferDesc token_desc,out_desc;DWORD shutdown_token=SCHANNEL_SHUTDOWN;ULONG attrs=0;TimeStamp expiry;SECURITY_STATUS status;int drained;if(!c||!op||!error||!c->handshake_complete)return PST_RESULT_INVALID_ARGUMENT;*error=PST_RESULT_OK;if(c->output){drained=drain_output(c,PST_DIAGNOSTIC_PHASE_SHUTDOWN);if(drained<0){*op=PST_BACKEND_OPERATION_FAILED;*error=PST_RESULT_TRANSPORT_FAILURE;c->failed=1;return PST_RESULT_OK;}if(drained==0){*op=PST_BACKEND_OPERATION_NEED_WRITE;return PST_RESULT_OK;}output_clear(c);if(c->shutdown_complete_pending){c->shutdown_complete_pending=0;*op=PST_BACKEND_OPERATION_COMPLETE;c->interest=PST_BACKEND_INTEREST_NONE;return PST_RESULT_OK;}}if(c->shutdown_started){*op=PST_BACKEND_OPERATION_COMPLETE;return PST_RESULT_OK;}memset(&token,0,sizeof(token));token.BufferType=SECBUFFER_TOKEN;token.pvBuffer=&shutdown_token;token.cbBuffer=sizeof(shutdown_token);token_desc.ulVersion=SECBUFFER_VERSION;token_desc.cBuffers=1;token_desc.pBuffers=&token;status=ApplyControlToken(&c->context,&token_desc);if(status!=SEC_E_OK)return fail_operation(c,PST_DIAGNOSTIC_PHASE_SHUTDOWN,status,op,error);memset(&out_buffer,0,sizeof(out_buffer));out_buffer.BufferType=SECBUFFER_TOKEN;out_desc.ulVersion=SECBUFFER_VERSION;out_desc.cBuffers=1;out_desc.pBuffers=&out_buffer;status=InitializeSecurityContextA(&c->credentials,&c->context,c->server_name,SCH_CONTEXT_FLAGS,0,SECURITY_NATIVE_DREP,NULL,0,&c->context,&out_desc,&attrs,&expiry);if(status!=SEC_E_OK&&status!=SEC_I_CONTEXT_EXPIRED){if(out_buffer.pvBuffer)FreeContextBuffer(out_buffer.pvBuffer);return fail_operation(c,PST_DIAGNOSTIC_PHASE_SHUTDOWN,status,op,error);}c->shutdown_started=1;if(out_buffer.pvBuffer&&out_buffer.cbBuffer){c->output=(unsigned char*)out_buffer.pvBuffer;c->output_size=out_buffer.cbBuffer;c->output_kind=SCH_OUT_SHUTDOWN;c->output_sspi_owned=1;c->shutdown_complete_pending=1;c->interest=PST_BACKEND_INTEREST_WRITE;*op=PST_BACKEND_OPERATION_NEED_WRITE;return PST_RESULT_OK;}if(out_buffer.pvBuffer)FreeContextBuffer(out_buffer.pvBuffer);*op=PST_BACKEND_OPERATION_COMPLETE;c->interest=PST_BACKEND_INTEREST_NONE;return PST_RESULT_OK;}
static void sch_diag(const void *v,pst_internal_diagnostic *out){if(!out)return;if(v)pst_diagnostic_copy(out,&((const sch_base*)v)->diagnostic);else pst_diagnostic_initialize(out);}
static const PST_BACKEND_VTABLE sch_vtable={sizeof(PST_BACKEND_VTABLE),PST_BACKEND_SPI_VERSION,sch_initialize,sch_shutdown,sch_runtime_create,sch_runtime_destroy,sch_query,sch_validate,sch_connection_create,sch_connection_destroy,sch_attach,sch_handshake,sch_interest,sch_wait,sch_read,sch_write,sch_close,NULL,NULL,sch_configure,NULL,sch_diag};
static const PST_BACKEND_METADATA sch_metadata={sizeof(PST_BACKEND_METADATA),PST_BACKEND_METADATA_VERSION,{PST_BACKEND_VERSION_AVAILABLE,1UL,1UL,0UL,"pst-schannel","tls-io"},1UL,{{0UL,0UL,0UL,0UL,"Schannel-SSPI","OS-provided"},{0}}};
static const PST_BACKEND_DESCRIPTOR sch_descriptor={sizeof(PST_BACKEND_DESCRIPTOR),PST_BACKEND_SPI_VERSION,"schannel","Windows Schannel/SSPI",PST_BACKEND_CAP_TLS_1_2|PST_BACKEND_CAP_SYSTEM_TRUST|PST_BACKEND_CAP_HOSTNAME_VERIFY|PST_BACKEND_CAP_NONBLOCKING|PST_BACKEND_CAP_BACKEND_WAIT,&sch_vtable,&sch_metadata};
const PST_BACKEND_DESCRIPTOR *pst_backend_schannel_descriptor(void){return &sch_descriptor;}
PST_RESULT pst_backend_schannel_register(void){return pst_backend_register(&sch_descriptor);}