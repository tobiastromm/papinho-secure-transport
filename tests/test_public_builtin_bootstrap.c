/* SPDX-License-Identifier: MPL-2.0 */
#include "papinho_secure_transport.h"
#include "papinho_secure_transport_win32.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) if(!(x)){printf("test_public_builtin_bootstrap: FAIL line %d\n",__LINE__);return 1;}
int main(void){PST_RUNTIME_OPTIONS options;PST_RUNTIME_INFO info;pst_runtime*runtime=NULL;CHECK(pst_win32_register_builtin_providers()==PST_RESULT_OK);CHECK(pst_win32_register_builtin_providers()==PST_RESULT_OK);memset(&options,0,sizeof(options));options.struct_size=sizeof(options);options.api_version=PST_API_VERSION;CHECK(pst_runtime_create(&options,&runtime)==PST_RESULT_OK&&runtime);memset(&info,0,sizeof(info));info.struct_size=sizeof(info);info.api_version=PST_API_VERSION;CHECK(pst_runtime_get_info(runtime,&info)==PST_RESULT_OK&&info.provider_count>0);CHECK(pst_win32_register_builtin_providers()==PST_RESULT_INVALID_STATE);pst_runtime_release(runtime);printf("PUBLIC_BUILTIN_BOOTSTRAP=PASS REPEAT=OK RUNTIME_SEALS=PASS\n");return 0;}
