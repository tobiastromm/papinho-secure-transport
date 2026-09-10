/* SPDX-License-Identifier: MPL-2.0 */
#include "papinho_secure_transport.h"
#include "pst_identity_internal.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x,n) if(!(x)){printf("test_lifecycle_ownership: FAIL %d\n",n);return n;}
static int rotation_cycle(pst_u8 tag)
{
 pst_u8 cert1[2]={1,2},key1[2]={3,4},ca1[2]={5,6};
 pst_u8 cert2[2]={7,8},key2[2]={9,10},ca2[2]={11,12};
 PST_DER_ITEM cert_item,ca_item;PST_CREDENTIAL_SOURCE cs;PST_TRUST_SOURCE ts;
 PST_CONNECTION_CONFIG c;pst_credentials *cred1=NULL,*cred2=NULL;pst_trust *trust1=NULL,*trust2=NULL;
 pst_connection_config_snapshot *old_snapshot=NULL,*new_snapshot=NULL;
 const PST_CONNECTION_CONFIG *old_config,*new_config;pst_size n;int ok=0;
 cert1[1]=tag;cert2[1]=(pst_u8)(tag+1);ca1[1]=(pst_u8)(tag+2);ca2[1]=(pst_u8)(tag+3);
 memset(&cs,0,sizeof(cs));cs.struct_size=sizeof(cs);cs.api_version=PST_API_VERSION;cs.kind=PST_CREDENTIAL_SOURCE_CERT_CHAIN_DER_PKCS8_DER;
 cert_item.data=cert1;cert_item.size=2;cs.certificate_chain=&cert_item;cs.certificate_count=1;cs.private_key_der=key1;cs.private_key_der_size=2;
 if(pst_credentials_create(&cs,&cred1)!=PST_RESULT_OK)goto done;
 cert_item.data=cert2;cs.private_key_der=key2;if(pst_credentials_create(&cs,&cred2)!=PST_RESULT_OK)goto done;
 memset(&ts,0,sizeof(ts));ts.struct_size=sizeof(ts);ts.api_version=PST_API_VERSION;ts.kind=PST_TRUST_SOURCE_CUSTOM_CA_DER;ca_item.size=2;ts.anchors=&ca_item;ts.anchor_count=1;
 ca_item.data=ca1;if(pst_trust_create(&ts,&trust1)!=PST_RESULT_OK)goto done;
 ca_item.data=ca2;if(pst_trust_create(&ts,&trust2)!=PST_RESULT_OK)goto done;
 memset(&c,0,sizeof(c));c.struct_size=sizeof(c);c.api_version=PST_API_VERSION;c.role=PST_CONNECTION_ROLE_CLIENT;
 c.provider_selection.struct_size=sizeof(c.provider_selection);c.provider_selection.api_version=PST_API_VERSION;c.provider_selection.mode=PST_BACKEND_SELECTION_AUTOMATIC;
 c.local_identity.struct_size=sizeof(c.local_identity);c.local_identity.api_version=PST_API_VERSION;c.local_identity.credentials=cred1;
 c.peer_authentication.struct_size=sizeof(c.peer_authentication);c.peer_authentication.api_version=PST_API_VERSION;c.peer_authentication.certificate_mode=PST_PEER_CERTIFICATE_REQUIRED;c.peer_authentication.trust=trust1;c.peer_authentication.expected_peer_name="localhost";c.peer_authentication.expected_peer_name_size=9;
 c.tls.struct_size=sizeof(c.tls);c.tls.api_version=PST_API_VERSION;c.tls.minimum_version=c.tls.maximum_version=PST_TLS_VERSION_1_2;c.alpn.struct_size=sizeof(c.alpn);c.alpn.api_version=PST_API_VERSION;
 if(pst_connection_config_snapshot_create(&c,&old_snapshot)!=PST_RESULT_OK)goto done;
 c.local_identity.credentials=cred2;c.peer_authentication.trust=trust2;
 if(pst_connection_config_snapshot_create(&c,&new_snapshot)!=PST_RESULT_OK)goto done;
 pst_credentials_release(cred1);cred1=NULL;pst_credentials_release(cred2);cred2=NULL;pst_trust_release(trust1);trust1=NULL;pst_trust_release(trust2);trust2=NULL;
 cert1[0]=cert2[0]=ca1[0]=ca2[0]=0;
 old_config=pst_connection_config_snapshot_public(old_snapshot);new_config=pst_connection_config_snapshot_public(new_snapshot);
 if(pst_credentials_certificate_der(old_config->local_identity.credentials,&n)[0]!=1||n!=2)goto done;
 if(pst_credentials_certificate_der(new_config->local_identity.credentials,&n)[0]!=7||n!=2)goto done;
 if(pst_trust_data(old_config->peer_authentication.trust,&n)[0]!=5||n!=2)goto done;
 if(pst_trust_data(new_config->peer_authentication.trust,&n)[0]!=11||n!=2)goto done;
 ok=1;
done:
 pst_connection_config_snapshot_release(old_snapshot);pst_connection_config_snapshot_release(new_snapshot);
 pst_credentials_release(cred1);pst_credentials_release(cred2);pst_trust_release(trust1);pst_trust_release(trust2);return ok;
}
int main(void){pst_u8 cert[2]={1,2},key[2]={3,4},ca[2]={5,6};PST_DER_ITEM cert_item,ca_item;PST_CREDENTIAL_SOURCE cs;PST_TRUST_SOURCE ts;PST_CONNECTION_CONFIG c;pst_credentials*credentials;pst_trust*trust;pst_connection_config_snapshot*s;const PST_CONNECTION_CONFIG*f;pst_size n;
 cert_item.data=cert;cert_item.size=2;memset(&cs,0,sizeof(cs));cs.struct_size=sizeof(cs);cs.api_version=PST_API_VERSION;cs.kind=PST_CREDENTIAL_SOURCE_CERT_CHAIN_DER_PKCS8_DER;cs.certificate_chain=&cert_item;cs.certificate_count=1;cs.private_key_der=key;cs.private_key_der_size=2;CHECK(pst_credentials_create(&cs,&credentials)==PST_RESULT_OK,1);ca_item.data=ca;ca_item.size=2;memset(&ts,0,sizeof(ts));ts.struct_size=sizeof(ts);ts.api_version=PST_API_VERSION;ts.kind=PST_TRUST_SOURCE_CUSTOM_CA_DER;ts.anchors=&ca_item;ts.anchor_count=1;CHECK(pst_trust_create(&ts,&trust)==PST_RESULT_OK,2);
 memset(&c,0,sizeof(c));c.struct_size=sizeof(c);c.api_version=PST_API_VERSION;c.role=PST_CONNECTION_ROLE_CLIENT;c.provider_selection.struct_size=sizeof(c.provider_selection);c.provider_selection.api_version=PST_API_VERSION;c.provider_selection.mode=PST_BACKEND_SELECTION_AUTOMATIC;c.local_identity.struct_size=sizeof(c.local_identity);c.local_identity.api_version=PST_API_VERSION;c.local_identity.credentials=credentials;c.peer_authentication.struct_size=sizeof(c.peer_authentication);c.peer_authentication.api_version=PST_API_VERSION;c.peer_authentication.certificate_mode=PST_PEER_CERTIFICATE_REQUIRED;c.peer_authentication.trust=trust;c.peer_authentication.expected_peer_name="localhost";c.peer_authentication.expected_peer_name_size=9;c.tls.struct_size=sizeof(c.tls);c.tls.api_version=PST_API_VERSION;c.tls.minimum_version=c.tls.maximum_version=PST_TLS_VERSION_1_2;c.alpn.struct_size=sizeof(c.alpn);c.alpn.api_version=PST_API_VERSION;
 CHECK(pst_connection_config_snapshot_create(&c,&s)==PST_RESULT_OK,3);pst_credentials_release(credentials);pst_trust_release(trust);cert[0]=9;key[0]=9;ca[0]=9;f=pst_connection_config_snapshot_public(s);CHECK(pst_credentials_certificate_der(f->local_identity.credentials,&n)[0]==1&&n==2,4);CHECK(pst_credentials_private_key_der(f->local_identity.credentials,&n)[0]==3&&n==2,5);CHECK(pst_trust_data(f->peer_authentication.trust,&n)[0]==5&&n==2,6);pst_connection_config_snapshot_release(s);
 for(n=0;n<50;n++)CHECK(rotation_cycle((pst_u8)n),7);
 printf("CONFIG_LIFETIME=PASS OWNED_COPIES=PASS PRIVATE_KEY_RELEASE_PATH=PASS\n");
 printf("IDENTITY_ROTATION_CYCLES=50 OLD_CONNECTION_IDENTITY_IMMUTABLE=PASS NEW_CONNECTION_USES_ROTATED_IDENTITY=PASS ROTATION_DOES_NOT_MUTATE_EXISTING_CONNECTION=PASS TRUST_POLICY_ISOLATION=PASS IDENTITY_SNAPSHOT_MUTATIONS=0\n");
 printf("test_lifecycle_ownership: PASS\n");return 0;}
