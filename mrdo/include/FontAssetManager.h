#pragma once
#include <memory>
#include <vector>
#include "SDL.h"
#include "IFontAssetManager.h"
#include <map>
#include <string>

class IConfigFile;
struct FontConfigData;

typedef std::map<std::string, SDL_Rect[256]> FontRectMap;

class FontAssetManager : public IFontAssetManager
{
public:
	FontAssetManager(const std::shared_ptr<IConfigFile>& config, SDL_Surface* windowSurface);
	~FontAssetManager();

	// IFontAssetManager begin
	virtual const SDL_Rect* GetFont(const std::string& fontName) const override;
	virtual SDL_Surface* GetSurface() override;
	virtual int GetTextTileSize() const override;
	// IFontAssetManager end

private:
	void PopulateFontRectMap(const FontConfigData& config);

private:
	SDL_Surface* FontSheet;
	FontRectMap Fonts;
	int CachedFontTileSize;
};