/* SPDX-License-Identifier: MPL-2.0 */
#ifndef PST_NSS_CLOSE_STATE_H
#define PST_NSS_CLOSE_STATE_H
#include "papinho_secure_transport.h"
#define PST_NSS_DESCRIPTOR_LAYER_NO_NATIVE_RESOURCE 0UL
#define PST_NSS_DESCRIPTOR_LAYER_LOWER_NSPR 1UL
#define PST_NSS_DESCRIPTOR_LAYER_SSL_AGGREGATE 2UL
#define PST_NSS_DESCRIPTOR_LAYER_UNKNOWN_AFTER_CLOSE_FAILURE 3UL
#define PST_NSS_CLOSE_SNAPSHOT_NONE 0UL
#define PST_NSS_CLOSE_SNAPSHOT_NO_NATIVE_RESOURCE 1UL
#define PST_NSS_CLOSE_SNAPSHOT_CLOSED_CONFIRMED 2UL
#define PST_NSS_CLOSE_SNAPSHOT_UNRESOLVED_RETAINED 3UL
typedef struct pst_nss_close_state {pst_u32 close_attempted; pst_u32 descriptor_layer; pst_u32 close_snapshot; pst_i32 native_error; pst_u32 native_error_valid;} pst_nss_close_state;
static void pst_nss_close_state_initialize(pst_nss_close_state *s){s->close_attempted=0UL;s->descriptor_layer=PST_NSS_DESCRIPTOR_LAYER_NO_NATIVE_RESOURCE;s->close_snapshot=PST_NSS_CLOSE_SNAPSHOT_NONE;s->native_error=0;s->native_error_valid=0UL;}
static void pst_nss_close_state_set_layer(pst_nss_close_state *s,pst_u32 layer){if(!s->close_attempted)s->descriptor_layer=layer;}
static int pst_nss_close_state_begin(pst_nss_close_state *s){if(s->close_attempted)return 0;s->close_attempted=1UL;return 1;}
static void pst_nss_close_state_no_resource(pst_nss_close_state *s){s->descriptor_layer=PST_NSS_DESCRIPTOR_LAYER_NO_NATIVE_RESOURCE;s->close_snapshot=PST_NSS_CLOSE_SNAPSHOT_NO_NATIVE_RESOURCE;s->native_error=0;s->native_error_valid=0UL;}
static void pst_nss_close_state_confirmed(pst_nss_close_state *s){s->descriptor_layer=PST_NSS_DESCRIPTOR_LAYER_NO_NATIVE_RESOURCE;s->close_snapshot=PST_NSS_CLOSE_SNAPSHOT_CLOSED_CONFIRMED;s->native_error=0;s->native_error_valid=0UL;}
static void pst_nss_close_state_unresolved(pst_nss_close_state *s,pst_u32 layer,pst_i32 error,pst_u32 valid){s->descriptor_layer=layer;s->close_snapshot=PST_NSS_CLOSE_SNAPSHOT_UNRESOLVED_RETAINED;s->native_error=error;s->native_error_valid=valid;}
#endif
