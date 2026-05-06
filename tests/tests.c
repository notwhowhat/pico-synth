#include "mocks.h"

#include "midi.h"
#include "voice.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

void write_data(fixed arr[], uint32_t len) {
    FILE *f = fopen("plot.csv", "w");
    if (f != NULL) {
        for (int i = 0; i < len; i++) {
            fprintf(f, "%d,", i);
        }
        fprintf(f, "\n");
        for (int i = 0; i < len; i++) {
            fprintf(f, "%d,", arr[i]);
        }
    }
    fclose(f);
}

void plot_data(void) {
    system("/bin/python3 /mnt/c/Users/jakob/coding-related/micontroller-projects/npicosyn/scripts/plot.py");
}

int main(void) {
    fixed arr[5] = {1, -2, 55, -37, 21};
    write_data(arr, 5);
    plot_data();

    return 0;

}
