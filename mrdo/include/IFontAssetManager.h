#pragma once
#include <string>

struct SDL_Rect;
struct SDL_Surface;

class IFontAssetManager
{
public:
	virtual const SDL_Rect* GetFont(const std::string& fontName) const = 0;
	virtual SDL_Surface* GetSurface() = 0;
	virtual int GetTextTileSize() const = 0;
};