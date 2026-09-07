/* SPDX-License-Identifier: MPL-2.0 */
#include "pst_identity_internal.h"
#include <stdlib.h>
#include <string.h>

struct pst_credentials {
    pst_u32 refs,kind;
    PST_DER_ITEM *chain;
    pst_size chain_n;
    pst_u8 *key;
    pst_size key_n;
};
struct pst_trust { pst_u32 refs,kind; PST_DER_ITEM *anchors; pst_size anchor_n; };
struct pst_connection_config_snapshot {
    PST_CONNECTION_CONFIG public_config;
    char *exact_id;
    char **ordered_ids;
    pst_size ordered_n;
    char *peer_name;
    PST_ALPN_PROTOCOL *protocols;
    pst_u8 *alpn_data;
    pst_u8 *alpn_wire;
    pst_size alpn_wire_n;
    pst_u32 required_capabilities,tls_capabilities;
};
struct pst_peer_info { PST_PEER_INFO_SUMMARY summary; pst_u8 *der; };

static int version_ok(pst_u32 v){return ((v>>16)&0xffffUL)==PST_API_VERSION_MAJOR;}
static pst_u8 *bytes_copy(const pst_u8 *p,pst_size n){pst_u8 *q;if(!p||!n)return NULL;q=(pst_u8*)malloc(n);if(q)memcpy(q,p,n);return q;}
static char *string_copy(const char *p,pst_size n){char *q;if(!p||!n)return NULL;q=(char*)malloc(n+1);if(q){memcpy(q,p,n);q[n]='\0';}return q;}
static void secret_clear(void *p,pst_size n){volatile pst_u8 *q=(volatile pst_u8*)p;while(p&&n){*q++=0;--n;}}
static void der_items_free(PST_DER_ITEM *v,pst_size n){pst_size i;if(!v)return;for(i=0;i<n;i++)free((void*)v[i].data);free(v);}
static PST_RESULT der_items_copy(const PST_DER_ITEM *source,pst_size count,PST_DER_ITEM **out)
{
    PST_DER_ITEM *items;pst_size i;
    if(!out)return PST_RESULT_INVALID_ARGUMENT;*out=NULL;
    if(!source||!count||count>((pst_size)-1)/sizeof(*items))return PST_RESULT_INVALID_ARGUMENT;
    items=(PST_DER_ITEM*)calloc(count,sizeof(*items));if(!items)return PST_RESULT_OUT_OF_MEMORY;
    for(i=0;i<count;i++){
        if(!source[i].data||!source[i].size){der_items_free(items,count);return PST_RESULT_INVALID_ARGUMENT;}
        items[i].data=bytes_copy(source[i].data,source[i].size);items[i].size=source[i].size;
        if(!items[i].data){der_items_free(items,count);return PST_RESULT_OUT_OF_MEMORY;}
    }
    *out=items;return PST_RESULT_OK;
}

PST_RESULT PST_CALL pst_credentials_create(const PST_CREDENTIAL_SOURCE *s,pst_credentials **out)
{
    pst_credentials *v;PST_RESULT r;
    if(!out)return PST_RESULT_INVALID_ARGUMENT;*out=NULL;
    if(!s||s->struct_size<PST_CREDENTIAL_SOURCE_MIN_SIZE)return PST_RESULT_INVALID_ARGUMENT;
    if(!version_ok(s->api_version))return PST_RESULT_INCOMPATIBLE_API;
    if(s->kind!=PST_CREDENTIAL_SOURCE_CERT_CHAIN_DER_PKCS8_DER)return PST_RESULT_UNSUPPORTED;
    if(!s->certificate_chain||!s->certificate_count||!s->private_key_der||!s->private_key_der_size)return PST_RESULT_INVALID_ARGUMENT;
    v=(pst_credentials*)calloc(1,sizeof(*v));if(!v)return PST_RESULT_OUT_OF_MEMORY;
    r=der_items_copy(s->certificate_chain,s->certificate_count,&v->chain);if(r!=PST_RESULT_OK){free(v);return r;}
    v->key=bytes_copy(s->private_key_der,s->private_key_der_size);
    if(!v->key){der_items_free(v->chain,s->certificate_count);free(v);return PST_RESULT_OUT_OF_MEMORY;}
    v->refs=1;v->kind=s->kind;v->chain_n=s->certificate_count;v->key_n=s->private_key_der_size;*out=v;return PST_RESULT_OK;
}
static void credentials_retain(pst_credentials *v){if(v)++v->refs;}
void PST_CALL pst_credentials_release(pst_credentials *v){if(!v||--v->refs)return;secret_clear(v->key,v->key_n);free(v->key);der_items_free(v->chain,v->chain_n);secret_clear(v,sizeof(*v));free(v);}

