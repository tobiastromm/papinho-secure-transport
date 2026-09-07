/* SPDX-License-Identifier: MPL-2.0 */
#include "papinho_secure_transport.h"
#include <stdio.h>
#include <string.h>
static void selection(PST_PROVIDER_SELECTION *value,pst_u32 mode,const char *exact,const char *const *ordered,pst_size count)
{memset(value,0,sizeof(*value));value->struct_size=sizeof(*value);value->api_version=PST_API_VERSION;value->mode=mode;value->exact_provider_id=exact;value->ordered_provider_ids=ordered;value->ordered_provider_count=count;}
int main(void)
{
 PST_PROVIDER_SELECTION automatic,exact,preferred;const char *order[]={"openssl","schannel"};
 selection(&automatic,PST_BACKEND_SELECTION_AUTOMATIC,NULL,NULL,0);
 selection(&exact,PST_BACKEND_SELECTION_EXACT,"openssl",NULL,0);
 selection(&preferred,PST_BACKEND_SELECTION_ORDERED,NULL,order,2);
 printf("Connection-level selection: AUTOMATIC follows registry order; EXACT never falls back; ORDERED follows caller order. modes=%lu,%lu,%lu\n",(unsigned long)automatic.mode,(unsigned long)exact.mode,(unsigned long)preferred.mode);return 0;
}
