#include "pst_backend.h"
#include <string.h>
#define PST_BACKEND_REGISTRY_CAPACITY 8
static const PST_BACKEND_DESCRIPTOR *pst_backend_registry[PST_BACKEND_REGISTRY_CAPACITY];
static pst_size pst_backend_registry_count;
static int pst_backend_registry_sealed;
static pst_size pst_backend_manifest_fail_after = (pst_size)-1;
static int pst_backend_id_valid(const char *id)
{
    const unsigned char *p;
    pst_size length;
    if (id == NULL || id[0] == '\0') return 0;
    p = (const unsigned char *)id;
    length = 0;
    while (length + 1UL < PST_BACKEND_ID_CAPACITY && *p != 0) {
        if (!((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9') ||
              *p == '-' || *p == '_' || *p == '.')) return 0;
        ++p; ++length;
    }
    return *p == 0;
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
        (v->struct_size < PST_BACKEND_VTABLE_FIELD_SIZE(connection_configure_identity) ||
         v->connection_configure_identity == NULL))
        return PST_RESULT_INVALID_ARGUMENT;
    if ((d->capabilities & PST_BACKEND_CAP_ALPN) != 0UL &&
        (v->struct_size < PST_BACKEND_VTABLE_FIELD_SIZE(connection_get_alpn) ||
         v->connection_get_alpn == NULL)) return PST_RESULT_INVALID_ARGUMENT;
    return PST_RESULT_OK;
}
PST_RESULT pst_backend_register(const PST_BACKEND_DESCRIPTOR *d)
{
    PST_RESULT r;
    pst_size i;
    if (pst_backend_registry_sealed) return PST_RESULT_INVALID_STATE;
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
PST_RESULT pst_backend_register_manifest(const PST_BACKEND_DESCRIPTOR *const *descriptors,pst_size count)
{
    const PST_BACKEND_DESCRIPTOR *snapshot[PST_BACKEND_REGISTRY_CAPACITY];
    const PST_BACKEND_DESCRIPTOR *found;
    PST_RESULT result;
    pst_size snapshot_count;
    pst_size missing;
    pst_size added;
    pst_size i;
    pst_size j;
    if (pst_backend_registry_sealed) return PST_RESULT_INVALID_STATE;
    if (count == 0) return PST_RESULT_UNAVAILABLE;
    if (descriptors == NULL) return PST_RESULT_INVALID_ARGUMENT;
    missing = 0;
    for (i = 0; i < count; ++i) {
        result = pst_backend_validate(descriptors[i]);
        if (result != PST_RESULT_OK) return result;
        for (j = 0; j < i; ++j)
            if (strcmp(descriptors[j]->id,descriptors[i]->id) == 0)
                return PST_RESULT_INVALID_STATE;
        found = pst_backend_find(descriptors[i]->id);
        if (found != NULL && found != descriptors[i])
            return PST_RESULT_INVALID_STATE;
        if (found == NULL) ++missing;
    }
    if (missing > PST_BACKEND_REGISTRY_CAPACITY - pst_backend_registry_count)
        return PST_RESULT_RESOURCE_FAILURE;
    snapshot_count = pst_backend_registry_count;
    for (i = 0; i < snapshot_count; ++i) snapshot[i] = pst_backend_registry[i];
    added = 0;
    for (i = 0; i < count; ++i) {
        if (pst_backend_find(descriptors[i]->id) != NULL) continue;
        if (added == pst_backend_manifest_fail_after) {
            result = PST_RESULT_BACKEND_FAILURE;
            goto rollback;
        }
        result = pst_backend_register(descriptors[i]);
        if (result != PST_RESULT_OK) goto rollback;
        ++added;
    }
    pst_backend_manifest_fail_after = (pst_size)-1;
    return PST_RESULT_OK;
rollback:
    for (i = 0; i < snapshot_count; ++i) pst_backend_registry[i] = snapshot[i];
    for (i = snapshot_count; i < pst_backend_registry_count; ++i)
        pst_backend_registry[i] = NULL;
    pst_backend_registry_count = snapshot_count;
    pst_backend_manifest_fail_after = (pst_size)-1;
    return result;
}
void pst_backend_registry_seal(void) { pst_backend_registry_sealed = 1; }
int pst_backend_registry_is_sealed(void) { return pst_backend_registry_sealed; }
void pst_backend_test_manifest_fail_after(pst_size count)
{
    pst_backend_manifest_fail_after = count;
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
    pst_backend_registry_sealed = 0;
    pst_backend_manifest_fail_after = (pst_size)-1;
}
