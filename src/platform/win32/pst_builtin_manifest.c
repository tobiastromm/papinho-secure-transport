#include "papinho_secure_transport_win32.h"
#include "pst_backend.h"
#if defined(PST_BUILTIN_RETROZILLA_NSS)
#include "backends/nss/pst_backend_nss.h"
#endif
#if defined(PST_BUILTIN_SCHANNEL)
#include "backends/schannel/pst_backend_schannel.h"
#endif
#if defined(PST_BUILTIN_OPENSSL)
#include "backends/openssl/pst_backend_openssl.h"
#endif
PST_RESULT PST_CALL pst_win32_register_builtin_providers(void)
{
    const PST_BACKEND_DESCRIPTOR *descriptors[3];
    pst_size count = 0;
#if defined(PST_BUILTIN_SCHANNEL)
    descriptors[count++] = pst_backend_schannel_descriptor();
#endif
#if defined(PST_BUILTIN_OPENSSL)
    descriptors[count++] = pst_backend_openssl_descriptor();
#endif
#if defined(PST_BUILTIN_RETROZILLA_NSS)
    descriptors[count++] = pst_backend_nss_descriptor();
#endif
    return pst_backend_register_manifest(descriptors, count);
}