#include <time.h>
#include <stdio.h>

#include "utils.h"

clock_t tic_time;

void tic() {
    tic_time = clock();
}
void toc() {
    clock_t now = clock();
    printf("Time Elapsed: %.1f us\n", 1E6 * (float) (now - tic_time) / CLOCKS_PER_SEC);
    fflush(stdout);
}