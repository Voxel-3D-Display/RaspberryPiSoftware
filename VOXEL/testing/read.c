/*
int main() {
	FILE* file = fopen("pixelmap.bin", "rb");
	if(!file) return -1;
	uint16_t row,col;
	uint8_t r,g,b;
	fread(&row, 2, 1, file);
	fread(&col, 2, 1, file);
	fread(&r, 1, 1, file);
	fread(&g, 1, 1, file);
	fread(&b, 1, 1, file);

	printf("%d, %d, %d, %d, %x\n", row, col, r, g, b);
}

*/

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

/*
WILL ALWAYS COMPACT 48x30 SLICES INTO 1920x1080 FRAME.

BEGINNING OF FILE: [2B, 2B]
uint16_t for number of slices
uint16_t for 1000x framerate (frames in 1000s)

MIDDLE OF FILE: [1B ...]

uint8_t specifies operation:
	0xFF = next frame
	0xFE = next slice
	0x__ = next pixel, row index

uint8_t col, uint8_t r, uint8_t g, uint8_t b

This protocol will keep the pixels held at their location.
*/

#define NUM_ROWS 48
#define NUM_COLS 30

int main(int argc, char* argv[]) {
	uint16_t num_slices, framerate;

	const uint8_t NEXT_FRAME = 0xFF;
	const uint8_t NEXT_SLICE = 0xFE;

	if (argc == 1) printf("Using default parameters.\n"); 

	char* filename = "pixelmap.bin";
	if (argc > 1) filename = argv[1];

	FILE* file;
	file = fopen(filename, "rb");
	if(!file) return -1;

	fread(&num_slices, 2, 1, file);
	fread(&framerate, 2, 1, file);

	printf("Reading \"%s\", with num_slices = %d, framerate = %d", 
			filename, num_slices);

	uint16_t frame = 0, slice = 0;
	size_t ret;

	uint8_t op;
	while (fread(&op, 1, 1, file)) {
		if (op == NEXT_FRAME) {
			printf("\nNow Reading Frame %d\n", ++frame);
			slice = 0;
		} else if (op == NEXT_SLICE) {
			++slice;
		} else {
			uint8_t col, r, g, b;
			fread(&col, 1, 1, file);
			fread(&r, 1, 1, file);
			fread(&g, 1, 1, file);
			fread(&b, 1, 1, file);
			printf("\t(%d, %d, %d, %x, %x, %x)\n", slice, op, col, r, g, b);
		}
	} 

	fclose(file);
}