PST_RESULT PST_CALL pst_trust_create(const PST_TRUST_SOURCE *s,pst_trust **out)
{
    pst_trust *v;PST_RESULT r;
    if(!out)return PST_RESULT_INVALID_ARGUMENT;*out=NULL;
    if(!s||s->struct_size<PST_TRUST_SOURCE_MIN_SIZE)return PST_RESULT_INVALID_ARGUMENT;
    if(!version_ok(s->api_version))return PST_RESULT_INCOMPATIBLE_API;
    if(s->kind!=PST_TRUST_SOURCE_CUSTOM_CA_DER&&s->kind!=PST_TRUST_SOURCE_SYSTEM)return PST_RESULT_UNSUPPORTED;
    if(s->kind==PST_TRUST_SOURCE_CUSTOM_CA_DER&&(!s->anchors||!s->anchor_count))return PST_RESULT_INVALID_ARGUMENT;
    if(s->kind==PST_TRUST_SOURCE_SYSTEM&&(s->anchors||s->anchor_count))return PST_RESULT_INVALID_ARGUMENT;
    v=(pst_trust*)calloc(1,sizeof(*v));if(!v)return PST_RESULT_OUT_OF_MEMORY;
    if(s->anchor_count){r=der_items_copy(s->anchors,s->anchor_count,&v->anchors);if(r!=PST_RESULT_OK){free(v);return r;}}
    v->refs=1;v->kind=s->kind;v->anchor_n=s->anchor_count;*out=v;return PST_RESULT_OK;
}
static void trust_retain(pst_trust *v){if(v)++v->refs;}
void PST_CALL pst_trust_release(pst_trust *v){if(!v||--v->refs)return;der_items_free(v->anchors,v->anchor_n);free(v);}

pst_u32 pst_credentials_kind(const pst_credentials *v){return v?v->kind:0UL;}
pst_size pst_credentials_certificate_count(const pst_credentials *v){return v?v->chain_n:0;}
const pst_u8 *pst_credentials_certificate_at(const pst_credentials *v,pst_size i,pst_size *n){if(n)*n=(v&&i<v->chain_n)?v->chain[i].size:0;return (v&&i<v->chain_n)?v->chain[i].data:NULL;}
const pst_u8 *pst_credentials_certificate_der(const pst_credentials *v,pst_size *n){return pst_credentials_certificate_at(v,0,n);}
const pst_u8 *pst_credentials_private_key_der(const pst_credentials *v,pst_size *n){if(n)*n=v?v->key_n:0;return v?v->key:NULL;}
pst_u32 pst_trust_kind(const pst_trust *v){return v?v->kind:0UL;}
pst_size pst_trust_anchor_count(const pst_trust *v){return v?v->anchor_n:0;}
const pst_u8 *pst_trust_anchor_at(const pst_trust *v,pst_size i,pst_size *n){if(n)*n=(v&&i<v->anchor_n)?v->anchors[i].size:0;return (v&&i<v->anchor_n)?v->anchors[i].data:NULL;}
const pst_u8 *pst_trust_data(const pst_trust *v,pst_size *n){return pst_trust_anchor_at(v,0,n);}

