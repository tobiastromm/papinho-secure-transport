/* SPDX-License-Identifier: MPL-2.0 */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/provider.h>
#include <openssl/ssl.h>
#include <openssl/sslerr.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/evp.h>
#include "backends/openssl/pst_backend_openssl.h"
#include "backends/openssl/platform/win32/pst_openssl_system_trust_win32.h"
#include "pst_identity_internal.h"
#include "pst_transport_internal.h"
#include <limits.h>
#define OSSL_CLIENT_CAPABILITIES (PST_BACKEND_CAP_TLS_1_2|PST_BACKEND_CAP_TLS_1_3|PST_BACKEND_CAP_ROLE_CLIENT|PST_BACKEND_CAP_LOCAL_IDENTITY|PST_BACKEND_CAP_PEER_CERT_AUTH|PST_BACKEND_CAP_CUSTOM_TRUST|PST_BACKEND_CAP_SYSTEM_TRUST|PST_BACKEND_CAP_PEER_NAME_VERIFY|PST_BACKEND_CAP_ALPN_CLIENT|PST_BACKEND_CAP_PEER_INFO|PST_BACKEND_CAP_NONBLOCKING|PST_BACKEND_CAP_BACKEND_WAIT|PST_BACKEND_CAP_SNI_CONTROL)
#define OSSL_SERVER_CAPABILITIES (PST_BACKEND_CAP_TLS_1_2|PST_BACKEND_CAP_TLS_1_3|PST_BACKEND_CAP_ROLE_SERVER|PST_BACKEND_CAP_LOCAL_IDENTITY|PST_BACKEND_CAP_PEER_CERT_AUTH|PST_BACKEND_CAP_PEER_CERT_OPTIONAL|PST_BACKEND_CAP_CUSTOM_TRUST|PST_BACKEND_CAP_SYSTEM_TRUST|PST_BACKEND_CAP_ALPN_SERVER|PST_BACKEND_CAP_PEER_INFO|PST_BACKEND_CAP_NONBLOCKING|PST_BACKEND_CAP_BACKEND_WAIT)
#define OSSL_CAPABILITIES (OSSL_CLIENT_CAPABILITIES|OSSL_SERVER_CAPABILITIES)
#include <stdlib.h>
#include <string.h>

typedef struct ossl_base { pst_internal_diagnostic diagnostic; } ossl_base;
typedef struct ossl_backend { pst_internal_diagnostic diagnostic; } ossl_backend;
typedef struct ossl_runtime { pst_internal_diagnostic diagnostic; OSSL_LIB_CTX *library_context; OSSL_PROVIDER *default_provider; } ossl_runtime;
typedef struct ossl_connection {
    pst_internal_diagnostic diagnostic;
    ossl_runtime *runtime;
    SSL_CTX *ssl_context;
    SSL *ssl;
    SOCKET socket_value;
    int owns_socket;
    int configured;
    int established;
    int failed;
    int shutdown_started;
    pst_u32 interest;
    pst_u32 negotiated_version;
    pst_u32 cipher_suite;
    pst_u8 *alpn_wire;
    pst_size alpn_wire_size;
    pst_u8 negotiated_alpn[255];
    pst_size negotiated_alpn_size;
    pst_u32 alpn_requirement;
    pst_u32 trust_kind;
    PST_RESULT system_trust_result;
    pst_u32 system_trust_reason;
    char *hostname;
    unsigned long private_errors[4];
    pst_u32 private_error_count;
    pst_u32 role;
    pst_u32 peer_certificate_mode;
    int peer_certificate_absent;
    int tls_policy_mismatch;
} ossl_connection;

