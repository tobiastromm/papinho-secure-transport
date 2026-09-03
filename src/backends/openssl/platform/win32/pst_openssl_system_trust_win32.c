#include "backends/openssl/platform/win32/pst_openssl_system_trust_win32.h"
#include <windows.h>
#include <wincrypt.h>
#include <string.h>
pst_u32 pst_ossl_windows_system_trust_evaluate(const pst_u8 *leaf_der,pst_size leaf_der_size,const pst_ossl_windows_certificate *intermediates,pst_size intermediate_count,pst_ossl_windows_trust_detail *detail)
{
    PCCERT_CONTEXT leaf=NULL;PCCERT_CHAIN_CONTEXT chain=NULL;HCERTSTORE memory_store=NULL;CERT_CHAIN_PARA chain_parameters;CERT_CHAIN_POLICY_PARA policy_parameters;CERT_CHAIN_POLICY_STATUS policy_status;LPSTR server_auth_oid=(LPSTR)szOID_PKIX_KP_SERVER_AUTH;DWORD flags;pst_size i;BOOL policy_result;pst_u32 result=PST_OSSL_WINDOWS_TRUST_INTERNAL_FAILURE;
    if(detail)memset(detail,0,sizeof(*detail));
    if(!leaf_der||!leaf_der_size||leaf_der_size>(pst_size)0xffffffffUL||(!intermediates&&intermediate_count))return PST_OSSL_WINDOWS_TRUST_CERTIFICATE_REJECTED;
    leaf=CertCreateCertificateContext(X509_ASN_ENCODING|PKCS_7_ASN_ENCODING,leaf_der,(DWORD)leaf_der_size);if(!leaf)return PST_OSSL_WINDOWS_TRUST_CERTIFICATE_REJECTED;
    memory_store=CertOpenStore(CERT_STORE_PROV_MEMORY,0,0,CERT_STORE_CREATE_NEW_FLAG,NULL);if(!memory_store)goto cleanup;
    for(i=0;i<intermediate_count;i++){if(!intermediates[i].der||!intermediates[i].der_size||intermediates[i].der_size>(pst_size)0xffffffffUL){result=PST_OSSL_WINDOWS_TRUST_CERTIFICATE_REJECTED;goto cleanup;}if(!CertAddEncodedCertificateToStore(memory_store,X509_ASN_ENCODING|PKCS_7_ASN_ENCODING,intermediates[i].der,(DWORD)intermediates[i].der_size,CERT_STORE_ADD_ALWAYS,NULL)){result=PST_OSSL_WINDOWS_TRUST_CERTIFICATE_REJECTED;goto cleanup;}}
    memset(&chain_parameters,0,sizeof(chain_parameters));chain_parameters.cbSize=sizeof(chain_parameters);chain_parameters.RequestedUsage.dwType=USAGE_MATCH_TYPE_AND;chain_parameters.RequestedUsage.Usage.cUsageIdentifier=1;chain_parameters.RequestedUsage.Usage.rgpszUsageIdentifier=&server_auth_oid;
    flags=CERT_CHAIN_DISABLE_AIA|CERT_CHAIN_CACHE_ONLY_URL_RETRIEVAL|CERT_CHAIN_DISABLE_AUTH_ROOT_AUTO_UPDATE;
    if(!CertGetCertificateChain(HCCE_CURRENT_USER,leaf,NULL,memory_store,&chain_parameters,flags,NULL,&chain))goto cleanup;
    if(detail){detail->chain_api_succeeded=1;detail->chain_error_present=chain->TrustStatus.dwErrorStatus?1UL:0UL;detail->explicit_distrust=(chain->TrustStatus.dwErrorStatus&CERT_TRUST_IS_EXPLICIT_DISTRUST)?1UL:0UL;}
    memset(&policy_parameters,0,sizeof(policy_parameters));policy_parameters.cbSize=sizeof(policy_parameters);memset(&policy_status,0,sizeof(policy_status));policy_status.cbSize=sizeof(policy_status);
    policy_result=CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_BASE,chain,&policy_parameters,&policy_status);if(detail){detail->policy_api_succeeded=policy_result?1UL:0UL;detail->policy_error_present=policy_status.dwError?1UL:0UL;}
    if(!policy_result)goto cleanup;if(chain->TrustStatus.dwErrorStatus||policy_status.dwError)result=PST_OSSL_WINDOWS_TRUST_CERTIFICATE_REJECTED;else result=PST_OSSL_WINDOWS_TRUST_OK;
cleanup:if(chain)CertFreeCertificateChain(chain);if(memory_store)CertCloseStore(memory_store,0);if(leaf)CertFreeCertificateContext(leaf);return result;
}