#include "pst_backend.h"
#include <string.h>
#define PST_BACKEND_REGISTRY_CAPACITY 8
static const PST_BACKEND_DESCRIPTOR *pst_backend_registry[PST_BACKEND_REGISTRY_CAPACITY];
static pst_size pst_backend_registry_count;
static int pst_backend_id_valid(const char *id)
{
    const unsigned char *p;
    if (id == NULL || id[0] == '\0') return 0;
    p = (const unsigned char *)id;
    while (*p != 0) {
        if (!((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9') ||
              *p == '-' || *p == '_' || *p == '.')) return 0;
        ++p;
    }
    return 1;
}
static int pst_backend_spi_compatible(pst_u32 version)
{
    return ((version >> 16) & 0xffffUL) == PST_BACKEND_SPI_VERSION_MAJOR;
}
PST_RESULT pst_backend_validate(const PST_BACKEND_DESCRIPTOR *d)
{
    const PST_BACKEND_VTABLE *v;
    if (d == NULL) return PST_RESULT_INVALID_ARGUMENT;
    if (d->struct_size < PST_BACKEND_DESCRIPTOR_MIN_SIZE)
        return PST_RESULT_INVALID_ARGUMENT;
    if (!pst_backend_spi_compatible(d->spi_version))
        return PST_RESULT_INCOMPATIBLE_API;
    if (!pst_backend_id_valid(d->id) || d->vtable == NULL)
        return PST_RESULT_INVALID_ARGUMENT;
    if (d->struct_size >= (pst_u32)(offsetof(PST_BACKEND_DESCRIPTOR, metadata) + sizeof(d->metadata)) && d->metadata != NULL) {
        if (d->metadata->struct_size < PST_BACKEND_METADATA_MIN_SIZE ||
            d->metadata->version != PST_BACKEND_METADATA_VERSION ||
            d->metadata->component_count > PST_BACKEND_METADATA_COMPONENT_CAPACITY)
            return PST_RESULT_INVALID_ARGUMENT;
    }
    v = d->vtable;
    if (v->struct_size < PST_BACKEND_VTABLE_MIN_SIZE)
        return PST_RESULT_INVALID_ARGUMENT;
    if (!pst_backend_spi_compatible(v->spi_version))
        return PST_RESULT_INCOMPATIBLE_API;
    if (v->initialize == NULL || v->shutdown == NULL ||
        v->runtime_create == NULL || v->runtime_destroy == NULL ||
        v->query_capabilities == NULL || v->validate_requirements == NULL ||
        v->connection_create == NULL || v->connection_destroy == NULL ||
        v->attach_transport == NULL || v->handshake_step == NULL ||
        v->get_interest == NULL || v->read == NULL || v->write == NULL ||
        v->shutdown_step == NULL) return PST_RESULT_INVALID_ARGUMENT;
    if ((d->capabilities & PST_BACKEND_CAP_EARLY_DATA) != 0UL &&
        (d->capabilities & PST_BACKEND_CAP_RESUMPTION) == 0UL)
        return PST_RESULT_INVALID_ARGUMENT;
    if ((d->capabilities & PST_BACKEND_CAP_BACKEND_WAIT) != 0UL &&
        v->wait == NULL) return PST_RESULT_INVALID_ARGUMENT;
    if ((d->capabilities & PST_BACKEND_CAP_PEER_INFO) != 0UL &&
        (v->peer_info_create == NULL || v->peer_info_destroy == NULL))
        return PST_RESULT_INVALID_ARGUMENT;
    if ((d->capabilities & (PST_BACKEND_CAP_CLIENT_AUTH |
                            PST_BACKEND_CAP_CUSTOM_TRUST |
                            PST_BACKEND_CAP_HOSTNAME_VERIFY)) != 0UL &&
        (v->struct_size < (pst_u32)sizeof(PST_BACKEND_VTABLE) ||
         v->connection_configure_identity == NULL))
        return PST_RESULT_INVALID_ARGUMENT;
    if ((d->capabilities & PST_BACKEND_CAP_ALPN) != 0UL &&
        (v->struct_size < (pst_u32)sizeof(PST_BACKEND_VTABLE) ||
         v->connection_get_alpn == NULL)) return PST_RESULT_INVALID_ARGUMENT;
    return PST_RESULT_OK;
}
PST_RESULT pst_backend_register(const PST_BACKEND_DESCRIPTOR *d)
{
    PST_RESULT r;
    pst_size i;
    r = pst_backend_validate(d);
    if (r != PST_RESULT_OK) return r;
    for (i = 0; i < pst_backend_registry_count; ++i)
        if (strcmp(pst_backend_registry[i]->id, d->id) == 0)
            return PST_RESULT_INVALID_STATE;
    if (pst_backend_registry_count == PST_BACKEND_REGISTRY_CAPACITY)
        return PST_RESULT_RESOURCE_FAILURE;
    pst_backend_registry[pst_backend_registry_count++] = d;
    return PST_RESULT_OK;
}
PST_RESULT pst_backend_unregister(const char *id)
{
    pst_size i;
    pst_size j;
    if (!pst_backend_id_valid(id)) return PST_RESULT_INVALID_ARGUMENT;
    for (i = 0; i < pst_backend_registry_count; ++i) {
        if (strcmp(pst_backend_registry[i]->id, id) == 0) {
            for (j = i + 1; j < pst_backend_registry_count; ++j)
                pst_backend_registry[j - 1] = pst_backend_registry[j];
            --pst_backend_registry_count;
            pst_backend_registry[pst_backend_registry_count] = NULL;
            return PST_RESULT_OK;
        }
    }
    return PST_RESULT_UNAVAILABLE;
}
const PST_BACKEND_DESCRIPTOR *pst_backend_find(const char *id)
{
    pst_size i;
    if (!pst_backend_id_valid(id)) return NULL;
    for (i = 0; i < pst_backend_registry_count; ++i)
        if (strcmp(pst_backend_registry[i]->id, id) == 0)
            return pst_backend_registry[i];
    return NULL;
}
const PST_BACKEND_DESCRIPTOR *pst_backend_find_by_index(pst_size index){return index<pst_backend_registry_count?pst_backend_registry[index]:NULL;}
pst_size pst_backend_count(void) { return pst_backend_registry_count; }
void pst_backend_registry_reset(void)
{
    pst_size i;
    for (i = 0; i < pst_backend_registry_count; ++i)
        pst_backend_registry[i] = NULL;
    pst_backend_registry_count = 0;
}
