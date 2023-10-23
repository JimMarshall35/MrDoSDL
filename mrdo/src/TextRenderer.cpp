#include "TextRenderer.h"
#include "IFontAssetManager.h"
#include "SDL.h"
#include "VectorTypes.h"
#include <cstringt.h>

TextRenderer::TextRenderer(const std::shared_ptr<IFontAssetManager>& fontAssetManager)
	:CachedFontAssetManager(fontAssetManager),
	CachedTileSize(CachedFontAssetManager->GetTextTileSize())
{
}

void TextRenderer::SetCurrentFont(const std::string& fontName)
{
	CurrentFont = CachedFontAssetManager->GetFont(fontName);
}

void TextRenderer::RenderText(const vec2& position, const char* text, SDL_Surface* windowSurface, float scale) const
{
	SDL_Surface* srcSurface = CachedFontAssetManager->GetSurface();
	SDL_Rect dstRect;
	dstRect.x = position.x * scale;
	dstRect.y = position.y * scale;
	dstRect.w = CachedTileSize * scale;
	dstRect.h = CachedTileSize * scale;
	while (*text != '\0')
	{
		char thisChar = *text++;
		const SDL_Rect& srcRect = CurrentFont[thisChar];
		SDL_BlitSurfaceScaled(srcSurface, &srcRect, windowSurface, &dstRect);
		dstRect.x += CachedTileSize * scale;
	}
}

int TextRenderer::GetTextWidthPixels(const char* text) const
{
	size_t stringLen = strlen(text);
	return stringLen * CachedTileSize;
}

int TextRenderer::GetTextTileSize() const
{
	return CachedTileSize;
}
