#include "pst_diagnostic.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef struct diagnostic_mapping { pst_u32 phase; pst_u32 operation; } diagnostic_mapping;
static const diagnostic_mapping mappings[] = {
    {PST_DIAGNOSTIC_PHASE_NONE,PST_DIAGNOSTIC_OPERATION_NONE},
    {PST_DIAGNOSTIC_PHASE_BACKEND_INITIALIZE,PST_DIAGNOSTIC_OPERATION_RUNTIME},
    {PST_DIAGNOSTIC_PHASE_RUNTIME_CREATE,PST_DIAGNOSTIC_OPERATION_RUNTIME},
    {PST_DIAGNOSTIC_PHASE_CAPABILITY_VALIDATE,PST_DIAGNOSTIC_OPERATION_RUNTIME},
    {PST_DIAGNOSTIC_PHASE_BACKEND_SELECT,PST_DIAGNOSTIC_OPERATION_RUNTIME},
    {PST_DIAGNOSTIC_PHASE_TLS_CONFIGURE,PST_DIAGNOSTIC_OPERATION_CONFIGURATION},
    {PST_DIAGNOSTIC_PHASE_ALPN,PST_DIAGNOSTIC_OPERATION_CONFIGURATION},
    {PST_DIAGNOSTIC_PHASE_IDENTITY_SETUP,PST_DIAGNOSTIC_OPERATION_CONFIGURATION},
    {PST_DIAGNOSTIC_PHASE_TRANSPORT_ATTACH,PST_DIAGNOSTIC_OPERATION_TRANSPORT},
    {PST_DIAGNOSTIC_PHASE_CONNECTION_CREATE,PST_DIAGNOSTIC_OPERATION_CONNECTION},
    {PST_DIAGNOSTIC_PHASE_HANDSHAKE,PST_DIAGNOSTIC_OPERATION_HANDSHAKE},
    {PST_DIAGNOSTIC_PHASE_PEER_AUTHENTICATE,PST_DIAGNOSTIC_OPERATION_AUTHENTICATION},
    {PST_DIAGNOSTIC_PHASE_HOSTNAME_VERIFY,PST_DIAGNOSTIC_OPERATION_AUTHENTICATION},
    {PST_DIAGNOSTIC_PHASE_READ,PST_DIAGNOSTIC_OPERATION_READ},
    {PST_DIAGNOSTIC_PHASE_WRITE,PST_DIAGNOSTIC_OPERATION_WRITE},
    {PST_DIAGNOSTIC_PHASE_WAIT,PST_DIAGNOSTIC_OPERATION_WAIT},
    {PST_DIAGNOSTIC_PHASE_SHUTDOWN,PST_DIAGNOSTIC_OPERATION_SHUTDOWN},
    {PST_DIAGNOSTIC_PHASE_PEER_INFO,PST_DIAGNOSTIC_OPERATION_PEER_INFO}
};

typedef struct larger_diagnostic {
    PST_DIAGNOSTIC_INFO base;
    unsigned char tail[8];
} larger_diagnostic;

#define CHECK(c,n) if(!(c))return(n)

static int check_layout(void)
{
    volatile pst_size size;
    volatile pst_size offset;
    size=sizeof(PST_DIAGNOSTIC_INFO);CHECK(size==56UL,1);
    offset=offsetof(PST_DIAGNOSTIC_INFO,struct_size);CHECK(offset==0UL,2);
    offset=offsetof(PST_DIAGNOSTIC_INFO,api_version);CHECK(offset==4UL,3);
    offset=offsetof(PST_DIAGNOSTIC_INFO,valid);CHECK(offset==8UL,4);
    offset=offsetof(PST_DIAGNOSTIC_INFO,generation);CHECK(offset==12UL,5);
    offset=offsetof(PST_DIAGNOSTIC_INFO,normalized_result);CHECK(offset==16UL,6);
    offset=offsetof(PST_DIAGNOSTIC_INFO,operation);CHECK(offset==20UL,7);
    offset=offsetof(PST_DIAGNOSTIC_INFO,backend_id);CHECK(offset==24UL,8);
    return 0;
}

