#include "MapMakerLayer.h"
#include "IAnimationAssetManager.h"
#include "IConfigFile.h"
#include "InputManager.h"
#include "TextRenderer.h"
#include <string>

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
	const LevelConfigData& lvlConfig = ConfigFile->GetMapMakerLevelsConfigData()[level->LevelIndex];
	LevelDims = ivec2{ lvlConfig.NumCols, lvlConfig.NumRows };
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
	dst.w = BackgroundTileSize * scale;
	dst.h = BackgroundTileSize * scale;
	dst.x = CursorPos.x * BackgroundTileSize * scale;
	dst.y = CursorPos.y * BackgroundTileSize * scale;

	SDL_Surface* srcSurface = AnimationAssetManager->GetAnimationsSpriteSheetSurface();
	SDL_BlitSurfaceScaled(srcSurface, &CursorSprite, windowSurface, &dst);
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
	if (input.UpPress())
	{
		--CursorPos.y;
		if (CursorPos.y == 0)
		{
			CursorPos.y = 1;
		}
		else if (CurrentTool == MM_BreakTile && input.CrystalBall)
		{
			MMTiledWorld.ConnectAdjacentCells(previousCursorPos, CursorPos);
		}
	}
	if (input.DownPress())
	{
		++CursorPos.y;
		if (CursorPos.y == LevelDims.y - 1)
		{
			CursorPos.y = LevelDims.y - 2;
		}
		else if (CurrentTool == MM_BreakTile && input.CrystalBall)
		{
			MMTiledWorld.ConnectAdjacentCells(previousCursorPos, CursorPos);
		}
	}
	if (input.LeftPress())
	{
		--CursorPos.x;
		if (CursorPos.x < 0)
		{
			CursorPos.x = 0;
		}
		else if (CurrentTool == MM_BreakTile && input.CrystalBall)
		{
			MMTiledWorld.ConnectAdjacentCells(previousCursorPos, CursorPos);
		}
	}
	if (input.RightPress())
	{
		++CursorPos.x;
		if (CursorPos.x == LevelDims.x)
		{
			CursorPos.x = LevelDims.x - 1;
		}
		else if (CurrentTool == MM_BreakTile && input.CrystalBall)
		{
			MMTiledWorld.ConnectAdjacentCells(previousCursorPos, CursorPos);
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
