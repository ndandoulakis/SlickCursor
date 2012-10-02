#ifndef BMP24_H
#define BMP24_H

#include "windows.h"

struct Bmp24 {
    static const int bits = 24;

    int width;
    int height;
    int rowLength; // includes padding
    BYTE * pixels; // top to bottom

    Bmp24() {
        width = 0;
        height = 0;
        rowLength = 0;
        pixels = 0;
    }

    // TODO copy ctor and assignment operator

    ~Bmp24() {
        resize(0, 0);
    }

    void resize(int w, int h) {
        width = w;
        height = h;
        rowLength = ((width * bits + 31) / 32) * 4;
        pixels = (BYTE*) realloc(pixels, rowLength * height);
    }

    DWORD hash() const;

    void copyDcAt(HDC dc, int x, int y, int width, int height);
    void copyToDc(HDC dc, int x, int y) const;

    void save(const char * fileName) const;

    void grayify();
    void filter();
    void binarize(BYTE threshold);
    BYTE threshold() const;

    static int dcWidth(HDC);
    static int dcHeight(HDC);

};

#endif
