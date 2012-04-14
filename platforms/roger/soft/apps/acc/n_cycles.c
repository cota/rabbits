#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "comcas.h"

int main(int argc, char *argv[])
{
	uint64_t val;

	if (comcas_get_attr_u64(COMCAS_N_CYCLES, &val)) {
		perror("n_cycles");
		exit(EXIT_FAILURE);
	}
	printf("%lld\n", val);
}
