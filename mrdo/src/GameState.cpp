#include "GameState.h"
#include "Event.h"
#include <cassert>
#include "IConfigFile.h"
#include "TiledWorld.h"
#include "TextRenderer.h"
#include <stdio.h>
#include "IAnimationAssetManager.h"
#include "GameLayer.h"

#ifdef ReplayValidator

GameState::GameState(const std::shared_ptr<IConfigFile>& configFile, Event<LevelLoadData>& NewLevelBegun, Event<LevelLoadData>& ResetAfterDeath, Game* g)
	: Score(0),
	Lives(0),
	LOnLevelLoad(this),
	LOnResetAfterDeath(this),
	Config(configFile),
	CherryPoints(configFile->GetUIntValue("CherryPoints")),
	EightCherryBonusPoints(configFile->GetUIntValue("EightCherryBonusPoints")),
	CrystalBallKillPoints(configFile->GetUIntValue("CrystalBallKillPoints")),
	BonusDiamondPoints(configFile->GetUIntValue("BonusDiamondPoints")),
	StartBonusTreetPoints(configFile->GetUIntValue("StartBonusTreetPoints")),
	BonusTreetIncrement(configFile->GetUIntValue("BonusTreetIncrement")),
	BonusTreetMax(configFile->GetUIntValue("BonusTreetMax")),
	pGame(g)
{
	NewLevelBegun += &LOnLevelLoad;
	ResetAfterDeath += &LOnResetAfterDeath;
	int size = configFile->GetArraySize("AppleKillPointsTable");
	for (int i = 0; i < size; i++)
	{
		AppleKillPointsTable.push_back(configFile->GetIntArrayValue("AppleKillPointsTable", i));
	}
}
#else
GameState::GameState(const std::shared_ptr<IConfigFile>& configFile,
	const std::shared_ptr<TextRenderer>& textRenderer,
	const std::shared_ptr<IAnimationAssetManager>& animAssetManager,
	Event<LevelLoadData>& NewLevelBegun,
	Event<LevelLoadData>& ResetAfterDeath,
	Game* g)
	: Score(0),
	Lives(0),
	LOnLevelLoad(this),
	LOnResetAfterDeath(this),
	Config(configFile),
	MyTextRenderer(textRenderer),
	CherryPoints(configFile->GetUIntValue("CherryPoints")),
	EightCherryBonusPoints(configFile->GetUIntValue("EightCherryBonusPoints")),
	CrystalBallKillPoints(configFile->GetUIntValue("CrystalBallKillPoints")),
	BonusDiamondPoints(configFile->GetUIntValue("BonusDiamondPoints")),
	StartBonusTreetPoints(configFile->GetUIntValue("StartBonusTreetPoints")),
	BonusTreetIncrement(configFile->GetUIntValue("BonusTreetIncrement")),
	BonusTreetMax(configFile->GetUIntValue("BonusTreetMax")),
	MyExtraState(animAssetManager, configFile, textRenderer),
	pGame(g)

{
	NewLevelBegun += &LOnLevelLoad;
	ResetAfterDeath += &LOnResetAfterDeath;
	int size = configFile->GetArraySize("AppleKillPointsTable");
	for (int i = 0; i < size; i++)
	{
		AppleKillPointsTable.push_back(configFile->GetIntArrayValue("AppleKillPointsTable", i));
	}
}
#endif

void GameState::RecieveMessage(const CherryEaten& message)
{
	Score += CherryPoints;
	--RemainingCherries;
	RefreshScoreBuffer();
	if (RemainingCherries <= 0)
	{
		printf("Victory: all cherries eaten.\n");
		GameFramework::SendFrameworkMessage<Victory>({ VictoryReason::Cherries });
	}
}

