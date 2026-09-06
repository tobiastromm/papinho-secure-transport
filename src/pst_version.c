/* SPDX-License-Identifier: MPL-2.0 */
#include "pst_internal.h"
pst_u32 PST_CALL pst_api_version(void) { return (pst_u32)PST_API_VERSION; }
pst_u32 PST_CALL pst_library_version(void) { return (pst_u32)PST_LIBRARY_VERSION; }
PST_RESULT PST_CALL pst_version_info_init(PST_VERSION_INFO *i)
{
    if (i == NULL) return PST_RESULT_INVALID_ARGUMENT;
    i->struct_size = (pst_u32)sizeof(PST_VERSION_INFO);
    i->api_version = (pst_u32)PST_API_VERSION;
    i->api_major = i->api_minor = i->api_patch = 0UL;
    i->library_major = i->library_minor = i->library_patch = 0UL;
    return PST_RESULT_OK;
}
PST_RESULT PST_CALL pst_get_version(PST_VERSION_INFO *i)
{
    PST_RESULT r = pst_validate_public_struct(i, PST_VERSION_INFO_MIN_SIZE);
    if (r != PST_RESULT_OK) return r;
    i->api_major = PST_API_VERSION_MAJOR; i->api_minor = PST_API_VERSION_MINOR;
    i->api_patch = PST_API_VERSION_PATCH;
    i->library_major = PST_LIBRARY_VERSION_MAJOR;
    i->library_minor = PST_LIBRARY_VERSION_MINOR;
    i->library_patch = PST_LIBRARY_VERSION_PATCH;
    return PST_RESULT_OK;
}
