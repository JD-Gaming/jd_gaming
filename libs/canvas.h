#ifdef __cplusplus
extern "C" {
#endif

#ifndef CANVAS_H
#define CANVAS_H

/****************************************************************/
/*          INCLUDE FILES                                       */
/****************************************************************/
#include <stdint.h>

/****************************************************************/
/*          DEFINES                                             */
/****************************************************************/
#ifdef MIN
#  undef MIN
#endif
#define MIN(a,b) (((a)<(b))?(a):(b))

#ifdef MAX
#  undef MAX
#endif
#define MAX(a,b) (((a)>(b))?(a):(b))


/****************************************************************/
/*          TYPES                                               */
/****************************************************************/

typedef enum {
  /* Color systems */
  Canvas_CtRgb    = 0x00,
  Canvas_CtRg     = 0x01,
  Canvas_CtGray   = 0x02,
  Canvas_CtYCbCr  = 0x03,
  Canvas_CtYCbCrs = 0x04,
  Canvas_CtCmy    = 0x05,
  Canvas_CtCmyk   = 0x06,
  Canvas_CtHsl    = 0x07,
  Canvas_CtHsv    = 0x08,
  Canvas_CtXyz    = 0x09,
  Canvas_CtXyy    = 0x0a,
  Canvas_CtColourMask = 0x0f,

  /* Extra bits */
  Canvas_CtAlpha  = 0x10,
} Canvas_ColourType;

typedef struct {
  /* Canvas_Palette *palette; */
  uint8_t   BitsPerPixel;
  uint8_t   BytesPerPixel;
  Canvas_ColourType cType;
  union {
    struct {
      uint8_t   Rloss,  Gloss,  Bloss,  Aloss;
      uint8_t   Rshift, Gshift, Bshift, Ashift;
      uint64_t  Rmask,  Gmask,  Bmask,  Amask;
    } rgb;
    struct {
      uint8_t   Rloss,  Gloss,  Aloss;
      uint8_t   Rshift, Gshift, Ashift;
      uint64_t  Rmask,  Gmask,  Amask;
    } rg;
    struct {
      uint8_t   Gloss,  Aloss;
      uint8_t   Gshift, Ashift;
      uint64_t  Gmask,  Amask;
    } gray;
    struct {
      uint8_t   Yloss,  Bloss,  Rloss,  Aloss;
      uint8_t   Yshift, Bshift, Rshift, Ashift;
      uint64_t  Ymask,  Bmask,  Rmask,  Amask;
    } ycbcr;
    struct {
      uint8_t   Yloss,  Bloss,  Rloss,  Aloss;
      uint8_t   Yshift, Bshift, Rshift, Ashift;
      uint64_t  Ymask,  Bmask,  Rmask,  Amask;
    } ycbcrs;
    struct {
      uint8_t   Closs,  Mloss,  Yloss,  Aloss;
      uint8_t   Cshift, Mshift, Yshift, Ashift;
      uint64_t  Cmask,  Mmask,  Ymask,  Amask;
    } cmy;
    struct {
      uint8_t   Closs,  Mloss,  Yloss,  Kloss,  Aloss;
      uint8_t   Cshift, Mshift, Yshift, Kshift, Ashift;
      uint64_t  Cmask,  Mmask,  Ymask,  Kmask,  Amask;
    } cmyk;
    struct {
      uint8_t   Hloss,  Sloss,  Lloss,  Aloss;
      uint8_t   Hshift, Sshift, Lshift, Ashift;
      uint64_t  Hmask,  Smask,  Lmask,  Amask;
    } hsl;
    struct {
      uint8_t   Hloss,  Sloss,  Vloss,  Aloss;
      uint8_t   Hshift, Sshift, Vshift, Ashift;
      uint64_t  Hmask,  Smask,  Vmask,  Amask;
    } hsv;
    struct {
      double    X, Y, Z;
    } xyz;
    struct {
      double    x, y, Y;
    } xyy; /* Not to be confused with xyz */
    struct {
      double    L, a, b;
    } lab; /* CIELAB */
  };
  uint32_t  colorkey;
  uint8_t   alpha;
} Canvas_PixelFormat;

typedef struct canvas_s {
  int width;
  int height;
  /* Number of bytes per row of pixel data */
  int pitch;
  /* Length in bytes between the beginning of two consecutive pixel values */
  int stride;
  Canvas_PixelFormat *pixelFormat;
  char *fileName; /* File name, if applicable */
  uint8_t *pixels;
  uint8_t *buffer;
} Canvas;

typedef enum {
  /* Black and white */
  PnmBitmapAscii,
  PnmBitmapBinary,
  /* Grayscale */
  PnmGraymapAscii,
  PnmGreyMapAscii = PnmGraymapAscii,
  PnmGraymapBinary,
  PnmGreymapBinary = PnmGraymapBinary,
  /* Full color */
  PnmPixmapAscii,
  PnmPixmapBinary,
} PnmType;

typedef struct {
    /* Pointer to a pixel format struct describing how to read the pixel data */
    Canvas_PixelFormat *pixelFormat;
    /* Data for a single pixel, packed according to the pixel format */
    uint64_t pixelData;
} CanvasPixel;

typedef struct {
  /* Values corresponding to the CIE xyY colour space */
  double x;
  double y;
} CanvasWhitepoint;

/****************************************************************/
/*          EXPORTED VARIABLES                                  */
/****************************************************************/
/* Let's define some standard pixel formats */
/* Alpha + RGB, 32 bpp */
extern Canvas_PixelFormat *ARGB_8888;
/* RGB, 24 bpp */
extern Canvas_PixelFormat *RGB_888;
/* Grayscale, 8 bpp */
extern Canvas_PixelFormat *G_8;
/* Alpha + Grayscale, 8 bpp */
extern Canvas_PixelFormat *AG_88;

/****************************************************************/
/*          EXPORTED FUNCTIONS                                  */
/****************************************************************/

/**
 * Initializes the canvas module.  No calls to any other canvas 
 * function are valid before a call to canvasInit().
 *
 * \retval   0 on success, -1 on failure.
 */
int canvasInit( void );

/**
 * The opposite of canvasInit().  Frees any global memory allocated 
 * by canvasInit(), thus rendering the canvas lib invalid to call.
 */
void canvasShutdown( void );

/**
 * Allocates memory for a new canvas with the properties specified
 * in the argument list.  <pixelFormat> isn't copied by the function,
 * so the argument must remain in memory until the canvas is no
 * longer used.
 *
 * The standard pixel formats ARGB_8888 and RGB_888 can be used.
 *
 * \param    w IN   Contains the width in pixels.
 *
 * \param    h IN   Contains the height in rows.
 *
 * \param    pixelFormat IN   Describes how each pixel is constructed.
 *
 * \retval   Pointer to a canvas on success, NULL on failure.
 */
Canvas *canvasCreate( int w, int h, Canvas_PixelFormat *pixelFormat );

/**
 * Destroys and frees a canvas.  Note, however, that the pixelFormat 
 * pointer isn't freed, since the pointer value is used as a quick
 * comparison when determining how to convert pixel data between
 * canvases.
 *
 * \param    c IN   The pointer to a canvas.
 */
void canvasDestroy( Canvas *c );

/**
 * Fills a canvas entirely with 0, which means black for most 
 * pixelFormats.
 *
 * \param    c IN   The canvas to clear.
 *
 * \retval   0 on success, -1 on failure.
 */
int canvasClear( Canvas *c );
int canvasFill( Canvas *c, uint32_t color );

/**
 * Takes two pixel values of a given format, and calculates a new
 * pixel which is effectively a mixture of them.  <fraction>, which
 * must be between 0.0 and 1.0 inclusive, determines how much of 
 * <fg> to use, the rest will be made up from <bg>.
 *
 * \param    pixelFormat IN   The pointer to a pixel format.
 *
 * \param    fg          IN   The foreground colour value.
 *
 * \param    bg          IN   The background colour value.
 *
 * \param    fraction    IN   The fraction to take from the foreground.
 *
 * \param    pixel       OUT  Pointer to the returned pixel value.
 *
 * \retval   0  Success
 * \retval  -1  NULL pointer arguments
 * \retval  -2  <fracion> out of bounds
 * \retval  -3  Conversion error
 * \retval  -4  Unsupported pixel format
 */
int canvasPixelBlend( Canvas_PixelFormat *pixelFormat,
		      uint32_t fg,
		      uint32_t bg,
		      double fraction,
		      uint32_t *pixel );

/*******   Whitepoint functions  *******/

/**
 * Returns a struct containing the data necessary to specify a 
 * whitepoint within the CIE illuminant series D.
 *
 * \param    CCT IN   The temperature, in Kelvin.
 *                    Valid values are [4000, 25000],
 *                    D50 corresponds to 5003K.
 *                    D55 corresponds to 5503K.
 *                    D65 corresponds to 6504K.
 *                    D75 corresponds to 7504K.
 *
 * \retval   A struct defining a whitepoint.  Invalid temperature
 *           values have an undefined result.
 */
CanvasWhitepoint canvasCreateWhitepoint( int CCT );

/*******   Pixel handling functions  *******/

/**
 * Returns the pixel value at a certain position in a canvas.
 * Reads done outside the canvas area will cause function to
 * return 0, effectively making illegal areas black.
 *
 * \param    c IN   The canvas on which to read.
 *
 * \param    x IN   The x-coordinate, valid from 0 to c->width-1.
 *
 * \param    y IN   The y-coordinate, valid from 0 to c->height-1.
 *
 * \retval   The pixel data of position (x,y) or 0 on illegal 
 *           positions
 */
CanvasPixel canvasGetPixel( Canvas *c, int x, int y );

/**
 * Converts a pixel in the image from RGB data to its HSV equivalency.
 * Reads done outside the canvas area will cause function to return 0 
 * values, effectively making illegal areas black.
 *
 * \param    c IN          The canvas on which to read.
 *
 * \param    x IN          The x-coordinate, valid from 0 to c->width-1.
 *
 * \param    y IN          The y-coordinate, valid from 0 to c->height-1.
 *
 * \param    H,S,V    OUT  The HSV data for the requested pixel.
 */
void canvasGetHSV( Canvas *c, int x, int y, uint16_t *h, 
		   uint8_t *s, uint8_t *v );

/**
 * Converts a pixel in the image from RGB data to its HSL equivalency.
 * Reads done outside the canvas area will cause function to return 0 
 * values, effectively making illegal areas black.
 *
 * \param    c IN          The canvas on which to read.
 *
 * \param    x IN          The x-coordinate, valid from 0 to c->width-1.
 *
 * \param    y IN          The y-coordinate, valid from 0 to c->height-1.
 *
 * \param    H,S,L    OUT  The HSL data for the requested pixel.
 */
void canvasGetHSL( Canvas *c, int x, int y, uint16_t *h, 
		   uint8_t *s, uint8_t *l );

/**
 * Converts a pixel in the image from RGB data to its Y'CbCr equivalency.
 * Reads done outside the canvas area will cause function to return 0 
 * values, effectively making illegal areas black.
 *
 * \param    c IN          The canvas on which to read.
 *
 * \param    x IN          The x-coordinate, valid from 0 to c->width-1.
 *
 * \param    y IN          The y-coordinate, valid from 0 to c->height-1.
 *
 * \param    Y,Cb,Cr  OUT  The Y'CbCr data for the requested pixel.
 */

/* Cb and Cr between -128 and 127 */
void canvasGetYCbCr_s( Canvas *c, int x, int y, uint8_t *Y, 
		       signed char *Cb, signed char *Cr );
/* Cb and Cr between 0 and 255 */
void canvasGetYCbCr( Canvas *c, int x, int y, uint8_t *Y, 
		     uint8_t *Cb, uint8_t *Cr );

/**
 * Returns the RG representation of a pixel at a certain position in a 
 * canvas.  Reads done outside the canvas area will cause 
 * function to return 0 values, effectively making illegal areas 
 * bright blue.
 *
 * \param    c IN       The canvas on which to read.
 *
 * \param    x IN       The x-coordinate, valid from 0 to c->width-1.
 *
 * \param    y IN       The y-coordinate, valid from 0 to c->height-1.
 *
 * \param    r,g OUT    The RG data for the requested pixel.
 */
void canvasGetRG( Canvas *c, int x, int y,
		  uint8_t *r, 
		  uint8_t *g );


/**
 * Returns the CMY value of a pixel at a certain position in a 
 * canvas.  Reads done outside the canvas area will cause 
 * function to return 0 values, effectively making illegal areas 
 * black.
 *
 * \param    c IN       The canvas on which to read.
 *
 * \param    x IN       The x-coordinate, valid from 0 to c->width-1.
 *
 * \param    y IN       The y-coordinate, valid from 0 to c->height-1.
 *
 * \param    C,M,Y OUT  The CMY data for the requested pixel.
 */
void canvasGetCMY( Canvas *c, int x, int y, uint8_t *C, 
		   uint8_t *M, uint8_t *Y );

/**
 * Returns the CMYK value of a pixel at a certain position in a 
 * canvas.  Reads done outside the canvas area will cause 
 * function to return 0 values, effectively making illegal areas 
 * black.
 *
 * \param    c IN        The canvas on which to read.
 *
 * \param    x IN        The x-coordinate, valid from 0 to c->width-1.
 *
 * \param    y IN        The y-coordinate, valid from 0 to c->height-1.
 *
 * \param    C,M,Y,K OUT The CMYK data for the requested pixel.
 */
void canvasGetCMYK( Canvas *c, int x, int y, 
		    uint8_t *C, uint8_t *M, 
		    uint8_t *Y, uint8_t *K );

/**
 * Returns the ARGB value of a pixel at a certain position in a 
 * canvas.  Reads done outside the canvas area will cause 
 * function to return 0 values, effectively making illegal areas 
 * fully transparent black.
 *
 * \param    c IN         The canvas on which to read.
 *
 * \param    x IN         The x-coordinate, valid from 0 to c->width-1.
 *
 * \param    y IN         The y-coordinate, valid from 0 to c->height-1.
 *
 * \param    A,R,G,B OUT  The ARGB data for the requested pixel.
 */
void canvasGetARGB( Canvas *c, int x, int y, uint8_t *A,
		    uint8_t *R, uint8_t *G, uint8_t *B );

/**
 * Returns the RGB value of a pixel at a certain position in a 
 * canvas.  Reads done outside the canvas area will cause 
 * function to return 0 values, effectively making illegal areas 
 * black.
 *
 * \param    c IN       The canvas on which to read.
 *
 * \param    x IN       The x-coordinate, valid from 0 to c->width-1.
 *
 * \param    y IN       The y-coordinate, valid from 0 to c->height-1.
 *
 * \param    R,G,B OUT  The RGB data for the requested pixel.
 */
void canvasGetRGB( Canvas *c, int x, int y, uint8_t *R, 
		   uint8_t *G, uint8_t *B );

/**
 * Sets the pixel value at a specific position within a canvas.  The
 * pixel parameter must be of the same pixelFormat as the canvas itself.
 *
 * \param    c IN       The canvas on which to write.
 *
 * \param    x IN       The x-coordinate, valid from 0 to c->width-1.
 *
 * \param    y IN       The y-coordinate, valid from 0 to c->height-1.
 *
 * \param    pixel IN   The pixel data to store.  Must be the same
 *                      pixel format as <c>
 *
 * \retval   0 on success, -1 on failure.
 */
int canvasSetPixel( Canvas *c, int x, int y, uint32_t pixel );


/**
 * Sets the HSV value at a specific position within a canvas.
 *
 * \param    c IN         The canvas on which to write.
 *
 * \param    x IN         The x-coordinate, valid from 0 to c->width-1.
 *
 * \param    y IN         The y-coordinate, valid from 0 to c->height-1.
 *
 * \param    H,S,V IN     The HSV triplet to convert and save.
 *
 * \retval   0 on success, -1 on failure.
 */
int canvasSetHSV( Canvas *c, int x, int y, uint16_t h, 
		  uint8_t s, uint8_t v );

/**
 * Sets the HSL value at a specific position within a canvas.
 *
 * \param    c IN         The canvas on which to write.
 *
 * \param    x IN         The x-coordinate, valid from 0 to c->width-1.
 *
 * \param    y IN         The y-coordinate, valid from 0 to c->height-1.
 *
 * \param    H,S,L IN     The HSL triplet to convert and save.
 *
 * \retval   0 on success, -1 on failure.
 */
int canvasSetHSL( Canvas *c, int x, int y, uint16_t h, 
		  uint8_t s, uint8_t l );

/**
 * Sets the Y'CbCr value at a specific position within a canvas.
 *
 * \param    c IN         The canvas on which to write.
 *
 * \param    x IN         The x-coordinate, valid from 0 to c->width-1.
 *
 * \param    y IN         The y-coordinate, valid from 0 to c->height-1.
 *
 * \param    Y,Cb,Cr IN   The Y'CbCr triplet to convert and save.
 *
 * \retval   0 on success, -1 on failure.
 */
/* Cb and Cr between -128 and 127 */
int canvasSetYCbCr_s( Canvas *c, int x, int y, uint8_t Y,
		      signed char Cb, signed char Cr );
/* Cb and Cr between 0 and 255 */
int canvasSetYCbCr( Canvas *c, int x, int y, uint8_t Y,
		    uint8_t Cb, uint8_t Cr );

/**
 * Sets the RG representation of an RGB value at a specific position
 * within a canvas.
 *
 * \param    c IN       The canvas on which to write.
 *
 * \param    x IN       The x-coordinate, valid from 0 to c->width-1.
 *
 * \param    y IN       The y-coordinate, valid from 0 to c->height-1.
 *
 * \param    G,G IN     The RG data to save.
 *
 * \retval   0 on success, -1 on failure.
 */
int canvasSetRG( Canvas *c, int x, int y, 
		 uint8_t R,
		 uint8_t G );

/**
 * Sets the CMY value at a specific position within a canvas.
 *
 * \param    c IN       The canvas on which to write.
 *
 * \param    x IN       The x-coordinate, valid from 0 to c->width-1.
 *
 * \param    y IN       The y-coordinate, valid from 0 to c->height-1.
 *
 * \param    C,M,Y IN   The CMY triplet to save.  Are assumed to be
 *                      fully expanded 8 bit values.
 *
 * \retval   0 on success, -1 on failure.
 */
int canvasSetCMY( Canvas *c, int x, int y, uint8_t C,
		  uint8_t M, uint8_t Y );

/**
 * Sets the CMYK value at a specific position within a canvas.
 *
 * \param    c IN       The canvas on which to write.
 *
 * \param    x IN       The x-coordinate, valid from 0 to c->width-1.
 *
 * \param    y IN       The y-coordinate, valid from 0 to c->height-1.
 *
 * \param    C,M,Y IN   The CMYK quadruplet to save.  Are assumed to be
 *                      fully expanded 8 bit values.
 *
 * \retval   0 on success, -1 on failure.
 */
int canvasSetCMYK( Canvas *c, int x, int y, 
		   uint8_t C, uint8_t M, 
		   uint8_t Y, uint8_t K );

/**
 * Sets the RGB value at a specific position within a canvas.
 *
 * \param    c IN       The canvas on which to write.
 *
 * \param    x IN       The x-coordinate, valid from 0 to c->width-1.
 *
 * \param    y IN       The y-coordinate, valid from 0 to c->height-1.
 *
 * \param    R,G,B IN   The RGB triplet to save.  Are assumed to be
 *                      fully expanded 8 bit values.
 *
 * \retval   0 on success, -1 on failure.
 */
int canvasSetRGB( Canvas *c, int x, int y, uint8_t R,
		  uint8_t G, uint8_t B );

/**
 * Sets the ARGB value at a specific position within a canvas.
 *
 * \param    c IN        The canvas on which to write.
 *
 * \param    x IN        The x-coordinate, valid from 0 to c->width-1.
 *
 * \param    y IN        The y-coordinate, valid from 0 to c->height-1.
 *
 * \param    A,R,G,B IN  The ARGB quad to save.  Are assumed to be
 *                       fully expanded 8 bit values.
 *
 * \retval   0 on success, -1 on failure.
 */
int canvasSetARGB( Canvas *c, int x, int y, uint8_t A, uint8_t R,
                   uint8_t G, uint8_t B );


/**
 * Takes a pixel value described by <pixelFormat> and converts it
 * to three full byte values returned in <r>, <g> and <b>.
 *
 * \param    pixelFormat IN    Contains a description of the pixel.
 *
 * \param    pixel IN          The actual pixeldata.
 *
 * \param    r, g, b OUT       Pointers to which to write the expanded data.
 *
 * \retval   0 on success, -1 on failure.
 */
int canvasPixelToRGB( Canvas_PixelFormat *pixelFormat, uint32_t pixel,
		      uint8_t *r, uint8_t *g, uint8_t *b );


/**
 * Takes three full byte values and returns as a pixel in the format
 * described by <pixelFormat>.
 *
 * \param    pixelFormat IN    Contains a description of the pixel.
 *
 * \param    r, g, b IN        The actual pixeldata.
 *
 * \param    pixelFormat OUT   A pixel with the rgb values stored 
 *                             according to <pixelFormat>
 *
 * \retval   0 on success, -1 on failure.
 */
int canvasRGBToPixel( Canvas_PixelFormat *pixelFormat,
		      uint8_t r, uint8_t g, 
		      uint8_t b, uint32_t *pixel );
    
/**
 * Takes a pixel value described by <pixelFormat> and converts it
 * to four full byte values returned in <a>, <r>, <g> and <b>.
 *
 * \param    pixelFormat IN    Contains a description of the pixel.
 *
 * \param    pixel IN          The actual pixeldata.
 *
 * \param    a, r, g, b OUT    Pointers to which to write the expanded data.
 *
 * \retval   0 on success, -1 on failure.
 */
int canvasPixelToARGB( Canvas_PixelFormat *pixelFormat, uint32_t pixel,
		       uint8_t *a, uint8_t *r, uint8_t *g, uint8_t *b );


/**
 * Takes four full byte values and returns as a pixel in the format
 * described by <pixelFormat>.
 *
 * \param    pixelFormat IN    Contains a description of the pixel.
 *
 * \param    a, r, g, b IN     The actual pixeldata.
 *
 * \param    pixelFormat OUT   A pixel with the rgb values stored 
 *                             according to <pixelFormat>
 *
 * \retval   0 on success, -1 on failure.
 */
int canvasARGBToPixel( Canvas_PixelFormat *pixelFormat,
		       uint8_t a,
		       uint8_t r, uint8_t g, 
		       uint8_t b, uint32_t *pixel );

/*******   Comparison functions  *******/
/**
 * Compares two pixels and returns a squared error value of their RGB triplets.
 *
 * \param    p1f, p2f IN    Pixel formats describing the two pixels to compare.
 *
 * \param    p1, p2 IN      The pixels to compare.
 *
 * \retval   A value between 0 and 3*255^2, with 0 meaning equal colour.
 */
unsigned int canvasComparePixels( Canvas_PixelFormat *p1f, unsigned int p1,
				  Canvas_PixelFormat *p2f, unsigned int p2 );

/**
 * Compares the overlapping areas of two canvases and returns the sum of 
 * all pixel differences as a ratio between 0 and 1.0.
 *
 * \param    c1, c2 IN      Canvases to compare.
 *
 * \retval   A value between 0.0 and 1.0 where 0.0 means no diff.
 */
double canvasDiffRatio( Canvas *c1, Canvas *c2 );

/**
 * Compares the overlapping areas of two canvases and returns the sum of 
 * all pixel differences.
 *
 * \param    c1, c2 IN      Canvases to compare.
 *
 * \retval   A value between 0.0 and \infty where 0.0 means no diff.
 */
uint64_t canvasDiff( Canvas *c1, Canvas *c2 );

/**
 * Compares the overlapping areas of two canvases and returns the sum of 
 * all pixel differences.
 *
 * \param    c1, c2 IN            Canvases to compare.
 *
 * \param    x1, y1, x2, y2 IN    Top and left of the two regions to compare.
 *
 * \param    w, h IN              Width and height of the regions.
 *
 * \retval   A value between 0.0 and \infty where 0.0 means no diff.
 */
uint64_t canvasDiffRegion( Canvas *c1, 
			   int x1, int y1,
			   Canvas *c2,
			   int x2, int y2,
			   unsigned int w, unsigned int h );

/**
 * Compares the overlapping areas of two canvases and returns the sum of 
 * all pixel differences per pixel compared
 *
 * \param    c1, c2 IN      Canvases to compare.
 *
 * \retval   A value between 0.0 and 1.0
 */
double canvasDiffPerPixel( Canvas *c1, Canvas *c2 );

/*******   Image functions  *******/
/**
 * Tries to figure out which format a file is and open it.
 *
 * \param   filename IN     File to open.
 *
 * \retval  Pointer to canvas if successful, NULL on failure.
 *
 */
Canvas *canvasLoadImage( const char *filename );

/*******   JPEG functions  *******/
/**
 * Reads a jpeg file and saves its pixel data in an RGB_888 formated
 * canvas.  CMYK-formated files are probably not supported.
 *
 * \param    filename IN    The file to try to open and read.
 *
 * \retval   The pointer to a new canvas if successful, NULL on failure.
 */
Canvas *canvasLoadJpeg( const char *filename );

/**
 * Saves a canvas to a jpeg-file.
 *
 * \param    c IN           Canvas to read pixel data from.
 *
 * \param    filename IN    The file to try to open and write.
 *
 * \retval   0 if successful, -1 on failure.
 */
int canvasSaveJpeg( Canvas *c, const char *filename, 
		    int quality );

/*******   PNG functions  *******/
/**
 * Reads a png file and saves its pixel data in an RGB_888 or ARGB_8888
 * formated canvas.
 *
 * \param    filename IN    The file to try to open and read.
 *
 * \retval   The pointer to a new canvas if successful, NULL on failure.
 */
Canvas *canvasLoadPng( const char *filename );

/**
 * Saves a canvas to a png-file.
 *
 * \param    c IN           Canvas to read pixel data from.
 *
 * \param    filename IN    The file to try to open and write.
 *
 * \retval   0 if successful, -1 on failure.
 */
int canvasSavePng( Canvas *c, const char *filename );


/*******  Bitmap functions *******/
/**
 * Reads a bmp file and saves its pixel data in an RGB_888 formated
 * canvas.  Compressed variants and palette images are not supported.
 *
 * \param    filename IN    The file to try to open and read.
 *
 * \retval   The pointer to a new canvas if successful, NULL on failure.
 */
Canvas *canvasLoadBmp( const char *filename );

/**
 * Saves a canvas to a bmp-file.
 *
 * \param    c IN           Canvas to read pixel data from.
 *
 * \param    filename IN    The file to try to open and write.
 *
 * \retval   0 if successful, -1 on failure.
 */
int canvasSaveBmp( Canvas *c, const char *filename );


/*******   PNM functions  *******/
/**
 * Reads a pnm file and saves its pixel data in an RGB_888 formated
 * canvas.  CMYK-formated files are probably not supported.
 *
 * \param    filename IN    The file to try to open and read.
 *
 * \retval   The pointer to a new canvas if successful, NULL on failure.
 */
Canvas *canvasLoadPnm( const char *filename );

/**
 * Saves a canvas to a pnm-file.
 *
 * \param    c IN           Canvas to read pixel data from.
 *
 * \param    filename IN    The file to try to open and write.
 *
 * \param    type IN        The file format of the image, PBM, 
 *                          PGM or PPM as well as ASCII or binary.
 *
 * \retval   0 if successful, -1 on failure.
 */
int canvasSavePnm( Canvas *c, const char *filename, PnmType type );

#endif

#ifdef __cplusplus
}
#endif

