#ifndef PAPINHO_SECURE_TRANSPORT_WIN32_H
#define PAPINHO_SECURE_TRANSPORT_WIN32_H
#include "papinho_secure_transport.h"
#ifdef __cplusplus
extern "C" {
#endif
PST_API PST_RESULT PST_CALL pst_win32_register_retrozilla_nss(void);
PST_API PST_RESULT PST_CALL pst_win32_register_builtin_providers(void);
PST_API PST_RESULT PST_CALL pst_win32_socket_transport_create(pst_size native_socket, pst_transport **out_transport);
#ifdef __cplusplus
}
#endif
#endif
