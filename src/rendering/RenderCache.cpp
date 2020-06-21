#include "RenderCache.h"


enum { FREE_FONT, SET_CLIP, DRAW_TEXT, DRAW_RECT };


#pragma region HELPER FUNCTIONS
static inline int min(int a, int b) { return a < b ? a : b; }
static inline int max(int a, int b) { return a > b ? a : b; }

/* 32bit fnv-1a hash */
#define HASH_INITIAL 2166136261

static void hash(unsigned* h, const void* data, int size) {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(data);
    while (size--) {
        *h = (*h ^ *p++) * 16777619;
    }
}

static inline int cellIndex(int x, int y) {
    return x + y * CELLS_X;
}

static inline bool doRectsOverlap(RenRect a, RenRect b) {
    return b.x + b.width >= a.x && b.x <= a.x + a.width
        && b.y + b.height >= a.y && b.y <= a.y + a.height;
}

static RenRect intersectRects(RenRect a, RenRect b) {
    int x1 = max(a.x, b.x);
    int y1 = max(a.y, b.y);
    int x2 = min(a.x + a.width, b.x + b.width);
    int y2 = min(a.y + a.height, b.y + b.height);
    return RenRect { x1, y1, max(0, x2 - x1), max(0, y2 - y1) };
}

static RenRect mergeRects(RenRect a, RenRect b) {
    int x1 = min(a.x, b.x);
    int y1 = min(a.y, b.y);
    int x2 = max(a.x + a.width, b.x + b.width);
    int y2 = max(a.y + a.height, b.y + b.height);
    return RenRect { x1, y1, x2 - x1, y2 - y1 };
}
#pragma endregion


RenderCache::RenderCache(Renderer& renderer) : renderer(renderer), rectBuffer(std::vector<RenRect>(CELLS_X * CELLS_Y / 2)),
    cellsBuffer1(std::vector<uint32_t>(CELLS_X * CELLS_Y)), cellsBuffer2(std::vector<uint32_t>(CELLS_X * CELLS_Y))
{
    cellsPrevious = &cellsBuffer1;
    cells = &cellsBuffer2;
}

RenderCache::~RenderCache()
{
}

void RenderCache::updateOverlappingCells(RenRect rect, uint32_t height)
{
    int x1 = rect.x / CELL_SIZE;
    int y1 = rect.y / CELL_SIZE;
    int x2 = (rect.x + rect.width) / CELL_SIZE;
    int y2 = (rect.y + rect.height) / CELL_SIZE;

    for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            int idx = cellIndex(x, y);
            hash(&(*cells)[idx], &height, sizeof(height));
        }
    }
}

void RenderCache::pushRect(RenRect rect, int& count)
{
    for (auto i = count - 1; i >= 0; i--)
    {
        auto rectP = &rectBuffer[i];
        if(doRectsOverlap(*rectP, rect))
        {
            *rectP = mergeRects(*rectP, rect);
            return;
        }
    }

    rectBuffer[count++] = rect;
}

Command* RenderCache::pushCommand(int type, int size)
{
    size += sizeof(Command);
    auto command = reinterpret_cast<Command*>(commandBuffer + commandBufferIdx);
    memset(command, 0, sizeof(Command));

    auto num = commandBufferIdx + size;
    if (num > COMMAND_BUF_SIZE)
        throw std::exception("Exhaused command buffer");

    commandBufferIdx = num;
    command->type = type;
    command->size = size;
    return command;
}

bool RenderCache::nextCommand(Command** previous)
{
    if (*previous == nullptr)
    {
        *previous = reinterpret_cast<Command*>(commandBufferIdx);
    }
    else
    {
        *previous = reinterpret_cast<Command*>(reinterpret_cast<char*>(*previous) + (*previous)->size);
    }

    return *previous != reinterpret_cast<Command*>(commandBuffer + commandBufferIdx);
}

void RenderCache::showDebug(bool enable)
{
    showDebugInfo = enable;
}

