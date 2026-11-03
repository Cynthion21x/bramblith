#include "rext.h"
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define MAX_GLYPHS  4096
#define MAX_ATLASES 8
#define ATLAS_SIZE  4096
#define MAX_PAD     8
#define MAX_RANKS   16
#define MAX_SHADES  1024

typedef struct {
    uint8_t  atlas;
    uint16_t x, y, w, h;
    uint8_t  cols, rows;
} RextGlyph;

typedef struct { Image img; int index; } Pending;

static RextGlyph glyph_table[MAX_GLYPHS];
static int       glyph_count = 1;

static Texture2D atlases[MAX_ATLASES];
static bool      atlas_shaded[MAX_ATLASES];
static int       atlas_count = 0;

static Pending pending[MAX_GLYPHS];
static int     pending_count = 0;
static int     batch_first = 1;
static bool    batch_open = false;
static bool    batch_shaded = false;

static uint32_t  shade_colors[MAX_SHADES][MAX_RANKS];
static uint8_t   shade_len[MAX_SHADES];
static int       shade_count = 1;
static bool      shade0_ready = false;
static bool      palette_dirty = true;
static Texture2D palette_tex;
static bool      palette_ready = false;
static Shader    glyph_shader;
static bool      shader_ready = false;
static int       palette_loc;
static bool      in_shade_mode = false;

static const char *glyph_fs =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "in vec4 fragColor;\n"
    "out vec4 finalColor;\n"
    "uniform sampler2D texture0;\n"
    "uniform sampler2D palette;\n"
    "void main() {\n"
    "    vec4 g = texture(texture0, fragTexCoord);\n"
    "    int rank  = int(g.r * 255.0 + 0.5);\n"
    "    int shade = int(fragColor.r * 255.0 + 0.5)\n"
    "              + 256 * int(fragColor.g * 255.0 + 0.5);\n"
    "    vec4 c = texelFetch(palette, ivec2(rank, shade), 0);\n"
    "    finalColor = vec4(c.rgb, c.a * g.a);\n"
    "}\n";

static void init_shade0(void) {
    if (shade0_ready) return;
    shade_len[0] = MAX_RANKS;
    for (int r = 0; r < MAX_RANKS; r++) {
        uint32_t v = 255 - r * 12;
        shade_colors[0][r] = v | v << 8 | v << 16 | 0xFF000000u;
    }
    shade0_ready = true;
}

int ShadeNew(int count) {
    init_shade0();
    if (count < 1 || count > MAX_RANKS || shade_count >= MAX_SHADES) return -1;
    int s = shade_count++;
    shade_len[s] = (uint8_t)count;
    for (int r = 0; r < MAX_RANKS; r++) shade_colors[s][r] = 0xFF000000u;
    palette_dirty = true;
    return s;
}

void ShadeSetColor(int shade, int rank, uint32_t rgba) {
    if (shade <= 0 || shade >= shade_count || rank < 0 || rank >= shade_len[shade]) return;
    for (int r = rank; r < MAX_RANKS; r += shade_len[shade])
        shade_colors[shade][r] = rgba;
    palette_dirty = true;
}


static void ensure_shading(void) {
    init_shade0();
    if (!shader_ready) {
        glyph_shader = LoadShaderFromMemory(NULL, glyph_fs);
        palette_loc  = GetShaderLocation(glyph_shader, "palette");
        shader_ready = true;
    }
    if (!palette_ready) {
        Image im = GenImageColor(MAX_RANKS, MAX_SHADES, BLANK);
        palette_tex = LoadTextureFromImage(im);
        UnloadImage(im);
        SetTextureFilter(palette_tex, TEXTURE_FILTER_POINT);
        palette_ready = true;
        palette_dirty = true;
    }
    if (palette_dirty) { UpdateTexture(palette_tex, shade_colors); palette_dirty = false; }
}

void GlyphShadeBegin(void) {
    ensure_shading();
    BeginShaderMode(glyph_shader);
    SetShaderValueTexture(glyph_shader, palette_loc, palette_tex);
    in_shade_mode = true;
}

void GlyphShadeEnd(void) { EndShaderMode(); in_shade_mode = false; }

static int lum(uint32_t rgb) {
    return 299 * (rgb & 255) + 587 * ((rgb >> 8) & 255) + 114 * ((rgb >> 16) & 255);
}

static bool brighter(uint32_t a, uint32_t b) {
    int la = lum(a), lb = lum(b);
    return la > lb || (la == lb && a > b);
}

