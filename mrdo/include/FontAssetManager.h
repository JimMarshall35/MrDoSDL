#pragma once
#include <memory>
#include <vector>
#include "SDL.h"
#include "IFontAssetManager.h"
#include <map>
#include <string>
#include "CommonTypedefs.h"

class IConfigFile;
struct FontConfigData;

// each value in the map is a lookup table, indexed into by ascii characters in the string
typedef std::map<std::string, SDL_Rect[256]> FontRectMap;

class FontAssetManager : public IFontAssetManager
{
public:
#ifdef ReplayValidator
	FontAssetManager(const std::shared_ptr<IConfigFile>& config);
#else
	FontAssetManager(const std::shared_ptr<IConfigFile>& config, SDL_Surface* windowSurface);
#endif
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