void RenderCache::freeFont(RenFont* font)
{
    auto command = pushCommand(FREE_FONT);
    command->font = font;
}

void RenderCache::setClipRect(RenRect rect)
{
    auto command = pushCommand(SET_CLIP);
    command->rect = intersectRects(rect, screenRect);
}

void RenderCache::drawRect(RenRect rect, RenColor color)
{
    auto command = pushCommand(DRAW_RECT);
    command->rect = rect;
    command->color = color;
}

int RenderCache::drawText(RenFont* font, std::string text, int x, int y, RenColor color)
{
    auto len = text.length() + 1;
    auto command = pushCommand(DRAW_TEXT, text.length() + 1);
    memcpy(command->text, text.c_str(), len - 1);
    command->text[len] = '\0'; // just to make sure
    command->font = font;
    command->rect.x = x;
    command->rect.y = y;
    command->rect.width = renderer.getFontWidth(font, text);
    command->rect.height = renderer.getFontHeight(font);
	return x + command->rect.width;
}

void RenderCache::beginFrame()
{
    int width, height;
    renderer.getSize(width, height);

    if (screenRect.width != 0 || height != screenRect.height)
    {
        screenRect.width = width;
        screenRect.height = height;
        InvalidateCache();
    }
}

void RenderCache::endFrame()
{
    Command* command = nullptr;
    auto clipRect = screenRect;

    while (nextCommand(&command))
    {
        if (command->type == SET_CLIP) clipRect = command->rect;

        auto rect = intersectRects(command->rect, clipRect);
        if (rect.width == 0 || rect.height == 0) continue;

        uint32_t hsh = HASH_INITIAL;
        hash(&hsh, command, command->size);
        updateOverlappingCells(rect, hsh);
    }

    auto rectCount = 0;
    auto maxX = screenRect.width / CELL_SIZE + 1;
    auto maxY = screenRect.height / CELL_SIZE + 1;
    for (auto y = 0; y < maxY; y++)
    {
        for (auto x = 0; x < maxX; x++)
        {
            auto idx = cellIndex(x, y);
            if (cells[idx] != cellsPrevious[idx])
            {
                pushRect(RenRect{ x, y, 1, 1 }, rectCount);
            }

            (*cellsPrevious)[idx] = HASH_INITIAL;
        }
    }

    for (auto i = 0; i < rectCount; i++)
    {
        auto rect = &rectBuffer[i];
        rect->x *= CELL_SIZE, rect->y *= CELL_SIZE;
        rect->width*= CELL_SIZE, rect->height *= CELL_SIZE;
        *rect = intersectRects(*rect, screenRect);
    }


    bool freeCommands = false;
    for (auto i = 0; i < rectCount; i++)
    {
        auto rect = rectBuffer[i];
        renderer.setClipRect(rect);

        command = nullptr;
        while (nextCommand(&command))
        {
            switch (command->type)
            {
            case FREE_FONT:
                freeCommands = true;
                break;
            case SET_CLIP:
                renderer.setClipRect(intersectRects(command->rect, rect));
                break;
            case DRAW_RECT:
                renderer.DrawRect(command->rect, command->color);
                break;
            case DRAW_TEXT:
                renderer.DrawText(command->font, command->text, command->rect.x, command->rect.y, command->color);
                break;
            }

            if (showDebugInfo)
            {
                // not needed for now
                // renderer.DrawRect(rect, RenColor{ (char)rand(), (char)rand(), (char)rand(), 50 });
            }
        }
    }

    if (rectCount > 0)
    {
        renderer.updateRects(rectBuffer);
    }

    if (freeCommands)
    {
        command = nullptr;
        while (nextCommand(&command))
        {
            if (command->type == FREE_FONT)
            {
                renderer.freeFont(command->font);
            }
        }
    }
    
    auto temp = cells;
    cells = cellsPrevious;
    cellsPrevious = temp;
    commandBufferIdx = 0;
}

void RenderCache::InvalidateCache()
{
    memset(cellsPrevious, 0xff, sizeof(cellsBuffer1));
}
