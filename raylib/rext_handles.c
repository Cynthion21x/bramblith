#include "rext.h"
#include <stdbool.h>

#define MAX_TEXTURES 256
#define GEN_MAX 0x7FFF

typedef struct {
    Texture2D tex;
    uint16_t  gen;
    bool      used;
} TextureSlot;

static TextureSlot textures[MAX_TEXTURES];

static TextureSlot *lookup_texture(int h) {
    uint32_t index = (uint32_t)h & 0xFFFF, gen = (uint32_t)h >> 16;
    if (index >= MAX_TEXTURES) return 0;
    TextureSlot *s = &textures[index];
    return (s->used && s->gen == gen) ? s : 0;
}

// filter 0 = point, 1 = bilinear, 2 = trilinear
int LoadTextureHandle(const char *path, int filter, int mipmaps) {
    for (int i = 0; i < MAX_TEXTURES; i++) {
        if (textures[i].used) continue;
        Texture2D t = LoadTexture(path);
        if (t.id == 0) {
            TraceLog(LOG_WARNING, "LoadTextureHandle: failed to load %s", path);
            return -1;
        }
        if (mipmaps) GenTextureMipmaps(&t);
        if (filter < 0 || filter > 2) filter = 0;
        SetTextureFilter(t, filter);
        textures[i].tex = t;
        textures[i].used = true;
        if (textures[i].gen == 0) textures[i].gen = 1;
        return (int)((uint32_t)textures[i].gen << 16 | (uint32_t)i);
    }
    TraceLog(LOG_WARNING, "LoadTextureHandle: texture pool full");
    return -1;
}

void UnloadTextureHandle(int h) {
    TextureSlot *s = lookup_texture(h);
    if (!s) return;
    UnloadTexture(s->tex);
    s->used = false;
    if (++s->gen > GEN_MAX) s->gen = 1;
}

int TextureHandleWidth(int h)  { TextureSlot *s = lookup_texture(h); return s ? s->tex.width  : 0; }
int TextureHandleHeight(int h) { TextureSlot *s = lookup_texture(h); return s ? s->tex.height : 0; }

void ClearBackgroundRGBA(uint32_t rgba) { ClearBackground(rext_unpack(rgba)); }

void DrawRectangleRGBA(int x, int y, int w, int h, uint32_t rgba) {
    DrawRectangle(x, y, w, h, rext_unpack(rgba));
}

void DrawTextureRegion(int h, float sx, float sy, float sw, float sh,
                       float dx, float dy, float dw, float dh, uint32_t tint) {
    TextureSlot *s = lookup_texture(h);
    if (!s) return;
    DrawTexturePro(s->tex, (Rectangle){ sx, sy, sw, sh },
                   (Rectangle){ dx, dy, dw, dh }, (Vector2){ 0, 0 }, 0.0f,
                   rext_unpack(tint));
}

// Call before CloseWindow the GL context must still exist to free textures
void ShutdownHandles(void) {
    for (int i = 0; i < MAX_TEXTURES; i++)
        if (textures[i].used) { UnloadTexture(textures[i].tex); textures[i].used = false; }
}
