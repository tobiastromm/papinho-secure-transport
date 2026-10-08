/* SPDX-License-Identifier: MPL-2.0 */
#include <winsock2.h>
#include <windows.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int connect_local(unsigned short port)
{
    SOCKET socket_value;
    struct sockaddr_in address;
    socket_value=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(socket_value==INVALID_SOCKET)return -1;
    memset(&address,0,sizeof(address));
    address.sin_family=AF_INET;
    address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    address.sin_port=htons(port);
    if(connect(socket_value,(struct sockaddr*)&address,sizeof(address))!=0){closesocket(socket_value);return -1;}
    return (int)socket_value;
}

int main(int argc,char **argv)
{
    WSADATA winsock;SSL_CTX *context;SSL *ssl;SOCKET socket_value;
    unsigned char byte;size_t amount;unsigned long queue_error;
    int ssl_result,ssl_error,wsa_error,socket_error,socket_error_size;
    if(argc!=3){fprintf(stderr,"usage: port clean|fin|rst\n");return 2;}
    if(WSAStartup(MAKEWORD(2,2),&winsock)!=0)return 3;
    context=SSL_CTX_new(TLS_client_method());if(!context){WSACleanup();return 4;}
    SSL_CTX_set_verify(context,SSL_VERIFY_NONE,NULL);
    if(!SSL_CTX_set_min_proto_version(context,TLS1_3_VERSION)||!SSL_CTX_set_max_proto_version(context,TLS1_3_VERSION)){SSL_CTX_free(context);WSACleanup();return 5;}
    socket_value=(SOCKET)connect_local((unsigned short)atoi(argv[1]));
    if(socket_value==INVALID_SOCKET){SSL_CTX_free(context);WSACleanup();return 6;}
    ssl=SSL_new(context);
    if(!ssl||SSL_set_fd(ssl,(int)socket_value)!=1||SSL_connect(ssl)!=1){if(ssl)SSL_free(ssl);else closesocket(socket_value);SSL_CTX_free(context);WSACleanup();return 7;}
    ERR_clear_error();WSASetLastError(0);amount=0;
    ssl_result=SSL_read_ex(ssl,&byte,1,&amount);wsa_error=WSAGetLastError();
    ssl_error=SSL_get_error(ssl,ssl_result);queue_error=ERR_peek_error();
    socket_error=0;socket_error_size=sizeof(socket_error);
    if(getsockopt(socket_value,SOL_SOCKET,SO_ERROR,(char*)&socket_error,&socket_error_size)!=0)socket_error=WSAGetLastError();
    printf("MODE=%s SSL_RESULT=%d AMOUNT=%lu SSL_ERROR=%d WSA_ERROR=%d SO_ERROR=%d ERR_LIB=%d ERR_REASON=%d\n",argv[2],ssl_result,(unsigned long)amount,ssl_error,wsa_error,socket_error,queue_error?(int)ERR_GET_LIB(queue_error):0,queue_error?(int)ERR_GET_REASON(queue_error):0);
    SSL_free(ssl);SSL_CTX_free(context);WSACleanup();
    if(ssl_result!=0||amount!=0)return 8;
    if(!strcmp(argv[2],"clean"))return ssl_error==SSL_ERROR_ZERO_RETURN?0:9;
    if(!strcmp(argv[2],"fin"))return ssl_error==SSL_ERROR_SSL&&wsa_error==0&&queue_error&&ERR_GET_LIB(queue_error)==ERR_LIB_SSL&&ERR_GET_REASON(queue_error)==SSL_R_UNEXPECTED_EOF_WHILE_READING?0:10;
    if(!strcmp(argv[2],"rst"))return ssl_error==SSL_ERROR_SYSCALL&&wsa_error==WSAECONNRESET?0:11;
    return 12;
}