static bool to_rank_image(Image *img, const char *path) {
    uint32_t tones[MAX_RANKS];
    int n = 0, count = img->width * img->height;
    uint8_t *d = img->data;

    for (int i = 0; i < count; i++) {
        uint8_t *p = d + i * 4;
        if (p[3] == 0) continue;
        uint32_t rgb = p[0] | p[1] << 8 | p[2] << 16;
        int j = 0;
        while (j < n && tones[j] != rgb) j++;
        if (j == n) {
            if (n == MAX_RANKS) {
                TraceLog(LOG_WARNING, "%s: more than %d distinct colours", path, MAX_RANKS);
                return false;
            }
            tones[n++] = rgb;
        }
    }

    //sort
    for (int i = 1; i < n; i++) {
        uint32_t v = tones[i];
        int j = i - 1;
        while (j >= 0 && brighter(v, tones[j])) { tones[j + 1] = tones[j]; j--; }
        tones[j + 1] = v;
    }
    for (int i = 0; i < count; i++) {
        uint8_t *p = d + i * 4;
        uint32_t rgb = p[0] | p[1] << 8 | p[2] << 16;
        int rank = 0;
        if (p[3]) while (tones[rank] != rgb) rank++;
        p[0] = (uint8_t)rank; p[1] = 0; p[2] = 0;
    }
    return true;
}

static void discard_pending(void) {
    for (int i = 0; i < pending_count; i++) UnloadImage(pending[i].img);
    pending_count = 0;
    batch_open = false;
}

static int reject(Image *img) { UnloadImage(*img); return -1; }

void GlyphBatchBegin(int shaded) {
    if (batch_open) {
        TraceLog(LOG_WARNING, "GlyphBatchBegin: discarding an unpacked batch");
        glyph_count = batch_first;
        discard_pending();
    }
    batch_first = glyph_count;
    batch_open = true;
    batch_shaded = shaded != 0;
}

int GlyphBatchAdd(const char *path, int cols, int rows) {
    if (!batch_open || glyph_count >= MAX_GLYPHS) {
        TraceLog(LOG_WARNING, "GlyphBatchAdd: no open batch or table full (%s)", path);
        return -1;
    }
    Image img = LoadImage(path);
    if (!img.data) {
        TraceLog(LOG_WARNING, "GlyphBatchAdd: cannot load %s", path);
        return -1;
    }
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    if (cols <= 0 || rows <= 0) {
        if (img.width % REXT_BASE_CELL_W || img.height % REXT_BASE_CELL_H) {
            TraceLog(LOG_WARNING, "%s: %dx%d is not a multiple of the %dx%d base cell",
                     path, img.width, img.height, REXT_BASE_CELL_W, REXT_BASE_CELL_H);
            return reject(&img);
        }
        cols = img.width / REXT_BASE_CELL_W;
        rows = img.height / REXT_BASE_CELL_H;
    } else if (img.width * rows * REXT_BASE_CELL_H !=
               img.height * cols * REXT_BASE_CELL_W) {
        TraceLog(LOG_WARNING, "%s: %dx%d doesn't match a %dx%d-cell footprint",
                 path, img.width, img.height, cols, rows);
        return reject(&img);
    }
    if (cols > 255 || rows > 255 ||
        img.width + 2 * MAX_PAD > ATLAS_SIZE || img.height + 2 * MAX_PAD > ATLAS_SIZE) {
        TraceLog(LOG_WARNING, "%s: size or footprint out of range", path);
        return reject(&img);
    }
    if (batch_shaded && !to_rank_image(&img, path)) return reject(&img);

    int index = glyph_count++;
    glyph_table[index] = (RextGlyph){ .w = img.width, .h = img.height,
                                      .cols = cols, .rows = rows };
    pending[pending_count++] = (Pending){ img, index };
    return index;
}

static int by_height_desc(const void *a, const void *b) {
    return pending[*(const int *)b].img.height - pending[*(const int *)a].img.height;
}

static uint8_t *px(const Image *im, int x, int y) {
    return (uint8_t *)im->data + ((size_t)y * im->width + x) * 4;
}

static void blit_padded(Image *dst, int dx, int dy, const Image *src, int pad) {
    for (int y = -pad; y < src->height + pad; y++) {
        int sy = y < 0 ? 0 : (y >= src->height ? src->height - 1 : y);
        for (int x = -pad; x < src->width + pad; x++) {
            int sx = x < 0 ? 0 : (x >= src->width ? src->width - 1 : x);
            memcpy(px(dst, dx + x, dy + y), px(src, sx, sy), 4);
        }
    }
}

static bool finish_atlas(Image *img, int filter, int mipmaps) {
    Texture2D t = LoadTextureFromImage(*img);
    UnloadImage(*img);
    img->data = NULL;
    if (t.id == 0) return false;
    if (mipmaps) GenTextureMipmaps(&t);
    SetTextureFilter(t, (filter < 0 || filter > 2) ? 0 : filter);
    SetTextureWrap(t, TEXTURE_WRAP_CLAMP);
    atlas_shaded[atlas_count] = batch_shaded;
    atlases[atlas_count++] = t;
    return true;
}

