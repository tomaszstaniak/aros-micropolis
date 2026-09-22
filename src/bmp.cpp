#include "bmp.h"

#include <cstring>

/* Little-endian readers; the sheets are LE on every platform we care
 * about (they are fixed art assets, not host-written data). */
static uint16_t rd16(const unsigned char *p)
{
    return (uint16_t)(p[0] | (p[1] << 8));
}

static uint32_t rd32(const unsigned char *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

bool loadBmp8(const std::string &path, BmpImage &image, std::string &error)
{
    FILE *f = fopen(path.c_str(), "rb");
    if (!f) {
        error = "cannot open " + path;
        return false;
    }

    unsigned char header[54];
    if (fread(header, 1, 54, f) != 54 || memcmp(header, "BM", 2) != 0) {
        error = "not a BMP file";
        fclose(f);
        return false;
    }

    uint32_t dataOffset = rd32(header + 10);
    uint32_t headerSize = rd32(header + 14);

    if (headerSize < 40) {
        error = "unsupported BMP header";
        fclose(f);
        return false;
    }

    unsigned char info[40];
    memcpy(info, header + 14, 40);

    int32_t w = (int32_t)rd32(info + 4);
    int32_t hRaw = (int32_t)rd32(info + 8);
    uint16_t bpp = rd16(info + 14);
    uint32_t compression = rd32(info + 16);

    bool bottomUp = hRaw > 0;
    int h = bottomUp ? hRaw : -hRaw;

    if (bpp != 8 || compression != 0 || w <= 0 || h <= 0) {
        error = "only uncompressed 8bpp BMPs are supported";
        fclose(f);
        return false;
    }

    /* Palette: 4-byte entries (BGR0) right after the info header. */
    uint32_t paletteCount = rd32(info + 32);
    if (paletteCount == 0) {
        paletteCount = 256;
    }
    if (paletteCount > 256) {
        paletteCount = 256;
    }

    unsigned char palette[256][4];
    long palettePos = 14 + headerSize;
    if (fseek(f, palettePos, SEEK_SET) != 0 ||
        fread(palette, 4, paletteCount, f) != paletteCount) {
        error = "cannot read palette";
        fclose(f);
        return false;
    }

    uint32_t colors[256];
    for (uint32_t i = 0; i < paletteCount; i++) {
        colors[i] = 0xff000000u |
                    ((uint32_t)palette[i][2] << 16) |   /* R */
                    ((uint32_t)palette[i][1] << 8) |    /* G */
                    (uint32_t)palette[i][0];            /* B */
    }

    // BMP stores each scanline on a four-byte boundary. Classic buttons
    // have widths such as 157: consume padding, but never render it.
    uint32_t rowBytes = ((uint32_t)w + 3u) & ~3u;

    if (fseek(f, dataOffset, SEEK_SET) != 0) {
        error = "cannot seek to pixel data";
        fclose(f);
        return false;
    }

    image.width = w;
    image.height = h;
    image.pixels.assign((size_t)w * h, 0);

    std::vector<unsigned char> row(rowBytes);
    for (int y = 0; y < h; y++) {
        if (fread(row.data(), 1, rowBytes, f) != rowBytes) {
            error = "truncated pixel data";
            fclose(f);
            return false;
        }
        int dstY = bottomUp ? (h - 1 - y) : y;
        uint32_t *dst = &image.pixels[(size_t)dstY * w];
        for (int x = 0; x < w; x++) {
            dst[x] = colors[row[x]];
        }
    }

    fclose(f);
    return true;
}