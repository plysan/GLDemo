#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <tiffio.h>

int main( int argc, char* argv[] )
{
    if (argc != 4) {
        printf("Usage: tiffcompose rgbInput alphaInput output\n");
    }
    TIFF *out= TIFFOpen(argv[3], "w");
    TIFF *rgb_in = TIFFOpen(argv[1], "r");
    TIFF *alpha_in = TIFFOpen(argv[2], "r");
    if (rgb_in != NULL && alpha_in != NULL && out != NULL) {
        uint32 image_w, image_h;
        TIFFGetField(rgb_in, TIFFTAG_IMAGEWIDTH, &image_w);
        TIFFGetField(rgb_in, TIFFTAG_IMAGELENGTH, &image_h);
        tsize_t linebytes = 4*image_w;
        TIFFSetField(out, TIFFTAG_IMAGEWIDTH, image_w);  // set the width of the image
        TIFFSetField(out, TIFFTAG_IMAGELENGTH, image_h);    // set the height of the image
        TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 4);   // set number of channels per pixel
        TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 8);    // set the size of the channels
        TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);    // set the origin of the image.
        TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, 1);
        TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);


        uint32* rgb_buf = (uint32*)_TIFFmalloc(linebytes);
        uint32* alpha_buf = (uint32*)_TIFFmalloc(linebytes);
        for (int i=0; i<image_h; i++) {
            TIFFReadRGBAStrip(rgb_in, i, rgb_buf);
            TIFFReadRGBAStrip(alpha_in, i, alpha_buf);
            for (int j=0; j<image_w; j++) {
                rgb_buf[j] = rgb_buf[j]&0x00ffffff|alpha_buf[j]&0xff000000;
            }
            if (TIFFWriteScanline(out, rgb_buf, i, 0) < 0) {
                break;
            }
        }
        _TIFFfree(rgb_buf);
        _TIFFfree(alpha_buf);
        TIFFClose(rgb_in);
        TIFFClose(alpha_in);
        TIFFClose(out);
    }
}
