#pragma once

#include "Renderer.h"

#define CELLS_X 80
#define CELLS_Y 50
#define CELL_SIZE 96
#define COMMAND_BUF_SIZE (1024 * 512)

typedef struct {
	int type, size;
	RenRect rect;
	RenColor color;
	RenFont* font;
	char* text;
} Command;

class RenderCache
{
private:
	Renderer& renderer;
	RenRect screenRect;

	std::vector<RenRect>& rectBuffer;
	std::vector<uint32_t>& cellsBuffer1;
	std::vector<uint32_t>& cellsBuffer2;

	// 
	std::vector<uint32_t>* cellsPrevious;
	std::vector<uint32_t>* cells;

	char commandBuffer[COMMAND_BUF_SIZE];
	int commandBufferIdx;
	bool showDebugInfo;

	void updateOverlappingCells(RenRect rect, uint32_t height);
	void pushRect(RenRect rect, int& count);

	Command* pushCommand(int type, int size = 0);
	bool nextCommand(Command** previous);

public:
	RenderCache(Renderer& renderer);
	~RenderCache();

	void showDebug(bool enable);
	void freeFont(RenFont* font);
	void setClipRect(RenRect rect);
	void drawRect(RenRect rect, RenColor color);
	int drawText(RenFont* font, std::string text, int x, int y, RenColor color);

	void beginFrame();
	void endFrame();

	void InvalidateCache();
};
