#include "mpiUtils.h"

void prepareDataForAsymetricOperaton(int processCount, int dataLength, int* out_count, int* out_skip) {
    // Funtion is taken from iSod with small refactor
    // Autor: Z. Krawczyk

    const int partLength = dataLength / processCount; /* calkowita czesc podzialu */
    int reminder = dataLength % processCount; /* reszta podzialu */

    for (int i=0; i<processCount; i++) {
        out_count[i] = partLength;

        if (reminder-- > 0) /* jesli zostala reszta to dodaj ja do tego procesu */
            out_count[i]++;

        out_skip[i] = 0;
        /* dla count = [0, 2, 4, 5] => skip = [0, 0+2, 0+2+4, 0+2+4+5] */
        for (int j=0; j<i; j++)
            out_skip[i] += out_count[j];
    }
}