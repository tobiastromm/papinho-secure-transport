/* SPDX-License-Identifier: MPL-2.0 */
#include "papinho_secure_transport.h"
#include "pst_identity_internal.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x,n) if(!(x))return(n)
static void config_init(PST_CONNECTION_CONFIG *c,pst_credentials *credentials,pst_trust *trust,const PST_ALPN_PROTOCOL *alpn)
{
 memset(c,0,sizeof(*c));c->struct_size=sizeof(*c);c->api_version=PST_API_VERSION;c->role=PST_CONNECTION_ROLE_CLIENT;
 c->provider_selection.struct_size=sizeof(c->provider_selection);c->provider_selection.api_version=PST_API_VERSION;c->provider_selection.mode=PST_BACKEND_SELECTION_AUTOMATIC;
 c->local_identity.struct_size=sizeof(c->local_identity);c->local_identity.api_version=PST_API_VERSION;c->local_identity.credentials=credentials;
 c->peer_authentication.struct_size=sizeof(c->peer_authentication);c->peer_authentication.api_version=PST_API_VERSION;c->peer_authentication.certificate_mode=PST_PEER_CERTIFICATE_REQUIRED;c->peer_authentication.trust=trust;c->peer_authentication.expected_peer_name="localhost";c->peer_authentication.expected_peer_name_size=9;
 c->tls.struct_size=sizeof(c->tls);c->tls.api_version=PST_API_VERSION;c->tls.minimum_version=PST_TLS_VERSION_1_2;c->tls.maximum_version=PST_TLS_VERSION_1_3;
 c->alpn.struct_size=sizeof(c->alpn);c->alpn.api_version=PST_API_VERSION;c->alpn.mode=alpn?PST_FEATURE_REQUIRED:PST_FEATURE_DISABLED;c->alpn.protocols=alpn;c->alpn.protocol_count=alpn?1:0;
}
int main(void)
{
 pst_u8 cert[3]={1,2,3},key[4]={4,5,6,7},ca[2]={8,9},copy[4],proto[3]={'p','s','t'};
 PST_DER_ITEM cert_item,ca_item;PST_ALPN_PROTOCOL alpn;PST_CREDENTIAL_SOURCE cs;PST_TRUST_SOURCE ts;PST_CONNECTION_CONFIG config;
 PST_PEER_INFO_SUMMARY in,out;pst_credentials *credentials;pst_trust *trust;pst_connection_config_snapshot *snapshot;pst_peer_info *peer;pst_size n;
 cert_item.data=cert;cert_item.size=3;memset(&cs,0,sizeof(cs));cs.struct_size=sizeof(cs);cs.api_version=PST_API_VERSION;cs.kind=PST_CREDENTIAL_SOURCE_CERT_CHAIN_DER_PKCS8_DER;cs.certificate_chain=&cert_item;cs.certificate_count=1;cs.private_key_der=key;cs.private_key_der_size=4;
 CHECK(pst_credentials_create(&cs,&credentials)==PST_RESULT_OK,1);key[0]=0;CHECK(pst_credentials_private_key_der(credentials,&n)[0]==4&&n==4,2);
 ca_item.data=ca;ca_item.size=2;memset(&ts,0,sizeof(ts));ts.struct_size=sizeof(ts);ts.api_version=PST_API_VERSION;ts.kind=PST_TRUST_SOURCE_CUSTOM_CA_DER;ts.anchors=&ca_item;ts.anchor_count=1;CHECK(pst_trust_create(&ts,&trust)==PST_RESULT_OK,3);
 alpn.data=proto;alpn.size=3;config_init(&config,credentials,trust,&alpn);CHECK(pst_connection_config_snapshot_create(&config,&snapshot)==PST_RESULT_OK,4);proto[0]='x';CHECK(pst_connection_config_alpn_wire(pst_connection_config_snapshot_public(snapshot),&n)[1]=='p',5);pst_connection_config_snapshot_release(snapshot);
 config_init(&config,credentials,trust,NULL);config.tls.require_graceful_shutdown=PST_FEATURE_DISABLED;CHECK(pst_connection_config_snapshot_create(&config,&snapshot)==PST_RESULT_OK&&!(pst_connection_config_required_capabilities(snapshot)&PST_CAP_GRACEFUL_SHUTDOWN),13);pst_connection_config_snapshot_release(snapshot);
 config.tls.require_graceful_shutdown=PST_FEATURE_OPTIONAL;CHECK(pst_connection_config_snapshot_create(&config,&snapshot)==PST_RESULT_OK&&!(pst_connection_config_required_capabilities(snapshot)&PST_CAP_GRACEFUL_SHUTDOWN),14);pst_connection_config_snapshot_release(snapshot);
 config.tls.require_graceful_shutdown=PST_FEATURE_REQUIRED;CHECK(pst_connection_config_snapshot_create(&config,&snapshot)==PST_RESULT_OK&&(pst_connection_config_required_capabilities(snapshot)&PST_CAP_GRACEFUL_SHUTDOWN),15);pst_connection_config_snapshot_release(snapshot);
 config.tls.require_graceful_shutdown=PST_FEATURE_REQUIRED+1UL;CHECK(pst_connection_config_snapshot_create(&config,&snapshot)==PST_RESULT_INVALID_ARGUMENT,16);
 memset(&in,0,sizeof(in));in.struct_size=sizeof(in);in.api_version=PST_API_VERSION;in.local_role=PST_CONNECTION_ROLE_CLIENT;in.certificate_present=PST_KNOWN_TRUE;in.leaf_der_size=3;CHECK(pst_peer_info_create_snapshot(&in,cert,&peer)==PST_RESULT_OK,6);
 memset(&out,0,sizeof(out));out.struct_size=sizeof(out);out.api_version=PST_API_VERSION;CHECK(pst_peer_info_get_summary(peer,&out)==PST_RESULT_OK&&out.leaf_der_size==3,7);CHECK(pst_peer_info_copy_leaf_der(peer,copy,sizeof(copy),&n)==PST_RESULT_OK&&n==3&&!memcmp(copy,cert,3),8);pst_peer_info_release(peer);
 pst_credentials_release(credentials);pst_trust_release(trust);
 ts.kind=PST_TRUST_SOURCE_SYSTEM;ts.anchors=NULL;ts.anchor_count=0;CHECK(pst_trust_create(&ts,&trust)==PST_RESULT_OK,9);pst_trust_release(trust);
 cs.private_key_der=NULL;cs.private_key_der_size=0;CHECK(pst_credentials_create(&cs,&credentials)==PST_RESULT_INVALID_ARGUMENT,10);
 ts.kind=99;CHECK(pst_trust_create(&ts,&trust)==PST_RESULT_UNSUPPORTED,11);
 config_init(&config,NULL,NULL,NULL);CHECK(pst_connection_config_snapshot_create(&config,&snapshot)==PST_RESULT_POLICY_VIOLATION,12);
 printf("test_identity: PASS\n");return 0;
}
