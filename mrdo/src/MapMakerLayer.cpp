#include "MapMakerLayer.h"
#include "IAnimationAssetManager.h"
#include "IConfigFile.h"
#include "InputManager.h"
#include "TextRenderer.h"
#include <string>
#include <algorithm>
#include <cassert>

struct LevelLoadData;
static const std::string sLayerName = "MapMaker";

const char* MapMakerLayer::ToolNames[MM_NumTools] = 
{
	"Break tile",
	"Fill tile",
	"Add cherry",
	"Add apple",
	"Add playerStart",
	"Add monsterspawer"
};

MapMakerLayer::MapMakerLayer(const std::shared_ptr<IConfigFile>& config, 
	const std::shared_ptr<IBackgroundTileAssetManager>& bgtam,
	const std::shared_ptr<IAnimationAssetManager>& aam,
	const std::shared_ptr<TextRenderer>& textRenderer)
	:ConfigFile(config),
	AnimationAssetManager(aam),
	CachedTextRenderer(textRenderer),
	MMTiledWorld(config, bgtam, aam)
{
	aam->MakeSingleSpriteRectFrame("MapMakerCursor", CursorSprite);
	aam->MakeSingleSpriteRectFrame("MonsterSpawner", MonsterSpawnerSprite);
	aam->MakeSingleSpriteRectFrame("Apple", AppleSprite);
	aam->MakeSingleSpriteRectFrame("MMPlayerStartIcon", PlayerStartSprite);
	BackgroundTileSize = config->GetBackgroundConfigData().TileSize;
}

void MapMakerLayer::Update(float deltaT)
{
}

bool MapMakerLayer::MasksPreviousUpdateableLayer() const
{
	return true;
}

const std::string& MapMakerLayer::GetUpdateableLayerName() const
{
	return sLayerName;
}

void MapMakerLayer::OnUpdatePush(void* data)
{
	const LevelLoadData* level = (const LevelLoadData*)data;
	MMLevelLoadData = *level;
	MMTiledWorld.LoadLevel(level);
	LevelConfigData& lvlConfig = ConfigFile->GetMapMakerLevelsConfigData()[level->LevelIndex];
	LevelDims = ivec2{ lvlConfig.NumCols, lvlConfig.NumRows };
	EditedLevel = &lvlConfig;
}

void MapMakerLayer::OnUpdatePop()
{
}

void MapMakerLayer::Draw(SDL_Surface* windowSurface, float scale) const
{
	CachedTextRenderer->SetCurrentFont("White_BlackBackground");
	CachedTextRenderer->RenderText({ 0,0 }, ToolNames[CurrentTool], windowSurface, scale);
	MMTiledWorld.DrawActiveLevel(windowSurface, scale);
	SDL_Rect dst;
	SDL_Surface* srcSurface = AnimationAssetManager->GetAnimationsSpriteSheetSurface();
	dst.w = BackgroundTileSize * scale;
	dst.h = BackgroundTileSize * scale;
	dst.x = CursorPos.x * BackgroundTileSize * scale;
	dst.y = CursorPos.y * BackgroundTileSize * scale;
	SDL_BlitSurfaceScaled(srcSurface, &CursorSprite, windowSurface, &dst);

	dst.x = EditedLevel->PlayerSpawnLocation.x * BackgroundTileSize * scale;
	dst.y = EditedLevel->PlayerSpawnLocation.y * BackgroundTileSize * scale;
	SDL_BlitSurfaceScaled(srcSurface, &PlayerStartSprite, windowSurface, &dst);

	for (const uvec2& apple : EditedLevel->Apples)
	{
		dst.x = apple.x * BackgroundTileSize * scale;
		dst.y = apple.y * BackgroundTileSize * scale;
		SDL_BlitSurfaceScaled(srcSurface, &AppleSprite, windowSurface, &dst);
	}

	for (const MonsterSpawnerData& spawner : EditedLevel->MonsterSpawners)
	{
		dst.x = spawner.TilePosition.x * BackgroundTileSize * scale;
		dst.y = spawner.TilePosition.y * BackgroundTileSize * scale;
		SDL_BlitSurfaceScaled(srcSurface, &AppleSprite, windowSurface, &dst);
	}
}

bool MapMakerLayer::MasksPreviousDrawableLayer() const
{
	return true;
}

const std::string& MapMakerLayer::GetDrawableLayerName() const
{
	return sLayerName;
}

void MapMakerLayer::OnDrawablePush(void* data)
{
}

void MapMakerLayer::OnDrawablePop()
{
}

