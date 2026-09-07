/* SPDX-License-Identifier: MPL-2.0 */
#include "papinho_secure_transport.h"
#include <stdio.h>
#include <string.h>
int main(void)
{
 PST_TRUST_SOURCE source;pst_trust *trust=NULL;PST_RESULT result;
 memset(&source,0,sizeof(source));source.struct_size=sizeof(source);source.api_version=PST_API_VERSION;source.kind=PST_TRUST_SOURCE_SYSTEM;
 result=pst_trust_create(&source,&trust);printf("SYSTEM_TRUST object result: %s; attach it to connection.peer_authentication.trust\n",pst_result_string(result));pst_trust_release(trust);return result==PST_RESULT_OK?0:2;
}
