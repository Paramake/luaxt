#include <stdio.h>
#include <assert.h>
#include <fstream>
#include <math.h>

#include "Renderer.h"


#pragma region PIXEL BLENDING
static inline RenColor blend_pixel(RenColor dst, RenColor src) {
    int ia = 0xff - src.a;
    dst.r = ((src.r * src.a) + (dst.r * ia)) >> 8;
    dst.g = ((src.g * src.a) + (dst.g * ia)) >> 8;
    dst.b = ((src.b * src.a) + (dst.b * ia)) >> 8;
    return dst;
}


static inline RenColor blend_pixel2(RenColor dst, RenColor src, RenColor color) {
    src.a = (src.a * color.a) >> 8;
    int ia = 0xff - src.a;
    dst.r = ((src.r * color.r * src.a) >> 16) + ((dst.r * ia) >> 8);
    dst.g = ((src.g * color.g * src.a) >> 16) + ((dst.g * ia) >> 8);
    dst.b = ((src.b * color.b * src.a) >> 16) + ((dst.b * ia) >> 8);
    return dst;
}

#pragma endregion

#pragma region RENDERER CONSTRUCTOR
Renderer::Renderer(SDL_Window* window) : window(window)
{
    assert(window);

    auto surf = SDL_GetWindowSurface(window);
    setClipRect(RenRect { 0, 0, surf->w, surf->h });
}

Renderer::~Renderer() { }
#pragma endregion

#pragma region RENDER GLYPHS
GlyphSet* Renderer::loadGlyphset(RenFont* font, int idx)
{
    GlyphSet* set = new GlyphSet();
    assert(set);

    int width = 128, height = 128;

retry:
    set->image = newImage(width, height);
    auto size = stbtt_ScaleForMappingEmToPixels(&font->stbfont, 1) /
        stbtt_ScaleForPixelHeight(&font->stbfont, 1);

    auto res = stbtt_BakeFontBitmap(reinterpret_cast<unsigned char*>(font->data), 0, font->size * size, 
        reinterpret_cast<unsigned char*>(set->image->pixels),width, height, idx * 256, 256, set->glyphs);

    if (res < 0)
    {
        width *= 2;
        height *= 2;
        freeImage(set->image);
        goto retry;
    }

    int asc, desc, linegap;
    stbtt_GetFontVMetrics(&font->stbfont, &asc, &desc, &linegap);
    auto scale = stbtt_ScaleForMappingEmToPixels(&font->stbfont, font->size);
    auto scaled_asc = asc * scale + 0.5;
    for (auto i = 0; i < 256; i++)
    {
        set->glyphs[i].yoff += scaled_asc;
        set->glyphs[i].xadvance = floor(set->glyphs[i].xadvance);
    }

    for (auto i = width * height - 1; i >= 0; i--)
    {
        auto n = *reinterpret_cast<uint8_t*>(set->image->pixels + i);
        set->image->pixels[i] = RenColor{ 255, 255, 255, n };
    }

    return set;
}

GlyphSet* Renderer::getGlyphset(RenFont* font, int codePoint)
{
    auto idx = (codePoint >> 8) & MAX_GLYPHSET;
    if (!font->sets[idx])
    {
        font->sets[idx] = loadGlyphset(font, idx);
    }

    return font->sets[idx];
}
#pragma endregion

#pragma region RENDER RECTS
void Renderer::updateRects(std::vector<RenRect> rects)
{
    SDL_UpdateWindowSurfaceRects(window, reinterpret_cast<const SDL_Rect*>(&rects[0]), rects.size());
}

void Renderer::setClipRect(RenRect rect)
{
    clip.left = rect.x;
    clip.top = rect.y;
    clip.right = rect.x + rect.width;
    clip.bottom = rect.y + rect.width;
}

void Renderer::getSize(int& x, int& y)
{
    auto surf = SDL_GetWindowSurface(window);
    x = surf->w, y = surf->h;
}
#pragma endregion

#pragma region RENDER IMAGES
RenImage* Renderer::newImage(int width, int height)
{
    assert(width > 0 && height > 0);

    auto image = new RenImage();
    assert(image);

    image->height = height;
    image->width = width;
    return image;
}

void Renderer::freeImage(RenImage* image)
{
    assert(image);
    delete image;
}
#pragma endregion

#pragma region RENDER FONTS
static const char* utf8_to_codepoint(const char* p, unsigned& dst) {
    unsigned res, n;
    switch (*p & 0xf0) {
        case 0xf0:  res = *p & 0x07;  n = 3;  break;
        case 0xe0:  res = *p & 0x0f;  n = 2;  break;
        case 0xd0:
        case 0xc0:  res = *p & 0x1f;  n = 1;  break;
        default:    res = *p;         n = 0;  break;
    }
    while (n--) {
        res = (res << 6) | (*(++p) & 0x3f);
    }

    dst = res;
    return p + 1;
}

