#pragma once
#include <memory>
#include "VectorTypes.h"
#include <vector>
#include "SDL.h"

class IConfigFile;
class IBackgroundTileAssetManager;

struct SDL_Surface;
enum class TileWallDirectionBit : u8
{
	Up = 0,
	Right = 1,
	Down = 2,
	Left = 3,
	Center = 4
};

class TiledWorld
{
public:
	TiledWorld(const std::shared_ptr<IConfigFile>& config, const std::shared_ptr<IBackgroundTileAssetManager>& bgtam);
	void LoadLevel(int level);
	uvec2 GetRequiredBaseWindowSize() const;
	void DrawActiveLevel(SDL_Surface* window, float scale) const;
	void ConnectAdjacentCells(const ivec2& cell1, const ivec2& cell2);
	int GetLevelLoaded() const;
	u8& GetCellAtIndex(const ivec2& coords);	
	u8 GetCellAtIndexValue(const ivec2& coords) const;
	u32 GetActiveLevelWidth() const { return ActiveLevelWidth; }
	u32 GetActiveLevelHeight() const { return ActiveLevelHeight; }
	bool IsBarrierBetween(const ivec2& cell1, const ivec2& cell2) const;
	inline u8 GetHUDTileRowsBottom() const { return HUDTileRowsBottom; }
	inline u8 GetHUDTileRowsTop() const { return HUDTileRowsTop; }

private:
	std::shared_ptr<IConfigFile> Config;
	std::shared_ptr<IBackgroundTileAssetManager> BackgroundTileAssetManager;
	std::unique_ptr<u8[]> ActiveLevel;
	const u8* CachedCellCaseToTileIndexLUT;
	int ActiveLevelWidth;
	int ActiveLevelHeight;
	int LevelLoaded;
	bool bLevelLoaded = false;
	std::vector<SDL_Rect> ActiveLevelTileset;
	int TileSize;
	u8 HUDTileRowsTop;
	u8 HUDTileRowsBottom;
};