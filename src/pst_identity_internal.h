/* SPDX-License-Identifier: MPL-2.0 */
#ifndef PST_IDENTITY_INTERNAL_H
#define PST_IDENTITY_INTERNAL_H
#include "papinho_secure_transport.h"
typedef struct pst_connection_config_snapshot pst_connection_config_snapshot;
pst_u32 pst_credentials_kind(const pst_credentials *);
pst_size pst_credentials_certificate_count(const pst_credentials *);
const pst_u8 *pst_credentials_certificate_der(const pst_credentials *,pst_size *);
const pst_u8 *pst_credentials_certificate_at(const pst_credentials *,pst_size,pst_size *);
const pst_u8 *pst_credentials_private_key_der(const pst_credentials *,pst_size *);
pst_u32 pst_trust_kind(const pst_trust *);
pst_size pst_trust_anchor_count(const pst_trust *);
const pst_u8 *pst_trust_data(const pst_trust *,pst_size *);
const pst_u8 *pst_trust_anchor_at(const pst_trust *,pst_size,pst_size *);
PST_RESULT pst_connection_config_snapshot_create(const PST_CONNECTION_CONFIG *,pst_connection_config_snapshot **);
void pst_connection_config_snapshot_release(pst_connection_config_snapshot *);
const PST_CONNECTION_CONFIG *pst_connection_config_snapshot_public(const pst_connection_config_snapshot *);
pst_u32 pst_connection_config_required_capabilities(const pst_connection_config_snapshot *);
pst_u32 pst_connection_config_tls_capabilities(const pst_connection_config_snapshot *);
const pst_credentials *pst_connection_config_local_credentials(const PST_CONNECTION_CONFIG *);
const pst_trust *pst_connection_config_peer_trust(const PST_CONNECTION_CONFIG *);
const char *pst_connection_config_expected_peer_name(const PST_CONNECTION_CONFIG *);
const char *pst_connection_config_server_name_indication(const PST_CONNECTION_CONFIG *);
pst_u32 pst_connection_config_server_name_indication_mode(const PST_CONNECTION_CONFIG *);
pst_u32 pst_connection_config_peer_certificate_mode(const PST_CONNECTION_CONFIG *);
pst_u32 pst_connection_config_minimum_version(const PST_CONNECTION_CONFIG *);
pst_u32 pst_connection_config_maximum_version(const PST_CONNECTION_CONFIG *);
const pst_u8 *pst_connection_config_alpn_wire(const PST_CONNECTION_CONFIG *,pst_size *);
pst_u32 pst_connection_config_alpn_mode(const PST_CONNECTION_CONFIG *);
PST_RESULT pst_peer_info_create_snapshot(const PST_PEER_INFO_SUMMARY *,const pst_u8 *,pst_peer_info **);
#endif
