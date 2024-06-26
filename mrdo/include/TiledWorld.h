#pragma once
#include <memory>
#include "VectorTypes.h"
#include <vector>
#include "SDL.h"
#include "LevelLoadData.h"
#include <list>
#include <set>

class IBackgroundTileAssetManager;
class IAnimationAssetManager;
class IConfigFile;
struct LevelConfigData;

struct SDL_Surface;
enum class TileWallDirectionBit : u8
{
	Up = 0,
	Right = 1,
	Down = 2,
	Left = 3,
	Center = 4
};

#define TileCherryBit 5

class TiledWorld
{
public:
	struct CellCollisionLines
	{
		std::pair<vec2, vec2> lines[4];
	};
public:
	TiledWorld(
		const std::shared_ptr<IConfigFile>& config, 
		const std::shared_ptr<IBackgroundTileAssetManager>& bgtam,
		const std::shared_ptr<IAnimationAssetManager>& aam);
	void LoadLevel(const LevelLoadData* level);
	ivec2 GetRequiredBaseWindowSize() const;
	void DrawActiveLevel(SDL_Surface* window, float scale) const;
	void ConnectAdjacentCells(const ivec2& cell1, const ivec2& cell2);
	u8& GetCellAtIndex(const ivec2& coords);	
	u8 GetCellAtIndexValue(const ivec2& coords) const;
	CellCollisionLines& GetCellCollisionLines(const ivec2& coords);
	const CellCollisionLines& GetCellCollisionLinesConst(const ivec2&) const;
	u32 GetActiveLevelWidth() const { return ActiveLevelWidth; }
	u32 GetActiveLevelHeight() const { return ActiveLevelHeight; }
	int GetTileSize() const { return TileSize; }
	bool IsBarrierBetween(const ivec2& cell1, const ivec2& cell2) const;
	inline u8 GetHUDTileRowsBottom() const { return HUDTileRowsBottom; }
	inline u8 GetHUDTileRowsTop() const { return HUDTileRowsTop; }
	bool IsCherryAtTile(const ivec2& coords) const;
	void RemoveCherryAtTile(const ivec2& coords);
	void BreakTileCenter(const ivec2& coords);
	void FillTile(const ivec2& coords);
	void AddCherry(const ivec2& coords);
	void FreeLevel();
	void CopyToLevelConfigData(LevelConfigData& dst);
	void PopulateTileLines(ivec2 coords);
	void GetLinesFromCells(std::vector<std::pair<vec2, vec2>>& outLines, const std::vector<ivec2>& inCellCoords) const;
private:
	std::shared_ptr<IConfigFile> Config;
	std::shared_ptr<IBackgroundTileAssetManager> BackgroundTileAssetManager;
	std::shared_ptr<IAnimationAssetManager> AnimationAssetManager;
	std::unique_ptr<u8[]> ActiveLevel;
	const u8* CachedCellCaseToTileIndexLUT;
	int ActiveLevelWidth;
	int ActiveLevelHeight;
	LevelLoadData LevelLoaded;
	bool bLevelLoaded = false;
	std::vector<SDL_Rect> ActiveLevelTileset;
	int TileSize;
	u8 HUDTileRowsTop;
	u8 HUDTileRowsBottom;
	SDL_Rect CherryRect;
	int CurrentTileset = -1;
	
	std::unique_ptr<CellCollisionLines[]> LineSegments;
};