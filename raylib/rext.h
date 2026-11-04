#ifndef REXT_H
#define REXT_H

#include "raylib.h"
#include <stdint.h>

#define REXT_BASE_CELL_W 32
#define REXT_BASE_CELL_H 48

static inline Color rext_unpack(uint32_t c) {
    return (Color){ c & 255, (c >> 8) & 255, (c >> 16) & 255, c >> 24 };
}

RLAPI int  LoadTextureHandle(const char *path, int filter, int mipmaps);
RLAPI int  TextureHandleWidth(int h);
RLAPI int  TextureHandleHeight(int h);
RLAPI void DrawTextureRegion(int h, float sx, float sy, float sw, float sh,
                             float dx, float dy, float dw, float dh, uint32_t tint);

RLAPI void ClearBackgroundRGBA(uint32_t rgba);
RLAPI void DrawRectangleRGBA(int x, int y, int w, int h, uint32_t rgba);
RLAPI void ShutdownHandles(void);

RLAPI void GlyphBatchBegin(int shaded);
RLAPI int  GlyphBatchAdd(const char *path, int cols, int rows);
RLAPI int  GlyphBatchPack(int filter, int mipmaps);
RLAPI int  GlyphCols(int g);
RLAPI int  GlyphRows(int g);
RLAPI int  ShadeNew(int count);
RLAPI void ShadeSetColor(int shade, int rank, uint32_t rgba);
RLAPI void GlyphShadeBegin(void);
RLAPI void GlyphShadeEnd(void);
RLAPI void DrawGlyphAt(int g, int shade, uint32_t tint, int cx, int cy,
                       float cw, float ch, float ox, float oy);

RLAPI void DrawGlyphGrid(const uint32_t *cells, int cols, int rows,
                         float cw, float ch, float ox, float oy);

RLAPI void ShutdownGlyphs(void);

#endif
