#include <math.h>
#include <stdio.h>
#include "bmp24.h"

DWORD Bmp24::hash() const
{
    // FNV-1 hash; row padding is included in the hash

    DWORD h = 0;
    BYTE * e = pixels + rowLength * height;
    for (BYTE * b = pixels; b < e; b++) {
        h *= 16777619;
        h ^= *b;
    }

    return h;
}

void Bmp24::copyDcAt(HDC hdc, int x, int y, int width, int height)
{
    resize(width, height);

    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBM = CreateCompatibleBitmap(hdc, width, height);
    SelectObject(memDC, memBM);
    BitBlt(memDC, 0, 0, width, height, hdc, x, y, SRCCOPY);

    BITMAPINFOHEADER bi = {0};
    bi.biSize = sizeof(bi);
    bi.biWidth = width;
    bi.biHeight = -height;
    bi.biPlanes = 1;
    bi.biBitCount = bits;
    bi.biCompression = BI_RGB;

    GetDIBits(hdc, memBM, 0, height, pixels, (BITMAPINFO*) &bi, DIB_RGB_COLORS);

    DeleteDC(memDC);
    DeleteObject(memBM);
}

void Bmp24::copyToDc(HDC hdc, int x, int y) const
{
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBM = CreateCompatibleBitmap(hdc, width, height);
    SelectObject(memDC, memBM);

    BITMAPINFOHEADER bi = {0};
    bi.biSize = sizeof(bi);
    bi.biWidth = width;
    bi.biHeight = -height;
    bi.biPlanes = 1;
    bi.biBitCount = bits;
    bi.biCompression = BI_RGB;

    SetDIBits(hdc, memBM, 0, height, pixels, (BITMAPINFO*) &bi, DIB_RGB_COLORS);
    BitBlt(hdc, x, y, width, height, memDC, 0, 0, SRCCOPY);

    DeleteDC(memDC);
    DeleteObject(memBM);
}

void Bmp24::save(const char * fileName) const
{
    BITMAPINFOHEADER bi = {0};
    bi.biSize = sizeof(bi);
    bi.biWidth = width;
    bi.biHeight = -height;
    bi.biPlanes = 1;
    bi.biBitCount = bits;
    bi.biCompression = BI_RGB;

    DWORD bmpSize = rowLength * height;

    BITMAPFILEHEADER bmfHeader;
    // Offset to where the actual bitmap bits start
    bmfHeader.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + (DWORD) sizeof(BITMAPINFOHEADER);
    bmfHeader.bfSize = bmpSize + bmfHeader.bfOffBits;
    bmfHeader.bfType = 0x4D42; // BM

    FILE * fp = fopen(fileName, "wb");

    if (!fp) return;

    fwrite(&bmfHeader, 1, sizeof(BITMAPFILEHEADER), fp);
    fwrite(&bi, 1, sizeof(BITMAPINFOHEADER), fp);
    fwrite(pixels, 1, bmpSize, fp);
    fclose(fp);
}

void Bmp24::grayify()
{
    for (int y = 0; y < height; y++) {
        BYTE * row = pixels + y * rowLength;
        BYTE * end = row + 3 * width;
        for (BYTE * rgb = row; rgb < end; rgb += 3) {
            // Intensity = 11% R + 59% G + 30% B
            BYTE I = (BYTE) (0.5 + rgb[0]*0.30 + rgb[1]*0.59 + rgb[2]*0.11);
            rgb[0] = I;
            rgb[1] = I;
            rgb[2] = I;
        }
    }
}

void Bmp24::filter()
{
    for (int y = 1; y < height-1; y++) {
        BYTE * row = pixels + y * rowLength;
        BYTE * prevRow = row - rowLength;
        BYTE * nextRow = row + rowLength;
        // assumimg gray-scale bmp
        for (int i=3; i < 3*(width-1); i += 3) {
            // edge enhancement; high-pass filtering
            // -1  -1  -1
            // -1  +9  -1
            // -1  -1  -1
            BYTE c00 = prevRow[i - 3];
            BYTE c01 = prevRow[i];
            BYTE c02 = prevRow[i + 3];
            BYTE c10 = row[i - 3];
            BYTE c11 = row[i];
            BYTE c12 = row[i + 3];
            BYTE c20 = nextRow[i - 3];
            BYTE c21 = nextRow[i];
            BYTE c22 = nextRow[i + 3];
            long I = -c00 -c01 -c02 -c10 + 9*c11 -c12 -c20 -c21 -c22;
            row[i+1] = (BYTE) max(0, min(I, 255));
        }
    }

    for (int y = 0; y < height; y++) {
        BYTE * row = pixels + y * rowLength;
        BYTE * end = row + 3 * width;
        // assumimg gray-scale bmp
        for (BYTE * rgb = row; rgb < end; rgb += 3) {
            rgb[0] = rgb[1];
            rgb[2] = rgb[1];
        }
    }
}

void Bmp24::binarize(BYTE threshold)
{
    for (int y = 0; y < height; y++) {
        BYTE * row = pixels + y * rowLength;
        BYTE * end = row + 3 * width;
        // assumimg gray-scale bmp
        for (BYTE * rgb = row; rgb < end; rgb += 3) {
            BYTE I = rgb[0] >= threshold? 245 : 100;
            rgb[0] = I;
            rgb[1] = I;
            rgb[2] = I;
        }
    }
}

// Otsu's moment-preservation method.
// thresho.c, "Practical Algorithms for Image Analysis", 2nd edition
BYTE Bmp24::threshold() const
{
    const int nhist = 256; // bins in histogram
    long hist[nhist] = {0}; // hist. of intensities
    double prob[nhist];

    // Histogram //
    for (int y = 0; y < height; y++) {
        BYTE * row = pixels + y * rowLength;
        BYTE * end = row + 3 * width;
        // assumimg gray-scale bmp
        for (BYTE * rgb = row; rgb < end; rgb += 3) {
            hist[rgb[0]]++;
        }
    }

    // Probabilities //
    int n = width * height;
    for (int i = 0; i < nhist; i++) {
        prob[i] = (double) hist[i] / (double) n;
    }

    // TODO memoize computations; speed can be improved x100

    // Find best threshold by computing moments for all thresholds //
    long thresh = 0;
    long nHistM1 = nhist - 1;
    double varWMin = 100000000.0;
    for (int i = 1; i < nHistM1; i++) {
        double   m0Low, m0High, m1Low, m1High, varLow, varHigh;
        m0Low = m0High = m1Low = m1High = varLow = varHigh = 0.0;

        for (int j = 0; j <= i; j++) {
            m0Low += prob[j];
            m1Low += j * prob[j];
        }

        m1Low = (m0Low != 0.0) ? m1Low / m0Low : i;

        for (int j = i + 1; j < nhist; j++) {
            m0High += prob[j];
            m1High += j * prob[j];
        }

        m1High = (m0High != 0.0) ? m1High / m0High : i;

        for (int j = 0; j <= i; j++) {
            varLow += pow(j - m1Low, 2) * prob[j];
        }

        for (int j = i + 1; j < nhist; j++) {
            varHigh += pow(j - m1High, 2) * prob[j];
        }

        double varWithin = m0Low * varLow + m0High * varHigh;
        if (varWithin < varWMin) {
            varWMin = varWithin;
            thresh = i;
        }
    }

    return (BYTE) thresh;
}

int Bmp24::dcWidth(HDC hdc)
{
    return GetDeviceCaps(hdc, HORZRES);
}

int Bmp24::dcHeight(HDC hdc)
{
    return GetDeviceCaps(hdc, VERTRES);
}
