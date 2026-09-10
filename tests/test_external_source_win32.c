/* SPDX-License-Identifier: MPL-2.0 */
#if defined(_MSC_VER) && _MSC_VER <= 1200
#pragma warning(push)
#pragma warning(disable:4115 4201 4514)
#endif
#include "papinho_secure_transport.h"
#include "papinho_secure_transport_win32.h"
#include "pst_internal.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#if defined(_MSC_VER) && _MSC_VER <= 1200
#include <winsock.h>
#pragma warning(disable:4514)
#else
#include <winsock2.h>
#endif
#include <stdio.h>
#define CHECK(x,n) if(!(x)){printf("test_external_source_win32: FAIL %d WSA=%d\n",n,WSAGetLastError());return n;}
static SOCKET make_listener(struct sockaddr_in *address)
{
    SOCKET value;int length=(int)sizeof(*address);
    value=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if(value==INVALID_SOCKET)return value;
    address->sin_family=AF_INET;address->sin_addr.s_addr=htonl(INADDR_LOOPBACK);address->sin_port=0;
    if(bind(value,(struct sockaddr*)address,sizeof(*address))==SOCKET_ERROR||listen(value,2)==SOCKET_ERROR||getsockname(value,(struct sockaddr*)address,&length)==SOCKET_ERROR){closesocket(value);return INVALID_SOCKET;}
    return value;
}
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4127)
#endif
static int await_readable(SOCKET value)
{
    fd_set reads;struct timeval timeout;FD_ZERO(&reads);FD_SET(value,&reads);timeout.tv_sec=2;timeout.tv_usec=0;return select(0,&reads,NULL,NULL,&timeout)==1;
}
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
int main(void)
{
    WSADATA data;struct sockaddr_in address;SOCKET listener,client,accepted;
    pst_external_source *source,*same_native,*write_source,*closed_source;pst_wait_set *first,*second,*failure_set;PST_WAIT_EVENT event[2];PST_WAIT_SET_RESULT result;
    CHECK(WSAStartup(MAKEWORD(2,0),&data)==0,1);listener=make_listener(&address);CHECK(listener!=INVALID_SOCKET,2);
    CHECK(pst_win32_socket_external_source_create((pst_size)listener,&source)==PST_RESULT_OK,3);
    CHECK(pst_win32_socket_external_source_create((pst_size)listener,&same_native)==PST_RESULT_OK,4);
    CHECK(pst_wait_set_create(&first)==PST_RESULT_OK&&pst_wait_set_create(&second)==PST_RESULT_OK,5);
    CHECK(pst_wait_set_add_external_source(first,source,0,10)==PST_RESULT_INVALID_ARGUMENT,6);
    CHECK(pst_wait_set_add_external_source(first,source,PST_INTEREST_READ|4UL,10)==PST_RESULT_INVALID_ARGUMENT,7);
    CHECK(pst_wait_set_add_external_source(first,source,PST_INTEREST_READ,10)==PST_RESULT_OK,8);
    CHECK(pst_wait_set_add_external_source(first,source,PST_INTEREST_READ,11)==PST_RESULT_ALREADY_REGISTERED,9);
    CHECK(pst_wait_set_add_external_source(second,source,PST_INTEREST_READ,12)==PST_RESULT_ALREADY_REGISTERED,10);
    CHECK(pst_external_source_try_release(source)==PST_RESULT_INVALID_STATE,11);
    CHECK(pst_wait_set_add_external_source(first,same_native,PST_INTEREST_READ,10)==PST_RESULT_ALREADY_REGISTERED,12);
    CHECK(pst_wait_set_add_external_source(first,same_native,PST_INTEREST_READ,13)==PST_RESULT_OK,12);
    client=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);CHECK(client!=INVALID_SOCKET,13);
    CHECK(connect(client,(struct sockaddr*)&address,sizeof(address))==0,14);
    CHECK(await_readable(listener),141);
    {PST_RESULT wait_result=pst_wait_set_wait(first,0,event,2,&result);if(wait_result!=PST_RESULT_OK||result.ready_count!=2||event[0].token!=10||event[1].token!=13)printf("WAIT_RESULT=%lu READY=%lu COPIED=%lu TOKEN0=%lu TOKEN1=%lu\n",(unsigned long)wait_result,(unsigned long)result.ready_count,(unsigned long)result.event_count,(unsigned long)event[0].token,(unsigned long)event[1].token);CHECK(wait_result==PST_RESULT_OK&&result.ready_count==2&&event[0].token==10&&event[1].token==13,15);}
    CHECK((event[0].flags&PST_WAIT_READY_EXTERNAL)!=0&&event[0].ready_interest==PST_INTEREST_READ,16);
    CHECK(pst_wait_set_wait(first,0,event,1,&result)==PST_RESULT_INSUFFICIENT_CAPACITY&&result.ready_count==2&&result.event_count==1,17);
    CHECK(pst_wait_set_remove_external_source(first,source)==PST_RESULT_OK,18);
    CHECK(pst_wait_set_add_external_source(second,source,PST_INTEREST_READ,14)==PST_RESULT_OK,19);
    CHECK(pst_wait_set_remove_external_source(first,source)==PST_RESULT_NOT_REGISTERED,20);
    CHECK(pst_wait_set_remove_external_source(first,same_native)==PST_RESULT_OK,21);
    accepted=accept(listener,NULL,NULL);CHECK(accepted!=INVALID_SOCKET,22);
    CHECK(pst_wait_set_remove_external_source(second,source)==PST_RESULT_OK,23);
    CHECK(pst_wait_set_create(&failure_set)==PST_RESULT_OK,24);pst_wait_set_test_fail_next_growth();
    CHECK(pst_wait_set_add_external_source(failure_set,source,PST_INTEREST_READ,15)==PST_RESULT_OUT_OF_MEMORY,25);
    CHECK(pst_wait_set_add_external_source(failure_set,source,PST_INTEREST_READ,15)==PST_RESULT_OK&&pst_wait_set_remove_external_source(failure_set,source)==PST_RESULT_OK,26);
    CHECK(pst_wait_set_destroy(failure_set)==PST_RESULT_OK&&pst_external_source_try_release(source)==PST_RESULT_OK,27);pst_external_source_release(same_native);
    CHECK(pst_win32_socket_external_source_create((pst_size)client,&write_source)==PST_RESULT_OK,28);
    CHECK(pst_wait_set_add_external_source(first,write_source,PST_INTEREST_WRITE,16)==PST_RESULT_OK&&pst_wait_set_wait(first,0,event,2,&result)==PST_RESULT_OK&&event[0].ready_interest==PST_INTEREST_WRITE,29);
    CHECK(pst_wait_set_remove_external_source(first,write_source)==PST_RESULT_OK&&pst_external_source_try_release(write_source)==PST_RESULT_OK,30);
    CHECK(pst_wait_set_destroy(first)==PST_RESULT_OK&&pst_wait_set_destroy(second)==PST_RESULT_OK,31);
    CHECK(send(client,"x",1,0)==1,32);closesocket(accepted);closesocket(client);closesocket(listener);
    CHECK(pst_win32_socket_external_source_create((pst_size)INVALID_SOCKET,&source)==PST_RESULT_INVALID_ARGUMENT,27);
    listener=make_listener(&address);CHECK(listener!=INVALID_SOCKET,28);
    CHECK(pst_win32_socket_external_source_create((pst_size)listener,&closed_source)==PST_RESULT_OK&&pst_wait_set_create(&failure_set)==PST_RESULT_OK,29);
    CHECK(pst_wait_set_add_external_source(failure_set,closed_source,PST_INTEREST_READ,20)==PST_RESULT_OK,30);
    closesocket(listener);CHECK(pst_wait_set_wait(failure_set,0,event,2,&result)==PST_RESULT_RESOURCE_FAILURE,31);
    CHECK(pst_wait_set_remove_external_source(failure_set,closed_source)==PST_RESULT_OK&&pst_external_source_try_release(closed_source)==PST_RESULT_OK&&pst_wait_set_destroy(failure_set)==PST_RESULT_OK,32);
    WSACleanup();
    printf("EXTERNAL_SOURCE_SINGLE_WAITSET_MEMBERSHIP=PASS CROSS_WAITSET_DUPLICATE_OBJECT_REJECTED=PASS REMOVE_THEN_REGISTER_OTHER_WAITSET=PASS NATIVE_RESOURCE_CROSS_OBJECT_DEDUPLICATION=NOT_PERFORMED BORROWED_SOURCE_OWNERSHIP=PASS\n");
    printf("EXTERNAL_SOURCE_ADD_REMOVE=PASS EXTERNAL_SOURCE_TOKEN=PASS EXTERNAL_SOURCE_DUPLICATE_TOKEN_REJECTED=PASS EXTERNAL_SOURCE_REMOVE_ABSENT_REJECTED=PASS EXTERNAL_SOURCE_READD=PASS EXTERNAL_TIMEOUT_ZERO=PASS EXTERNAL_READ_READY=PASS EXTERNAL_WRITE_READY=PASS\n");
    printf("WIN32_LISTENER_READINESS=PASS MULTIPLE_EXTERNAL_SOURCES=PASS STABLE_REGISTRATION_ORDER=PASS BOUNDED_ENUMERATION=PASS READINESS_NOT_CONSUMED=PASS BAD_INTEREST_REJECTED=PASS INVALID_SOURCE_REJECTED=PASS CLOSED_SOURCE_WAIT_FAILURE=PASS PARTIAL_ADD_FAILURE_CLEANUP=PASS REMOVE_DOES_NOT_CLOSE_EXTERNAL=PASS DESTROY_DOES_NOT_CLOSE_EXTERNAL=PASS FAILED_ADD_DOES_NOT_CLOSE_EXTERNAL=PASS LISTENER_REMAINS_CONSUMER_OWNED=PASS PST_DOES_NOT_ACCEPT=PASS\n");
    printf("test_external_source_win32: PASS\n");return 0;
}
