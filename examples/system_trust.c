/* SPDX-License-Identifier: MPL-2.0 */
#include "papinho_secure_transport.h"
#include "papinho_secure_transport_win32.h"
#include <stdio.h>
#include <string.h>
int main(void){PST_RUNTIME_OPTIONS options;pst_runtime*runtime=NULL;PST_RESULT result;if(pst_win32_register_builtin_providers()!=PST_RESULT_OK)return 1;memset(&options,0,sizeof(options));options.struct_size=sizeof(options);options.api_version=PST_API_VERSION;options.selection=PST_BACKEND_SELECTION_AUTOMATIC;options.required_capabilities=PST_CAP_TLS_1_2|PST_CAP_SYSTEM_TRUST|PST_CAP_HOSTNAME_VERIFY;result=pst_runtime_create(&options,&runtime);printf("SYSTEM_TRUST runtime result: %s\n",pst_result_string(result));pst_runtime_release(runtime);return result==PST_RESULT_OK?0:2;}