static void ossl_clear_errors(void){while(ERR_get_error()!=0UL){} }
static void ossl_collect_errors(ossl_connection *c,int *unexpected_eof,int *auth_alert){unsigned long code,reason;c->private_error_count=0;if(unexpected_eof)*unexpected_eof=0;if(auth_alert)*auth_alert=0;while((code=ERR_get_error())!=0UL){if(c->private_error_count<4UL)c->private_errors[c->private_error_count++]=code;reason=ERR_GET_REASON(code);
#ifdef SSL_R_UNEXPECTED_EOF_WHILE_READING
if(unexpected_eof&&ERR_GET_LIB(code)==ERR_LIB_SSL&&ERR_GET_REASON(code)==SSL_R_UNEXPECTED_EOF_WHILE_READING)*unexpected_eof=1;
#endif
#ifdef SSL_R_TLSV13_ALERT_CERTIFICATE_REQUIRED
if(auth_alert&&ERR_GET_LIB(code)==ERR_LIB_SSL&&reason==SSL_R_TLSV13_ALERT_CERTIFICATE_REQUIRED)*auth_alert=1;
#endif
#ifdef SSL_R_PEER_DID_NOT_RETURN_A_CERTIFICATE
if(auth_alert&&ERR_GET_LIB(code)==ERR_LIB_SSL&&reason==SSL_R_PEER_DID_NOT_RETURN_A_CERTIFICATE){*auth_alert=1;c->peer_certificate_absent=1;}
#endif
#ifdef SSL_R_TLSV1_ALERT_UNKNOWN_CA
if(auth_alert&&ERR_GET_LIB(code)==ERR_LIB_SSL&&reason==SSL_R_TLSV1_ALERT_UNKNOWN_CA)*auth_alert=1;
#endif
#ifdef SSL_R_SSLV3_ALERT_BAD_CERTIFICATE
if(auth_alert&&ERR_GET_LIB(code)==ERR_LIB_SSL&&reason==SSL_R_SSLV3_ALERT_BAD_CERTIFICATE)*auth_alert=1;
#endif
#ifdef SSL_R_SSLV3_ALERT_HANDSHAKE_FAILURE
if(auth_alert&&ERR_GET_LIB(code)==ERR_LIB_SSL&&reason==SSL_R_SSLV3_ALERT_HANDSHAKE_FAILURE)*auth_alert=1;
#endif
#ifdef SSL_R_UNSUPPORTED_PROTOCOL
if(ERR_GET_LIB(code)==ERR_LIB_SSL&&reason==SSL_R_UNSUPPORTED_PROTOCOL)c->tls_policy_mismatch=1;
#endif
#ifdef SSL_R_VERSION_TOO_LOW
if(ERR_GET_LIB(code)==ERR_LIB_SSL&&reason==SSL_R_VERSION_TOO_LOW)c->tls_policy_mismatch=1;
#endif
#ifdef SSL_R_VERSION_TOO_HIGH
if(ERR_GET_LIB(code)==ERR_LIB_SSL&&reason==SSL_R_VERSION_TOO_HIGH)c->tls_policy_mismatch=1;
#endif
}}
static void ossl_capture(ossl_base *s,PST_RESULT result,pst_u32 phase){pst_diagnostic_capture(&s->diagnostic,result,phase,"openssl",PST_DIAGNOSTIC_DOMAIN_NONE,0,0,0);}
static PST_RESULT ossl_initialize(void **out){ossl_backend *s;if(!out)return PST_RESULT_INVALID_ARGUMENT;*out=NULL;ossl_clear_errors();if(OPENSSL_version_major()!=3||OPENSSL_version_minor()!=5||OPENSSL_version_patch()!=8)return PST_RESULT_INCOMPATIBLE_API;if(!OPENSSL_init_ssl(OPENSSL_INIT_NO_LOAD_CONFIG,NULL)){ossl_clear_errors();return PST_RESULT_BACKEND_FAILURE;}s=(ossl_backend*)calloc(1,sizeof(*s));if(!s){ossl_clear_errors();return PST_RESULT_OUT_OF_MEMORY;}pst_diagnostic_initialize(&s->diagnostic);ossl_clear_errors();*out=s;return PST_RESULT_OK;}
static void ossl_shutdown(void *v){ossl_clear_errors();free(v);}
static PST_RESULT ossl_runtime_create(void *backend,void **out){ossl_runtime *r;if(!backend||!out)return PST_RESULT_INVALID_ARGUMENT;*out=NULL;ossl_clear_errors();r=(ossl_runtime*)calloc(1,sizeof(*r));if(!r)return PST_RESULT_OUT_OF_MEMORY;pst_diagnostic_initialize(&r->diagnostic);r->library_context=OSSL_LIB_CTX_new();if(!r->library_context){ossl_capture((ossl_base*)r,PST_RESULT_BACKEND_FAILURE,PST_DIAGNOSTIC_PHASE_RUNTIME_CREATE);ossl_clear_errors();free(r);return PST_RESULT_BACKEND_FAILURE;}r->default_provider=OSSL_PROVIDER_load(r->library_context,"default");if(!r->default_provider){ossl_capture((ossl_base*)r,PST_RESULT_BACKEND_FAILURE,PST_DIAGNOSTIC_PHASE_RUNTIME_CREATE);ossl_clear_errors();OSSL_LIB_CTX_free(r->library_context);free(r);return PST_RESULT_BACKEND_FAILURE;}ossl_clear_errors();*out=r;return PST_RESULT_OK;}
static void ossl_runtime_destroy(void *v){ossl_runtime *r=(ossl_runtime*)v;if(!r)return;ossl_clear_errors();if(r->default_provider)OSSL_PROVIDER_unload(r->default_provider);OSSL_LIB_CTX_free(r->library_context);ossl_clear_errors();free(r);}
static PST_RESULT ossl_query(void *v,pst_u32 *caps){if(!v||!caps)return PST_RESULT_INVALID_ARGUMENT;*caps=OSSL_CAPABILITIES;return PST_RESULT_OK;}
static PST_RESULT ossl_validate(void *v,pst_u32 required){pst_u32 caps=OSSL_CAPABILITIES;return v&&!(required&~caps)?PST_RESULT_OK:PST_RESULT_UNSUPPORTED;}
static PST_RESULT ossl_configure(void *,const PST_CONNECTION_CONFIG *);
static void ossl_connection_destroy(void *);
static PST_RESULT ossl_connection_create(void *v,const PST_BACKEND_CONNECTION_OPTIONS *options,void **out){ossl_connection *c;PST_RESULT result;if(!v||!options||!out||options->struct_size<PST_BACKEND_CONNECTION_OPTIONS_MIN_SIZE||options->spi_version!=PST_BACKEND_SPI_VERSION)return PST_RESULT_INVALID_ARGUMENT;if(options->role!=PST_CONNECTION_ROLE_CLIENT)return PST_RESULT_UNSUPPORTED;*out=NULL;c=(ossl_connection*)calloc(1,sizeof(*c));if(!c)return PST_RESULT_OUT_OF_MEMORY;pst_diagnostic_initialize(&c->diagnostic);c->runtime=(ossl_runtime*)v;c->socket_value=INVALID_SOCKET;c->interest=PST_BACKEND_INTEREST_NONE;result=ossl_configure(c,options->configuration);if(result!=PST_RESULT_OK){ossl_connection_destroy(c);return result;}*out=c;return PST_RESULT_OK;}
static void ossl_connection_destroy(void *v){ossl_connection *c=(ossl_connection*)v;if(!c)return;ossl_clear_errors();if(c->ssl)SSL_free(c->ssl);if(c->ssl_context)SSL_CTX_free(c->ssl_context);free(c->alpn_wire);free(c->hostname);if(c->owns_socket&&c->socket_value!=INVALID_SOCKET)closesocket(c->socket_value);ossl_clear_errors();memset(c->private_errors,0,sizeof(c->private_errors));free(c);}
static int ossl_system_verify(X509_STORE_CTX *context,void *argument){ossl_connection*c=(ossl_connection*)argument;X509*leaf;STACK_OF(X509)*chain;pst_ossl_windows_certificate*items=NULL;unsigned char**owned=NULL;int count=0,used=0,i,n,host_result;pst_u32 trust_result=PST_OSSL_WINDOWS_TRUST_INTERNAL_FAILURE;if(!c||!context)return 0;c->system_trust_result=PST_RESULT_BACKEND_FAILURE;leaf=X509_STORE_CTX_get0_cert(context);if(!leaf){X509_STORE_CTX_set_error(context,X509_V_ERR_CERT_REJECTED);c->system_trust_result=PST_RESULT_AUTH_FAILURE;return 0;}chain=X509_STORE_CTX_get0_untrusted(context);if(chain)count=sk_X509_num(chain);if(count>0){items=(pst_ossl_windows_certificate*)calloc((size_t)count,sizeof(*items));owned=(unsigned char**)calloc((size_t)count,sizeof(*owned));if(!items||!owned)goto done;for(i=0;i<count;i++){X509*cert=sk_X509_value(chain,i);unsigned char*cursor;if(!cert||X509_cmp(cert,leaf)==0)continue;n=i2d_X509(cert,NULL);if(n<=0)goto done;owned[used]=(unsigned char*)malloc((size_t)n);if(!owned[used])goto done;cursor=owned[used];if(i2d_X509(cert,&cursor)!=n)goto done;items[used].der=owned[used];items[used].der_size=(pst_size)n;used++;}}
{unsigned char*leaf_der=NULL,*cursor;int leaf_size=i2d_X509(leaf,NULL);if(leaf_size<=0)goto done;leaf_der=(unsigned char*)malloc((size_t)leaf_size);if(!leaf_der)goto done;cursor=leaf_der;if(i2d_X509(leaf,&cursor)!=leaf_size){free(leaf_der);goto done;}trust_result=pst_ossl_windows_system_trust_evaluate(c->role,leaf_der,(pst_size)leaf_size,items,(pst_size)used,NULL);free(leaf_der);}
if(trust_result==PST_OSSL_WINDOWS_TRUST_CERTIFICATE_UNTRUSTED||trust_result==PST_OSSL_WINDOWS_TRUST_CERTIFICATE_INVALID){c->system_trust_result=PST_RESULT_AUTH_FAILURE;c->system_trust_reason=trust_result==PST_OSSL_WINDOWS_TRUST_CERTIFICATE_UNTRUSTED?PST_DIAGNOSTIC_REASON_PEER_CERT_UNTRUSTED:PST_DIAGNOSTIC_REASON_PEER_CERT_INVALID;goto done;}if(trust_result!=PST_OSSL_WINDOWS_TRUST_OK)goto done;if(c->role==PST_CONNECTION_ROLE_CLIENT){host_result=X509_check_host(leaf,c->hostname,0,0,NULL);if(host_result!=1){c->system_trust_result=host_result==0?PST_RESULT_PEER_NAME_MISMATCH:PST_RESULT_BACKEND_FAILURE;goto done;}}c->system_trust_result=PST_RESULT_OK;X509_STORE_CTX_set_error(context,X509_V_OK);
done:if(owned){for(i=0;i<count;i++)free(owned[i]);}free(owned);free(items);if(c->system_trust_result!=PST_RESULT_OK){X509_STORE_CTX_set_error(context,X509_V_ERR_CERT_REJECTED);return 0;}return 1;}
static PST_RESULT ossl_configure(void *v,const PST_CONNECTION_CONFIG *config){ossl_connection *c=(ossl_connection*)v;const pst_trust *trust;const pst_credentials *credentials;const pst_u8 *der,*cert_der,*key_der,*alpn;const unsigned char *cursor;pst_size der_size,cert_size,key_size,alpn_size;pst_u32 minimum,maximum;int min_proto,max_proto;X509 *anchor=NULL,*client_cert=NULL;EVP_PKEY *client_key=NULL;X509_STORE *store;const char *hostname,*sni;if(!c||!config||config->role!=PST_CONNECTION_ROLE_CLIENT||c->configured)return PST_RESULT_INVALID_ARGUMENT;if(pst_connection_config_peer_certificate_mode(config)==PST_PEER_CERTIFICATE_DISABLED)return PST_RESULT_POLICY_VIOLATION;trust=pst_connection_config_peer_trust(config);if(!trust)return PST_RESULT_UNSUPPORTED;c->trust_kind=pst_trust_kind(trust);if(c->trust_kind!=PST_TRUST_SOURCE_CUSTOM_CA_DER&&c->trust_kind!=PST_TRUST_SOURCE_SYSTEM)return PST_RESULT_UNSUPPORTED;hostname=pst_connection_config_expected_peer_name(config);sni=pst_connection_config_server_name_indication(config);if(!hostname||!hostname[0])return PST_RESULT_POLICY_VIOLATION;der=NULL;der_size=0;minimum=pst_connection_config_minimum_version(config);maximum=pst_connection_config_maximum_version(config);if(minimum==PST_TLS_VERSION_1_2)min_proto=TLS1_2_VERSION;else if(minimum==PST_TLS_VERSION_1_3)min_proto=TLS1_3_VERSION;else return PST_RESULT_INVALID_ARGUMENT;if(maximum==PST_TLS_VERSION_1_2)max_proto=TLS1_2_VERSION;else if(maximum==PST_TLS_VERSION_1_3)max_proto=TLS1_3_VERSION;else return PST_RESULT_INVALID_ARGUMENT;if(maximum<minimum)return PST_RESULT_INVALID_ARGUMENT;ossl_clear_errors();c->ssl_context=SSL_CTX_new_ex(c->runtime->library_context,NULL,TLS_client_method());if(!c->ssl_context)goto backend_failure;if(!SSL_CTX_set_min_proto_version(c->ssl_context,min_proto)||!SSL_CTX_set_max_proto_version(c->ssl_context,max_proto))goto backend_failure;SSL_CTX_set_verify(c->ssl_context,SSL_VERIFY_PEER,NULL);SSL_CTX_clear_options(c->ssl_context,SSL_OP_IGNORE_UNEXPECTED_EOF);if(c->trust_kind==PST_TRUST_SOURCE_CUSTOM_CA_DER){der=pst_trust_data(trust,&der_size);if(!der||!der_size||der_size>(pst_size)LONG_MAX)return PST_RESULT_INVALID_ARGUMENT;cursor=(const unsigned char*)der;anchor=d2i_X509(NULL,&cursor,(long)der_size);if(!anchor||cursor!=(const unsigned char*)der+der_size)goto auth_failure;store=SSL_CTX_get_cert_store(c->ssl_context);if(!store||X509_STORE_add_cert(store,anchor)!=1)goto auth_failure;X509_free(anchor);anchor=NULL;}else{c->hostname=(char*)malloc(strlen(hostname)+1);if(!c->hostname)return PST_RESULT_OUT_OF_MEMORY;strcpy(c->hostname,hostname);SSL_CTX_set_cert_verify_callback(c->ssl_context,ossl_system_verify,c);}credentials=pst_connection_config_local_credentials(config);if(credentials){cert_der=pst_credentials_certificate_der(credentials,&cert_size);key_der=pst_credentials_private_key_der(credentials,&key_size);if(!cert_der||!key_der||cert_size>(pst_size)LONG_MAX||key_size>(pst_size)LONG_MAX)goto auth_failure;cursor=(const unsigned char*)cert_der;client_cert=d2i_X509(NULL,&cursor,(long)cert_size);if(!client_cert||cursor!=(const unsigned char*)cert_der+cert_size)goto auth_failure;cursor=(const unsigned char*)key_der;client_key=d2i_AutoPrivateKey(NULL,&cursor,(long)key_size);if(!client_key||cursor!=(const unsigned char*)key_der+key_size)goto auth_failure;if(SSL_CTX_use_certificate(c->ssl_context,client_cert)!=1||SSL_CTX_use_PrivateKey(c->ssl_context,client_key)!=1||SSL_CTX_check_private_key(c->ssl_context)!=1)goto auth_failure;X509_free(client_cert);client_cert=NULL;EVP_PKEY_free(client_key);client_key=NULL;}c->ssl=SSL_new(c->ssl_context);if(!c->ssl)goto backend_failure;if(SSL_set1_host(c->ssl,hostname)!=1||(sni&&sni[0]&&SSL_set_tlsext_host_name(c->ssl,sni)!=1))goto backend_failure;alpn=pst_connection_config_alpn_wire(config,&alpn_size);c->alpn_requirement=pst_connection_config_alpn_mode(config);if(alpn_size){if(alpn_size>(pst_size)UINT_MAX)return PST_RESULT_INVALID_ARGUMENT;c->alpn_wire=(pst_u8*)malloc(alpn_size);if(!c->alpn_wire)return PST_RESULT_OUT_OF_MEMORY;memcpy(c->alpn_wire,alpn,alpn_size);c->alpn_wire_size=alpn_size;if(SSL_set_alpn_protos(c->ssl,alpn,(unsigned int)alpn_size)!=0)goto backend_failure;}SSL_clear_mode(c->ssl,SSL_MODE_AUTO_RETRY|SSL_MODE_ENABLE_PARTIAL_WRITE|SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);SSL_set_connect_state(c->ssl);c->configured=1;ossl_clear_errors();return PST_RESULT_OK;
auth_failure:if(anchor)X509_free(anchor);if(client_cert)X509_free(client_cert);if(client_key)EVP_PKEY_free(client_key);ossl_clear_errors();ossl_capture((ossl_base*)c,PST_RESULT_AUTH_FAILURE,PST_DIAGNOSTIC_PHASE_IDENTITY_SETUP);return PST_RESULT_AUTH_FAILURE;
backend_failure:if(anchor)X509_free(anchor);if(client_cert)X509_free(client_cert);if(client_key)EVP_PKEY_free(client_key);ossl_clear_errors();ossl_capture((ossl_base*)c,PST_RESULT_BACKEND_FAILURE,PST_DIAGNOSTIC_PHASE_TLS_CONFIGURE);return PST_RESULT_BACKEND_FAILURE;}
static int ossl_alpn_offered(const ossl_connection *c,const unsigned char *value,pst_size size){const pst_u8 *p=c->alpn_wire,*end=p+c->alpn_wire_size;while(p<end){pst_size n=*p++;if(n>(pst_size)(end-p))return 0;if(n==size&&memcmp(p,value,size)==0)return 1;p+=n;}return 0;}static PST_RESULT ossl_attach(void *v,void *native,pst_u32 ownership,pst_u32 *accepted){ossl_connection *c=(ossl_connection*)v;PST_NATIVE_TRANSPORT *t=(PST_NATIVE_TRANSPORT*)native;u_long nonblocking=1UL;if(!accepted)return PST_RESULT_INVALID_ARGUMENT;*accepted=0;if(!c||!t||!c->configured||!c->ssl||c->owns_socket)return PST_RESULT_INVALID_ARGUMENT;if(ownership!=PST_BACKEND_OWNERSHIP_TRANSFERRED)return PST_RESULT_UNSUPPORTED;if(t->struct_size<PST_NATIVE_TRANSPORT_MIN_SIZE||t->version!=PST_NATIVE_TRANSPORT_VERSION||t->kind!=PST_NATIVE_TRANSPORT_KIND_WIN32_SOCKET)return PST_RESULT_INVALID_ARGUMENT;if(ioctlsocket((SOCKET)t->native_socket,FIONBIO,&nonblocking)!=0){ossl_capture((ossl_base*)c,PST_RESULT_TRANSPORT_FAILURE,PST_DIAGNOSTIC_PHASE_TRANSPORT_ATTACH);return PST_RESULT_TRANSPORT_FAILURE;}ossl_clear_errors();if(SSL_set_fd(c->ssl,(int)(SOCKET)t->native_socket)!=1){ossl_clear_errors();ossl_capture((ossl_base*)c,PST_RESULT_BACKEND_FAILURE,PST_DIAGNOSTIC_PHASE_TRANSPORT_ATTACH);return PST_RESULT_BACKEND_FAILURE;}ossl_clear_errors();c->socket_value=(SOCKET)t->native_socket;c->owns_socket=1;c->interest=PST_BACKEND_INTEREST_WRITE;*accepted=1;return PST_RESULT_OK;}
static PST_RESULT ossl_fail(ossl_connection *c,pst_u32 phase,PST_RESULT result,pst_u32 *operation,PST_RESULT *error){long verify;if(c->diagnostic.valid&&c->diagnostic.phase==PST_DIAGNOSTIC_PHASE_ALPN&&result==PST_RESULT_PROTOCOL_FAILURE){phase=PST_DIAGNOSTIC_PHASE_ALPN;result=PST_RESULT_POLICY_VIOLATION;}c->failed=1;c->interest=PST_BACKEND_INTEREST_NONE;*operation=PST_BACKEND_OPERATION_FAILED;*error=result;ossl_capture((ossl_base*)c,result,phase);c->diagnostic.role=c->role;if(c->role==PST_CONNECTION_ROLE_SERVER&&result==PST_RESULT_AUTH_FAILURE){verify=SSL_get_verify_result(c->ssl);if(c->peer_certificate_absent)c->diagnostic.reason=PST_DIAGNOSTIC_REASON_PEER_CERT_ABSENT;else if(c->system_trust_reason)c->diagnostic.reason=c->system_trust_reason;else if(verify!=X509_V_OK)c->diagnostic.reason=PST_DIAGNOSTIC_REASON_PEER_CERT_UNTRUSTED;else c->diagnostic.reason=PST_DIAGNOSTIC_REASON_PEER_CERT_INVALID;}else if(c->role==PST_CONNECTION_ROLE_SERVER&&c->tls_policy_mismatch)c->diagnostic.reason=PST_DIAGNOSTIC_REASON_TLS_POLICY_MISMATCH;return PST_RESULT_OK;}
static PST_RESULT ossl_want(ossl_connection *c,int ssl_error,pst_u32 *operation,PST_RESULT *error){*error=PST_RESULT_OK;if(ssl_error==SSL_ERROR_WANT_READ){c->interest=PST_BACKEND_INTEREST_READ;*operation=PST_BACKEND_OPERATION_NEED_READ;ossl_clear_errors();return PST_RESULT_OK;}if(ssl_error==SSL_ERROR_WANT_WRITE){c->interest=PST_BACKEND_INTEREST_WRITE;*operation=PST_BACKEND_OPERATION_NEED_WRITE;ossl_clear_errors();return PST_RESULT_OK;}return PST_RESULT_UNAVAILABLE;}
static PST_RESULT ossl_verify_result(long verify){
#ifdef X509_V_ERR_HOSTNAME_MISMATCH
if(verify==X509_V_ERR_HOSTNAME_MISMATCH)return PST_RESULT_PEER_NAME_MISMATCH;
#endif
return PST_RESULT_AUTH_FAILURE;}
static PST_RESULT ossl_classify_failure(ossl_connection *c,int ssl_error,int ret,pst_u32 phase,pst_u32 *operation,PST_RESULT *error,pst_u32 *close_kind){int unexpected=0,auth_alert=0;PST_RESULT result;ossl_collect_errors(c,&unexpected,&auth_alert);if(close_kind)*close_kind=PST_BACKEND_CLOSE_NONE;if(ssl_error==SSL_ERROR_ZERO_RETURN){if(close_kind)*close_kind=PST_BACKEND_CLOSE_CLEAN;c->interest=PST_BACKEND_INTEREST_NONE;*operation=PST_BACKEND_OPERATION_CLOSED;*error=PST_RESULT_OK;return PST_RESULT_OK;}if(c->system_trust_result!=PST_RESULT_OK)result=c->system_trust_result;else if(unexpected||(ssl_error==SSL_ERROR_SYSCALL&&(ret==0||(c->established&&(phase==PST_DIAGNOSTIC_PHASE_READ||phase==PST_DIAGNOSTIC_PHASE_SHUTDOWN))))){result=PST_RESULT_TRUNCATED;if(close_kind)*close_kind=PST_BACKEND_CLOSE_TRUNCATED;}else if(ssl_error==SSL_ERROR_SYSCALL)result=PST_RESULT_TRANSPORT_FAILURE;else if(auth_alert)result=PST_RESULT_AUTH_FAILURE;else if(ssl_error==SSL_ERROR_SSL&&SSL_get_verify_result(c->ssl)!=X509_V_OK)result=ossl_verify_result(SSL_get_verify_result(c->ssl));else if(ssl_error==SSL_ERROR_SSL)result=PST_RESULT_PROTOCOL_FAILURE;else result=PST_RESULT_BACKEND_FAILURE;return ossl_fail(c,phase,result,operation,error);}
static PST_RESULT ossl_handshake(void *v,pst_u32 *operation,PST_RESULT *error){ossl_connection *c=(ossl_connection*)v;const SSL_CIPHER *cipher;int ret,ssl_error,protocol;PST_RESULT mapped;if(!c||!operation||!error||!c->configured||!c->owns_socket||c->failed)return PST_RESULT_INVALID_ARGUMENT;ossl_clear_errors();ret=SSL_do_handshake(c->ssl);if(ret!=1){ssl_error=SSL_get_error(c->ssl,ret);mapped=ossl_want(c,ssl_error,operation,error);if(mapped==PST_RESULT_OK)return PST_RESULT_OK;return ossl_classify_failure(c,ssl_error,ret,PST_DIAGNOSTIC_PHASE_HANDSHAKE,operation,error,NULL);}ossl_clear_errors();if(SSL_get_verify_result(c->ssl)!=X509_V_OK)return ossl_fail(c,PST_DIAGNOSTIC_PHASE_HANDSHAKE,ossl_verify_result(SSL_get_verify_result(c->ssl)),operation,error);{const unsigned char *selected=NULL;unsigned int selected_size=0;SSL_get0_alpn_selected(c->ssl,&selected,&selected_size);if(selected_size){if(selected_size>sizeof(c->negotiated_alpn)||!ossl_alpn_offered(c,selected,selected_size))return ossl_fail(c,PST_DIAGNOSTIC_PHASE_ALPN,PST_RESULT_POLICY_VIOLATION,operation,error);memcpy(c->negotiated_alpn,selected,selected_size);c->negotiated_alpn_size=selected_size;}else if(c->alpn_requirement==PST_FEATURE_REQUIRED)return ossl_fail(c,PST_DIAGNOSTIC_PHASE_ALPN,PST_RESULT_POLICY_VIOLATION,operation,error);}protocol=SSL_version(c->ssl);if(protocol==TLS1_2_VERSION)c->negotiated_version=PST_TLS_VERSION_1_2;else if(protocol==TLS1_3_VERSION)c->negotiated_version=PST_TLS_VERSION_1_3;else return ossl_fail(c,PST_DIAGNOSTIC_PHASE_HANDSHAKE,PST_RESULT_PROTOCOL_FAILURE,operation,error);cipher=SSL_get_current_cipher(c->ssl);if(!cipher)return ossl_fail(c,PST_DIAGNOSTIC_PHASE_HANDSHAKE,PST_RESULT_PROTOCOL_FAILURE,operation,error);c->cipher_suite=(pst_u32)SSL_CIPHER_get_protocol_id(cipher);c->established=1;c->interest=PST_BACKEND_INTEREST_READ;*operation=PST_BACKEND_OPERATION_COMPLETE;*error=PST_RESULT_OK;return PST_RESULT_OK;}
static PST_RESULT ossl_interest(void *v,pst_u32 *interest){ossl_connection *c=(ossl_connection*)v;if(!c||!interest||!c->owns_socket||c->failed)return PST_RESULT_INVALID_ARGUMENT;if(c->established&&(SSL_pending(c->ssl)>0||SSL_has_pending(c->ssl)))c->interest|=PST_BACKEND_INTEREST_READ;*interest=c->interest;return PST_RESULT_OK;}
static PST_RESULT ossl_wait(void *v,pst_u32 interest,pst_u32 timeout_ms,PST_BACKEND_WAIT_RESULT *result){ossl_connection *c=(ossl_connection*)v;fd_set reads,writes,errors;struct timeval timeout;int ready;if(!c||!result||!c->owns_socket||c->failed)return PST_RESULT_INVALID_ARGUMENT;memset(result,0,sizeof(*result));if((interest&PST_BACKEND_INTEREST_READ)&&c->established&&(SSL_pending(c->ssl)>0||SSL_has_pending(c->ssl))){result->ready_interest=PST_BACKEND_INTEREST_READ;return PST_RESULT_OK;}FD_ZERO(&reads);FD_ZERO(&writes);FD_ZERO(&errors);if(interest&PST_BACKEND_INTEREST_READ)FD_SET(c->socket_value,&reads);if(interest&PST_BACKEND_INTEREST_WRITE)FD_SET(c->socket_value,&writes);FD_SET(c->socket_value,&errors);timeout.tv_sec=(long)(timeout_ms/1000UL);timeout.tv_usec=(long)((timeout_ms%1000UL)*1000UL);ready=select(0,&reads,&writes,&errors,&timeout);if(ready==SOCKET_ERROR||FD_ISSET(c->socket_value,&errors)){ossl_capture((ossl_base*)c,PST_RESULT_TRANSPORT_FAILURE,PST_DIAGNOSTIC_PHASE_WAIT);c->failed=1;c->interest=PST_BACKEND_INTEREST_NONE;return PST_RESULT_TRANSPORT_FAILURE;}if(ready==0){result->timed_out=1;return PST_RESULT_OK;}if(FD_ISSET(c->socket_value,&reads))result->ready_interest|=PST_BACKEND_INTEREST_READ;if(FD_ISSET(c->socket_value,&writes))result->ready_interest|=PST_BACKEND_INTEREST_WRITE;return PST_RESULT_OK;}
static void ossl_io_reset(PST_BACKEND_IO_RESULT *r){memset(r,0,sizeof(*r));r->operation=PST_BACKEND_OPERATION_FAILED;r->error=PST_RESULT_OK;}
static PST_RESULT ossl_read(void *v,void *buffer,pst_size capacity,PST_BACKEND_IO_RESULT *result){ossl_connection *c=(ossl_connection*)v;size_t read_size=0;int ret,ssl_error;PST_RESULT mapped,error;pst_u32 operation;if(!c||!result||(!buffer&&capacity))return PST_RESULT_INVALID_ARGUMENT;ossl_io_reset(result);if(!c->established||c->failed)return PST_RESULT_INVALID_STATE;if(capacity==0){result->operation=PST_BACKEND_OPERATION_COMPLETE;return PST_RESULT_OK;}ossl_clear_errors();ret=SSL_read_ex(c->ssl,buffer,(size_t)capacity,&read_size);if(ret==1){ossl_clear_errors();result->bytes_transferred=(pst_size)read_size;result->operation=PST_BACKEND_OPERATION_COMPLETE;c->interest=PST_BACKEND_INTEREST_READ;return PST_RESULT_OK;}ssl_error=SSL_get_error(c->ssl,ret);mapped=ossl_want(c,ssl_error,&operation,&error);if(mapped==PST_RESULT_OK){result->operation=operation;result->error=error;return PST_RESULT_OK;}ossl_classify_failure(c,ssl_error,ret,PST_DIAGNOSTIC_PHASE_READ,&operation,&error,&result->close_kind);result->operation=operation;result->error=error;return PST_RESULT_OK;}
static PST_RESULT ossl_write(void *v,const void *buffer,pst_size length,PST_BACKEND_IO_RESULT *result){ossl_connection *c=(ossl_connection*)v;size_t written=0;int ret,ssl_error;PST_RESULT mapped,error;pst_u32 operation;if(!c||!result||(!buffer&&length))return PST_RESULT_INVALID_ARGUMENT;ossl_io_reset(result);if(!c->established||c->failed)return PST_RESULT_INVALID_STATE;if(length==0){result->operation=PST_BACKEND_OPERATION_COMPLETE;return PST_RESULT_OK;}ossl_clear_errors();ret=SSL_write_ex(c->ssl,buffer,(size_t)length,&written);if(ret==1){ossl_clear_errors();result->bytes_transferred=(pst_size)written;result->operation=PST_BACKEND_OPERATION_COMPLETE;c->interest=PST_BACKEND_INTEREST_READ;return PST_RESULT_OK;}ssl_error=SSL_get_error(c->ssl,ret);mapped=ossl_want(c,ssl_error,&operation,&error);if(mapped==PST_RESULT_OK){result->operation=operation;result->error=error;return PST_RESULT_OK;}ossl_classify_failure(c,ssl_error,ret,PST_DIAGNOSTIC_PHASE_WRITE,&operation,&error,NULL);result->operation=operation;result->error=error;return PST_RESULT_OK;}
static PST_RESULT ossl_close(void *v,pst_u32 *operation,PST_RESULT *error){ossl_connection *c=(ossl_connection*)v;int ret,ssl_error;PST_RESULT mapped;if(!c||!operation||!error||!c->established||c->failed)return PST_RESULT_INVALID_ARGUMENT;ossl_clear_errors();ret=SSL_shutdown(c->ssl);if(ret==1){ossl_clear_errors();c->interest=PST_BACKEND_INTEREST_NONE;*operation=PST_BACKEND_OPERATION_COMPLETE;*error=PST_RESULT_OK;return PST_RESULT_OK;}if(ret==0){ossl_clear_errors();c->shutdown_started=1;c->interest=PST_BACKEND_INTEREST_READ;*operation=PST_BACKEND_OPERATION_NEED_READ;*error=PST_RESULT_OK;return PST_RESULT_OK;}ssl_error=SSL_get_error(c->ssl,ret);mapped=ossl_want(c,ssl_error,operation,error);if(mapped==PST_RESULT_OK)return PST_RESULT_OK;return ossl_classify_failure(c,ssl_error,ret,PST_DIAGNOSTIC_PHASE_SHUTDOWN,operation,error,NULL);}
static PST_RESULT ossl_peer(void *v,void **out){ossl_connection *c=(ossl_connection*)v;X509 *cert;PST_PEER_INFO_SUMMARY summary;unsigned char *der,*cursor;int der_size;size_t digest_size=0;PST_RESULT result;if(!c||!out)return PST_RESULT_INVALID_ARGUMENT;*out=NULL;if(!c->established)return PST_RESULT_INVALID_STATE;cert=SSL_get1_peer_certificate(c->ssl);if(!cert)return PST_RESULT_UNAVAILABLE;der_size=i2d_X509(cert,NULL);if(der_size<=0){X509_free(cert);ossl_clear_errors();return PST_RESULT_BACKEND_FAILURE;}der=(unsigned char*)malloc((size_t)der_size);if(!der){X509_free(cert);return PST_RESULT_OUT_OF_MEMORY;}cursor=der;if(i2d_X509(cert,&cursor)!=der_size){free(der);X509_free(cert);ossl_clear_errors();return PST_RESULT_BACKEND_FAILURE;}X509_free(cert);memset(&summary,0,sizeof(summary));summary.struct_size=sizeof(summary);summary.api_version=PST_API_VERSION;summary.local_role=PST_CONNECTION_ROLE_CLIENT;strncpy(summary.provider_id,"openssl",sizeof(summary.provider_id)-1);summary.certificate_present=PST_KNOWN_TRUE;summary.chain_validated=PST_KNOWN_TRUE;summary.peer_name_validated=PST_KNOWN_TRUE;summary.peer_authenticated=PST_KNOWN_TRUE;summary.tls_version=c->negotiated_version;summary.cipher_suite=c->cipher_suite;summary.alpn_available=c->negotiated_alpn_size?PST_KNOWN_TRUE:PST_KNOWN_FALSE;summary.session_resumed=PST_KNOWN_UNKNOWN;summary.early_data_accepted=PST_KNOWN_UNSUPPORTED;summary.certificate_sha256_size=32;summary.leaf_der_size=(pst_size)der_size;if(EVP_Q_digest(c->runtime->library_context,"SHA256",NULL,der,(size_t)der_size,summary.certificate_sha256,&digest_size)!=1||digest_size!=32){free(der);ossl_clear_errors();return PST_RESULT_BACKEND_FAILURE;}result=pst_peer_info_create_snapshot(&summary,der,(pst_peer_info**)out);free(der);ossl_clear_errors();return result;}
static void ossl_peer_destroy(void *v){pst_peer_info_release((pst_peer_info*)v);}
static PST_RESULT ossl_alpn(void *v,pst_u8 *buffer,pst_size capacity,pst_size *size){ossl_connection *c=(ossl_connection*)v;if(!c||!size)return PST_RESULT_INVALID_ARGUMENT;*size=c->negotiated_alpn_size;if(!c->negotiated_alpn_size)return PST_RESULT_UNAVAILABLE;if(capacity<c->negotiated_alpn_size)return PST_RESULT_TRUNCATED;if(!buffer)return PST_RESULT_INVALID_ARGUMENT;memcpy(buffer,c->negotiated_alpn,c->negotiated_alpn_size);return PST_RESULT_OK;}static void ossl_diagnostic(const void *v,pst_internal_diagnostic *out){if(!out)return;if(v)pst_diagnostic_copy(out,&((const ossl_base*)v)->diagnostic);else pst_diagnostic_initialize(out);}

