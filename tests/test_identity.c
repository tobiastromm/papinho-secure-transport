#include "papinho_secure_transport.h"
#include "pst_identity_internal.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x,n) if(!(x))return(n)
int main(void)
{
 pst_u8 cert[3]={1,2,3},key[4]={4,5,6,7},ca[2]={8,9},copy[4];
 PST_CREDENTIAL_SOURCE cs;PST_TRUST_SOURCE ts;PST_IDENTITY_CONFIG id;
 PST_PEER_INFO_SUMMARY in,out;pst_credentials *c;pst_trust *t;pst_config *cfg;pst_peer_info *peer;pst_size n;
 memset(&cs,0,sizeof(cs));cs.struct_size=sizeof(cs);cs.api_version=PST_API_VERSION;cs.kind=PST_CREDENTIAL_SOURCE_CERT_DER_PKCS8_DER;cs.certificate_der=cert;cs.certificate_der_size=3;cs.private_key_der=key;cs.private_key_der_size=4;
 CHECK(pst_credentials_create(&cs,&c)==PST_RESULT_OK,1);key[0]=0;CHECK(pst_credentials_private_key_der(c,&n)[0]==4&&n==4,2);
 memset(&ts,0,sizeof(ts));ts.struct_size=sizeof(ts);ts.api_version=PST_API_VERSION;ts.kind=PST_TRUST_SOURCE_CUSTOM_CA_DER;ts.data=ca;ts.data_size=2;CHECK(pst_trust_create(&ts,&t)==PST_RESULT_OK,3);
 CHECK(pst_config_create(&cfg)==PST_RESULT_OK,4);memset(&id,0,sizeof(id));id.struct_size=sizeof(id);id.api_version=PST_API_VERSION;id.credentials=c;id.trust=t;id.expected_hostname="localhost";id.expected_hostname_size=9;id.require_peer_authentication=1;id.require_client_authentication=1;
 CHECK(pst_config_set_identity(cfg,&id)==PST_RESULT_OK,5);CHECK(pst_config_freeze(cfg)==PST_RESULT_OK,6);CHECK(pst_config_set_identity(cfg,&id)==PST_RESULT_INVALID_STATE,7);pst_credentials_release(c);pst_trust_release(t);
 memset(&in,0,sizeof(in));in.struct_size=sizeof(in);in.api_version=PST_API_VERSION;in.certificate_present=PST_KNOWN_TRUE;in.leaf_der_size=3;CHECK(pst_peer_info_create_snapshot(&in,cert,&peer)==PST_RESULT_OK,8);
 memset(&out,0,sizeof(out));out.struct_size=sizeof(out);out.api_version=PST_API_VERSION;CHECK(pst_peer_info_get_summary(peer,&out)==PST_RESULT_OK&&out.leaf_der_size==3,9);CHECK(pst_peer_info_copy_leaf_der(peer,copy,sizeof(copy),&n)==PST_RESULT_OK&&n==3&&!memcmp(copy,cert,3),10);
 pst_config_release(cfg);CHECK(pst_peer_info_copy_leaf_der(peer,copy,sizeof(copy),&n)==PST_RESULT_OK,11);pst_peer_info_release(peer);
 ts.kind=PST_TRUST_SOURCE_SYSTEM;ts.data=NULL;ts.data_size=0;CHECK(pst_trust_create(&ts,&t)==PST_RESULT_OK,12);pst_trust_release(t);
 cs.private_key_der=NULL;cs.private_key_der_size=0;CHECK(pst_credentials_create(&cs,&c)==PST_RESULT_INVALID_ARGUMENT,13);
 ts.kind=99;CHECK(pst_trust_create(&ts,&t)==PST_RESULT_UNSUPPORTED,14);
 CHECK(pst_config_create(&cfg)==PST_RESULT_OK,15);memset(&id,0,sizeof(id));id.struct_size=sizeof(id);id.api_version=PST_API_VERSION;id.require_peer_authentication=1;CHECK(pst_config_set_identity(cfg,&id)==PST_RESULT_OK,16);CHECK(pst_config_freeze(cfg)==PST_RESULT_POLICY_VIOLATION,17);pst_config_release(cfg);
 printf("test_identity: PASS\n");return 0;
}
