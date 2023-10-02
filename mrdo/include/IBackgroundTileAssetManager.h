#pragma once
#include "CommonTypedefs.h"
#include <vector>

struct SDL_Surface;
struct SDL_Rect;

class IBackgroundTileAssetManager
{
public:
	virtual const u8* GetCellCaseToTileIndexLUT() = 0;
	virtual SDL_Surface* GetSurface() = 0;
	virtual const std::vector<SDL_Rect>& GetLevelTileset(int level) const = 0;
	virtual const std::vector<std::vector<SDL_Rect>>& GetLevelTiles() const = 0;
	virtual int GetBackgroundTileSize() const = 0;
};