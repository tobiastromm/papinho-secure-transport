/* SPDX-License-Identifier: MPL-2.0 */
#ifndef PST_TEST_UPGRADE_PLAINTEXT_H
#define PST_TEST_UPGRADE_PLAINTEXT_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int pst_test_recv_exact(SOCKET socket_value,const char *expected){char byte;size_t i,n=strlen(expected);for(i=0;i<n;i++){if(recv(socket_value,&byte,1,0)!=1||byte!=expected[i])return 0;}return 1;}
static int pst_test_plaintext_upgrade(SOCKET socket_value){const char*style=getenv("PST_TEST_UPGRADE_STYLE");const char*request;const char*ready;char preread;if(!style||!*style)return 1;if(!strcmp(style,"starttls")){if(!pst_test_recv_exact(socket_value,"PLAINTEXT_GREETING\r\n"))return 0;request="UPGRADE_REQUEST\r\n";ready="UPGRADE_READY\r\n";}else if(!strcmp(style,"connect")){request="PLAINTEXT_TUNNEL_REQUEST\r\n";ready="PLAINTEXT_TUNNEL_READY\r\n";}else return 0;if(send(socket_value,request,(int)strlen(request),0)!=(int)strlen(request))return 0;if(!pst_test_recv_exact(socket_value,ready))return 0;printf("PLAINTEXT_UPGRADE STYLE=%s SOCKET=%lu BOUNDARY_BYTES=%lu EXACT=1\n",style,(unsigned long)socket_value,(unsigned long)strlen(ready));if(getenv("PST_TEST_PREREAD_TLS")){if(recv(socket_value,&preread,1,0)!=1)return 0;printf("PRE_READ_TLS_BYTES=1 BYTE=0x%02x UNSUPPORTED_EXPECTED_FAILURE=1\n",(unsigned int)(unsigned char)preread);}fflush(stdout);return 1;}
#endif
