#include "pst_identity_internal.h"
#include <stdlib.h>
#include <string.h>
struct pst_credentials { pst_u32 refs, kind; pst_u8 *cert, *key; pst_size cert_n, key_n; };
struct pst_trust { pst_u32 refs, kind; pst_u8 *data; pst_size data_n; };
struct pst_config { pst_u32 refs;pst_credentials *credentials; pst_trust *trust; char *hostname; pst_u32 peer, client; pst_u32 min_version,max_version,alpn_requirement,resumption,early_data,graceful; pst_u8 *alpn; pst_size alpn_n; int frozen; };
struct pst_peer_info { PST_PEER_INFO_SUMMARY summary; pst_u8 *der; };
static int version_ok(pst_u32 v) { return ((v >> 16) & 0xffffUL) == PST_API_VERSION_MAJOR; }
static pst_u8 *bytes_copy(const pst_u8 *p, pst_size n) { pst_u8 *q; if (!p || !n) return NULL; q=(pst_u8*)malloc(n); if(q) memcpy(q,p,n); return q; }
static void secret_clear(void *p, pst_size n) { volatile pst_u8 *q=(volatile pst_u8*)p; while(n){*q++=0;--n;} }
PST_RESULT PST_CALL pst_credentials_create(const PST_CREDENTIAL_SOURCE *s, pst_credentials **out)
{
 pst_credentials *v; if(!out)return PST_RESULT_INVALID_ARGUMENT; *out=NULL;
 if(!s||s->struct_size<PST_CREDENTIAL_SOURCE_MIN_SIZE)return PST_RESULT_INVALID_ARGUMENT;
 if(!version_ok(s->api_version))return PST_RESULT_INCOMPATIBLE_API;
 if(s->kind!=PST_CREDENTIAL_SOURCE_CERT_DER_PKCS8_DER)return PST_RESULT_UNSUPPORTED;
 if(!s->certificate_der||!s->certificate_der_size||!s->private_key_der||!s->private_key_der_size)return PST_RESULT_INVALID_ARGUMENT;
 v=(pst_credentials*)calloc(1,sizeof(*v)); if(!v)return PST_RESULT_OUT_OF_MEMORY;
 v->cert=bytes_copy(s->certificate_der,s->certificate_der_size); v->key=bytes_copy(s->private_key_der,s->private_key_der_size);
 if(!v->cert||!v->key){if(v->key){secret_clear(v->key,s->private_key_der_size);free(v->key);}free(v->cert);free(v);return PST_RESULT_OUT_OF_MEMORY;}
 v->refs=1;v->kind=s->kind;v->cert_n=s->certificate_der_size;v->key_n=s->private_key_der_size;*out=v;return PST_RESULT_OK;
}
static void credentials_retain(pst_credentials *v){if(v)++v->refs;}
void PST_CALL pst_credentials_release(pst_credentials *v){if(!v||--v->refs)return;secret_clear(v->key,v->key_n);free(v->key);free(v->cert);secret_clear(v,sizeof(*v));free(v);}
PST_RESULT PST_CALL pst_trust_create(const PST_TRUST_SOURCE *s,pst_trust **out)
{
 pst_trust *v;if(!out)return PST_RESULT_INVALID_ARGUMENT;*out=NULL;
 if(!s||s->struct_size<PST_TRUST_SOURCE_MIN_SIZE)return PST_RESULT_INVALID_ARGUMENT;
 if(!version_ok(s->api_version))return PST_RESULT_INCOMPATIBLE_API;
 if(s->kind!=PST_TRUST_SOURCE_CUSTOM_CA_DER&&s->kind!=PST_TRUST_SOURCE_SYSTEM)return PST_RESULT_UNSUPPORTED;
 if(s->kind==PST_TRUST_SOURCE_CUSTOM_CA_DER&&(!s->data||!s->data_size))return PST_RESULT_INVALID_ARGUMENT;
 if(s->kind==PST_TRUST_SOURCE_SYSTEM&&(s->data||s->data_size))return PST_RESULT_INVALID_ARGUMENT;
 v=(pst_trust*)calloc(1,sizeof(*v));if(!v)return PST_RESULT_OUT_OF_MEMORY;
 if(s->data_size){v->data=bytes_copy(s->data,s->data_size);if(!v->data){free(v);return PST_RESULT_OUT_OF_MEMORY;}}
 v->refs=1;v->kind=s->kind;v->data_n=s->data_size;*out=v;return PST_RESULT_OK;
}
static void trust_retain(pst_trust *v){if(v)++v->refs;}
void PST_CALL pst_trust_release(pst_trust *v){if(!v||--v->refs)return;free(v->data);free(v);}
PST_RESULT PST_CALL pst_config_create(pst_config **out){if(!out)return PST_RESULT_INVALID_ARGUMENT;*out=(pst_config*)calloc(1,sizeof(pst_config));if(!*out)return PST_RESULT_OUT_OF_MEMORY;(*out)->refs=1;(*out)->min_version=PST_TLS_VERSION_1_2;(*out)->max_version=PST_TLS_VERSION_1_3;return PST_RESULT_OK;}
PST_RESULT PST_CALL pst_config_set_identity(pst_config *c,const PST_IDENTITY_CONFIG *s)
{
 char *h=NULL;if(!c||!s||s->struct_size<PST_IDENTITY_CONFIG_MIN_SIZE)return PST_RESULT_INVALID_ARGUMENT;
 if(!version_ok(s->api_version))return PST_RESULT_INCOMPATIBLE_API;if(c->frozen)return PST_RESULT_INVALID_STATE;
 if(s->require_peer_authentication>1UL||s->require_client_authentication>1UL)return PST_RESULT_INVALID_ARGUMENT;
 if((s->expected_hostname==NULL)!=(s->expected_hostname_size==0))return PST_RESULT_INVALID_ARGUMENT;
 if(s->expected_hostname_size){h=(char*)malloc(s->expected_hostname_size+1);if(!h)return PST_RESULT_OUT_OF_MEMORY;memcpy(h,s->expected_hostname,s->expected_hostname_size);h[s->expected_hostname_size]='\0';if(memchr(h,'\0',s->expected_hostname_size)){free(h);return PST_RESULT_INVALID_ARGUMENT;}}
 credentials_retain(s->credentials);trust_retain(s->trust);pst_credentials_release(c->credentials);pst_trust_release(c->trust);free(c->hostname);
 c->credentials=s->credentials;c->trust=s->trust;c->hostname=h;c->peer=s->require_peer_authentication;c->client=s->require_client_authentication;return PST_RESULT_OK;
}
PST_RESULT PST_CALL pst_config_freeze(pst_config *c){if(!c)return PST_RESULT_INVALID_ARGUMENT;if(c->frozen)return PST_RESULT_OK;if(c->peer&&(!c->trust||!c->hostname))return PST_RESULT_POLICY_VIOLATION;if(c->client&&!c->credentials)return PST_RESULT_POLICY_VIOLATION;c->frozen=1;return PST_RESULT_OK;}
PST_RESULT PST_CALL pst_config_set_tls_policy(pst_config *c,const PST_TLS_POLICY *p){pst_u8 *wire=NULL;pst_size total=0,i,at=0;if(!c||!p||p->struct_size<PST_TLS_POLICY_MIN_SIZE)return PST_RESULT_INVALID_ARGUMENT;if(!version_ok(p->api_version))return PST_RESULT_INCOMPATIBLE_API;if(c->frozen)return PST_RESULT_INVALID_STATE;if((p->minimum_version!=PST_TLS_VERSION_1_2&&p->minimum_version!=PST_TLS_VERSION_1_3)||p->maximum_version<p->minimum_version||p->maximum_version>PST_TLS_VERSION_1_3||p->alpn_requirement>PST_FEATURE_REQUIRED||p->resumption>PST_FEATURE_REQUIRED||p->early_data>PST_FEATURE_REQUIRED)return PST_RESULT_INVALID_ARGUMENT;if(p->resumption==PST_FEATURE_REQUIRED||p->early_data!=PST_FEATURE_DISABLED)return PST_RESULT_UNSUPPORTED;if((p->alpn_protocols==NULL)!=(p->alpn_protocol_count==0))return PST_RESULT_INVALID_ARGUMENT;for(i=0;i<p->alpn_protocol_count;i++){if(!p->alpn_protocols[i].data||!p->alpn_protocols[i].size||p->alpn_protocols[i].size>255)return PST_RESULT_INVALID_ARGUMENT;total+=1+p->alpn_protocols[i].size;}if(total){wire=(pst_u8*)malloc(total);if(!wire)return PST_RESULT_OUT_OF_MEMORY;for(i=0;i<p->alpn_protocol_count;i++){wire[at++]=(pst_u8)p->alpn_protocols[i].size;memcpy(wire+at,p->alpn_protocols[i].data,p->alpn_protocols[i].size);at+=p->alpn_protocols[i].size;}}free(c->alpn);c->alpn=wire;c->alpn_n=total;c->min_version=p->minimum_version;c->max_version=p->maximum_version;c->alpn_requirement=p->alpn_requirement;c->resumption=p->resumption;c->early_data=p->early_data;c->graceful=p->require_graceful_shutdown;return PST_RESULT_OK;}
void pst_config_retain(pst_config *c){if(c)++c->refs;}
void PST_CALL pst_config_release(pst_config *c){if(c&&!--c->refs){pst_credentials_release(c->credentials);pst_trust_release(c->trust);free(c->hostname);free(c->alpn);free(c);}}
pst_u32 pst_credentials_kind(const pst_credentials *v){return v?v->kind:0UL;}
const pst_u8 *pst_credentials_certificate_der(const pst_credentials *v,pst_size *n){if(n)*n=v?v->cert_n:0;return v?v->cert:NULL;}
const pst_u8 *pst_credentials_private_key_der(const pst_credentials *v,pst_size *n){if(n)*n=v?v->key_n:0;return v?v->key:NULL;}
pst_u32 pst_trust_kind(const pst_trust *v){return v?v->kind:0UL;}
const pst_u8 *pst_trust_data(const pst_trust *v,pst_size *n){if(n)*n=v?v->data_n:0;return v?v->data:NULL;}
const pst_credentials *pst_config_credentials(const pst_config *v){return v?v->credentials:NULL;}
const pst_trust *pst_config_trust(const pst_config *v){return v?v->trust:NULL;}
const char *pst_config_expected_hostname(const pst_config *v){return v?v->hostname:NULL;}
pst_u32 pst_config_require_peer_authentication(const pst_config *v){return v?v->peer:0UL;}
pst_u32 pst_config_require_client_authentication(const pst_config *v){return v?v->client:0UL;}
int pst_config_is_frozen(const pst_config *v){return v&&v->frozen;}
pst_u32 pst_config_minimum_version(const pst_config *v){return v?v->min_version:0;}
pst_u32 pst_config_maximum_version(const pst_config *v){return v?v->max_version:0;}
const pst_u8 *pst_config_alpn_wire(const pst_config *v,pst_size *n){if(n)*n=v?v->alpn_n:0;return v?v->alpn:NULL;}
pst_u32 pst_config_alpn_requirement(const pst_config *v){return v?v->alpn_requirement:0;}
pst_u32 pst_config_required_capabilities(const pst_config *v){pst_u32 x=PST_CAP_NONBLOCKING|PST_CAP_BACKEND_WAIT;if(!v)return x;if(v->min_version==PST_TLS_VERSION_1_3)x|=PST_CAP_TLS_1_3;else x|=PST_CAP_TLS_1_2;if(v->peer)x|=PST_CAP_HOSTNAME_VERIFY;if(v->trust){x|=pst_trust_kind(v->trust)==PST_TRUST_SOURCE_SYSTEM?PST_CAP_SYSTEM_TRUST:PST_CAP_CUSTOM_TRUST;}if(v->client)x|=PST_CAP_CLIENT_AUTH;if(v->alpn_n)x|=PST_CAP_ALPN;return x;}
PST_RESULT pst_peer_info_create_snapshot(const PST_PEER_INFO_SUMMARY *s,const pst_u8 *der,pst_peer_info **out){pst_peer_info *v;if(!s||!out||(s->leaf_der_size&&!der))return PST_RESULT_INVALID_ARGUMENT;*out=NULL;v=(pst_peer_info*)calloc(1,sizeof(*v));if(!v)return PST_RESULT_OUT_OF_MEMORY;v->summary=*s;if(s->leaf_der_size){v->der=bytes_copy(der,s->leaf_der_size);if(!v->der){free(v);return PST_RESULT_OUT_OF_MEMORY;}}*out=v;return PST_RESULT_OK;}
PST_RESULT PST_CALL pst_peer_info_get_summary(const pst_peer_info *v,PST_PEER_INFO_SUMMARY *s){pst_u32 n;if(!v||!s||s->struct_size<PST_PEER_INFO_SUMMARY_MIN_SIZE)return PST_RESULT_INVALID_ARGUMENT;if(!version_ok(s->api_version))return PST_RESULT_INCOMPATIBLE_API;n=s->struct_size;*s=v->summary;s->struct_size=n;s->api_version=PST_API_VERSION;return PST_RESULT_OK;}
PST_RESULT PST_CALL pst_peer_info_copy_leaf_der(const pst_peer_info *v,pst_u8 *b,pst_size cap,pst_size *out){if(!v||!out)return PST_RESULT_INVALID_ARGUMENT;*out=v->summary.leaf_der_size;if(cap<v->summary.leaf_der_size)return PST_RESULT_TRUNCATED;if(v->summary.leaf_der_size&&!b)return PST_RESULT_INVALID_ARGUMENT;if(v->summary.leaf_der_size)memcpy(b,v->der,v->summary.leaf_der_size);return PST_RESULT_OK;}
void PST_CALL pst_peer_info_release(pst_peer_info *v){if(v){free(v->der);free(v);}}