RenFont* Renderer::loadFont(const std::string& fileName, float size)
{
    RenFont* font = new RenFont();
    assert(font);

    font->size = size;

    // load the font into the buffer
    std::streampos fileSize = 0;
    auto file = std::ifstream(fileName, std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        delete font;
        return nullptr;
    }

    fileSize = file.tellg();
    file.seekg(0, std::ios::end);
    fileSize = file.tellg() - fileSize;
    file.seekg(0, std::ios::beg);
    font->data = new char[fileSize];
    file.read(reinterpret_cast<char*>(font->data), fileSize);
    file.close();

    // init stbfont
    auto res = stbtt_InitFont(&font->stbfont, reinterpret_cast<const uint8_t*>(font->data), 0);
    if (!res) 
    {
        delete[] font->data;
        delete font;
    }


    // get height and scale
    int asc, desc, linegap;
    stbtt_GetFontVMetrics(&font->stbfont, &asc, &desc, &linegap);
    auto scale = stbtt_ScaleForMappingEmToPixels(&font->stbfont, font->size);
    font->height = (asc - desc + linegap) * scale + 0.5;

    // make tab and newline glyphs invisible
    auto glyphs = getGlyphset(font, '\n')->glyphs;
    glyphs['\t'].x1 = glyphs['\t'].x0;
    glyphs['\n'].x1 = glyphs['\n'].x0;

    return font;
}

void Renderer::freeFont(RenFont* font)
{
    for (auto i = 0; i < MAX_GLYPHSET; i++)
    {
        auto set = font->sets[i];
        if (set)
        {
            freeImage(set->image);
            delete set;
        }
    }

    delete[] font->data;
    delete font;
}

void Renderer::setFontTabWidth(RenFont* font, int width)
{
    auto set = getGlyphset(font, '\t');
    set->glyphs['\t'].xadvance = width;
}

int Renderer::getFontWidth(RenFont* font, std::string text)
{
    int x = 0;
    unsigned codePoint;
    const char* p = text.c_str();
    while (*p) 
    {
        p = utf8_to_codepoint(p, codePoint);
        auto set = getGlyphset(font, codePoint);
        auto glyph = &set->glyphs[codePoint & 0xff];
        x += glyph->xadvance;
    }
    return x;
}

int Renderer::getFontHeight(RenFont* font)
{
    return font->height;
}
#pragma endregion

#pragma region RENDER DRAWING
#define IMAGE_N_MANIP(x, y, z) \
    if((n = x) > 0) { y -= n; z; }

#define RECT_DRAW_LOOP(expr)        \
  for (int j = y1; j < y2; j++) {   \
    for (int i = x1; i < x2; i++) { \
      *pixels = expr;               \
      pixels++;                     \
    }                               \
    pixels += pixelsR;              \
  }



void Renderer::DrawRect(RenRect rect, RenColor color)
{
    if (color.a == 0) return;
    
    int x1 = rect.x < clip.left ? clip.left : rect.x,
        y1 = rect.y < clip.top ? clip.top : rect.y,
        x2 = rect.x + rect.width,
        y2 = rect.y + rect.height;

    x2 = x2 > clip.right ? clip.right : x2;
    y2 = y2 > clip.bottom ? clip.bottom : y2;

    auto surface = SDL_GetWindowSurface(window);
    auto pixels = reinterpret_cast<RenColor*>(surface->pixels);

    pixels += x1 + y1 * surface->w;
    auto pixelsR = surface->w - (x2 - x1);

    if (color.a == 0xff)
    {
        RECT_DRAW_LOOP(color);
    }
    else
    {
        RECT_DRAW_LOOP(blend_pixel(*pixels, color));
    }
}

void Renderer::DrawImage(RenImage* image, RenRect* sub, int x, int y, RenColor color)
{
    if (color.a == 0) return;

    int n;

    IMAGE_N_MANIP(clip.left - x, sub->width, sub->x += n; x += n);
    IMAGE_N_MANIP(clip.top  - y, sub->height, sub->y += n; y += n);
    IMAGE_N_MANIP(x + sub->width  - clip.right,  sub->width);
    IMAGE_N_MANIP(y + sub->height - clip.bottom, sub->height);

    if (sub->width <= 0 || sub->height <= 0) return;

    // draw image
    auto surf = SDL_GetWindowSurface(window);
    auto imgPixels = image->pixels;
    auto srfPixels = reinterpret_cast<RenColor*>(surf->pixels);

    imgPixels += sub->x + sub->y * image->width;
    srfPixels += x + y * surf->w;

    auto imgPixelsR = image->width - sub->width;
    auto srfPixelsR = surf->w - sub->width;

    for (int j = 0; j < sub->height; j++) {
        for (int i = 0; i < sub->width; i++) {
            *imgPixels = blend_pixel2(*imgPixels, *srfPixels, color);

            imgPixels++;
            srfPixels++;
        }
        imgPixels += imgPixelsR;
        srfPixels += srfPixelsR;
    }
}

int Renderer::DrawText(RenFont* font, std::string text, int x, int y, RenColor color)
{
    RenRect rect;
    const char* p = text.c_str();
    unsigned codePoint;
    while (*p)
    {
        p = utf8_to_codepoint(p, codePoint);
        auto set = getGlyphset(font, codePoint);
        auto glyph = &set->glyphs[codePoint & 0xff];
        rect.x = glyph->x0, rect.y = glyph->y0;
        rect.width = glyph->x1 - glyph->x0;
        rect.height = glyph->y1 - glyph->y0;
        DrawImage(set->image, &rect, x + glyph->xoff, y + glyph->y0, color);
        x += glyph->xadvance;
    }

    return x;
}
#pragma endregion