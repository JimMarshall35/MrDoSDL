#include "TiledWorld.h"
#include "IConfigFile.h"
#include "IBackgroundTileAssetManager.h"
#include "IAnimationAssetManager.h"
#include <cassert>
#include <stdlib.h>     /* abs */
#include <cmath>
TiledWorld::TiledWorld(
	const std::shared_ptr<IConfigFile>& config, 
	const std::shared_ptr<IBackgroundTileAssetManager>& bgtam,
	const std::shared_ptr<IAnimationAssetManager>& aam)
	:Config(config),
	BackgroundTileAssetManager(bgtam),
	AnimationAssetManager(aam),
	ActiveLevel(std::unique_ptr<u8[]>()),
	CachedCellCaseToTileIndexLUT(BackgroundTileAssetManager->GetCellCaseToTileIndexLUT()),
	ActiveLevelWidth(-1),
	ActiveLevelHeight(-1),
	LevelLoaded({LevelSource::Undefined, -1}),
	bLevelLoaded(false),
	ActiveLevelTileset(std::vector<SDL_Rect>()),
	TileSize(BackgroundTileAssetManager->GetBackgroundTileSize()),
	HUDTileRowsTop(config->GetUIntValue("HUDTileRowsTop")),
	HUDTileRowsBottom(config->GetUIntValue("HUDTileRowsBottom"))
{
	AnimationAssetManager->MakeSingleSpriteRectFrame("Cherry", CherryRect);
}

void TiledWorld::LoadLevel(const LevelLoadData* level)
{
	LevelLoaded = *level;
	if (!bLevelLoaded)
	{
		bLevelLoaded = true;
	}
	else
	{
		assert(ActiveLevel.get());
		ActiveLevel.reset();
	}
	const std::vector<LevelConfigData>& levels = LevelLoaded.Source == LevelSource::ArcadeLevels ?  Config->GetLevelsConfigData() : Config->GetMapMakerLevelsConfigData();
	const LevelConfigData& selectedLevel = levels[level->LevelIndex];
	ActiveLevelWidth = selectedLevel.NumCols;
	ActiveLevelHeight = selectedLevel.NumRows;
	ActiveLevel = std::make_unique<u8[]>(selectedLevel.TileData.size());
	memcpy(ActiveLevel.get(), selectedLevel.TileData.data(), selectedLevel.TileData.size());
	LineSegments = std::make_unique<CellCollisionLines[]>(selectedLevel.TileData.size());

	CurrentTileset = selectedLevel.BackgroundTileset;
	ActiveLevelTileset = BackgroundTileAssetManager->GetLevelTileset(selectedLevel.BackgroundTileset);
	for (int y = 0; y < ActiveLevelHeight; y++)
	{
		for (int x = 0; x < ActiveLevelWidth; x++)
		{
			ivec2 coords = { x, y };
			PopulateTileLines(coords);
		}
	}
	
}

ivec2 TiledWorld::GetRequiredBaseWindowSize() const
{
	assert(bLevelLoaded);
	const BackgroundTileConfigData& backgroundConfig = Config->GetBackgroundConfigData();
	return { (u32)backgroundConfig.TileSize * ActiveLevelWidth,  (u32)backgroundConfig.TileSize * ActiveLevelHeight };
}

