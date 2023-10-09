#include "BackgroundTileAssetManager.h"
#include <cassert>

BackgroundTileAssetManager::BackgroundTileAssetManager(const std::shared_ptr<IConfigFile>& configFile)
	:BackgroundTileSheet(SDL_LoadBMP(configFile->GetBackgroundConfigData().SpriteSheetAssetPath.c_str())),
	ConfigFile(configFile)
{
	assert(BackgroundTileSheet);
	PopulateLevelTiles();
	BuildCellCaseToTileIndexLUT();
}

BackgroundTileAssetManager::~BackgroundTileAssetManager()
{
	if (BackgroundTileSheet)
	{
		SDL_free(BackgroundTileSheet);
	}
}

void BackgroundTileAssetManager::PopulateLevelTiles()
{
	const BackgroundTileConfigData& data = ConfigFile->GetBackgroundConfigData();
	int tileSizeX = BackgroundTileSheet->w / data.NumCols;
	int tileSizeY = BackgroundTileSheet->h / data.NumRows;
	assert(tileSizeX == tileSizeY);
	int tileSize = tileSizeX;

	for (int y = 0; y < data.NumRows; y++)
	{
		LevelTiles.push_back({});
		for (int x = 0; x < data.NumCols; x++)
		{
			SDL_Rect rect;

			rect.x = tileSize * x;
			rect.y = tileSize * y;
			rect.w = tileSize;
			rect.h = tileSize;
				
			LevelTiles[y].push_back(rect);
		}
	}
}

void BackgroundTileAssetManager::BuildCellCaseToTileIndexLUT()
{
	const BackgroundTileConfigData& data = ConfigFile->GetBackgroundConfigData();
	for (int i=0; i<data.RowPattern.size(); i++)
	{
		u8 val = data.RowPattern[i];
		CellCaseToTileIndexLUT[val] = i;
	}
}