static int check_init(void)
{
    PST_DIAGNOSTIC_INFO value;
    larger_diagnostic larger;
    pst_size i;
    pst_internal_diagnostic source;
    CHECK(pst_diagnostic_info_init(NULL)==PST_RESULT_INVALID_ARGUMENT,20);
    memset(&value,0xa5,sizeof(value));CHECK(pst_diagnostic_info_init(&value)==PST_RESULT_OK,21);
    CHECK(value.struct_size==sizeof(value)&&value.api_version==PST_API_VERSION&&!value.valid,22);
    pst_diagnostic_initialize(&source);
    value.struct_size=PST_DIAGNOSTIC_INFO_MIN_SIZE-1UL;value.api_version=PST_API_VERSION;
    CHECK(pst_diagnostic_export_public(&source,&value)==PST_RESULT_INVALID_ARGUMENT,23);
    value.struct_size=sizeof(value);value.api_version=0x00020000UL;
    CHECK(pst_diagnostic_export_public(&source,&value)==PST_RESULT_INCOMPATIBLE_API,24);
    value.struct_size=sizeof(value);value.api_version=0x00010000UL;
    CHECK(pst_diagnostic_export_public(&source,&value)==PST_RESULT_OK,25);
    memset(&larger,0xa5,sizeof(larger));larger.base.struct_size=sizeof(larger);larger.base.api_version=PST_API_VERSION;
    CHECK(pst_diagnostic_export_public(&source,&larger.base)==PST_RESULT_OK,26);
    CHECK(larger.base.struct_size==sizeof(larger),27);
    for(i=0;i<sizeof(larger.tail);++i)CHECK(larger.tail[i]==0xa5,28);
    return 0;
}

static int check_export(void)
{
    pst_internal_diagnostic source;
    PST_DIAGNOSTIC_INFO first;
    PST_DIAGNOSTIC_INFO saved;
    char long_id[48];
    pst_size i;
    pst_diagnostic_initialize(&source);
    memset(&first,0,sizeof(first));first.struct_size=sizeof(first);first.api_version=PST_API_VERSION;
    CHECK(pst_diagnostic_export_public(&source,&first)==PST_RESULT_OK,30);
    CHECK(!first.valid&&!first.generation&&first.operation==PST_DIAGNOSTIC_OPERATION_NONE,31);
    pst_diagnostic_capture(&source,PST_RESULT_AUTH_FAILURE,PST_DIAGNOSTIC_PHASE_HOSTNAME_VERIFY,
        "retrozilla-nss",PST_DIAGNOSTIC_DOMAIN_NSS,-8179,-12286,
        PST_DIAGNOSTIC_FLAG_NATIVE|PST_DIAGNOSTIC_FLAG_SECONDARY);
    first.struct_size=sizeof(first);first.api_version=PST_API_VERSION;
    CHECK(pst_diagnostic_export_public(&source,&first)==PST_RESULT_OK,32);
    CHECK(first.valid&&first.generation==1UL,33);
    CHECK(first.normalized_result==PST_RESULT_AUTH_FAILURE,34);
    CHECK(first.operation==PST_DIAGNOSTIC_OPERATION_AUTHENTICATION,35);
    CHECK(!strcmp(first.backend_id,"retrozilla-nss"),36);
    saved=first;pst_diagnostic_clear(&source);
    CHECK(saved.valid&&saved.generation==1UL&&!strcmp(saved.backend_id,"retrozilla-nss"),37);
    for(i=0;i<sizeof(long_id)-1;++i)long_id[i]='x';long_id[sizeof(long_id)-1]='\0';
    pst_diagnostic_capture(&source,PST_RESULT_BACKEND_FAILURE,PST_DIAGNOSTIC_PHASE_READ,
        long_id,PST_DIAGNOSTIC_DOMAIN_WIN32,1234,5678,PST_DIAGNOSTIC_FLAG_NATIVE);
    first.struct_size=sizeof(first);first.api_version=PST_API_VERSION;
    CHECK(pst_diagnostic_export_public(&source,&first)==PST_RESULT_OK,38);
    CHECK(first.operation==PST_DIAGNOSTIC_OPERATION_READ,39);
    CHECK(strlen(first.backend_id)==PST_DIAGNOSTIC_BACKEND_ID_CAPACITY-1UL,40);
    CHECK(first.backend_id[PST_DIAGNOSTIC_BACKEND_ID_CAPACITY-1UL]=='\0',41);
    for(i=0;i<sizeof(mappings)/sizeof(mappings[0]);++i){
        pst_diagnostic_capture(&source,PST_RESULT_BACKEND_FAILURE,mappings[i].phase,"map",PST_DIAGNOSTIC_DOMAIN_NSS,-1,-2,PST_DIAGNOSTIC_FLAG_NATIVE);
        first.struct_size=sizeof(first);first.api_version=PST_API_VERSION;
        CHECK(pst_diagnostic_export_public(&source,&first)==PST_RESULT_OK,42);
        CHECK(first.operation==mappings[i].operation,43);
    }
    return 0;
}