void TiledWorld::DrawActiveLevel(SDL_Surface* window, float scale) const
{
	SDL_Surface* surface = BackgroundTileAssetManager->GetSurface();
	SDL_Surface* animationManagerSurface = AnimationAssetManager->GetAnimationsSpriteSheetSurface();
	SDL_Rect dst;
	
	dst.w = TileSize * scale;
	dst.h = TileSize * scale;
	for (int y = 1; y < ActiveLevelHeight-1; y++)
	{
		for (int x = 0; x < ActiveLevelWidth; x++)
		{
			u8 cell = ActiveLevel[y * ActiveLevelWidth + x];
			const SDL_Rect* rect = &ActiveLevelTileset[CachedCellCaseToTileIndexLUT[cell & 0x1f]]; // 0x1f is all the bits set that represent walls and the middle of the tile
			dst.x = x * TileSize * scale;
			dst.y = y * TileSize * scale;
			SDL_BlitSurfaceScaled(surface, rect, window, &dst);
			if (cell & (1 << TileCherryBit))
			{
				SDL_BlitSurfaceScaled(animationManagerSurface, &CherryRect, window, &dst);
			}
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

	PopulateTileLines(cell1);
	PopulateTileLines(cell2);
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

TiledWorld::CellCollisionLines& TiledWorld::GetCellCollisionLines(const ivec2& coords)
{
	int i = coords.y * ActiveLevelWidth + coords.x;
	return LineSegments[i];
}

const TiledWorld::CellCollisionLines& TiledWorld::GetCellCollisionLinesConst(const ivec2& coords) const
{
	int i = coords.y * ActiveLevelWidth + coords.x;
	return LineSegments[i];
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

void TiledWorld::RemoveCherryAtTile(const ivec2& coords)
{
	u8& tile = GetCellAtIndex(coords);
	tile &= 0x1f;
}

void TiledWorld::BreakTileCenter(const ivec2& coords)
{
	u8& cell = GetCellAtIndex(coords);
	cell &= ~(1 << (u32)TileWallDirectionBit::Center);
}

void TiledWorld::FillTile(const ivec2& coords)
{
	u8& cell = GetCellAtIndex(coords);
	cell |= 0x1f;
	ivec2 coordsCopy = coords; // for fuck sake
	if (coords.x - 1 >= 0)
	{
		u8& neighbourcell = GetCellAtIndex(coordsCopy + ivec2{ -1,0 });
		neighbourcell |= (1 << (u32)TileWallDirectionBit::Right);
	}
	if (coords.x < ActiveLevelWidth)
	{
		u8& neighbourcell = GetCellAtIndex(coordsCopy + ivec2{ 1,0 });
		neighbourcell |= (1 << (u32)TileWallDirectionBit::Left);
	}
	if (coords.y - 1 >= 0)
	{
		u8& neighbourcell = GetCellAtIndex(coordsCopy + ivec2{ 0,-1 });
		neighbourcell |= (1 << (u32)TileWallDirectionBit::Down);
	}
	if (coords.y + 1 < ActiveLevelHeight)
	{
		u8& neighbourcell = GetCellAtIndex(coordsCopy + ivec2{ 0,1 });
		neighbourcell |= (1 << (u32)TileWallDirectionBit::Up);
	}
	PopulateTileLines(coords);
}

void TiledWorld::AddCherry(const ivec2& coords)
{
	u8& cell = GetCellAtIndex(coords);
	cell |= (1 << TileCherryBit);
}

void TiledWorld::FreeLevel()
{
	if (ActiveLevel.get())
	{
		assert(bLevelLoaded);
		ActiveLevel.reset();
		bLevelLoaded = false;
	}
}

void TiledWorld::CopyToLevelConfigData(LevelConfigData& dst)
{
	assert(dst.NumCols == ActiveLevelWidth);
	assert(dst.NumRows == ActiveLevelHeight);
	assert(dst.TileData.size() == ActiveLevelHeight * ActiveLevelWidth);
	assert(bLevelLoaded);
	assert(CurrentTileset >= 0);
	memcpy(dst.TileData.data(), ActiveLevel.get(), ActiveLevelHeight * ActiveLevelWidth);
	dst.BackgroundTileset = CurrentTileset;
}

void TiledWorld::PopulateTileLines(ivec2 coords)
{
	CellCollisionLines& l = GetCellCollisionLines(coords);
	u8 cell = GetCellAtIndexValue(coords);
	size_t index = coords.y * ActiveLevelWidth + coords.x;
	if ((u8)(1 << (int)TileWallDirectionBit::Up) & cell)
	{
		l.lines[(int)TileWallDirectionBit::Up].first  = vec2{ (float)coords.x * TileSize,            (float)coords.y * TileSize };
		l.lines[(int)TileWallDirectionBit::Up].second = vec2{ (float)coords.x * TileSize + TileSize, (float)coords.y * TileSize };
	}
	if ((u8)(1 << (int)TileWallDirectionBit::Down) & cell)
	{
		l.lines[(int)TileWallDirectionBit::Down].first  = vec2{ (float)coords.x * TileSize,            (float)coords.y * TileSize + TileSize };
		l.lines[(int)TileWallDirectionBit::Down].second = vec2{ (float)coords.x * TileSize + TileSize, (float)coords.y * TileSize + TileSize };
	}
	if ((u8)(1 << (int)TileWallDirectionBit::Left) & cell)
	{
		l.lines[(int)TileWallDirectionBit::Left].first  = vec2{ (float)coords.x * TileSize,            (float)coords.y * TileSize };
		l.lines[(int)TileWallDirectionBit::Left].second = vec2{ (float)coords.x * TileSize,            (float)coords.y * TileSize + TileSize    };
	}
	if ((u8)(1 << (int)TileWallDirectionBit::Right) & cell)
	{
		l.lines[(int)TileWallDirectionBit::Right].first  = vec2{ (float)coords.x * TileSize + TileSize, (float)coords.y * TileSize };
		l.lines[(int)TileWallDirectionBit::Right].second = vec2{ (float)coords.x * TileSize + TileSize, (float)coords.y * TileSize + TileSize };
	}

}

std::pair<long, long> FloatPairToLongPair(const vec2& p)
{
	return std::pair<long, long>(std::round(p.x * 1e5), std::round(p.y * 1e5));
}

std::pair<std::pair<long, long>, std::pair<long, long>> LLineFromVec2Line(const std::pair<vec2, vec2>& l)
{
	return {
		FloatPairToLongPair(l.first),
		FloatPairToLongPair(l.second)
	};
}

void TiledWorld::GetLinesFromCells(std::vector<std::pair<vec2, vec2>>& outLines, const std::vector<ivec2>& inCellCoords) const
{
	std::set<std::pair<std::pair<long, long>, std::pair<long, long>>> encountered;
	for (const ivec2& coord : inCellCoords)
	{
		const CellCollisionLines& lines = GetCellCollisionLinesConst(coord);
		u8 cellVal = GetCellAtIndexValue(coord);
		for (int i = (int)TileWallDirectionBit::Up; i < (int)TileWallDirectionBit::Center; i++)
		{
			if (cellVal & (1 << i))
			{
				std::pair<vec2,vec2> p = lines.lines[i];
				std::pair<std::pair<long, long>, std::pair<long, long>> lLine = LLineFromVec2Line(p);
				if (encountered.find(lLine) == encountered.end())
				{
					outLines.push_back(p);
					encountered.insert(lLine);
				}
			}
		}
	}
}

bool TiledWorld::IsCherryAtTile(const ivec2& coords) const
{
	u8 tile = GetCellAtIndexValue(coords);
	return tile & (1 << TileCherryBit);
}
