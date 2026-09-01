#include "pst_diagnostic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(c,n) if(!(c))return(n)
static int contains_text(const pst_internal_diagnostic *d,const char *text)
{
    const unsigned char *bytes;
    pst_size i;
    pst_size length;
    bytes=(const unsigned char *)d;
    length=strlen(text);
    for(i=0;i+length<=sizeof(*d);++i)
        if(memcmp(bytes+i,text,length)==0)return 1;
    return 0;
}
int main(void)
{
    pst_internal_diagnostic source,copy,a,b;
    pst_internal_diagnostic *owned;
    pst_u32 generation;
    pst_diagnostic_initialize(&source);
    memset(&copy,0x5a,sizeof(copy));
    pst_diagnostic_copy(&copy,&source);
    CHECK(!copy.valid&&copy.generation==0UL,1);
    pst_diagnostic_capture(&source,PST_RESULT_AUTH_FAILURE,PST_DIAGNOSTIC_PHASE_PEER_AUTHENTICATE,"retrozilla-nss",PST_DIAGNOSTIC_DOMAIN_NSS,-8179,-12276,PST_DIAGNOSTIC_FLAG_NATIVE|PST_DIAGNOSTIC_FLAG_SECONDARY);
    pst_diagnostic_copy(&copy,&source);
    CHECK(copy.valid&&copy.result==source.result&&copy.phase==source.phase,2);
    CHECK(copy.native_domain==source.native_domain&&copy.native_code==-8179,3);
    CHECK(copy.secondary_native_code==-12276&&copy.flags==source.flags,4);
    CHECK(copy.generation==source.generation&&!strcmp(copy.backend_id,"retrozilla-nss"),5);
    pst_diagnostic_copy(&copy,&copy);
    CHECK(copy.valid&&copy.native_code==-8179,6);
    generation=copy.generation;
    pst_diagnostic_clear(&copy);
    CHECK(!copy.valid&&copy.generation==generation+1UL,7);
    CHECK(copy.phase==PST_DIAGNOSTIC_PHASE_NONE&&copy.native_domain==PST_DIAGNOSTIC_DOMAIN_NONE,8);
    CHECK(copy.native_code==0&&copy.secondary_native_code==0&&copy.backend_id[0]=='\0',9);
    pst_diagnostic_initialize(&a); pst_diagnostic_initialize(&b);
    pst_diagnostic_capture(&a,PST_RESULT_AUTH_FAILURE,PST_DIAGNOSTIC_PHASE_HANDSHAKE,"backend-a",PST_DIAGNOSTIC_DOMAIN_NSS,-8179,0,PST_DIAGNOSTIC_FLAG_NATIVE);
    pst_diagnostic_capture(&b,PST_RESULT_TRANSPORT_FAILURE,PST_DIAGNOSTIC_PHASE_READ,"backend-b",PST_DIAGNOSTIC_DOMAIN_NSPR,-5961,0,PST_DIAGNOSTIC_FLAG_NATIVE);
    pst_diagnostic_copy(&copy,&b); pst_diagnostic_clear(&a);
    CHECK(copy.valid&&copy.result==PST_RESULT_TRANSPORT_FAILURE&&copy.native_code==-5961,10);
    CHECK(!a.valid&&b.valid&&b.native_code==-5961,11);
    owned=(pst_internal_diagnostic *)malloc(sizeof(*owned)); CHECK(owned!=NULL,12);
    pst_diagnostic_capture(owned,PST_RESULT_AUTH_FAILURE,PST_DIAGNOSTIC_PHASE_PEER_AUTHENTICATE,"retrozilla-nss",PST_DIAGNOSTIC_DOMAIN_NSS,-8172,0,PST_DIAGNOSTIC_FLAG_NATIVE);
    pst_diagnostic_copy(&copy,owned); free(owned);
    CHECK(copy.valid&&copy.native_code==-8172&&!strcmp(copy.backend_id,"retrozilla-nss"),13);
    pst_diagnostic_capture(&copy,PST_RESULT_INVALID_STATE,PST_DIAGNOSTIC_PHASE_READ,"",PST_DIAGNOSTIC_DOMAIN_NONE,0,0,0);
    CHECK(copy.valid&&copy.native_domain==PST_DIAGNOSTIC_DOMAIN_NONE&&copy.native_code==0,14);
    pst_diagnostic_clear(&copy);
    CHECK(!copy.valid,15);
    CHECK(!contains_text(&copy,"secret.example")&&!contains_text(&copy,"C:\\private"),16);
    CHECK(!contains_text(&copy,"password")&&!contains_text(&copy,"application-payload"),17);
    printf("test_diagnostic_transport: PASS\n"); return 0;
}