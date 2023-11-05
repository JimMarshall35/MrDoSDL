#include "FontAssetManager.h"
#include "IConfigFile.h"
#include <cassert>

FontAssetManager::FontAssetManager(const std::shared_ptr<IConfigFile>& config, SDL_Surface* windowSurface)
	:FontSheet(SDL_LoadBMP(config->GetFontConfigData().SpriteSheetAssetPath.c_str()))
{
	// ensure loaded bmp is the same format as the screen
	FontSheet = SDL_ConvertSurface(FontSheet, windowSurface->format);

	// set colour of pixel to be treated as transparent when blitting
	const FontConfigData& data = config->GetFontConfigData();
	SDL_SetSurfaceColorKey(FontSheet, SDL_TRUE, SDL_MapRGB(windowSurface->format, data.ColourKeyR, data.ColourKeyG, data.ColourKeyB));
	CachedFontTileSize = data.TileSize;

	PopulateFontRectMap(data);
}

FontAssetManager::~FontAssetManager()
{
	if (FontSheet)
	{
		SDL_free(FontSheet);
	}
}

const SDL_Rect* FontAssetManager::GetFont(const std::string& fontName) const
{
	assert(Fonts.count(fontName));
	return Fonts.at(fontName);
}

SDL_Surface* FontAssetManager::GetSurface()
{
	return FontSheet;
}

int FontAssetManager::GetTextTileSize() const
{
	return CachedFontTileSize;
}

void FontAssetManager::PopulateFontRectMap(const FontConfigData& config)
{
	ivec2 blockDims = config.BlockDims;
	int tileSize = config.TileSize;
	for (const auto& pair : config.Blocks)
	{
		// block coords IN BLOCKS
		ivec2 blockCoords = pair.second;

		// top left corner of the block in pixels
		ivec2 blockStartingPointPixels = {
			blockCoords.x * blockDims.x * tileSize,
			blockCoords.y * blockDims.y * tileSize
		};

		// array to write to
		SDL_Rect* rects = Fonts[pair.first];
		
		for (const std::pair<u8, u8>& mapping : config.BlockMapping)
		{
			// index of letter - describes where it is on the image.
			// top left corner is index 0, increases going right and down
			u8 letterIndex = mapping.first;

			// which ascii code does this index correspond to
			u8 ascii = mapping.second;

			// get coords of the letter in tiles within the block
			u32 letterX = letterIndex % blockDims.x;
			u32 letterY = letterIndex / blockDims.x;

			// cordinate of letter tile top left in pixels
			ivec2 letterRectTL = blockStartingPointPixels + ivec2{ letterX * config.TileSize, letterY * config.TileSize };
			
			// make rect and save to array
			SDL_Rect r;
			r.x = letterRectTL.x;
			r.y = letterRectTL.y;
			r.w = tileSize;
			r.h = tileSize;
			rects[ascii] = r;

			if (((ascii >= 'A') && (ascii <= 'Z')) && config.LetterAvailability == FontLetterAvailability::AllCaps)
			{
				// if only caps, then fill in index at the corresponding lower case ascii code for case insensitivity
				rects[ascii + ('a' - 'A')] = r;
			}
			else if (((ascii >= 'a') && (ascii <= 'z')) && config.LetterAvailability == FontLetterAvailability::AllLowercase)
			{
				// if all lower case then fill in the index at the corresponding caps ascii code for case insensitivity
				rects[ascii - ('a' - 'A')] = r;
			}
		}
	}
}
