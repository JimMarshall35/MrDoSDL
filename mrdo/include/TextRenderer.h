#pragma once
#include <memory>
#include <string>
#include "CommonTypedefs.h"

struct SDL_Surface;
struct SDL_Rect;
class IFontAssetManager;
struct vec2;


class TextRenderer
{
public:
	TextRenderer(const std::shared_ptr<IFontAssetManager>& fontAssetManager);
	void SetCurrentFont(const std::string& fontName);
	void RenderText(const vec2& position, const char* text, SDL_Surface* windowSurface, float scale) const;
	int GetTextWidthPixels(const char* text) const;
	int GetTextTileSize() const;

private:
	std::shared_ptr<IFontAssetManager> CachedFontAssetManager;
	u32 CachedTileSize;
	const SDL_Rect* CurrentFont = nullptr;
};