static PST_RESULT check_record(const void *v,pst_u32 size,pst_u32 minimum,pst_u32 version)
{if(!v||size<minimum)return PST_RESULT_INVALID_ARGUMENT;return version_ok(version)?PST_RESULT_OK:PST_RESULT_INCOMPATIBLE_API;}
static int feature_ok(pst_u32 v){return v<=PST_FEATURE_REQUIRED;}
static PST_RESULT copy_selection(pst_connection_config_snapshot *s,const PST_PROVIDER_SELECTION *p)
{
    pst_size i,n;PST_RESULT r=check_record(p,p->struct_size,PST_PROVIDER_SELECTION_MIN_SIZE,p->api_version);
    if(r!=PST_RESULT_OK)return r;
    if(p->mode<PST_BACKEND_SELECTION_EXACT||p->mode>PST_BACKEND_SELECTION_AUTOMATIC||(p->required_capabilities&~PST_CAP_KNOWN_MASK))return PST_RESULT_INVALID_ARGUMENT;
    if(p->mode==PST_BACKEND_SELECTION_EXACT){
        if(!p->exact_provider_id||!p->exact_provider_id[0]||p->ordered_provider_ids||p->ordered_provider_count)return PST_RESULT_INVALID_ARGUMENT;
        n=strlen(p->exact_provider_id);if(n>=PST_DIAGNOSTIC_BACKEND_ID_CAPACITY)return PST_RESULT_INVALID_ARGUMENT;
        s->exact_id=string_copy(p->exact_provider_id,n);if(!s->exact_id)return PST_RESULT_OUT_OF_MEMORY;
        s->public_config.provider_selection.exact_provider_id=s->exact_id;
    }else if(p->mode==PST_BACKEND_SELECTION_ORDERED){
        if(p->exact_provider_id||!p->ordered_provider_ids||!p->ordered_provider_count||p->ordered_provider_count>((pst_size)-1)/sizeof(char*))return PST_RESULT_INVALID_ARGUMENT;
        s->ordered_ids=(char**)calloc(p->ordered_provider_count,sizeof(char*));if(!s->ordered_ids)return PST_RESULT_OUT_OF_MEMORY;s->ordered_n=p->ordered_provider_count;
        for(i=0;i<p->ordered_provider_count;i++){
            if(!p->ordered_provider_ids[i]||!p->ordered_provider_ids[i][0])return PST_RESULT_INVALID_ARGUMENT;
            n=strlen(p->ordered_provider_ids[i]);if(n>=PST_DIAGNOSTIC_BACKEND_ID_CAPACITY)return PST_RESULT_INVALID_ARGUMENT;
            s->ordered_ids[i]=string_copy(p->ordered_provider_ids[i],n);if(!s->ordered_ids[i])return PST_RESULT_OUT_OF_MEMORY;
        }
        s->public_config.provider_selection.ordered_provider_ids=(const char *const*)s->ordered_ids;
    }else if(p->exact_provider_id||p->ordered_provider_ids||p->ordered_provider_count)return PST_RESULT_INVALID_ARGUMENT;
    return PST_RESULT_OK;
}
static PST_RESULT copy_alpn(pst_connection_config_snapshot *s,const PST_ALPN_CONFIG *a)
{
    pst_size i,j,total=0,wire_total=0,at=0,wat=0;PST_RESULT r=check_record(a,a->struct_size,PST_ALPN_CONFIG_MIN_SIZE,a->api_version);
    if(r!=PST_RESULT_OK)return r;
    if(!feature_ok(a->mode)||(a->protocols==NULL)!=(a->protocol_count==0))return PST_RESULT_INVALID_ARGUMENT;
    if(a->mode==PST_FEATURE_DISABLED&&a->protocol_count)return PST_RESULT_INVALID_ARGUMENT;
    if(a->mode==PST_FEATURE_REQUIRED&&!a->protocol_count)return PST_RESULT_POLICY_VIOLATION;
    for(i=0;i<a->protocol_count;i++){
        if(!a->protocols[i].data||!a->protocols[i].size||a->protocols[i].size>255)return PST_RESULT_INVALID_ARGUMENT;
        for(j=0;j<i;j++)if(a->protocols[i].size==a->protocols[j].size&&!memcmp(a->protocols[i].data,a->protocols[j].data,a->protocols[i].size))return PST_RESULT_INVALID_ARGUMENT;
        if(total>(pst_size)-1-a->protocols[i].size||wire_total>(pst_size)-1-(1+a->protocols[i].size))return PST_RESULT_INVALID_ARGUMENT;
        total+=a->protocols[i].size;wire_total+=1+a->protocols[i].size;
    }
    if(!a->protocol_count)return PST_RESULT_OK;
    if(a->protocol_count>((pst_size)-1)/sizeof(*s->protocols))return PST_RESULT_INVALID_ARGUMENT;
    s->protocols=(PST_ALPN_PROTOCOL*)calloc(a->protocol_count,sizeof(*s->protocols));s->alpn_data=(pst_u8*)malloc(total);s->alpn_wire=(pst_u8*)malloc(wire_total);
    if(!s->protocols||!s->alpn_data||!s->alpn_wire)return PST_RESULT_OUT_OF_MEMORY;
    s->alpn_wire_n=wire_total;
    for(i=0;i<a->protocol_count;i++){
        memcpy(s->alpn_data+at,a->protocols[i].data,a->protocols[i].size);s->protocols[i].data=s->alpn_data+at;s->protocols[i].size=a->protocols[i].size;at+=a->protocols[i].size;
        s->alpn_wire[wat++]=(pst_u8)a->protocols[i].size;memcpy(s->alpn_wire+wat,a->protocols[i].data,a->protocols[i].size);wat+=a->protocols[i].size;
    }
    s->public_config.alpn.protocols=s->protocols;return PST_RESULT_OK;
}
PST_RESULT pst_connection_config_snapshot_create(const PST_CONNECTION_CONFIG *c,pst_connection_config_snapshot **out)
{
    pst_connection_config_snapshot *s;PST_RESULT r;pst_u32 required,tls_caps;
    if(!out)return PST_RESULT_INVALID_ARGUMENT;*out=NULL;
    if(!c||c->struct_size<PST_CONNECTION_CONFIG_MIN_SIZE)return PST_RESULT_INVALID_ARGUMENT;
    if(!version_ok(c->api_version))return PST_RESULT_INCOMPATIBLE_API;
    if(c->role!=PST_CONNECTION_ROLE_CLIENT&&c->role!=PST_CONNECTION_ROLE_SERVER)return PST_RESULT_INVALID_ARGUMENT;
    r=check_record(&c->local_identity,c->local_identity.struct_size,PST_LOCAL_IDENTITY_MIN_SIZE,c->local_identity.api_version);if(r!=PST_RESULT_OK)return r;
    r=check_record(&c->peer_authentication,c->peer_authentication.struct_size,PST_PEER_AUTH_CONFIG_MIN_SIZE,c->peer_authentication.api_version);if(r!=PST_RESULT_OK)return r;
    r=check_record(&c->tls,c->tls.struct_size,PST_TLS_POLICY_MIN_SIZE,c->tls.api_version);if(r!=PST_RESULT_OK)return r;
    s=(pst_connection_config_snapshot*)calloc(1,sizeof(*s));if(!s)return PST_RESULT_OUT_OF_MEMORY;s->public_config=*c;s->public_config.local_identity.credentials=NULL;s->public_config.peer_authentication.trust=NULL;
    r=copy_selection(s,&c->provider_selection);if(r!=PST_RESULT_OK)goto fail;
    if(c->peer_authentication.certificate_mode>PST_PEER_CERTIFICATE_REQUIRED){r=PST_RESULT_INVALID_ARGUMENT;goto fail;}
    if((c->peer_authentication.expected_peer_name==NULL)!=(c->peer_authentication.expected_peer_name_size==0)){r=PST_RESULT_INVALID_ARGUMENT;goto fail;}
    if(c->peer_authentication.certificate_mode==PST_PEER_CERTIFICATE_DISABLED&&(c->peer_authentication.trust||c->peer_authentication.expected_peer_name_size)){r=PST_RESULT_POLICY_VIOLATION;goto fail;}
    if(c->peer_authentication.certificate_mode!=PST_PEER_CERTIFICATE_DISABLED&&!c->peer_authentication.trust){r=PST_RESULT_POLICY_VIOLATION;goto fail;}
    if(c->role==PST_CONNECTION_ROLE_CLIENT&&c->peer_authentication.certificate_mode!=PST_PEER_CERTIFICATE_DISABLED&&!c->peer_authentication.expected_peer_name_size){r=PST_RESULT_POLICY_VIOLATION;goto fail;}
    if(c->role==PST_CONNECTION_ROLE_SERVER&&c->peer_authentication.expected_peer_name_size){r=PST_RESULT_POLICY_VIOLATION;goto fail;}
    if(c->role==PST_CONNECTION_ROLE_SERVER&&!c->local_identity.credentials){r=PST_RESULT_POLICY_VIOLATION;goto fail;}
    if(c->peer_authentication.expected_peer_name_size){s->peer_name=string_copy(c->peer_authentication.expected_peer_name,c->peer_authentication.expected_peer_name_size);if(!s->peer_name){r=PST_RESULT_OUT_OF_MEMORY;goto fail;}if(memchr(s->peer_name,'\0',c->peer_authentication.expected_peer_name_size)){r=PST_RESULT_INVALID_ARGUMENT;goto fail;}s->public_config.peer_authentication.expected_peer_name=s->peer_name;}
    credentials_retain(c->local_identity.credentials);trust_retain(c->peer_authentication.trust);s->public_config.local_identity.credentials=c->local_identity.credentials;s->public_config.peer_authentication.trust=c->peer_authentication.trust;
    if((c->tls.minimum_version!=PST_TLS_VERSION_1_2&&c->tls.minimum_version!=PST_TLS_VERSION_1_3)||c->tls.maximum_version<c->tls.minimum_version||c->tls.maximum_version>PST_TLS_VERSION_1_3||!feature_ok(c->tls.resumption)||!feature_ok(c->tls.early_data)||c->tls.require_graceful_shutdown>1UL){r=PST_RESULT_INVALID_ARGUMENT;goto fail;}
    if(c->tls.early_data!=PST_FEATURE_DISABLED&&c->tls.resumption==PST_FEATURE_DISABLED){r=PST_RESULT_POLICY_VIOLATION;goto fail;}
    r=copy_alpn(s,&c->alpn);if(r!=PST_RESULT_OK)goto fail;
    required=c->provider_selection.required_capabilities|PST_CAP_NONBLOCKING|PST_CAP_BACKEND_WAIT;
    required|=c->role==PST_CONNECTION_ROLE_CLIENT?PST_CAP_ROLE_CLIENT:PST_CAP_ROLE_SERVER;
    if(c->local_identity.credentials)required|=PST_CAP_LOCAL_IDENTITY;
    if(c->peer_authentication.certificate_mode!=PST_PEER_CERTIFICATE_DISABLED)required|=PST_CAP_PEER_CERT_AUTH;
    if(c->peer_authentication.certificate_mode==PST_PEER_CERTIFICATE_OPTIONAL)required|=PST_CAP_PEER_CERT_OPTIONAL;
    if(c->peer_authentication.trust)required|=pst_trust_kind(c->peer_authentication.trust)==PST_TRUST_SOURCE_SYSTEM?PST_CAP_SYSTEM_TRUST:PST_CAP_CUSTOM_TRUST;
    if(c->peer_authentication.expected_peer_name_size)required|=PST_CAP_PEER_NAME_VERIFY;
    if(c->alpn.protocol_count)required|=c->role==PST_CONNECTION_ROLE_CLIENT?PST_CAP_ALPN_CLIENT:PST_CAP_ALPN_SERVER;
    if(c->tls.resumption==PST_FEATURE_REQUIRED)required|=PST_CAP_RESUMPTION;
    if(c->tls.early_data==PST_FEATURE_REQUIRED)required|=PST_CAP_EARLY_DATA;
    tls_caps=PST_CAP_TLS_1_2;if(c->tls.minimum_version==PST_TLS_VERSION_1_3)tls_caps=PST_CAP_TLS_1_3;else if(c->tls.maximum_version==PST_TLS_VERSION_1_3)tls_caps|=PST_CAP_TLS_1_3;
    s->required_capabilities=required;s->tls_capabilities=tls_caps;*out=s;return PST_RESULT_OK;
fail:pst_connection_config_snapshot_release(s);return r;
}
void pst_connection_config_snapshot_release(pst_connection_config_snapshot *s){pst_size i;if(!s)return;pst_credentials_release(s->public_config.local_identity.credentials);pst_trust_release(s->public_config.peer_authentication.trust);for(i=0;i<s->ordered_n;i++)free(s->ordered_ids[i]);free(s->ordered_ids);free(s->exact_id);free(s->peer_name);free(s->protocols);free(s->alpn_data);free(s->alpn_wire);free(s);}
const PST_CONNECTION_CONFIG *pst_connection_config_snapshot_public(const pst_connection_config_snapshot *s){return s?&s->public_config:NULL;}
pst_u32 pst_connection_config_required_capabilities(const pst_connection_config_snapshot *s){return s?s->required_capabilities:0UL;}
pst_u32 pst_connection_config_tls_capabilities(const pst_connection_config_snapshot *s){return s?s->tls_capabilities:0UL;}
const pst_credentials *pst_connection_config_local_credentials(const PST_CONNECTION_CONFIG *c){return c?c->local_identity.credentials:NULL;}
const pst_trust *pst_connection_config_peer_trust(const PST_CONNECTION_CONFIG *c){return c?c->peer_authentication.trust:NULL;}
const char *pst_connection_config_expected_peer_name(const PST_CONNECTION_CONFIG *c){return c?c->peer_authentication.expected_peer_name:NULL;}
pst_u32 pst_connection_config_peer_certificate_mode(const PST_CONNECTION_CONFIG *c){return c?c->peer_authentication.certificate_mode:PST_PEER_CERTIFICATE_DISABLED;}
pst_u32 pst_connection_config_minimum_version(const PST_CONNECTION_CONFIG *c){return c?c->tls.minimum_version:0UL;}
pst_u32 pst_connection_config_maximum_version(const PST_CONNECTION_CONFIG *c){return c?c->tls.maximum_version:0UL;}
const pst_u8 *pst_connection_config_alpn_wire(const PST_CONNECTION_CONFIG *c,pst_size *n){const pst_connection_config_snapshot *s=(const pst_connection_config_snapshot*)c;if(n)*n=s?s->alpn_wire_n:0;return s?s->alpn_wire:NULL;}
pst_u32 pst_connection_config_alpn_mode(const PST_CONNECTION_CONFIG *c){return c?c->alpn.mode:PST_FEATURE_DISABLED;}

