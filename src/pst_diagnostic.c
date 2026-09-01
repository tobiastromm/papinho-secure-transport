#include "pst_diagnostic.h"
#include <string.h>
void pst_diagnostic_initialize(pst_internal_diagnostic *d){if(d)memset(d,0,sizeof(*d));}
void pst_diagnostic_clear(pst_internal_diagnostic *d){pst_u32 g;if(!d)return;g=d->generation+1UL;memset(d,0,sizeof(*d));d->generation=g;}
void pst_diagnostic_copy(pst_internal_diagnostic *d,const pst_internal_diagnostic *s){if(!d||!s||d==s)return;memcpy(d,s,sizeof(*d));}
void pst_diagnostic_capture(pst_internal_diagnostic *d,PST_RESULT r,pst_u32 p,const char *id,pst_u32 domain,pst_i32 code,pst_i32 secondary,pst_u32 flags){pst_u32 g;pst_size n;if(!d)return;g=d->generation+1UL;memset(d,0,sizeof(*d));d->generation=g;d->valid=1UL;d->result=r;d->phase=p;d->native_domain=domain;d->native_code=code;d->secondary_native_code=secondary;d->flags=flags;if(id){n=strlen(id);if(n>=PST_DIAGNOSTIC_BACKEND_ID_CAPACITY)n=PST_DIAGNOSTIC_BACKEND_ID_CAPACITY-1;memcpy(d->backend_id,id,n);d->backend_id[n]='\0';}}