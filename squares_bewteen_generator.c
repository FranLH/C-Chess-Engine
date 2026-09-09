#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

int main(void) {
    FILE *f = fopen("squares_data.c", "w");
    if (!f) {
        printf("Error: Could not create squares_data.c!\n");
        return 1;
    }

    // Write the headers and the full array definition text directly into the file
    fprintf(f, "#include <stdint.h>\n");
    fprintf(f, "#include \"squares_data.h\"\n\n");
    fprintf(f, "const uint64_t squares_between[64][64] = {\n");

    for (int a = 0; a < 64; a++) {
        fprintf(f, "{ ");
        for (int b = 0; b < 64; b++) {
            uint64_t mask = 0ULL;

            int rankA = a / 8;
            int fileA = a % 8;
            int rankB = b / 8;
            int fileB = b % 8;

            int rDelta = rankB - rankA;
            int fDelta = fileB - fileA;

            if (a != b && (rDelta == 0 || fDelta == 0 || abs(rDelta) == abs(fDelta))) {
                int rStep = (rDelta > 0) - (rDelta < 0);
                int fStep = (fDelta > 0) - (fDelta < 0);

                int row = rankA;
                int col = fileA;

                row += rStep;
                col += fStep;

                while (row >= 0 && row < 8 && col >= 0 && col < 8 && row * 8 + col != b) {
                    int cur = row * 8 + col;
                    mask |= 1ULL << cur;
                    row += rStep;
                    col += fStep;
                }
            }

            fprintf(f, "0x%016llXULL", (unsigned long long)mask);
            if (b < 63) fprintf(f, ", ");
        }

        if (a < 63) fprintf(f, " },\n");
        else        fprintf(f, " }\n");
    }

    fprintf(f, "};\n");
    fclose(f);
    puts("Successfully generated squares_data.c");
    return 0;
}