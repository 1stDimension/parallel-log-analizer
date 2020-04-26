#pragma once

/* generowanie tablic count i skip dla MPI_Scatterv/Gatherv na podstawie ilosci */
/* slow nwords i liczby procesow size */
void prepareDataForAsymetricOperaton(int processCount, int dataLength, int* out_count, int* out_skip);
