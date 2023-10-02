#pragma once
#include <memory>
#include "SDL_surface.h"
#include "CommonTypedefs.h"
#include "IBackgroundTileAssetManager.h"
#include "IConfigFile.h"



class BackgroundTileAssetManager : public IBackgroundTileAssetManager
{
public:
	BackgroundTileAssetManager(const std::shared_ptr<IConfigFile>& configFile);
	~BackgroundTileAssetManager();
	virtual const u8* GetCellCaseToTileIndexLUT() override { return CellCaseToTileIndexLUT; };
	virtual const std::vector<std::vector<SDL_Rect>>& GetLevelTiles() const override { return LevelTiles; }
	virtual SDL_Surface* GetSurface() override { return BackgroundTileSheet; }
	virtual const std::vector<SDL_Rect>& GetLevelTileset(int level) const override { return LevelTiles[level]; }
	virtual int GetBackgroundTileSize() const override { return ConfigFile->GetBackgroundConfigData().TileSize; }
	
private:
	void PopulateLevelTiles();
	void BuildCellCaseToTileIndexLUT();
	
private:
	u8 CellCaseToTileIndexLUT[256];
	SDL_Surface* BackgroundTileSheet;
	std::vector<std::vector<SDL_Rect>> LevelTiles;
	std::shared_ptr<IConfigFile> ConfigFile;
};