void MapMakerLayer::ReceiveInput(const GameInputState& input)
{
	ivec2 previousCursorPos = CursorPos;
	bool hasMoved = false;
	if (input.UpPress())
	{
		--CursorPos.y;
		if (CursorPos.y == 0)
		{
			CursorPos.y = 1;
		}
		else
		{
			hasMoved = true;
		}
	}
	if (input.DownPress())
	{
		++CursorPos.y;
		if (CursorPos.y == LevelDims.y - 1)
		{
			CursorPos.y = LevelDims.y - 2;
		}
		else 
		{
			hasMoved = true;
		}
	}
	if (input.LeftPress())
	{
		--CursorPos.x;
		if (CursorPos.x < 0)
		{
			CursorPos.x = 0;
		}
		else
		{
			hasMoved = true;
		}
	}
	if (input.RightPress())
	{
		++CursorPos.x;
		if (CursorPos.x == LevelDims.x)
		{
			CursorPos.x = LevelDims.x - 1;
		}
		else
		{
			hasMoved = true;
		}
	}
	if (input.MapMakerChangeToolPress())
	{
		++CurrentTool;
		if (CurrentTool == MM_NumTools)
		{
			CurrentTool = 0;
		}
	}
	if (input.CrystalBallPress())
	{
		switch (CurrentTool)
		{
		case MM_BreakTile:
			MMTiledWorld.BreakTileCenter(CursorPos);
			break;
		case MM_FillTile:
			MMTiledWorld.FillTile(CursorPos);
			break;
		case MM_AddCherry:
			if (MMTiledWorld.IsCherryAtTile(CursorPos))
			{
				MMTiledWorld.RemoveCherryAtTile(CursorPos);
			}
			else
			{
				MMTiledWorld.AddCherry(CursorPos);
			}
			break;
		case MM_AddPlayerStart:
			EditedLevel->PlayerSpawnLocation.x = CursorPos.x;
			EditedLevel->PlayerSpawnLocation.y = CursorPos.y;
			break;
		case MM_AddApple:
			if (AppleAtCoords(CursorPos))
			{
				RemoveApple(CursorPos);
			}
			else
			{
				AddApple(CursorPos);
			}
			break;
		}
	}

	if (hasMoved && input.CrystalBall)
	{
		// moved cursor while holding down crystal ball key
		switch (CurrentTool)
		{
		case MM_BreakTile:
			MMTiledWorld.ConnectAdjacentCells(previousCursorPos, CursorPos);
			break;
		case MM_FillTile:
			MMTiledWorld.FillTile(CursorPos);
			break;
		case MM_AddCherry:
			if (MMTiledWorld.IsCherryAtTile(CursorPos))
			{
				MMTiledWorld.RemoveCherryAtTile(CursorPos);
			}
			else
			{
				MMTiledWorld.AddCherry(CursorPos);
			}
			break;
		}
	}
}

bool MapMakerLayer::MasksPreviousInputLayer() const
{
	return true;
}

const std::string& MapMakerLayer::GetInputLayerName() const
{
	return sLayerName;
}

void MapMakerLayer::OnInputPush(void* data)
{
}

void MapMakerLayer::OnInputPop()
{
}

void MapMakerLayer::AddApple(const ivec2& coords)
{
	assert(coords.x >= 0);
	assert(coords.y >= 0);
	uvec2 asUvec = uvec2{ (u32)coords.x, (u32)coords.y };
	auto itr = std::find(EditedLevel->Apples.begin(), EditedLevel->Apples.end(), asUvec);
	if (itr == EditedLevel->Apples.end())
	{
		EditedLevel->Apples.push_back(asUvec);
	}
}

void MapMakerLayer::RemoveApple(const ivec2& coords)
{
	assert(coords.x >= 0);
	assert(coords.y >= 0);
	uvec2 asUvec = uvec2{ (u32)coords.x, (u32)coords.y };
	auto itr = std::find(EditedLevel->Apples.begin(), EditedLevel->Apples.end(), asUvec);
	if (itr != EditedLevel->Apples.end())
	{
		EditedLevel->Apples.erase(itr);
	}
}

bool MapMakerLayer::AppleAtCoords(const ivec2& coords)
{
	assert(coords.x >= 0);
	assert(coords.y >= 0);
	uvec2 asUvec = uvec2{ (u32)coords.x, (u32)coords.y };
	auto itr = std::find(EditedLevel->Apples.begin(), EditedLevel->Apples.end(), asUvec);
	return itr != EditedLevel->Apples.end();
}

void MapMakerLayer::AddMonsterSpawner(const ivec2& coords)
{
	assert(coords.x >= 0);
	assert(coords.y >= 0);
	uvec2 asUvec = uvec2{ (u32)coords.x, (u32)coords.y };
	MonsterSpawnerData msData;
	msData.TilePosition = asUvec;
	auto itr = std::find(EditedLevel->MonsterSpawners.begin(), EditedLevel->MonsterSpawners.end(), msData);
	if (itr == EditedLevel->MonsterSpawners.end())
	{
		EditedLevel->MonsterSpawners.push_back(msData);
	}
}

void MapMakerLayer::RemoveMonsterSpawner(const ivec2& coords)
{
	assert(coords.x >= 0);
	assert(coords.y >= 0);
	uvec2 asUvec = uvec2{ (u32)coords.x, (u32)coords.y };
	MonsterSpawnerData msData;
	msData.TilePosition = asUvec;
	auto itr = std::find(EditedLevel->MonsterSpawners.begin(), EditedLevel->MonsterSpawners.end(), msData);
	if (itr != EditedLevel->MonsterSpawners.end())
	{
		EditedLevel->MonsterSpawners.erase(itr);
	}
}

MonsterSpawnerData* MapMakerLayer::MonsterSpawnerAtCoords(const ivec2& coords)
{
	assert(coords.x >= 0);
	assert(coords.y >= 0);
	uvec2 asUvec = uvec2{ (u32)coords.x, (u32)coords.y };
	MonsterSpawnerData msData;
	msData.TilePosition = asUvec;
	auto itr = std::find(EditedLevel->MonsterSpawners.begin(), EditedLevel->MonsterSpawners.end(), msData);
	if (itr == EditedLevel->MonsterSpawners.end())
	{
		return nullptr;
	}
	return &(*itr);
}
