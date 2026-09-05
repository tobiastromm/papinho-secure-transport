/* SPDX-License-Identifier: MPL-2.0 */
#include "papinho_secure_transport.h"
#include "papinho_secure_transport_win32.h"
#include <stdio.h>
#include <string.h>
int main(void)
{
#ifdef PST_RELEASE_EXPECT_OPENSSL
    PST_RUNTIME_OPTIONS options;
    PST_RUNTIME_INFO info;
    pst_runtime *runtime;
#endif
    if (pst_api_version() != 0x00010300UL || pst_library_version() != 0x00000400UL) return 10;
    if (pst_win32_register_builtin_providers() != PST_RESULT_OK) return 11;
#ifdef PST_RELEASE_EXPECT_OPENSSL
    memset(&options, 0, sizeof(options)); options.struct_size = sizeof(options); options.api_version = PST_API_VERSION;
    options.selection = PST_BACKEND_SELECTION_AUTOMATIC; options.required_capabilities = PST_CAP_TLS_1_3 | PST_CAP_SYSTEM_TRUST; runtime = NULL;
    if (pst_runtime_create(&options, &runtime) != PST_RESULT_OK) return 12;
    memset(&info, 0, sizeof(info)); info.struct_size = sizeof(info); info.api_version = PST_API_VERSION;
    if (pst_runtime_get_info(runtime, &info) != PST_RESULT_OK || strcmp(info.backend_id, "openssl") != 0) { pst_runtime_release(runtime); return 13; }
    pst_runtime_release(runtime);
#endif
    printf("PACKAGE_CONSUMER API=1.3.0 LIBRARY=0.4.0 PASS\n");
    return 0;
}
