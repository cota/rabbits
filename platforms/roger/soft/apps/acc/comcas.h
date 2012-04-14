#ifndef _COMCAS_FS_H_
#define _COMCAS_FS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define COMCAS_N_CYCLES "/debug/comcas/n_cycles"

int comcas_get_attr_u64(const char *path, uint64_t *valp);

#ifdef __cplusplus
}
#endif

#endif /* _COMCAS_FS_H_ */
