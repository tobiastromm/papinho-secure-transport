#include "pst_backend.h"
#include "pst_transport_internal.h"
#include <stdio.h>
#include <string.h>
#define ABI_ASSERT(name,expression) typedef char name[(expression)?1:-1]
#define CHECK(expression,code) if (!(expression)) return (code)
ABI_ASSERT(spi_u32_width,sizeof(pst_u32)==4);
ABI_ASSERT(spi_version_value,PST_BACKEND_SPI_VERSION==0x00020004UL);
ABI_ASSERT(spi_descriptor_minimum,PST_BACKEND_DESCRIPTOR_MIN_SIZE==(pst_u32)(offsetof(PST_BACKEND_DESCRIPTOR,vtable)+sizeof(((PST_BACKEND_DESCRIPTOR*)0)->vtable)));
ABI_ASSERT(spi_vtable_minimum,PST_BACKEND_VTABLE_MIN_SIZE==(pst_u32)offsetof(PST_BACKEND_VTABLE,connection_configure_identity));
ABI_ASSERT(spi_metadata_size,sizeof(PST_BACKEND_METADATA)==180);
ABI_ASSERT(spi_metadata_components_offset,offsetof(PST_BACKEND_METADATA,components)==68);
#if defined(_WIN64)
ABI_ASSERT(spi_descriptor_size_x64,sizeof(PST_BACKEND_DESCRIPTOR)==48);
ABI_ASSERT(spi_descriptor_vtable_x64,offsetof(PST_BACKEND_DESCRIPTOR,vtable)==32);
ABI_ASSERT(spi_descriptor_metadata_x64,offsetof(PST_BACKEND_DESCRIPTOR,metadata)==40);
ABI_ASSERT(spi_vtable_size_x64,sizeof(PST_BACKEND_VTABLE)==168);
ABI_ASSERT(spi_vtable_config_x64,offsetof(PST_BACKEND_VTABLE,connection_configure_identity)==144);
ABI_ASSERT(spi_vtable_alpn_x64,offsetof(PST_BACKEND_VTABLE,connection_get_alpn)==152);
ABI_ASSERT(spi_vtable_diagnostic_x64,offsetof(PST_BACKEND_VTABLE,diagnostic_copy)==160);
ABI_ASSERT(spi_transport_size_x64,sizeof(PST_NATIVE_TRANSPORT)==32);
ABI_ASSERT(spi_io_result_size_x64,sizeof(PST_BACKEND_IO_RESULT)==24);
#else
ABI_ASSERT(spi_descriptor_size_x86,sizeof(PST_BACKEND_DESCRIPTOR)==28);
ABI_ASSERT(spi_descriptor_vtable_x86,offsetof(PST_BACKEND_DESCRIPTOR,vtable)==20);
ABI_ASSERT(spi_descriptor_metadata_x86,offsetof(PST_BACKEND_DESCRIPTOR,metadata)==24);
ABI_ASSERT(spi_vtable_size_x86,sizeof(PST_BACKEND_VTABLE)==88);
ABI_ASSERT(spi_vtable_config_x86,offsetof(PST_BACKEND_VTABLE,connection_configure_identity)==76);
ABI_ASSERT(spi_vtable_alpn_x86,offsetof(PST_BACKEND_VTABLE,connection_get_alpn)==80);
ABI_ASSERT(spi_vtable_diagnostic_x86,offsetof(PST_BACKEND_VTABLE,diagnostic_copy)==84);
ABI_ASSERT(spi_transport_size_x86,sizeof(PST_NATIVE_TRANSPORT)==20);
ABI_ASSERT(spi_io_result_size_x86,sizeof(PST_BACKEND_IO_RESULT)==16);
#endif
ABI_ASSERT(spi_cap_tls12,PST_BACKEND_CAP_TLS_1_2==0x00000001UL);
ABI_ASSERT(spi_cap_tls13,PST_BACKEND_CAP_TLS_1_3==0x00000002UL);
ABI_ASSERT(spi_cap_client_auth,PST_BACKEND_CAP_CLIENT_AUTH==0x00000004UL);
ABI_ASSERT(spi_cap_alpn,PST_BACKEND_CAP_ALPN==0x00000008UL);
ABI_ASSERT(spi_cap_custom_trust,PST_BACKEND_CAP_CUSTOM_TRUST==0x00000010UL);
ABI_ASSERT(spi_cap_system_trust,PST_BACKEND_CAP_SYSTEM_TRUST==0x00000020UL);
ABI_ASSERT(spi_cap_hostname,PST_BACKEND_CAP_HOSTNAME_VERIFY==0x00000040UL);
ABI_ASSERT(spi_cap_resumption,PST_BACKEND_CAP_RESUMPTION==0x00000080UL);
ABI_ASSERT(spi_cap_early_data,PST_BACKEND_CAP_EARLY_DATA==0x00000100UL);
ABI_ASSERT(spi_cap_peer_info,PST_BACKEND_CAP_PEER_INFO==0x00000200UL);
ABI_ASSERT(spi_cap_nonblocking,PST_BACKEND_CAP_NONBLOCKING==0x00000400UL);
ABI_ASSERT(spi_cap_backend_wait,PST_BACKEND_CAP_BACKEND_WAIT==0x00000800UL);
static PST_RESULT ok_initialize(void **state){*state=state;return PST_RESULT_OK;}
static void no_destroy(void *state){(void)state;}
static PST_RESULT ok_create(void *state,void **out){*out=state;return PST_RESULT_OK;}
static PST_RESULT ok_query(void *state,pst_u32 *caps){(void)state;*caps=0;return PST_RESULT_OK;}
static PST_RESULT ok_validate(void *state,pst_u32 caps){(void)state;(void)caps;return PST_RESULT_OK;}
static PST_RESULT ok_attach(void *state,void *transport,pst_u32 ownership,pst_u32 *accepted){(void)state;(void)transport;(void)ownership;*accepted=0;return PST_RESULT_OK;}
static PST_RESULT ok_step(void *state,pst_u32 *operation,PST_RESULT *error){(void)state;*operation=PST_BACKEND_OPERATION_COMPLETE;*error=PST_RESULT_OK;return PST_RESULT_OK;}
static PST_RESULT ok_interest(void *state,pst_u32 *interest){(void)state;*interest=0;return PST_RESULT_OK;}
static PST_RESULT ok_read(void *state,void *buffer,pst_size size,PST_BACKEND_IO_RESULT *result){(void)state;(void)buffer;(void)size;memset(result,0,sizeof(*result));return PST_RESULT_OK;}
static PST_RESULT ok_write(void *state,const void *buffer,pst_size size,PST_BACKEND_IO_RESULT *result){(void)state;(void)buffer;(void)size;memset(result,0,sizeof(*result));return PST_RESULT_OK;}
static PST_RESULT ok_configure(void *state,const pst_config *config){(void)state;(void)config;return PST_RESULT_OK;}
static PST_RESULT ok_alpn(void *state,pst_u8 *buffer,pst_size capacity,pst_size *size){(void)state;(void)buffer;(void)capacity;*size=0;return PST_RESULT_UNAVAILABLE;}
int main(void)
{
    PST_BACKEND_VTABLE v;PST_BACKEND_DESCRIPTOR d;char id31[PST_BACKEND_ID_CAPACITY];char id32[PST_BACKEND_ID_CAPACITY+1UL];
    memset(&v,0,sizeof(v));v.struct_size=PST_BACKEND_VTABLE_FIELD_SIZE(connection_get_alpn);v.spi_version=PST_BACKEND_SPI_VERSION;
    v.initialize=ok_initialize;v.shutdown=no_destroy;v.runtime_create=ok_create;v.runtime_destroy=no_destroy;v.query_capabilities=ok_query;v.validate_requirements=ok_validate;v.connection_create=ok_create;v.connection_destroy=no_destroy;v.attach_transport=ok_attach;v.handshake_step=ok_step;v.get_interest=ok_interest;v.read=ok_read;v.write=ok_write;v.shutdown_step=ok_step;v.connection_configure_identity=ok_configure;v.connection_get_alpn=ok_alpn;
    memset(&d,0,sizeof(d));d.struct_size=sizeof(d);d.spi_version=PST_BACKEND_SPI_VERSION;d.id="abi-provider";d.name="ABI provider";d.capabilities=PST_BACKEND_CAP_TLS_1_2|PST_BACKEND_CAP_ALPN;d.vtable=&v;
    CHECK(pst_backend_validate(&d)==PST_RESULT_OK,1);
    d.capabilities=PST_BACKEND_CAP_TLS_1_2;v.struct_size=PST_BACKEND_VTABLE_MIN_SIZE-1UL;CHECK(pst_backend_validate(&d)==PST_RESULT_INVALID_ARGUMENT,7);
    v.struct_size=PST_BACKEND_VTABLE_MIN_SIZE;CHECK(pst_backend_validate(&d)==PST_RESULT_OK,8);
    v.struct_size=sizeof(v);CHECK(pst_backend_validate(&d)==PST_RESULT_OK,9);
    v.struct_size=sizeof(v)+16UL;CHECK(pst_backend_validate(&d)==PST_RESULT_OK,10);
    d.spi_version=0x00020005UL;v.spi_version=0x00020005UL;CHECK(pst_backend_validate(&d)==PST_RESULT_OK,11);
    d.spi_version=0x00030000UL;CHECK(pst_backend_validate(&d)==PST_RESULT_INCOMPATIBLE_API,12);
    d.spi_version=PST_BACKEND_SPI_VERSION;v.spi_version=PST_BACKEND_SPI_VERSION;v.struct_size=PST_BACKEND_VTABLE_FIELD_SIZE(connection_get_alpn);d.capabilities=PST_BACKEND_CAP_TLS_1_2|PST_BACKEND_CAP_ALPN;
    d.struct_size=PST_BACKEND_DESCRIPTOR_MIN_SIZE-1UL;CHECK(pst_backend_validate(&d)==PST_RESULT_INVALID_ARGUMENT,2);
    d.struct_size=PST_BACKEND_DESCRIPTOR_MIN_SIZE;CHECK(pst_backend_validate(&d)==PST_RESULT_OK,3);
    d.struct_size=sizeof(d)+16UL;CHECK(pst_backend_validate(&d)==PST_RESULT_OK,4);
    memset(id31,'a',sizeof(id31));id31[sizeof(id31)-1UL]='\0';d.id=id31;CHECK(pst_backend_validate(&d)==PST_RESULT_OK,5);
    memset(id32,'a',sizeof(id32));id32[sizeof(id32)-1UL]='\0';d.id=id32;CHECK(pst_backend_validate(&d)==PST_RESULT_INVALID_ARGUMENT,6);
    printf("SPI_ABI=2.4 POINTER_BITS=%lu DESCRIPTOR=%lu VTABLE=%lu METADATA=%lu ID_MAX=31 PREFIX=PASS FUTURE_TAIL=PASS OPTIONAL_DIAGNOSTIC=PASS\n",(unsigned long)(sizeof(void*)*8UL),(unsigned long)sizeof(PST_BACKEND_DESCRIPTOR),(unsigned long)sizeof(PST_BACKEND_VTABLE),(unsigned long)sizeof(PST_BACKEND_METADATA));
    printf("test_provider_spi_abi: PASS\n");return 0;
}
