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
	uint16_t num_slices = 360;
	uint16_t framerate = 15000;
	uint16_t num_frames = 15;
	float pixels_to_change = 0.01;

	const uint8_t NEXT_FRAME = 0xFF;
	const uint8_t NEXT_SLICE = 0xFE;

	if (argc == 1) printf("Using default parameters.\n"); 

	char* filename = "pixelmap.bin";
	if (argc > 1) filename = argv[1];

	FILE* file;
	file = fopen(filename, "wb");

	if (argc > 2) num_slices = atoi(argv[2]);
	if (argc > 3) framerate = atoi(argv[3]);
	if (argc > 4) num_frames = atoi(argv[4]);
	if (argc > 5) pixels_to_change = atof(argv[5]);
	printf("Creating \"%s\", with num_slices = %d, framerate = %d, num_frames = %d, pixels_to_change = %f\n", 
			filename, num_slices, framerate, num_frames, pixels_to_change);

	uint32_t rand_num = pixels_to_change * INT_MAX;

	// Beginning.
	fwrite(&num_slices, 2, 1, file);
	fwrite(&framerate, 2, 1, file);

	fwrite(&NEXT_FRAME, 1, 1, file);
	for (int slice = 0; slice < num_slices; ++slice) {
		fwrite(&NEXT_SLICE, 1, 1, file);
		for (int row = 0; row < NUM_ROWS; ++row) {
			for (int col = 0; col < NUM_COLS; ++col) {
				uint8_t c = 255;
				uint8_t z = 0;
				if (row == 0 && col < 3){
					fwrite(&row, 1, 1, file);
					fwrite(&col, 1, 1, file);
					if (col == 0) {
						fwrite(&c, 1, 1, file);
						fwrite(&z, 1, 1, file);
						fwrite(&z, 1, 1, file);
					} else if (col == 1) {
						fwrite(&z, 1, 1, file);
						fwrite(&c, 1, 1, file);
						fwrite(&z, 1, 1, file);
					} else if (col == 2) {
						fwrite(&z, 1, 1, file);
						fwrite(&z, 1, 1, file);
						fwrite(&c, 1, 1, file);
					}
					//uint8_t r = row * 255 / NUM_ROWS;
					//uint8_t g = col * 255 / NUM_COLS;
					//uint8_t b = slice * 255 / num_slices;
					//fwrite(&r, 1, 1, file);
					//fwrite(&g, 1, 1, file);
					//fwrite(&b, 1, 1, file);
				}
			} 
		}
	}

	/*
	// Middle
	for (int frame = 0; frame < num_frames; ++frame) {
		fwrite(&NEXT_FRAME, 1, 1, file);
		for (int slice = 0; slice < num_slices; ++slice) {
			fwrite(&NEXT_SLICE, 1, 1, file);
			for (int row = 0; row < NUM_ROWS; ++row) {
				for (int col = 0; col < NUM_COLS; ++col) {
					if (rand() < rand_num) {
						fwrite(&row, 1, 1, file);
						fwrite(&col, 1, 1, file);
						// uint32_t rgb = rand();
						uint32_t rgb = 0;
						uint8_t* rgb_ptr = (uint8_t*) &rgb;
						fwrite(rgb_ptr, 1, 3, file);
					}
				} 
			}
		}
	}
	*/

	fclose(file);
}