static int ossl_server_alpn_select(SSL *ssl,const unsigned char **out,
 unsigned char *out_size,const unsigned char *client,unsigned int client_size,
 void *argument)
{
    ossl_connection *c=(ossl_connection*)argument;
    const unsigned char *server,*server_end,*offered,*offered_end;
    unsigned int server_size,offered_size;
    (void)ssl;
    server=c->alpn_wire;server_end=server+c->alpn_wire_size;
    while(server<server_end){
        server_size=*server++;offered=client;offered_end=client+client_size;
        while(offered<offered_end){
            offered_size=*offered++;
            if(offered_size>(unsigned int)(offered_end-offered))break;
            if(server_size==offered_size&&!memcmp(server,offered,server_size)){
                *out=server;*out_size=(unsigned char)server_size;
                return SSL_TLSEXT_ERR_OK;
            }
            offered+=offered_size;
        }
        server+=server_size;
    }
    if(c->alpn_requirement==PST_FEATURE_REQUIRED){
        ossl_capture((ossl_base*)c,PST_RESULT_POLICY_VIOLATION,
            PST_DIAGNOSTIC_PHASE_ALPN);
        c->diagnostic.role=PST_CONNECTION_ROLE_SERVER;
        c->diagnostic.reason=PST_DIAGNOSTIC_REASON_ALPN_MISMATCH;
        return SSL_TLSEXT_ERR_ALERT_FATAL;
    }
    return SSL_TLSEXT_ERR_NOACK;
}