void GameState::RecieveMessage(const EnemyDeath& message)
{
	if (message.NumberKilledTotal == 0)
	{
		printf("message.NumberKilledTotal == 0 !!\n");
		return;
	}
	u32 scoreAddtion;
	if (message.NumberKilledTotal < AppleKillPointsTable.size())
	{
		scoreAddtion = AppleKillPointsTable[message.NumberKilledTotal - 1];
	}
	else
	{
		scoreAddtion = AppleKillPointsTable[AppleKillPointsTable.size() - 1];
	}
	Score += scoreAddtion;
	RefreshScoreBuffer();
	RemainingEnemies -= message.NumberSignificantKilled;
	printf("%i %s killed. Of which %i were significant. remaining enemies: %i\n", 
		message.NumberKilledTotal,
		message.NumberKilledTotal == 1 ? "enemy" : "enemies",
		message.NumberSignificantKilled,
		RemainingEnemies);
	if (RemainingEnemies <= 0)
	{
		printf("Victory: All monsters killed\n");
		GameFramework::SendFrameworkMessage<Victory>({VictoryReason::Monsters});
	}
}

void GameState::Draw(SDL_Surface* windowSurface, float scale) const
{
#ifndef ReplayValidator
	MyTextRenderer->RenderText({ 0,0 }, ScoreBuffer, windowSurface, scale);
	MyTextRenderer->RenderText(LivesPositionToRender, LivesBuffer, windowSurface, scale);
	MyExtraState.Draw(windowSurface, scale);
#endif
}

void GameState::Update(float deltaT)
{
	MyExtraState.Update(deltaT);
}

void GameState::OnLevelLoad(LevelLoadData level)
{
	if (level.LevelIndex == 0 || level.Source == LevelSource::MapMaker)
	{
		Lives = 3;
		Score = 0;
		RefreshLivesBuffer();
		RefreshScoreBuffer();
	}
	const std::vector<LevelConfigData>& levels = level.Source == LevelSource::ArcadeLevels ? Config->GetLevelsConfigData() : Config->GetMapMakerLevelsConfigData();
	assert(level.LevelIndex < levels.size());
	const LevelConfigData& lvl = levels[level.LevelIndex];
	RemainingCherries = GetNumCherries(lvl);
	RemainingEnemies = GetNumEnemies(lvl);
	printf("New level loaded: remaining enemies: %i, remaining cherries: %i \n", RemainingEnemies, RemainingCherries);
	LivesPositionToRender.y = lvl.NumRows * (Config->GetBackgroundConfigData().TileSize - 1);
	LivesPositionToRender.x = 0;
}

void GameState::OnResetAfterDeath(LevelLoadData levelLoadData)
{
	--Lives;
	RefreshLivesBuffer();
	if (Lives < 0)
	{
		GameFramework::SendFrameworkMessage<GameOver>({ Score });
	}
}

void GameState::RefreshScoreBuffer()
{
	sprintf_s(ScoreBuffer, ScoreBufferSize, "%i", Score);
}

void GameState::RefreshLivesBuffer()
{
	sprintf_s(LivesBuffer, LivesBufferSize, "%i", Lives);
}

int GameState::GetNumCherries(const LevelConfigData& lvl)
{
	int r = 0;
	for (u8 tile : lvl.TileData)
	{
		if (tile & (1 << TileCherryBit))
		{
			++r;
		}
	}
	return r;
}

int GameState::GetNumEnemies(const LevelConfigData& lvl)
{
	int r = 0;
	for (const MonsterSpawnerData& spawner : lvl.MonsterSpawners)
	{
		r += spawner.NumMonsters;
	}
	return r;
}

#ifdef ReplayValidator
ExtraState::ExtraState(const std::shared_ptr<IConfigFile>& configFile)
	:pConfigFile(configFile)
{
	memset(LetterStates, NotGot, sizeof(LetterState) * 5);
	LetterStates[Selected] = NotGotSelected;
	LetterChangeIntervalMs = configFile->GetUIntValue("ExtraLetterChangeInterval");
}
#else
ExtraState::ExtraState(const std::shared_ptr<IAnimationAssetManager>& animAssetManager, const std::shared_ptr<IConfigFile>& configFile, const std::shared_ptr<TextRenderer>& textRenderer)
	:pAnimAssetManager(animAssetManager), pConfigFile(configFile), pTextRenderer(textRenderer)
{
	memset(LetterStates, NotGot, sizeof(LetterState) * 5);
	LetterStates[Selected] = NotGotSelected;
	LetterChangeIntervalMs = configFile->GetUIntValue("ExtraLetterChangeInterval");

	pAnimAssetManager->MakeSingleSpriteRectFrame("E_Dim", ExtraMenUnlit[0]);
	pAnimAssetManager->MakeSingleSpriteRectFrame("X_Dim", ExtraMenUnlit[1]);
	pAnimAssetManager->MakeSingleSpriteRectFrame("T_Dim", ExtraMenUnlit[2]);
	pAnimAssetManager->MakeSingleSpriteRectFrame("R_Dim", ExtraMenUnlit[3]);
	pAnimAssetManager->MakeSingleSpriteRectFrame("A_Dim", ExtraMenUnlit[4]);

	pAnimAssetManager->MakeSingleSpriteRectFrame("E_Lit", ExtraMenLit[0]);
	pAnimAssetManager->MakeSingleSpriteRectFrame("X_Lit", ExtraMenLit[1]);
	pAnimAssetManager->MakeSingleSpriteRectFrame("T_Lit", ExtraMenLit[2]);
	pAnimAssetManager->MakeSingleSpriteRectFrame("R_Lit", ExtraMenLit[3]);
	pAnimAssetManager->MakeSingleSpriteRectFrame("A_Lit", ExtraMenLit[4]);
}
#endif