int GlyphBatchPack(int filter, int mipmaps) {
    if (!batch_open || pending_count == 0) return -1;
    if (batch_shaded) { filter = 0; mipmaps = 0; }
    const int pad = mipmaps ? MAX_PAD : 1;
    const int atlas_start = atlas_count;

    int order[MAX_GLYPHS];
    for (int i = 0; i < pending_count; i++) order[i] = i;
    qsort(order, pending_count, sizeof(int), by_height_desc);

    Image atlas = { 0 };
    int cx = 0, cy = 0, shelf_h = 0;

    for (int k = 0; k < pending_count; k++) {
        Pending *p = &pending[order[k]];
        int w = p->img.width + 2 * pad, h = p->img.height + 2 * pad;

        if (atlas.data && cx + w > ATLAS_SIZE) { cx = 0; cy += shelf_h; shelf_h = 0; }
        if (atlas.data && cy + h > ATLAS_SIZE) {
            if (!finish_atlas(&atlas, filter, mipmaps)) goto fail;
        }
        if (!atlas.data) {
            if (atlas_count >= MAX_ATLASES) {
                TraceLog(LOG_WARNING, "GlyphBatchPack: out of atlases");
                goto fail;
            }
            atlas = GenImageColor(ATLAS_SIZE, ATLAS_SIZE, BLANK);
            cx = cy = shelf_h = 0;
        }
        blit_padded(&atlas, cx + pad, cy + pad, &p->img, pad);
        RextGlyph *g = &glyph_table[p->index];
        g->atlas = (uint8_t)atlas_count;
        g->x = cx + pad;
        g->y = cy + pad;
        cx += w;
        if (h > shelf_h) shelf_h = h;
    }
    if (!finish_atlas(&atlas, filter, mipmaps)) goto fail;
    discard_pending();
    return 0;

fail:
    if (atlas.data) UnloadImage(atlas);
    for (int i = atlas_start; i < atlas_count; i++) UnloadTexture(atlases[i]);
    atlas_count = atlas_start;
    glyph_count = batch_first;
    discard_pending();
    return -1;
}

int GlyphCols(int g) { return (g > 0 && g < glyph_count) ? glyph_table[g].cols : 0; }
int GlyphRows(int g) { return (g > 0 && g < glyph_count) ? glyph_table[g].rows : 0; }

static Rectangle span_rect(int cx, int cy, int cols, int rows,
                           float cw, float ch, float ox, float oy) {
    int x0 = (int)floorf(ox + cx * cw + 0.5f);
    int x1 = (int)floorf(ox + (cx + cols) * cw + 0.5f);
    int y0 = (int)floorf(oy + cy * ch + 0.5f);
    int y1 = (int)floorf(oy + (cy + rows) * ch + 0.5f);
    return (Rectangle){ x0, y0, x1 - x0, y1 - y0 };
}

static void draw_glyph(int gi, int shade, uint32_t tint, int cx, int cy,
                       float cw, float ch, float ox, float oy) {
    int limit = batch_open ? batch_first : glyph_count;
    if (gi <= 0 || gi >= limit) return;
    const RextGlyph *g = &glyph_table[gi];
    Color c = atlas_shaded[g->atlas]
        ? (Color){ shade & 255, (shade >> 8) & 255, 0, 255 }
        : rext_unpack(tint);
    DrawTexturePro(atlases[g->atlas], (Rectangle){ g->x, g->y, g->w, g->h },
                   span_rect(cx, cy, g->cols, g->rows, cw, ch, ox, oy),
                   (Vector2){ 0, 0 }, 0.0f, c);
}

void DrawGlyphAt(int g, int shade, uint32_t tint, int cx, int cy,
                 float cw, float ch, float ox, float oy) {
    bool shaded = g > 0 && g < glyph_count && atlas_shaded[glyph_table[g].atlas];
    bool own = shaded && !in_shade_mode;
    if (own) GlyphShadeBegin();
    draw_glyph(g, shade, tint, cx, cy, cw, ch, ox, oy);
    if (own) GlyphShadeEnd();
}

void DrawGlyphGrid(const uint32_t *cells, int cols, int rows,
                   float cw, float ch, float ox, float oy) {
    for (int y = 0; y < rows; y++)
        for (int x = 0; x < cols; x++) {
            uint32_t bg = cells[(y * cols + x) * 2 + 1];
            if (bg >> 24)
                DrawRectangleRec(span_rect(x, y, 1, 1, cw, ch, ox, oy), rext_unpack(bg));
        }
    GlyphShadeBegin();
    for (int y = 0; y < rows; y++)
        for (int x = 0; x < cols; x++) {
            uint32_t w0 = cells[(y * cols + x) * 2];
            int g = (int)(w0 & 0xFFFF);
            if (g > 0 && g < glyph_count && atlas_shaded[glyph_table[g].atlas])
                draw_glyph(g, (int)(w0 >> 16), 0, x, y, cw, ch, ox, oy);
        }
    GlyphShadeEnd();
}

void ShutdownGlyphs(void) {
    discard_pending();
    for (int i = 0; i < atlas_count; i++) UnloadTexture(atlases[i]);
    atlas_count = 0;
    glyph_count = 1;
    if (shader_ready)  { UnloadShader(glyph_shader);  shader_ready = false; }
    if (palette_ready) { UnloadTexture(palette_tex);  palette_ready = false; }
    palette_dirty = true;
}
