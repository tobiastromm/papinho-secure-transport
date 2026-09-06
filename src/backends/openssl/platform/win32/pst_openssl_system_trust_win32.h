/* SPDX-License-Identifier: MPL-2.0 */
#ifndef PST_OPENSSL_SYSTEM_TRUST_WIN32_H
#define PST_OPENSSL_SYSTEM_TRUST_WIN32_H
#include "papinho_secure_transport.h"
#define PST_OSSL_WINDOWS_TRUST_OK 0UL
#define PST_OSSL_WINDOWS_TRUST_CERTIFICATE_REJECTED 1UL
#define PST_OSSL_WINDOWS_TRUST_INTERNAL_FAILURE 2UL
typedef struct pst_ossl_windows_certificate { const pst_u8 *der; pst_size der_size; } pst_ossl_windows_certificate;
typedef struct pst_ossl_windows_trust_detail { pst_u32 chain_api_succeeded; pst_u32 chain_error_present; pst_u32 policy_api_succeeded; pst_u32 policy_error_present; pst_u32 explicit_distrust; } pst_ossl_windows_trust_detail;
pst_u32 pst_ossl_windows_system_trust_evaluate(const pst_u8 *leaf_der,pst_size leaf_der_size,const pst_ossl_windows_certificate *intermediates,pst_size intermediate_count,pst_ossl_windows_trust_detail *detail);
#endif