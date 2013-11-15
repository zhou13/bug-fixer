#include "model.h"

Texture::Texture(const char *filename) {
    hbm = (HBITMAP)LoadImage(NULL, filename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
//printf("%s\n", hbm?"successful load bitmap!":"fail to load bitmap!");
    GetObject(hbm, sizeof(bitmap), &bitmap);
    width = bitmap.bmWidth;
    height = bitmap.bmHeight;
    buffer = (int*)malloc(width*height*4);
    GetBitmapBits(hbm, width*height*4, buffer);
//printf("width=%d height=%d\n", width, height);
}

Colors Texture::GetPixel(double x, double y) {
    int x0 = max(0, min(width-1, (int)(x*(double)width)));
    int y0 = max(0, min(height-1, (int)(y*(double)height)));
    int c = buffer[y0*width+x0];
    int cR = c >> 16;
    int cG = (c >> 8) & 255;
    int cB = c & 255;
    return Colors((double)cR/256.0, (double)cG/256.0,(double)cB/256.0);
}
