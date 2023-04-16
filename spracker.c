#define _DEFAULT_SOURCE

#include <dirent.h>
#include <float.h>
#include <getopt.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tga.h"
#include "spr.h"

#define COORDS_TO_INDEX(x, y, width) (y * width + x)

// Define color channels
#define RED 0
#define GREEN 1
#define BLUE 2

const char tga_extension[] = ".tga";

char* strcomb(unsigned int num, ...)
{
	va_list valist;

	char *combined_str;
	unsigned int combined_str_s = 1;

	char *tmp_str;
	unsigned int i;

	va_start(valist, num);
	combined_str = calloc(1, sizeof(char));

	for (i = 0; i < num; i++) {
		tmp_str = va_arg(valist, char*);

		combined_str_s += strlen(tmp_str);
		combined_str = realloc(combined_str, combined_str_s);

		strcat(combined_str, tmp_str);
	}
	va_end(valist);

	return combined_str;
}

int pstrcmp( const struct dirent** a, const struct dirent** b )
{
	int a_size = strlen((*a)->d_name);
	int b_size = strlen((*b)->d_name);
	
	if (a_size == b_size)
		return strcmp( (const char*)(*a)->d_name, (const char*)(*b)->d_name );
	
	if (a_size < b_size)
		return -1;
	else
		return 1;
}

char *substr(char *start, char *end) {
	int length = end - start;
	char *sub = calloc(length + 1, sizeof(char));
	strncpy(sub, start, length);
	sub[length] = '\0';
	return sub;
}

void add_tga_to_spr(SPR *spr, TGA *tga)
{
	int i, palette_index;
	int image_size = 0;
	static int frame_count = 0;
	uint8_t smallest_distance_index;
	double current_squared_distance, previous_squared_distance;
	
	if (frame_count == 0) {
		spr->frames = malloc(sizeof(spr->frames));
	} else {
		spr->frames = realloc(spr->frames, sizeof(spr->frames) * (spr->frame_count + 1));
	}
	
	spr->frames[frame_count] = malloc(sizeof(SPR_FRAME));
	
	spr->frames[frame_count]->frame_group = 0;
	spr->frames[frame_count]->origin_x = 0 - tga->imageSpec.width / 2;
	spr->frames[frame_count]->origin_y = tga->imageSpec.height / 2;
	spr->frames[frame_count]->width = tga->imageSpec.width;
	spr->frames[frame_count]->height = tga->imageSpec.height;
	
	/* Change max width and height */
	if (spr->frames[frame_count]->width > spr->frame_max_width)
		spr->frame_max_width = spr->frames[frame_count]->width;
	
	if (spr->frames[frame_count]->height > spr->frame_max_height)
		spr->frame_max_height = spr->frames[frame_count]->height;
	
	image_size = spr->frames[frame_count]->width * spr->frames[frame_count]->height;
	spr->frames[frame_count]->image_data = malloc(image_size);
	
	for (i=0; i < image_size; i++) {
		uint8_t red = tga->colorMapData[tga->imageData[i]].red;
		uint8_t green = tga->colorMapData[tga->imageData[i]].green;
		uint8_t blue= tga->colorMapData[tga->imageData[i]].blue;
		
		/* Check if color is present in the palette */
		for (palette_index=0; palette_index < spr->palette.palette_size; palette_index++) {
			if (spr->palette.palette[palette_index * 3] == red && spr->palette.palette[palette_index * 3 + 1] == green && spr->palette.palette[palette_index * 3 + 2] == blue) {
				spr->frames[frame_count]->image_data[i] = palette_index;
				break;
			}
		}
		
		/* If color was not in palette and the palette is full, find closest match */
		if (palette_index == 256) {
			previous_squared_distance = DBL_MAX;
			
			for (palette_index=0; palette_index < spr->palette.palette_size; palette_index++) {
				current_squared_distance =
				pow(red - spr->palette.palette[palette_index * 3 + RED], 2)
				+ pow(green - spr->palette.palette[palette_index * 3 + GREEN], 2)
				+ pow(blue - spr->palette.palette[palette_index * 3 + BLUE], 2);
				if ( current_squared_distance < previous_squared_distance) {
					previous_squared_distance = current_squared_distance;
					smallest_distance_index = palette_index;
				}
			}
			spr->frames[frame_count]->image_data[i] = smallest_distance_index;
		/* Add color to color pallete */
		} else if (palette_index == spr->palette.palette_size) {
			if (spr->palette.palette_size == 0) {
				spr->palette.palette = malloc(3);
			} else {
				spr->palette.palette = realloc(spr->palette.palette,
									  (spr->palette.palette_size + 1) * 3);
			}
			
			spr->palette.palette[spr->palette.palette_size * 3 + RED] = red;
			spr->palette.palette[spr->palette.palette_size * 3 + GREEN] = green;
			spr->palette.palette[spr->palette.palette_size * 3 + BLUE] = blue;
			spr->frames[frame_count]->image_data[i] = spr->palette.palette_size;
			spr->palette.palette_size++;
		}
	}
	
	/* Here we write the size after we incremented frame_count, since the size is
	 always the current index + 1 */
	frame_count++;
	spr->frame_count = frame_count;
}

void convert_tga_to_spr(SPR *spr, char *file_path) {
	TGA tga;
	tga_load_file(&tga, file_path);
	add_tga_to_spr(spr, &tga);
}

int main(int argc, char *argv[])
{
	int opt;
	int n;
	struct dirent **namelist;
	char *dir_path = "./"; // Use root when nothing supplied
	char *output_file = "./output.spr";
	char *file_path, *extension_start;

	while ((opt = getopt(argc, argv, "d:o:h")) != -1) {
	switch (opt) {
		case 'd':
			dir_path = optarg;
			break;
		case 'o':
			output_file = optarg;
			break;
		case 'h': /* Add help function here */
			exit(0);
			break;
		case '?':
			exit(1);
			break;
		default:
			fprintf(stderr, "No options given\n");
			exit(1);
		}
	}

	// Always add extra '/' to complete path to file
	if (dir_path[strlen(dir_path) - 1] != '/')
		dir_path = strcat(dir_path, "/");
	printf("Dir path: %s\n\n", dir_path);
	
	SPR spr = spr_init(SPR_TYPE_VP_PARALLEL, SPR_TEX_FORMAT_ADDITIVE,
					   SPR_SYNC_TYPE_RANDOM);
	
	// Find all .tga files in dir
	n = scandir("test_files/", &namelist, NULL, pstrcmp);
	if (n < 0)
		perror("scandir");
	else {
		for (int i = 0; i < n; i++) {
			// Find last '.' to get extension 
			extension_start = strrchr(namelist[i]->d_name, '.'); 
			
			if(extension_start == NULL)
				continue;
			// Load tga into wad when it is a tga file
			if(strcmp(extension_start, tga_extension) == 0){
				file_path = strcomb(2, dir_path, namelist[i]->d_name);

				convert_tga_to_spr(&spr, file_path);
				printf("tga file added to spr: %s\n", file_path);
			}
			free(namelist[i]);
		}
	}
	free(namelist);

	spr_update(&spr);
	spr_save_file(&spr, output_file);

	return 0;
}
