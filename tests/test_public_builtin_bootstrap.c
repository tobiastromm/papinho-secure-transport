#include "papinho_secure_transport.h"
#include "papinho_secure_transport_win32.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) if(!(x)){printf("test_public_builtin_bootstrap: FAIL line %d\n",__LINE__);return 1;}
int main(void){PST_RUNTIME_OPTIONS options;pst_runtime*runtime=NULL;CHECK(pst_win32_register_builtin_providers()==PST_RESULT_OK);CHECK(pst_win32_register_builtin_providers()==PST_RESULT_OK);memset(&options,0,sizeof(options));options.struct_size=sizeof(options);options.api_version=PST_API_VERSION;options.selection=PST_BACKEND_SELECTION_EXACT;options.exact_backend_id="not-built-in";CHECK(pst_runtime_create(&options,&runtime)==PST_RESULT_UNSUPPORTED&&!runtime);CHECK(pst_win32_register_builtin_providers()==PST_RESULT_INVALID_STATE);printf("PUBLIC_BUILTIN_BOOTSTRAP=PASS REPEAT=OK FAILED_RUNTIME_SEALS=PASS\n");return 0;}