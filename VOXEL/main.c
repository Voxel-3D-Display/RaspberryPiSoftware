//-------------------------------------------------------------------------
//
// The MIT License (MIT)
//
// Copyright (c) 2013 Andrew Duncan
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//-------------------------------------------------------------------------

#define _GNU_SOURCE

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include <inttypes.h>
#include <limits.h>

#include "bcm_host.h"

#include "backgroundLayer.h"
#include "image.h"
#include "key.h"

#include "./matrixmap.h"

//-----------------------------------------------------------------------

#define NUM_ROWS 30
#define NUM_COLS 48

#define NDEBUG

//-----------------------------------------------------------------------

const char *program = NULL;

//-------------------------------------------------------------------------


int main(int argc, char *argv[])
{
	int opt = 0;

	const char* imageTypeName = "RGBA32";
	VC_IMAGE_TYPE_T imageType = VC_IMAGE_MIN;
	uint16_t  background = 0x000F;
	uint32_t displayNumber = 0;
	uint32_t hold = 1;
	char* filename = "bins/bigboi.bin";

	program = basename(argv[0]);

	//-------------------------------------------------------------------

	while ((opt = getopt(argc, argv, "b:d:t:f:h:")) != -1)
	{
		switch (opt)
		{
			case 'b':

				background = strtol(optarg, NULL, 16);
				break;

			case 'd':

				displayNumber = strtol(optarg, NULL, 10);
				break;

			case 't':

				imageTypeName = optarg;
				break;

			case 'f':
				filename = optarg;
				break;

			case 'h':
				hold = atoi(optarg);
				break;

			default:

				fprintf(stderr, "Usage: %s \n", program);
				fprintf(stderr, "[-b <RGBA>] [-d <number>] [-t <type>]\n");
				fprintf(stderr, "    -b - set background colour 16 bit RGBA\n");
				fprintf(stderr, "         e.g. 0x000F is opaque black\n");
				fprintf(stderr, "    -d - Raspberry Pi display number\n");
				fprintf(stderr, "    -t - type of image to create\n");
				fprintf(stderr, "         can be one of the following:");
				printImageTypes(stderr,
						" ",
						"",
						IMAGE_TYPES_WITH_ALPHA |
						IMAGE_TYPES_DIRECT_COLOUR);
				fprintf(stderr, "\n");

				exit(EXIT_FAILURE);
				break;
		}
	}

	//-------------------------------------------------------------------

	IMAGE_TYPE_INFO_T typeInfo;

	if (findImageType(&typeInfo,
				imageTypeName,
				IMAGE_TYPES_WITH_ALPHA | IMAGE_TYPES_DIRECT_COLOUR))
	{
		imageType = typeInfo.type;
	}
	else
	{
		fprintf(stderr,
				"%s: unknown image type %s\n",
				program,
				imageTypeName);

		exit(EXIT_FAILURE);
	}

	//---------------------------------------------------------------------

	bcm_host_init();

	//---------------------------------------------------------------------

	DISPMANX_DISPLAY_HANDLE_T display
		= vc_dispmanx_display_open(displayNumber);
	assert(display != 0);

	//---------------------------------------------------------------------

	DISPMANX_MODEINFO_T info;
	int result = vc_dispmanx_display_get_info(display, &info);
	assert(result == 0);

	//---------------------------------------------------------------------

	BACKGROUND_LAYER_T backgroundLayer;
	initBackgroundLayer(&backgroundLayer, background, 0);

	//---------------------------------------------------------------------

	IMAGE_T bitmap_image;
	initImage(&bitmap_image, imageType, info.width, info.height, false);

	uint32_t vc_bitmap_image_ptr;

	DISPMANX_RESOURCE_HANDLE_T bitmap_image_front_resource = 
		vc_dispmanx_resource_create(imageType, 
				info.width | (bitmap_image.pitch << 16),
				info.height | (bitmap_image.alignedHeight << 16),
				&vc_bitmap_image_ptr);

	DISPMANX_RESOURCE_HANDLE_T bitmap_image_back_resource = 
		vc_dispmanx_resource_create(imageType, 
				info.width | (bitmap_image.pitch << 16),
				info.height | (bitmap_image.alignedHeight << 16),
				&vc_bitmap_image_ptr);

	result = 0;

	VC_RECT_T dst_rect;
	vc_dispmanx_rect_set(&dst_rect, 0, 0, bitmap_image.width, bitmap_image.height);
	result = vc_dispmanx_resource_write_data(bitmap_image_front_resource,
			bitmap_image.type, bitmap_image.pitch, bitmap_image.buffer, &dst_rect);

	assert(result == 0);

	//---------------------------------------------------------------------

	DISPMANX_UPDATE_HANDLE_T update = vc_dispmanx_update_start(0);
	assert(update != 0);

	VC_DISPMANX_ALPHA_T alpha = { DISPMANX_FLAGS_ALPHA_FROM_SOURCE, 0, 0 }; 
	VC_RECT_T src_rect;
	vc_dispmanx_rect_set(&src_rect, 0, 0, bitmap_image.width << 16, bitmap_image.height << 16);
	vc_dispmanx_rect_set(&dst_rect, 0, 0, bitmap_image.width, bitmap_image.height);
	DISPMANX_ELEMENT_HANDLE_T bitmap_image_element = 
		vc_dispmanx_element_add(update, display, 2000, &dst_rect,
				bitmap_image_front_resource, &src_rect,
				DISPMANX_PROTECTION_NONE, &alpha, NULL,
				DISPMANX_NO_ROTATE);

	addElementBackgroundLayer(&backgroundLayer, display, update);

	result = vc_dispmanx_update_submit_sync(update);
	assert(result == 0);

	int key;

	do {
		key = 0;

		uint16_t num_slices, framerate;

		const uint8_t NEXT_FRAME = 0xFF;
		const uint8_t NEXT_SLICE = 0xFE;

		FILE* file;
		file = fopen(filename, "rb");
		if(!file) exit(-1);

		fread(&num_slices, 2, 1, file);
		fread(&framerate, 2, 1, file);

		printf("Reading \"%s\", hold = %d, with num_slices = %d, framerate = %d\n", 
				filename, hold, num_slices, framerate);

		uint32_t frame = 0, slice = 0, interval = (1000000*1000)/framerate;
		uint8_t op = 1, nleave = 1;

		struct timeval start_time;
		gettimeofday(&start_time, NULL);

		struct timeval prev_time;
		gettimeofday(&prev_time, NULL);

		for(int slice = 0; slice < 360; ++slice){
			for(int row = 0; row < 30; ++row){
				for(int col = 0; col < 48; ++col){
					RGBA8_T color = { 0, 0, 0, 255 };
					uint16_t xind, yind;
					matrixmap(&xind, &yind, col, row, slice);
					setPixelRGB(&bitmap_image, xind, yind, &color);
				}
			}
		}

		do {
			keyPressed(&key);

			struct timeval curr_time;
			gettimeofday(&curr_time, NULL);
			struct timeval time_diff;
			timersub(&curr_time, &prev_time, &time_diff);

			// printf("%ld,%ld\n",interval, time_diff.tv_usec);

			if(time_diff.tv_usec >= interval && nleave){
				gettimeofday(&prev_time, NULL);

				// printf("\nNow Reading Frame %d\n", frame);

				op = 1;
				slice = -1;
				while (op != NEXT_FRAME && nleave) {
					nleave = fread(&op, 1, 1, file);
					if (op == NEXT_SLICE) {
						++slice;
					} else if(op != NEXT_FRAME) {
						uint8_t col, r, g, b;
						fread(&col, 1, 1, file);
						fread(&r, 1, 1, file);
						fread(&g, 1, 1, file);
						fread(&b, 1, 1, file);
						//printf("\t(%d, %d, %d, %x, %x, %x)\n", slice, op, col, r, g, b);

						RGBA8_T color = { r, g, b, 255 };

						uint16_t xind, yind;
						matrixmap(&xind, &yind, col, op, slice);
						// printf("%d, %d\n", xind, yind);
						setPixelRGB(&bitmap_image, xind, yind, &color);
					}
				}
				++frame;

				// UPDATE DISPLAY

				vc_dispmanx_rect_set(&dst_rect, 0, 0, bitmap_image.width, bitmap_image.height);
				result = vc_dispmanx_resource_write_data(bitmap_image_back_resource,
						bitmap_image.type, bitmap_image.pitch, bitmap_image.buffer, &dst_rect); // assert(result == 0);

				update = vc_dispmanx_update_start(0); // assert(update != 0);

				result = vc_dispmanx_element_change_source(update, 
						bitmap_image_element, bitmap_image_back_resource); // assert(result == 0);
				DISPMANX_RESOURCE_HANDLE_T tmp_rh = bitmap_image_front_resource;
				bitmap_image_front_resource = bitmap_image_back_resource;
				bitmap_image_back_resource = tmp_rh;

				result = vc_dispmanx_update_submit_sync(update); // assert(result == 0);
			}
		} while (key != 27 && key != 96 && (nleave || hold == 1));

		fclose(file);

		//---------------------------------------------------------------------

		struct timeval end_time;
		gettimeofday(&end_time, NULL);

		struct timeval diff;
		timersub(&end_time, &start_time, &diff);

		int32_t time_taken = (diff.tv_sec * 1000000) + diff.tv_usec;
		double frames_per_second = (frame * 1000000.0) / time_taken;

		printf("%0.1f frames per second\n", frames_per_second);

	} while((key != 27 && hold == 2) || key == 96);

	//---------------------------------------------------------------------

	keyboardReset();

	//---------------------------------------------------------------------

	destroyBackgroundLayer(&backgroundLayer);

	//---------------------------------------------------------------------

	result = vc_dispmanx_display_close(display);
	assert(result == 0);

	//---------------------------------------------------------------------

	return 0;
}
