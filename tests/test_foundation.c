#include "papinho_secure_transport.h"
#include <stdio.h>
#include <string.h>
typedef struct larger_info { PST_VERSION_INFO base; pst_u32 extra; } larger_info;
int main(void)
{
    PST_VERSION_INFO i;
    larger_info l;
    PST_RESULT r;
    if (pst_version_info_init(NULL)!=PST_RESULT_INVALID_ARGUMENT) return 2;
    if (pst_get_version(NULL)!=PST_RESULT_INVALID_ARGUMENT) return 3;
    memset(&i,0,sizeof(i)); if (pst_version_info_init(&i)!=PST_RESULT_OK) return 4;
    if (pst_get_version(&i)!=PST_RESULT_OK) return 5;
    if (i.api_major!=1UL || i.api_minor!=3UL || i.api_patch!=0UL ||
        i.library_major!=0UL || i.library_minor!=3UL || i.library_patch!=0UL)
        return 12;
    if (pst_api_version()!=PST_API_VERSION || pst_library_version()!=PST_LIBRARY_VERSION) return 6;
    i.struct_size=PST_VERSION_INFO_MIN_SIZE-1UL;
    if (pst_get_version(&i)!=PST_RESULT_INVALID_ARGUMENT) return 7;
    i.struct_size=PST_VERSION_INFO_MIN_SIZE; i.api_version=0x00020000UL;
    if (pst_get_version(&i)!=PST_RESULT_INCOMPATIBLE_API) return 8;
    memset(&l,0,sizeof(l)); l.base.struct_size=(pst_u32)sizeof(l);
    l.base.api_version=PST_API_VERSION; l.extra=0xa5a5a5a5UL;
    if (pst_get_version(&l.base)!=PST_RESULT_OK || l.extra!=0xa5a5a5a5UL) return 9;
    for (r=PST_RESULT_OK;r<=PST_RESULT_INCOMPATIBLE_API;++r)
        if (!strcmp(pst_result_string(r),"unknown result")) return 10;
    if (strcmp(pst_result_string((PST_RESULT)9999),"unknown result")) return 11;
    printf("test_foundation: PASS\n"); return 0;
}
