#include "papinho_secure_transport.h"
const char *PST_CALL pst_result_string(PST_RESULT r)
{
    static const char *s[] = {
        "success", "invalid argument", "invalid state", "unsupported",
        "unavailable", "out of memory", "resource failure", "transport failure",
        "secure protocol failure", "authentication failure", "hostname mismatch",
        "policy violation", "backend failure", "truncated secure transport",
        "closed", "incompatible API version"
    };
    if (r < PST_RESULT_OK || r > PST_RESULT_INCOMPATIBLE_API)
        return "unknown result";
    return s[(unsigned int)r];
}