static int public_contains(const PST_DIAGNOSTIC_INFO *value,const char *text)
{
    const unsigned char *bytes; pst_size i,length;
    bytes=(const unsigned char *)value;length=strlen(text);
    for(i=0;i+length<=sizeof(*value);++i)if(memcmp(bytes+i,text,length)==0)return 1;
    return 0;
}

static int check_boundaries(void)
{
    static const pst_size lengths[]={0UL,1UL,30UL,31UL,32UL,47UL};
    static const pst_u32 sizes[]={0UL,1UL,3UL,4UL,7UL,8UL,15UL,16UL,23UL,24UL,55UL,56UL,64UL};
    static const pst_u32 versions[]={0UL,0x00000100UL,0x00010000UL,0x00010100UL,0x0001ffffUL,0x00020000UL};
    pst_internal_diagnostic source; PST_DIAGNOSTIC_INFO out; char id[48]; pst_size i,j,expected; PST_RESULT validation_result;
    memset(id,'b',sizeof(id));id[sizeof(id)-1]='\0';
    for(i=0;i<sizeof(lengths)/sizeof(lengths[0]);++i){
        id[lengths[i]]='\0';pst_diagnostic_initialize(&source);
        pst_diagnostic_capture(&source,PST_RESULT_BACKEND_FAILURE,PST_DIAGNOSTIC_PHASE_RUNTIME_CREATE,id,PST_DIAGNOSTIC_DOMAIN_NSS,-1,-2,PST_DIAGNOSTIC_FLAG_NATIVE);
        CHECK(pst_diagnostic_info_init(&out)==PST_RESULT_OK,60);
        CHECK(pst_diagnostic_export_public(&source,&out)==PST_RESULT_OK,61);
        expected=lengths[i]<31UL?lengths[i]:31UL;CHECK(strlen(out.backend_id)==expected,62);
        CHECK(out.backend_id[31]=='\0',63);memset(id,'b',sizeof(id));id[sizeof(id)-1]='\0';
    }
    pst_diagnostic_initialize(&source);
    for(i=0;i<sizeof(sizes)/sizeof(sizes[0]);++i)for(j=0;j<sizeof(versions)/sizeof(versions[0]);++j){
        memset(&out,0xa5,sizeof(out));out.struct_size=sizes[i];out.api_version=versions[j];
        validation_result=pst_diagnostic_export_public(&source,&out);
        if(sizes[i]<PST_DIAGNOSTIC_INFO_MIN_SIZE){CHECK(validation_result==PST_RESULT_INVALID_ARGUMENT,64);}
        else if(((versions[j]>>16)&0xffffUL)!=PST_API_VERSION_MAJOR){CHECK(validation_result==PST_RESULT_INCOMPATIBLE_API,65);}
        else {CHECK(validation_result==PST_RESULT_OK,66);}
    }
    pst_diagnostic_initialize(&source);source.generation=0xffffffffUL;
    pst_diagnostic_capture(&source,PST_RESULT_TRANSPORT_FAILURE,PST_DIAGNOSTIC_PHASE_READ,"wrap",PST_DIAGNOSTIC_DOMAIN_NSPR,-1,0,PST_DIAGNOSTIC_FLAG_NATIVE);
    CHECK(source.valid&&source.generation==0UL,67);CHECK(pst_diagnostic_info_init(&out)==PST_RESULT_OK,68);
    CHECK(pst_diagnostic_export_public(&source,&out)==PST_RESULT_OK&&out.generation==0UL,69);
    return 0;
}

