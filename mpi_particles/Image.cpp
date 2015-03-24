// 8-bit per channel images

#include "Image.hpp"
#include <stdio.h>
#include <string.h>
#include <assert.h>


#ifdef _WIN32
// don't complain if we use standard IO functions instead of windows-only
#pragma warning( disable: 4996 )
#endif


// blank image, to be filled in later
Image::Image() {width=height=depth=0; data=0;}

// free data when done
Image::~Image() {
	delete[] data;
}

// copy constructor
Image::Image(const Image &im)
{
	name  = im.name;
	width = im.width;
	height= im.height;
	depth = im.depth;

	// deep copy if image has data
	if (im.data) {
		alloc();
		memcpy(data, im.data, width*height*depth);
	}
	else
		data = 0;
}

// copy on assignment
Image &Image::operator=(const Image &im)
{
	// copy meta-data
	name   = im.name;
	width  = im.width;
	height = im.height;
	depth  = im.depth;

	// deep copy if image has data
	delete[] data;				// free old data
	if (im.data) {
		alloc();
		memcpy(data, im.data, width*height*depth);
	}
	else
		data = 0;

	return *this;
}

// (re)allocate, given image size
void Image::alloc() {
	data = new unsigned char[height*width*depth];
}

// read header from PAM file, return opened file handle
// set width, height, and depth based on header
FILE *PAMopen(const char *filename, Image &image) {
	image.name = filename;

	// open file
	FILE *fp = fopen(filename,"rb");
	if (!fp) {
		perror(filename);		// print what happened
		assert(false);			// assert to catch in debugger
	}

	// check that "magic number" at beginning of file is P7
	if (fgetc(fp) != 'P' || fgetc(fp) != '7' || fgetc(fp) != '\n') {
		fprintf(stderr, "unknown image format %s\n", filename);
		fclose(fp);
		assert(false);
	}

	// read image header
	image.width = image.height = image.depth = 0;
	int maxval = 0;
	char line[256];
	while(fgets(line, sizeof(line), fp)) {
		if (sscanf(line, "WIDTH %d",  &image.width )) continue;
		if (sscanf(line, "HEIGHT %d", &image.height)) continue;
		if (sscanf(line, "DEPTH %d",  &image.depth )) continue;
		if (sscanf(line, "MAXVAL %d", &maxval	   )) continue;
		if (strcmp(line, "ENDHDR\n") == 0) break;
	}
	assert(image.width > 0 && image.height > 0); // legal size?
	assert(image.depth > 0 && image.depth <= 4 && maxval == 255); // format?

	return fp;
}

// Finish read of image data after PAMopen. Closes file handle when done
void PAMread(FILE *fp, Image &image) {
	image.alloc();
	fread(image.data, image.depth, image.width * image.height, fp);
	fclose(fp);
}
