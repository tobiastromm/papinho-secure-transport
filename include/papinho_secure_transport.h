#ifndef PAPINHO_SECURE_TRANSPORT_H
#define PAPINHO_SECURE_TRANSPORT_H

#include <limits.h>
#include <stddef.h>

#if defined(_MSC_VER)
# if defined(PST_BUILD_DLL)
#  define PST_API __declspec(dllexport)
# elif defined(PST_USE_DLL)
#  define PST_API __declspec(dllimport)
# else
#  define PST_API
# endif
# define PST_CALL __cdecl
#else
# define PST_API
# define PST_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define PST_API_VERSION_MAJOR 1UL
#define PST_API_VERSION_MINOR 0UL
#define PST_API_VERSION_PATCH 0UL
#define PST_API_VERSION 0x00010000UL
#define PST_LIBRARY_VERSION_MAJOR 0UL
#define PST_LIBRARY_VERSION_MINOR 1UL
#define PST_LIBRARY_VERSION_PATCH 0UL
#define PST_LIBRARY_VERSION 0x00000100UL

typedef unsigned char pst_u8;
#if USHRT_MAX == 0xffffU
typedef unsigned short pst_u16;
#else
# error PapinhoSecureTransport requires a 16-bit unsigned integer type
#endif
#if UINT_MAX == 0xffffffffUL
typedef unsigned int pst_u32;
typedef signed int pst_i32;
#elif ULONG_MAX == 0xffffffffUL
typedef unsigned long pst_u32;
typedef signed long pst_i32;
#else
# error PapinhoSecureTransport requires a 32-bit integer type
#endif
typedef size_t pst_size;

typedef char pst_check_u8_is_1[(sizeof(pst_u8) == 1) ? 1 : -1];
typedef char pst_check_u16_is_2[(sizeof(pst_u16) == 2) ? 1 : -1];
typedef char pst_check_u32_is_4[(sizeof(pst_u32) == 4) ? 1 : -1];
typedef char pst_check_i32_is_4[(sizeof(pst_i32) == 4) ? 1 : -1];

typedef pst_i32 PST_RESULT;
#define PST_RESULT_OK                  ((PST_RESULT)0)
#define PST_RESULT_INVALID_ARGUMENT    ((PST_RESULT)1)
#define PST_RESULT_INVALID_STATE       ((PST_RESULT)2)
#define PST_RESULT_UNSUPPORTED         ((PST_RESULT)3)
#define PST_RESULT_UNAVAILABLE         ((PST_RESULT)4)
#define PST_RESULT_OUT_OF_MEMORY       ((PST_RESULT)5)
#define PST_RESULT_RESOURCE_FAILURE    ((PST_RESULT)6)
#define PST_RESULT_TRANSPORT_FAILURE   ((PST_RESULT)7)
#define PST_RESULT_PROTOCOL_FAILURE    ((PST_RESULT)8)
#define PST_RESULT_AUTH_FAILURE        ((PST_RESULT)9)
#define PST_RESULT_HOSTNAME_MISMATCH   ((PST_RESULT)10)
#define PST_RESULT_POLICY_VIOLATION    ((PST_RESULT)11)
#define PST_RESULT_BACKEND_FAILURE     ((PST_RESULT)12)
#define PST_RESULT_TRUNCATED           ((PST_RESULT)13)
#define PST_RESULT_CLOSED              ((PST_RESULT)14)
#define PST_RESULT_INCOMPATIBLE_API    ((PST_RESULT)15)

typedef struct pst_runtime pst_runtime;
typedef struct pst_config pst_config;
typedef struct pst_credentials pst_credentials;
typedef struct pst_trust pst_trust;
typedef struct pst_connection pst_connection;
typedef struct pst_peer_info pst_peer_info;

typedef struct PST_VERSION_INFO {
    pst_u32 struct_size;
    pst_u32 api_version;
    pst_u32 api_major;
    pst_u32 api_minor;
    pst_u32 api_patch;
    pst_u32 library_major;
    pst_u32 library_minor;
    pst_u32 library_patch;
} PST_VERSION_INFO;

#define PST_VERSION_INFO_MIN_SIZE ((pst_u32)sizeof(PST_VERSION_INFO))

PST_API pst_u32 PST_CALL pst_api_version(void);
PST_API pst_u32 PST_CALL pst_library_version(void);
PST_API PST_RESULT PST_CALL pst_version_info_init(PST_VERSION_INFO *info);
PST_API PST_RESULT PST_CALL pst_get_version(PST_VERSION_INFO *info);
PST_API const char *PST_CALL pst_result_string(PST_RESULT result);

#ifdef __cplusplus
}
#endif
#endif