static int check_result_consistency(void)
{
    static const PST_RESULT results[]={PST_RESULT_TRANSPORT_FAILURE,PST_RESULT_PROTOCOL_FAILURE,PST_RESULT_AUTH_FAILURE,PST_RESULT_HOSTNAME_MISMATCH,PST_RESULT_CLOSED,PST_RESULT_TRUNCATED};
    static const pst_u32 phases[]={PST_DIAGNOSTIC_PHASE_TRANSPORT_ATTACH,PST_DIAGNOSTIC_PHASE_HANDSHAKE,PST_DIAGNOSTIC_PHASE_PEER_AUTHENTICATE,PST_DIAGNOSTIC_PHASE_HOSTNAME_VERIFY,PST_DIAGNOSTIC_PHASE_READ,PST_DIAGNOSTIC_PHASE_READ};
    pst_internal_diagnostic source; PST_DIAGNOSTIC_INFO out; pst_size i;
    for(i=0;i<sizeof(results)/sizeof(results[0]);++i){
        pst_diagnostic_initialize(&source);pst_diagnostic_capture(&source,results[i],phases[i],"retrozilla-nss",PST_DIAGNOSTIC_DOMAIN_NSPR,-5961,0,PST_DIAGNOSTIC_FLAG_NATIVE);
        CHECK(pst_diagnostic_info_init(&out)==PST_RESULT_OK,85);CHECK(pst_diagnostic_export_public(&source,&out)==PST_RESULT_OK,86);
        CHECK(out.valid&&out.normalized_result==results[i],87);
    }
    CHECK(results[4]!=results[5],88);return 0;
}

static int check_redaction_abuse(void)
{
    struct fixture {
        char password[24];
        char token[24];
        char path[32];
        char hostname[24];
        char alpn[24];
        unsigned char certificate[16];
        pst_internal_diagnostic internal;
        char payload[32];
    } fixture;
    PST_DIAGNOSTIC_INFO out; PST_DIAGNOSTIC_INFO saved;
    memset(&fixture,0,sizeof(fixture));
    strcpy(fixture.password,"password=secret-value");
    strcpy(fixture.token,"token=credential-value");
    strcpy(fixture.path,"C:\\private\\fixture.key");
    strcpy(fixture.hostname,"private.example.test");
    strcpy(fixture.alpn,"peer-controlled-alpn");
    memcpy(fixture.certificate,"DER-CERT-BYTES",14UL);
    strcpy(fixture.payload,"application-payload-bytes");
    pst_diagnostic_capture(&fixture.internal,PST_RESULT_AUTH_FAILURE,PST_DIAGNOSTIC_PHASE_HOSTNAME_VERIFY,"retrozilla-nss",PST_DIAGNOSTIC_DOMAIN_NSS,-8179,-12286,PST_DIAGNOSTIC_FLAG_NATIVE|PST_DIAGNOSTIC_FLAG_SECONDARY);
    CHECK(pst_diagnostic_info_init(&out)==PST_RESULT_OK,70);CHECK(pst_diagnostic_export_public(&fixture.internal,&out)==PST_RESULT_OK,71);
    CHECK(!public_contains(&out,"password")&&!public_contains(&out,"secret-value")&&!public_contains(&out,"token"),72);
    CHECK(!public_contains(&out,"private")&&!public_contains(&out,"example.test")&&!public_contains(&out,"peer-controlled"),77);
    CHECK(!public_contains(&out,"DER-CERT")&&!public_contains(&out,"payload"),78);
    CHECK(!public_contains(&out,"-8179")&&!public_contains(&out,"-12286"),73);
    saved=out;pst_diagnostic_clear(&fixture.internal);
    CHECK(saved.valid&&saved.normalized_result==PST_RESULT_AUTH_FAILURE&&!strcmp(saved.backend_id,"retrozilla-nss"),74);
    CHECK(pst_diagnostic_info_init(&out)==PST_RESULT_OK&&pst_diagnostic_export_public(&fixture.internal,&out)==PST_RESULT_OK&&!out.valid,75);
    CHECK(saved.valid&&saved.operation==PST_DIAGNOSTIC_OPERATION_AUTHENTICATION,76);
    return 0;
}

int main(void)
{
    int result; volatile pst_u32 constant;
    constant=PST_API_VERSION;CHECK(constant==0x00010300UL,50);
    constant=PST_LIBRARY_VERSION;CHECK(constant==0x00000300UL,51);
    constant=PST_DIAGNOSTIC_BACKEND_ID_CAPACITY;CHECK(constant==32UL,52);
    constant=PST_DIAGNOSTIC_INFO_MIN_SIZE;CHECK(constant==56UL,55);
    constant=PST_DIAGNOSTIC_OPERATION_NONE;CHECK(constant==0UL,53);
    constant=PST_DIAGNOSTIC_OPERATION_PEER_INFO;CHECK(constant==11UL,54);
    result=check_layout();if(result)return result;
    result=check_init();if(result)return result;
    result=check_export();if(result)return result;
    result=check_boundaries();if(result)return result;
    result=check_result_consistency();if(result)return result;
    result=check_redaction_abuse();if(result)return result;
    printf("test_public_diagnostic: PASS\n");return 0;
}