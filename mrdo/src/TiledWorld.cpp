#include "TiledWorld.h"
#include "IConfigFile.h"
#include "IBackgroundTileAssetManager.h"
#include <cassert>
#include <stdlib.h>     /* abs */

TiledWorld::TiledWorld(const std::shared_ptr<IConfigFile>& config, const std::shared_ptr<IBackgroundTileAssetManager>& bgtam)
	:Config(config),
	BackgroundTileAssetManager(bgtam),
	ActiveLevel(std::unique_ptr<u8[]>()),
	CachedCellCaseToTileIndexLUT(BackgroundTileAssetManager->GetCellCaseToTileIndexLUT()),
	ActiveLevelWidth(-1),
	ActiveLevelHeight(-1),
	LevelLoaded(-1),
	bLevelLoaded(false),
	ActiveLevelTileset(std::vector<SDL_Rect>()),
	TileSize(BackgroundTileAssetManager->GetBackgroundTileSize())
{

}

void TiledWorld::LoadLevel(int level)
{
	LevelLoaded = level;
	if (!bLevelLoaded)
	{
		bLevelLoaded = true;
	}
	else
	{
		assert(ActiveLevel.get());
		ActiveLevel.reset();
	}
	const std::vector<LevelConfigData>& levels = Config->GetLevelsConfigData();
	const LevelConfigData& selectedLevel = levels[level];
	ActiveLevelWidth = selectedLevel.NumCols;
	ActiveLevelHeight = selectedLevel.NumRows;
	ActiveLevel = std::make_unique<u8[]>(selectedLevel.TileData.size());
	memcpy(ActiveLevel.get(), selectedLevel.TileData.data(), selectedLevel.TileData.size());
	ActiveLevelTileset = BackgroundTileAssetManager->GetLevelTileset(selectedLevel.BackgroundTileset);
}

uvec2 TiledWorld::GetRequiredBaseWindowSize() const
{
	assert(bLevelLoaded);
	const BackgroundTileConfigData& backgroundConfig = Config->GetBackgroundConfigData();
	return { (u32)backgroundConfig.TileSize * ActiveLevelWidth,  (u32)backgroundConfig.TileSize * ActiveLevelHeight };
}

void TiledWorld::DrawActiveLevel(SDL_Surface* window, float scale) const
{
	SDL_Surface* surface = BackgroundTileAssetManager->GetSurface();
	SDL_Rect dst;
	
	dst.w = TileSize * scale;
	dst.h = TileSize * scale;
	for (int y = 0; y < ActiveLevelHeight; y++)
	{
		for (int x = 0; x < ActiveLevelWidth; x++)
		{
			u8 cell = ActiveLevel[y * ActiveLevelWidth + x];
			const SDL_Rect* rect = &ActiveLevelTileset[CachedCellCaseToTileIndexLUT[cell]];
			dst.x = x * TileSize * scale;
			dst.y = y * TileSize * scale;
			SDL_BlitSurfaceScaled(surface, rect, window, &dst);
		}
	}
}

void TiledWorld::ConnectAdjacentCells(const ivec2& cell1, const ivec2& cell2)
{
	if (cell1.x < 0 || cell1.y < 0 || cell2.x < 0 || cell2.y < 0)
	{
		return;
	}
	if (cell1.x >= ActiveLevelWidth || cell1.y >= ActiveLevelHeight || cell2.x >= ActiveLevelWidth || cell2.y >= ActiveLevelHeight)
	{
		return;
	}
	int dx = cell1.x - cell2.x;
	int dy = cell1.y - cell2.y;
	assert((dx >= -1) && (dx <= 1));
	assert((dy >= -1) && (dy <= 1));
	assert((dx == 0) || (dy == 0));

	u8& cell1Val = GetCellAtIndex(cell1);
	u8& cell2Val = GetCellAtIndex(cell2);
	if (dx == 1)
	{
		// break cell1's left wall and cell2's right wall
		cell1Val &= ~(1 << (u8)TileWallDirectionBit::Left);
		cell2Val &= ~(1 << (u8)TileWallDirectionBit::Right);
	}
	else if (dx == -1)
	{
		// break cell1's right wall and cell2's left wall
		cell1Val &= ~(1 << (u8)TileWallDirectionBit::Right);
		cell2Val &= ~(1 << (u8)TileWallDirectionBit::Left);
	}
	else if (dy == 1)
	{
		// break cell1's top wall and cell2's bottom wall
		cell1Val &= ~(1 << (u8)TileWallDirectionBit::Up);
		cell2Val &= ~(1 << (u8)TileWallDirectionBit::Down);
	}
	else if (dy == -1)
	{
		// break cell1's bottom wall and cell2's top wall
		cell1Val &= ~(1 << (u8)TileWallDirectionBit::Down);
		cell2Val &= ~(1 << (u8)TileWallDirectionBit::Up);
	}

	// both cells now have no center
	cell1Val &= ~(1 << (u8)TileWallDirectionBit::Center);
	cell2Val &= ~(1 << (u8)TileWallDirectionBit::Center);
}

int TiledWorld::GetLevelLoaded() const
{
	assert(bLevelLoaded);
	return LevelLoaded;
}

u8& TiledWorld::GetCellAtIndex(const ivec2& coords)
{
	assert(coords.x >= 0 && coords.y >= 0);
	assert((coords.x < ActiveLevelWidth) && (coords.y < ActiveLevelHeight));
	int i = coords.y * ActiveLevelWidth + coords.x;
	return ActiveLevel[i];
}

u8 TiledWorld::GetCellAtIndexValue(const ivec2& coords) const
{
	static const u8 fullCell = 0xff;
	if (!(coords.x >= 0 && coords.y >= 0) ||
		!((coords.x < ActiveLevelWidth) && (coords.y < ActiveLevelHeight)))
	{
		return fullCell;
	}
	int i = coords.y * ActiveLevelWidth + coords.x;
	return ActiveLevel[i];
}

bool TiledWorld::IsBarrierBetween(const ivec2& cell1, const ivec2& cell2) const
{
	if (cell1.x < 0 || cell1.y < 0 || cell2.x < 0 || cell2.y < 0)
	{
		return true;
	}
	if (cell1.x >= ActiveLevelWidth || cell1.y >= ActiveLevelHeight || cell2.x >= ActiveLevelWidth || cell2.y >= ActiveLevelHeight)
	{
		return true;
	}
	int dx = cell2.x - cell1.x;
	int dy = cell2.y - cell1.y;
	assert((dx >= -1) && (dx <= 1));
	assert((dy >= -1) && (dy <= 1));
	assert((dx == 0) || (dy == 0));
	u8 cell1Val = GetCellAtIndexValue(cell1);
	u8 cell2Val = GetCellAtIndexValue(cell2);
	if (dx == 1)
	{
		return (cell1Val & (u8)(1 << (u8)TileWallDirectionBit::Right)) || (cell2Val & (u8)(1 << (u8)TileWallDirectionBit::Left));
	}
	else if (dx == -1)
	{
		return (cell1Val & (u8)(1 << (u8)TileWallDirectionBit::Left)) || (cell2Val & (u8)(1 << (u8)TileWallDirectionBit::Right));
	}
	else if (dy == 1)
	{
		return (cell1Val & (u8)(1 << (u8)TileWallDirectionBit::Down)) || (cell2Val & (u8)(1 << (u8)TileWallDirectionBit::Up));
	}
	else if (dy == -1)
	{
		return (cell1Val & (u8)(1 << (u8)TileWallDirectionBit::Up)) || (cell2Val & (u8)(1 << (u8)TileWallDirectionBit::Down));
	}

}
