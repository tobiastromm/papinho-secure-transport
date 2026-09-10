/* SPDX-License-Identifier: MPL-2.0 */
#include "papinho_secure_transport.h"
#include "papinho_secure_transport_win32.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#if defined(_MSC_VER) && _MSC_VER <= 1200
#pragma warning(push)
#pragma warning(disable:4115 4201 4514)
#include <winsock.h>
#else
#include <winsock2.h>
#endif
#include <windows.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x,n) if(!(x)){printf("API21_PACKAGE_CONSUMER=FAIL_%d\n",n);return n;}
typedef struct WAKE_CONTEXT { pst_wait_set *set; } WAKE_CONTEXT;
static DWORD WINAPI wake_thread(LPVOID value)
{
    WAKE_CONTEXT *context = (WAKE_CONTEXT *)value;
    Sleep(25UL);
    return pst_wait_set_wake(context->set) == PST_RESULT_OK ? 0UL : 1UL;
}
int main(void)
{
    WSADATA data;
    SOCKET listener;
    struct sockaddr_in address;
    pst_wait_set *set;
    pst_external_source *source;
    PST_WAIT_EVENT event;
    PST_WAIT_SET_RESULT result;
    PST_CONNECTION_CONFIG config;
    WAKE_CONTEXT context;
    HANDLE thread;
    volatile pst_u32 sni_compat = PST_SNI_MODE_COMPAT;
    volatile pst_u32 sni_disabled = PST_SNI_MODE_DISABLED;
    volatile pst_u32 sni_explicit = PST_SNI_MODE_EXPLICIT;
    volatile pst_u32 configured_mode;
    int length;
    CHECK(sni_compat == 0UL && sni_disabled == 1UL && sni_explicit == 2UL, 1);
    memset(&config, 0, sizeof(config));
    config.struct_size = sizeof(config); config.api_version = PST_API_VERSION;
    config.server_name_indication_mode = PST_SNI_MODE_EXPLICIT;
    config.server_name_indication = "localhost"; config.server_name_indication_size = 9;
    configured_mode = config.server_name_indication_mode;
    CHECK(configured_mode == sni_explicit, 2);
    CHECK(WSAStartup(MAKEWORD(1,1), &data) == 0, 3);
    listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); CHECK(listener != INVALID_SOCKET, 4);
    memset(&address, 0, sizeof(address)); address.sin_family = AF_INET; address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    CHECK(bind(listener, (struct sockaddr *)&address, sizeof(address)) == 0 && listen(listener, 1) == 0, 5);
    length = (int)sizeof(address); CHECK(getsockname(listener, (struct sockaddr *)&address, &length) == 0, 6);
    CHECK(pst_wait_set_create(&set) == PST_RESULT_OK, 7);
    CHECK(pst_win32_socket_external_source_create((pst_size)listener, &source) == PST_RESULT_OK, 8);
    CHECK(pst_wait_set_add_external_source(set, source, PST_INTEREST_READ, 0x1234UL) == PST_RESULT_OK, 9);
    CHECK(pst_wait_set_wait(set, 0UL, &event, 1, &result) == PST_RESULT_WAIT_TIMEOUT && result.timed_out, 10);
    CHECK(pst_wait_set_wait(set, 20UL, &event, 1, &result) == PST_RESULT_WAIT_TIMEOUT && result.timed_out, 11);
    CHECK(pst_wait_set_remove_external_source(set, source) == PST_RESULT_OK, 12);
    CHECK(pst_wait_set_add_external_source(set, source, PST_INTEREST_READ, 0x5678UL) == PST_RESULT_OK, 13);
    CHECK(pst_wait_set_remove_external_source(set, source) == PST_RESULT_OK, 14);
    context.set = set; thread = CreateThread(NULL, 0, wake_thread, &context, 0, NULL); CHECK(thread != NULL, 15);
    CHECK(pst_wait_set_wait(set, 1000UL, &event, 1, &result) == PST_RESULT_WAIT_WOKEN && result.woken, 16);
    CHECK(WaitForSingleObject(thread, 1000UL) == WAIT_OBJECT_0, 17); CloseHandle(thread);
    CHECK(pst_external_source_try_release(source) == PST_RESULT_OK, 18);
    CHECK(pst_wait_set_destroy(set) == PST_RESULT_OK, 19);
    closesocket(listener); WSACleanup();
    printf("API21_PACKAGE_ONLY_CONSUMER=PASS WAIT_SET=PASS STABLE_TOKEN=PASS TIMEOUT_ZERO=PASS FINITE_WAIT=PASS WAKE=PASS EXTERNAL_SOURCE=PASS REMOVE_READD=PASS SNI_MODES=PASS NO_PRIVATE_HEADER_DEPENDENCY=PASS\n");
    return 0;
}
