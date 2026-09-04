#include "papinho_secure_transport.h"
#include "papinho_secure_transport_win32.h"
#include <stdio.h>
#include <string.h>
static PST_RESULT custom_trust_from_der(const pst_u8*ca_der,pst_size ca_size,pst_trust**out){PST_TRUST_SOURCE source;memset(&source,0,sizeof(source));source.struct_size=sizeof(source);source.api_version=PST_API_VERSION;source.kind=PST_TRUST_SOURCE_CUSTOM_CA_DER;source.data=ca_der;source.data_size=ca_size;return pst_trust_create(&source,out);}
int main(int argc,char**argv){pst_trust*trust=NULL;(void)argv;(void)pst_win32_register_builtin_providers();printf("Pass bytes loaded from a deployment CA DER to custom_trust_from_der; CUSTOM_TRUST never falls back to SYSTEM_TRUST.\n");if(argc==999)(void)custom_trust_from_der(NULL,0,&trust);pst_trust_release(trust);return 0;}