void ExtraState::AdvanceSelection()
{
	switch (LetterStates[Selected])
	{
	case GotSelected:
		LetterStates[Selected] = Got;
		break;
	case NotGotSelected:
		LetterStates[Selected] = NotGot;
		break;
	default:
		assert(false);
		break;
	}
	++Selected;
	if (Selected >= 5)
	{
		Selected = 0;
	}
	switch (LetterStates[Selected])
	{
	case Got:
		LetterStates[Selected] = GotSelected;
		break;
	case NotGot:
		LetterStates[Selected] = NotGotSelected;
		break;
	}
}

void ExtraState::Draw(SDL_Surface* windowSurface, float scale) const
{
#ifndef ReplayValidator
	vec2 cursor = { 8.0f * pTextRenderer->GetTextTileSize(), 0 * pTextRenderer->GetTextTileSize() };
	const BackgroundTileConfigData& backgroundConfig = pConfigFile->GetBackgroundConfigData();
	int ts = backgroundConfig.TileSize;
	for (int i = 0; i < 5; i++)
	{
		switch (LetterStates[i])
		{
		case Got:
			pTextRenderer->SetCurrentFont(LitFont);
			pTextRenderer->RenderText(cursor + vec2{ pTextRenderer->GetTextTileSize() / 2.0f, pTextRenderer->GetTextTileSize() / 2.0f}, LettersS[i], windowSurface, scale);
			break;
		case NotGot:
			pTextRenderer->SetCurrentFont(UnlitFont);
			pTextRenderer->RenderText(cursor + vec2{ pTextRenderer->GetTextTileSize() / 2.0f, pTextRenderer->GetTextTileSize() / 2.0f },  LettersS[i], windowSurface, scale);
			break;
		case GotSelected:
		{
			SDL_Surface* animationManagerSurface = pAnimAssetManager->GetAnimationsSpriteSheetSurface();
			SDL_Rect dst;

			dst.w = ts * scale;
			dst.h = ts * scale;
			dst.x = cursor.x * scale;
			dst.y = cursor.y * scale;
			const SDL_Rect* rect = &ExtraMenLit[i];
			SDL_BlitSurfaceScaled(animationManagerSurface, rect, windowSurface, &dst);

		}
			break;
		case NotGotSelected:
		{
			SDL_Surface* animationManagerSurface = pAnimAssetManager->GetAnimationsSpriteSheetSurface();
			SDL_Rect dst;

			dst.w = ts * scale;
			dst.h = ts * scale;
			dst.x = cursor.x * scale;
			dst.y = cursor.y * scale;
			const SDL_Rect* rect = &ExtraMenUnlit[i];
			SDL_BlitSurfaceScaled(animationManagerSurface, rect, windowSurface, &dst);
		}
			break;
		default:
			assert(false);
			break;
		}
		cursor.x += ts;
	}
	pTextRenderer->SetCurrentFont("White_BlackBackground");
#endif
}

void ExtraState::ObtainLetter(char letter)
{
}

void ExtraState::Update(float deltaT)
{
	Timer += deltaT;
	if (Timer >= LetterChangeIntervalMs)
	{
		AdvanceSelection();
		Timer = 0;
	}
}
