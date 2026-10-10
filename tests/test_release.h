/* SPDX-License-Identifier: MPL-2.0 */
#ifndef PST_TEST_RELEASE_H
#define PST_TEST_RELEASE_H
#include "papinho_secure_transport.h"
static PST_RESULT test_connection_release(pst_connection *connection)
{
    PST_RELEASE_INFO info;
    PST_RESULT result;
    result=pst_release_info_init(&info,(pst_u32)sizeof(info));
    if(result!=PST_RESULT_OK)return result;
    return pst_connection_try_release(connection,&info);
}
#endif
