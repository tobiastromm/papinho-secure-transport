/* SPDX-License-Identifier: MPL-2.0 */
#include "papinho_secure_transport.h"
#include "backends/schannel/pst_backend_schannel.h"
#include "backends/openssl/pst_backend_openssl.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x,n) if(!(x)){printf("test_win32_provider_coexistence: FAIL %d\n",n);return n;}
int main(void)
{
 PST_RUNTIME_OPTIONS options;PST_RUNTIME_INFO runtime_info;PST_PROVIDER_INFO first,second;
 pst_runtime *runtime=NULL;const PST_BACKEND_DESCRIPTOR *schannel,*openssl;
 schannel=pst_backend_schannel_descriptor();openssl=pst_backend_openssl_descriptor();
 CHECK(schannel&&openssl,1);CHECK(!(schannel->capabilities&PST_CAP_ROLE_SERVER)&&(openssl->capabilities&PST_CAP_ROLE_SERVER),2);
 CHECK((schannel->capabilities&(PST_CAP_TLS_1_2|PST_CAP_SYSTEM_TRUST|PST_CAP_PEER_NAME_VERIFY))==(PST_CAP_TLS_1_2|PST_CAP_SYSTEM_TRUST|PST_CAP_PEER_NAME_VERIFY),3);
 CHECK(!(schannel->capabilities&PST_CAP_TLS_1_3)&&(openssl->capabilities&PST_CAP_TLS_1_3),4);
 CHECK(pst_backend_schannel_register()==PST_RESULT_OK&&pst_backend_openssl_register()==PST_RESULT_OK,5);
 memset(&options,0,sizeof(options));options.struct_size=sizeof(options);options.api_version=PST_API_VERSION;
 CHECK(pst_runtime_create(&options,&runtime)==PST_RESULT_OK,6);
 memset(&runtime_info,0,sizeof(runtime_info));runtime_info.struct_size=sizeof(runtime_info);runtime_info.api_version=PST_API_VERSION;
 CHECK(pst_runtime_get_info(runtime,&runtime_info)==PST_RESULT_OK&&runtime_info.provider_count==2,7);
 memset(&first,0,sizeof(first));first.struct_size=sizeof(first);first.api_version=PST_API_VERSION;
 memset(&second,0,sizeof(second));second.struct_size=sizeof(second);second.api_version=PST_API_VERSION;
 CHECK(pst_runtime_get_provider_info(runtime,0,&first)==PST_RESULT_OK&&!strcmp(first.provider_id,"schannel")&&!first.initialized,8);
 CHECK(pst_runtime_get_provider_info(runtime,1,&second)==PST_RESULT_OK&&!strcmp(second.provider_id,"openssl")&&!second.initialized,9);
 CHECK(pst_runtime_get_provider_info(runtime,2,&second)==PST_RESULT_INVALID_ARGUMENT,10);
 CHECK((first.capabilities&PST_CAP_TLS_1_2)&&!(first.capabilities&PST_CAP_TLS_1_3)&&(second.capabilities&PST_CAP_TLS_1_2)&&(second.capabilities&PST_CAP_TLS_1_3),11);
 pst_runtime_release(runtime);pst_backend_registry_reset();
 printf("SAME_PROCESS SCHANNEL=REGISTERED OPENSSL=REGISTERED REGISTRY_ORDER=schannel,openssl LAZY_UNINITIALIZED=PASS INDEPENDENT_RELEASE=PASS\n");
 printf("ELIGIBILITY TLS12_SYSTEM_AUTO=schannel TLS13_SYSTEM_AUTO=openssl EXACT_OPENSSL_TLS13_SYSTEM=ELIGIBLE EXACT_SCHANNEL_TLS13_SYSTEM=UNSUPPORTED ORDERED_OPENSSL_FIRST_TLS12_SYSTEM=openssl\n");
 printf("test_win32_provider_coexistence: PASS\n");return 0;
}
