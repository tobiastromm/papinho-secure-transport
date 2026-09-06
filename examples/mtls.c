/* SPDX-License-Identifier: MPL-2.0 */
#include "papinho_secure_transport.h"
#include "papinho_secure_transport_win32.h"
#include <stdio.h>
#include <string.h>
static PST_RESULT credentials_from_der(const pst_u8*cert,pst_size cert_size,const pst_u8*key,pst_size key_size,pst_credentials**out){PST_CREDENTIAL_SOURCE source;memset(&source,0,sizeof(source));source.struct_size=sizeof(source);source.api_version=PST_API_VERSION;source.kind=PST_CREDENTIAL_SOURCE_CERT_DER_PKCS8_DER;source.certificate_der=cert;source.certificate_der_size=cert_size;source.private_key_der=key;source.private_key_der_size=key_size;return pst_credentials_create(&source,out);}
int main(int argc,char**argv){pst_credentials*credentials=NULL;(void)argv;(void)pst_win32_register_builtin_providers();printf("Load certificate DER and unencrypted PKCS#8 DER from controlled storage. Configure server trust and hostname separately; mTLS does not replace server authentication.\n");if(argc==999)(void)credentials_from_der(NULL,0,NULL,0,&credentials);pst_credentials_release(credentials);return 0;}