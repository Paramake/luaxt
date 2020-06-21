#pragma once

#include <string>
#include <stdint.h>
#include <vector>

#include <SDL.h>
#include "../stb/stb_truetype.h"

#define MAX_GLYPHSET 256

#pragma region RENDERER STRUCTS
struct RenColor
{
	uint8_t b, g, r, a;
};

struct RenRect
{
	int x, y, width, height;
};

struct RenImage {
	RenColor* pixels;
	int width, height;
};

struct GlyphSet {
	RenImage* image;
	stbtt_bakedchar glyphs[256];
};

struct RenFont {
	void* data;
	stbtt_fontinfo stbfont;
	GlyphSet* sets[MAX_GLYPHSET];
	float size;
	int height;
};
#pragma endregion

class Renderer
{
private:
	SDL_Window* window;
	struct { int left, top, right, bottom; } clip;

	GlyphSet* loadGlyphset(RenFont* font, int idx);
	GlyphSet* getGlyphset(RenFont* font, int codePoint);

public:
	Renderer(SDL_Window* window);
	~Renderer();

	// rects stuff
	void updateRects(std::vector<RenRect> rects);
	void setClipRect(RenRect rect);
	void getSize(int& x, int& y);

	// image stuff
	RenImage* newImage(int width, int height);
	void freeImage(RenImage* image);

	// font stuff
	RenFont* loadFont(const std::string& fileName, float size);
	void freeFont(RenFont* font);
	void setFontTabWidth(RenFont* font, int width);
	int getFontWidth(RenFont* font, std::string text);
	int getFontHeight(RenFont* font);

	// actual rendering
	void DrawRect(RenRect rect, RenColor color);
	void DrawImage(RenImage* image, RenRect* sub, int x, int y, RenColor color);
	int DrawText(RenFont* font, std::string text, int x, int y, RenColor color);
};
