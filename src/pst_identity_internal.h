#ifndef PST_IDENTITY_INTERNAL_H
#define PST_IDENTITY_INTERNAL_H
#include "papinho_secure_transport.h"
pst_u32 pst_credentials_kind(const pst_credentials *v);
const pst_u8 *pst_credentials_certificate_der(const pst_credentials *v, pst_size *n);
const pst_u8 *pst_credentials_private_key_der(const pst_credentials *v, pst_size *n);
pst_u32 pst_trust_kind(const pst_trust *v);
const pst_u8 *pst_trust_data(const pst_trust *v, pst_size *n);
const pst_credentials *pst_config_credentials(const pst_config *v);
const pst_trust *pst_config_trust(const pst_config *v);
const char *pst_config_expected_hostname(const pst_config *v);
pst_u32 pst_config_require_peer_authentication(const pst_config *v);
pst_u32 pst_config_require_client_authentication(const pst_config *v);
int pst_config_is_frozen(const pst_config *v);
void pst_config_retain(pst_config *v);
pst_u32 pst_config_minimum_version(const pst_config *v);
pst_u32 pst_config_maximum_version(const pst_config *v);
const pst_u8 *pst_config_alpn_wire(const pst_config *v,pst_size *n);
pst_u32 pst_config_alpn_requirement(const pst_config *v);
pst_u32 pst_config_required_capabilities(const pst_config *v);
PST_RESULT pst_peer_info_create_snapshot(const PST_PEER_INFO_SUMMARY *s,
 const pst_u8 *der, pst_peer_info **out);
#endif
