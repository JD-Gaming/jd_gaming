#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#ifndef BMP_H
#define BMP_H

typedef struct {
  /* Values pointed to are writable, but not the pointer itself */
  unsigned char * const data;
  /* These values are here for information purposes only, not to
      be messed with */
  const unsigned int width;
  const unsigned int height;
  const unsigned int bpp;
  const unsigned int cpp;
  const unsigned int pitch;
} bmpFile;

/* 
 * Opens an existing file and returns a bitmap file 
 *  handle on success.  Returns NULL on failure.
 * <skip> determines whether any actual image data are
 *  read from the file, or if it is skipped.
 */
bmpFile *openBitmap( const char *name, bool skip );

/*
 * Creates a new bitmap file according to the 
 *  specifications.  Returns a pointer to a new
 *  bitmap file handle on success.  Returns 
 *  NULL on failure.
 */
bmpFile *createBitmap( const char *name, 
					   unsigned int w, unsigned int h,
					   int bitsPerPixel );

/*
 * Saves data in an opened bitmap to the file.  Returns
 *  0 on success, -1 on failure.
 */
int saveBitmap( bmpFile *bitmap );

/*
 * Saves data within a rectangle in an opened bitmap to 
 *  the file.  Returns 0 on success, -1 on failure.
 */
int saveBitmapRect( bmpFile *bitmap,
		    int x_pos, int y_pos,
		    unsigned int width, unsigned int height );

/*
 * Re-reads data from file, effectively removing any changes
 *  done to the image data since the last save.  Returns 0
 *  on success and -1 on failure.
 */
int revertBitmap( bmpFile *bitmap );

/*
 * Closes a bitmap handle, frees any memory used.  
 *  Returns 0 on success, -1 on failure.
 */
int closeBitmap( bmpFile *bitmap );

/*
 * Sets the colour of pixel (x,y) to the <r,g,b> triplet.  Returns
 *  0 on success, -1 on failure.
 */
int setBitmapColour( bmpFile *bitmap, unsigned int x, unsigned int y, 
		     unsigned char a,
		     unsigned char r, unsigned char g, unsigned char b );
int setBitmapColourRed( bmpFile *bitmap, unsigned int x, unsigned int y, 
		     unsigned char r );
int setBitmapColourGreen( bmpFile *bitmap, unsigned int x, unsigned int y, 
		     unsigned char g );
int setBitmapColourBlue( bmpFile *bitmap, unsigned int x, unsigned int y, 
		     unsigned char b );

/*
 * Finds the colour of pixel (x,y) and saves it to the <r,g,b> triplet.
 *  Any of the three return values may be NULL, in which case they are
 *  simply ignored.  Returns 0 on success, -1 on failure.
 */
int getBitmapColour( bmpFile *bitmap, unsigned int x, unsigned int y, 
		     unsigned char *a,
		     unsigned char *r, unsigned char *g, unsigned char *b );

#endif /* BMP_H */

#ifdef __cplusplus
}
#endif
