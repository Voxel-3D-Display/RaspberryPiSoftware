
/**
 * Maps matrix (x,y,slice) --> hdmi (x,y).
 * Accounts for desired ordering on FPGA side.
 * Desired: order that they would be in to directly write the LED drivers.
 *
 * 
 */


#ifndef MATRIXMAP_H
#define MATRIXMAP_H

#include "stdio.h"

// SUBMATRIX INDECES TO DRIVER INDEX
const uint8_t __subi_driver__ [4][4] = {
	{ 7, 3, 11, 15 },
	{ 6, 2, 10, 14 },
	{ 1, 5, 13,  9 },
	{ 4, 0, 12,  8 }
};
/*
const uint8_t __subi_driver__ [4][4] = {
	{ 1, 2, 3, 4 },
	{ 5, 6, 7, 8 },
	{ 9, 10, 11, 12  },
	{ 13, 14, 15, 16 }
};
*/

/**
 * Outputs HDMI (x,y) indices on pointer inputs hdmi_x and hdmi_y given matrix (x,y,slice) indices matrix_x, matrix_y, slice.
 */
uint8_t matrixmap (uint16_t* hdmi_x, uint16_t* hdmi_y, uint16_t matrix_x, uint16_t matrix_y, uint16_t slice) {
	if(!hdmi_x || !hdmi_y) return -1;

	uint8_t submatrix = matrix_x / 4; // 0 to 11
	uint8_t subdriver = matrix_y / 4; // 0 to 7
	uint8_t driver = (submatrix << 3) | subdriver; // 0 to 95
	uint8_t sdo = driver >> 1; // 0 to 47 (last bits of driver)
	uint8_t daisy = driver % 2; // 0 to 1 (first bit of driver)
	uint8_t sub_idx = 15 - __subi_driver__[matrix_y % 4][matrix_x % 4]; // 0 to 15

	*hdmi_x = 48*sub_idx + sdo;
	*hdmi_y = slice*2 + daisy;

	//printf("matrix_x: %d, matrix_y: %d\n", matrix_x, matrix_y);
	//printf("submatrix: %d, subdriver: %d, driver: %d, sdo: %d, daisy: %d, sub_idx: %d, sdo: %d, daisy: %d, slice: %d\n", 
			//submatrix, subdriver, driver, sdo, daisy, sub_idx, sdo, daisy, slice);
	//printf("*hdmi_x: %d, *hdmi_y: %d\n\n", *hdmi_x, *hdmi_y);


	/*
	uint8_t output = driver / 2; // 0 to 3
	uint8_t 
	uint8_t daisy = driver % 2;  // 0 to 1

	*hdmi_x = (submatrix*4 + output)*16 + sub_idx;
	*hdmi_y = slice*2 + daisy;
	*/

	// DRIVER BY DRIVER (ALL FOR EACH)
	// *hdmi_x = (submatrix*8 + driver)*16 + __subi_driver__[matrix_y % 4][matrix_x % 4];
	// *hdmi_y = slice;

	return 0;
}


#endif
