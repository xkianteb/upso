// 8-bit per channel images
#ifndef Image_hpp
#define Image_hpp

#include <stdio.h>
#include <string>

////////////////////
// general image
struct Image {
	std::string name;				   // image name
	unsigned int width, height, depth; // image size

	unsigned char *data;  // image data in [height][width][channels] order

// public methods
public:
	Image();					// empty image
	~Image();					// free data when done

	// copy image with data (constructor or assignment)
	Image(const Image &);
	Image &operator=(const Image &);

	// individual pixel as image(x,y)[channel] (versions for Image and const Image)
	const unsigned char *operator()(unsigned int tx, unsigned int ty) const;
	unsigned char *operator()(unsigned int tx, unsigned int ty);

	// allocate image data after width, height and depth are filled in
	void alloc();
};

inline const unsigned char *Image::operator()(unsigned int tx, unsigned int ty) const {
	return data + (tx + ty*width)*depth;
}
inline unsigned char *Image::operator()(unsigned int tx, unsigned int ty) {
	return data + (tx + ty*width)*depth;
}

// read header from PAM file, return opened file handle
// Set image sizes based on header
FILE *PAMopen(const char *filename, Image &image);

// Finish read of image data after PAMopen. Closes file handle when done
void PAMread(FILE *imageFile, Image &image);

#endif
