#ifndef REXT_H
#define REXT_H

#include "raylib.h"
#include <stdint.h>

// RGBA packed little-endian: red in the low byte
static inline Color rext_unpack(uint32_t c) {
    return (Color){ c & 255, (c >> 8) & 255, (c >> 16) & 255, c >> 24 };
}

RLAPI int  LoadTextureHandle(const char *path, int filter, int mipmaps);
RLAPI void UnloadTextureHandle(int h);
RLAPI int  TextureHandleWidth(int h);
RLAPI int  TextureHandleHeight(int h);
RLAPI void ClearBackgroundRGBA(uint32_t rgba);
RLAPI void DrawRectangleRGBA(int x, int y, int w, int h, uint32_t rgba);
RLAPI void DrawTextureRegion(int h, float sx, float sy, float sw, float sh,
                             float dx, float dy, float dw, float dh, uint32_t tint);
RLAPI void ShutdownHandles(void);

#endif