static PST_RESULT ossl_server_configure(ossl_connection *c,
 const PST_CONNECTION_CONFIG *config)
{
    const pst_credentials *credentials;
    const pst_trust *trust;
    const pst_u8 *der,*key_der,*alpn;
    const unsigned char *cursor;
    pst_size size,key_size,alpn_size,count,index;
    pst_u32 minimum,maximum;
    int minimum_native,maximum_native,verify_mode;
    X509 *certificate=NULL;
    EVP_PKEY *private_key=NULL;
    X509_STORE *store;
    minimum=pst_connection_config_minimum_version(config);
    maximum=pst_connection_config_maximum_version(config);
    minimum_native=minimum==PST_TLS_VERSION_1_2?TLS1_2_VERSION:
        minimum==PST_TLS_VERSION_1_3?TLS1_3_VERSION:0;
    maximum_native=maximum==PST_TLS_VERSION_1_2?TLS1_2_VERSION:
        maximum==PST_TLS_VERSION_1_3?TLS1_3_VERSION:0;
    if(!minimum_native||!maximum_native||maximum<minimum)
        return PST_RESULT_INVALID_ARGUMENT;
    credentials=pst_connection_config_local_credentials(config);
    if(!credentials)return PST_RESULT_POLICY_VIOLATION;
    ossl_clear_errors();
    c->ssl_context=SSL_CTX_new_ex(c->runtime->library_context,NULL,
        TLS_server_method());
    if(!c->ssl_context)goto backend_failure;
    if(!SSL_CTX_set_min_proto_version(c->ssl_context,minimum_native)||
       !SSL_CTX_set_max_proto_version(c->ssl_context,maximum_native))
        goto backend_failure;
    SSL_CTX_clear_options(c->ssl_context,SSL_OP_IGNORE_UNEXPECTED_EOF);
    count=pst_credentials_certificate_count(credentials);
    if(!count)goto identity_failure;
    for(index=0;index<count;index++){
        der=pst_credentials_certificate_at(credentials,index,&size);
        if(!der||!size||size>(pst_size)LONG_MAX)goto identity_failure;
        cursor=der;certificate=d2i_X509(NULL,&cursor,(long)size);
        if(!certificate||cursor!=der+size)goto identity_failure;
        if(index==0){
            if(SSL_CTX_use_certificate(c->ssl_context,certificate)!=1)
                goto identity_failure;
        }else if(SSL_CTX_add1_chain_cert(c->ssl_context,certificate)!=1)
            goto identity_failure;
        X509_free(certificate);certificate=NULL;
    }
    key_der=pst_credentials_private_key_der(credentials,&key_size);
    if(!key_der||!key_size||key_size>(pst_size)LONG_MAX)
        goto identity_failure;
    cursor=key_der;
    private_key=d2i_AutoPrivateKey(NULL,&cursor,(long)key_size);
    if(!private_key||cursor!=key_der+key_size||
       SSL_CTX_use_PrivateKey(c->ssl_context,private_key)!=1||
       SSL_CTX_check_private_key(c->ssl_context)!=1)
        goto identity_failure;
    EVP_PKEY_free(private_key);private_key=NULL;
    c->peer_certificate_mode=
        pst_connection_config_peer_certificate_mode(config);
    verify_mode=SSL_VERIFY_NONE;
    if(c->peer_certificate_mode!=PST_PEER_CERTIFICATE_DISABLED){
        trust=pst_connection_config_peer_trust(config);
        if(!trust||(pst_trust_kind(trust)!=PST_TRUST_SOURCE_CUSTOM_CA_DER&&
                   pst_trust_kind(trust)!=PST_TRUST_SOURCE_SYSTEM))
            return PST_RESULT_UNSUPPORTED;
        c->trust_kind=pst_trust_kind(trust);
        if(c->trust_kind==PST_TRUST_SOURCE_CUSTOM_CA_DER){
            store=SSL_CTX_get_cert_store(c->ssl_context);
            count=pst_trust_anchor_count(trust);
            if(!store||!count)goto identity_failure;
            for(index=0;index<count;index++){
                der=pst_trust_anchor_at(trust,index,&size);
                if(!der||!size||size>(pst_size)LONG_MAX)
                    goto identity_failure;
                cursor=der;certificate=d2i_X509(NULL,&cursor,(long)size);
                if(!certificate||cursor!=der+size||
                   X509_STORE_add_cert(store,certificate)!=1)
                    goto identity_failure;
                X509_free(certificate);certificate=NULL;
            }
        }else SSL_CTX_set_cert_verify_callback(c->ssl_context,
                                                ossl_system_verify,c);
        verify_mode=SSL_VERIFY_PEER;
        if(c->peer_certificate_mode==PST_PEER_CERTIFICATE_REQUIRED)
            verify_mode|=SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
    }
    SSL_CTX_set_verify(c->ssl_context,verify_mode,NULL);
    alpn=pst_connection_config_alpn_wire(config,&alpn_size);
    c->alpn_requirement=pst_connection_config_alpn_mode(config);
    if(alpn_size){
        c->alpn_wire=(pst_u8*)malloc(alpn_size);
        if(!c->alpn_wire)return PST_RESULT_OUT_OF_MEMORY;
        memcpy(c->alpn_wire,alpn,alpn_size);
        c->alpn_wire_size=alpn_size;
        SSL_CTX_set_alpn_select_cb(c->ssl_context,
            ossl_server_alpn_select,c);
    }
    c->ssl=SSL_new(c->ssl_context);
    if(!c->ssl)goto backend_failure;
    SSL_clear_mode(c->ssl,SSL_MODE_AUTO_RETRY|
        SSL_MODE_ENABLE_PARTIAL_WRITE|SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
    SSL_set_accept_state(c->ssl);
    c->configured=1;ossl_clear_errors();return PST_RESULT_OK;
identity_failure:
    if(certificate)X509_free(certificate);
    if(private_key)EVP_PKEY_free(private_key);
    ossl_clear_errors();
    ossl_capture((ossl_base*)c,PST_RESULT_AUTH_FAILURE,
        PST_DIAGNOSTIC_PHASE_IDENTITY_SETUP);
    return PST_RESULT_AUTH_FAILURE;
backend_failure:
    if(certificate)X509_free(certificate);
    if(private_key)EVP_PKEY_free(private_key);
    ossl_clear_errors();
    ossl_capture((ossl_base*)c,PST_RESULT_BACKEND_FAILURE,
        PST_DIAGNOSTIC_PHASE_TLS_CONFIGURE);
    return PST_RESULT_BACKEND_FAILURE;
}

static PST_RESULT ossl_connection_create_role(void *runtime_state,
 const PST_BACKEND_CONNECTION_OPTIONS *options,void **out)
{
    ossl_connection *c;PST_RESULT result;
    if(!options||options->role==PST_CONNECTION_ROLE_CLIENT){
        result=ossl_connection_create(runtime_state,options,out);
        if(result==PST_RESULT_OK)((ossl_connection*)*out)->role=
            PST_CONNECTION_ROLE_CLIENT;
        return result;
    }
    if(!runtime_state||!out||
       options->struct_size<PST_BACKEND_CONNECTION_OPTIONS_MIN_SIZE||
       options->spi_version!=PST_BACKEND_SPI_VERSION||
       options->role!=PST_CONNECTION_ROLE_SERVER||!options->configuration)
        return PST_RESULT_INVALID_ARGUMENT;
    *out=NULL;c=(ossl_connection*)calloc(1,sizeof(*c));
    if(!c)return PST_RESULT_OUT_OF_MEMORY;
    pst_diagnostic_initialize(&c->diagnostic);
    c->runtime=(ossl_runtime*)runtime_state;
    c->socket_value=INVALID_SOCKET;
    c->interest=PST_BACKEND_INTEREST_NONE;
    c->role=PST_CONNECTION_ROLE_SERVER;
    result=ossl_server_configure(c,options->configuration);
    if(result!=PST_RESULT_OK){ossl_connection_destroy(c);return result;}
    *out=c;return PST_RESULT_OK;
}

static PST_RESULT ossl_server_peer(ossl_connection *c,void **out)
{
    X509 *certificate;PST_PEER_INFO_SUMMARY summary;
    unsigned char *der=NULL,*cursor;int der_size=0;size_t digest_size=0;
    PST_RESULT result;
    if(!c||!out)return PST_RESULT_INVALID_ARGUMENT;*out=NULL;
    if(!c->established)return PST_RESULT_INVALID_STATE;
    certificate=SSL_get1_peer_certificate(c->ssl);
    memset(&summary,0,sizeof(summary));summary.struct_size=sizeof(summary);
    summary.api_version=PST_API_VERSION;
    summary.local_role=PST_CONNECTION_ROLE_SERVER;
    strncpy(summary.provider_id,"openssl",sizeof(summary.provider_id)-1);
    summary.peer_name_validated=PST_KNOWN_NOT_APPLICABLE;
    summary.tls_version=c->negotiated_version;
    summary.cipher_suite=c->cipher_suite;
    summary.alpn_available=c->negotiated_alpn_size?
        PST_KNOWN_TRUE:PST_KNOWN_FALSE;
    summary.session_resumed=PST_KNOWN_UNKNOWN;
    summary.early_data_accepted=PST_KNOWN_UNSUPPORTED;
    if(!certificate){
        summary.certificate_present=PST_KNOWN_FALSE;
        summary.chain_validated=PST_KNOWN_NOT_APPLICABLE;
        summary.peer_authenticated=PST_KNOWN_FALSE;
        return pst_peer_info_create_snapshot(&summary,NULL,(pst_peer_info**)out);
    }
    der_size=i2d_X509(certificate,NULL);
    if(der_size<=0){X509_free(certificate);return PST_RESULT_BACKEND_FAILURE;}
    der=(unsigned char*)malloc((size_t)der_size);
    if(!der){X509_free(certificate);return PST_RESULT_OUT_OF_MEMORY;}
    cursor=der;
    if(i2d_X509(certificate,&cursor)!=der_size){
        free(der);X509_free(certificate);return PST_RESULT_BACKEND_FAILURE;
    }
    X509_free(certificate);
    summary.certificate_present=PST_KNOWN_TRUE;
    summary.chain_validated=SSL_get_verify_result(c->ssl)==X509_V_OK?
        PST_KNOWN_TRUE:PST_KNOWN_FALSE;
    summary.peer_authenticated=summary.chain_validated;
    summary.certificate_sha256_size=32;
    summary.leaf_der_size=(pst_size)der_size;
    if(EVP_Q_digest(c->runtime->library_context,"SHA256",NULL,der,
       (size_t)der_size,summary.certificate_sha256,&digest_size)!=1||
       digest_size!=32){free(der);return PST_RESULT_BACKEND_FAILURE;}
    result=pst_peer_info_create_snapshot(&summary,der,(pst_peer_info**)out);
    free(der);ossl_clear_errors();return result;
}

static PST_RESULT ossl_peer_role(void *state,void **out)
{
    ossl_connection *c=(ossl_connection*)state;
    return c&&c->role==PST_CONNECTION_ROLE_SERVER?
        ossl_server_peer(c,out):ossl_peer(state,out);
}

static const PST_BACKEND_VTABLE ossl_vtable={sizeof(PST_BACKEND_VTABLE),PST_BACKEND_SPI_VERSION,ossl_initialize,ossl_shutdown,ossl_runtime_create,ossl_runtime_destroy,ossl_query,ossl_validate,ossl_connection_create_role,ossl_connection_destroy,ossl_attach,ossl_handshake,ossl_interest,ossl_wait,ossl_read,ossl_write,ossl_close,ossl_peer_role,ossl_peer_destroy,ossl_alpn,ossl_diagnostic};
static const PST_BACKEND_METADATA ossl_metadata={sizeof(PST_BACKEND_METADATA),PST_BACKEND_METADATA_VERSION,{PST_BACKEND_VERSION_AVAILABLE,0UL,3UL,0UL,"pst-openssl","identity"},1UL,{{PST_BACKEND_VERSION_AVAILABLE,3UL,5UL,8UL,"OpenSSL","LTS"},{0}}};
static const PST_BACKEND_DESCRIPTOR ossl_descriptor={sizeof(PST_BACKEND_DESCRIPTOR),PST_BACKEND_SPI_VERSION,"openssl","OpenSSL 3.5.8",OSSL_CAPABILITIES,&ossl_vtable,&ossl_metadata,OSSL_CLIENT_CAPABILITIES,OSSL_SERVER_CAPABILITIES};
PST_RESULT pst_backend_openssl_connection_info(void *v,pst_u32 *tls_version,pst_u32 *cipher_suite){ossl_connection *c=(ossl_connection*)v;if(!c||!tls_version||!cipher_suite)return PST_RESULT_INVALID_ARGUMENT;if(!c->established)return PST_RESULT_INVALID_STATE;*tls_version=c->negotiated_version;*cipher_suite=c->cipher_suite;return PST_RESULT_OK;}
const PST_BACKEND_DESCRIPTOR *pst_backend_openssl_descriptor(void){return &ossl_descriptor;}
PST_RESULT pst_backend_openssl_register(void){return pst_backend_register(&ossl_descriptor);}
