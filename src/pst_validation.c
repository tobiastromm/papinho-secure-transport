#include "pst_internal.h"
typedef struct pst_public_struct_header {
    pst_u32 struct_size;
    pst_u32 api_version;
} pst_public_struct_header;
PST_RESULT pst_validate_public_struct(const void *value, pst_u32 minimum_size)
{
    const pst_public_struct_header *header;
    pst_u32 major;
    if (value == NULL) return PST_RESULT_INVALID_ARGUMENT;
    header = (const pst_public_struct_header *)value;
    if (header->struct_size < minimum_size) return PST_RESULT_INVALID_ARGUMENT;
    major = (header->api_version >> 16) & 0xffffUL;
    if (major != PST_API_VERSION_MAJOR) return PST_RESULT_INCOMPATIBLE_API;
    return PST_RESULT_OK;
}