PST_RESULT pst_peer_info_create_snapshot(const PST_PEER_INFO_SUMMARY *p,const pst_u8 *der,pst_peer_info **out){pst_peer_info *v;if(!p||!out||(p->leaf_der_size&&!der))return PST_RESULT_INVALID_ARGUMENT;*out=NULL;v=(pst_peer_info*)calloc(1,sizeof(*v));if(!v)return PST_RESULT_OUT_OF_MEMORY;v->summary=*p;if(p->leaf_der_size){v->der=bytes_copy(der,p->leaf_der_size);if(!v->der){free(v);return PST_RESULT_OUT_OF_MEMORY;}}*out=v;return PST_RESULT_OK;}
PST_RESULT PST_CALL pst_peer_info_get_summary(const pst_peer_info *v,PST_PEER_INFO_SUMMARY *p){pst_u32 n;if(!p||p->struct_size<PST_PEER_INFO_SUMMARY_MIN_SIZE)return PST_RESULT_INVALID_ARGUMENT;if(!version_ok(p->api_version))return PST_RESULT_INCOMPATIBLE_API;n=p->struct_size;memset(p,0,sizeof(*p));p->struct_size=n;p->api_version=PST_API_VERSION;if(!v)return PST_RESULT_INVALID_ARGUMENT;*p=v->summary;p->struct_size=n;p->api_version=PST_API_VERSION;return PST_RESULT_OK;}
PST_RESULT PST_CALL pst_peer_info_copy_leaf_der(const pst_peer_info *v,pst_u8 *b,pst_size cap,pst_size *out){if(!out)return PST_RESULT_INVALID_ARGUMENT;*out=0;if(!v)return PST_RESULT_INVALID_ARGUMENT;*out=v->summary.leaf_der_size;if(cap<v->summary.leaf_der_size)return PST_RESULT_TRUNCATED;if(v->summary.leaf_der_size&&!b)return PST_RESULT_INVALID_ARGUMENT;if(v->summary.leaf_der_size)memcpy(b,v->der,v->summary.leaf_der_size);return PST_RESULT_OK;}
void PST_CALL pst_peer_info_release(pst_peer_info *v){if(v){free(v->der);free(v);}}
