/*
 * Minimal BMP loader for the Micropolis tile sheets.
 *
 * The upstream classic tileset (tiles.bmp) is a Windows 3.x 8bpp BMP with
 * a 256-color palette: 512x480 = 32x30 tiles of 16x16, indexed row-major
 * by engine tile number. Only what that file needs is implemented:
 * uncompressed 8bpp with a BITMAPINFOHEADER palette, top-down or bottom-up,
 * including padded rows used by the classic startup buttons.
 * Host-testable: no AROS dependencies.
 */

#ifndef MICROPOLIS_BMP_H
#define MICROPOLIS_BMP_H

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

struct BmpImage {
    int width = 0;
    int height = 0;
    std::vector<uint32_t> pixels;  // 0xAARRGGBB, top-down, width*height
};

/* Load an 8bpp BMP and convert to top-down 32-bit ARGB. Returns false and
 * leaves a reason in `error` on failure. */
bool loadBmp8(const std::string &path, BmpImage &image, std::string &error);

#endif /* MICROPOLIS_BMP_H */