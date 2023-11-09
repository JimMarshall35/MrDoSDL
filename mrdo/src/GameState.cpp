#include "GameState.h"
#include "Event.h"
#include <cassert>
#include "IConfigFile.h"
#include "TiledWorld.h"
#include "TextRenderer.h"
#include <stdio.h>

GameState::GameState(const std::shared_ptr<IConfigFile>& configFile,
	const std::shared_ptr<TextRenderer>& textRenderer,
	Event<LevelLoadData>& NewLevelBegun,
	Event<LevelLoadData>& ResetAfterDeath)
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
	BonusTreetMax(configFile->GetUIntValue("BonusTreetMax"))
{
	NewLevelBegun += &LOnLevelLoad;
	ResetAfterDeath += &LOnResetAfterDeath;
	int size = configFile->GetArraySize("AppleKillPointsTable");
	for (int i = 0; i < size; i++)
	{
		AppleKillPointsTable.push_back(configFile->GetIntArrayValue("AppleKillPointsTable", i));
	}
}

void GameState::RecieveMessage(const CherryEaten& message)
{
	Score += CherryPoints;
	--RemainingCherries;
	RefreshScoreBuffer();
	if (RemainingCherries <= 0)
	{
		GameFramework::SendFrameworkMessage<Victory>({ VictoryReason::Cherries });
	}
}

void GameState::RecieveMessage(const EnemyDeath& message)
{
	u32 scoreAddtion;
	if (message.NumberKilledTotal < AppleKillPointsTable.size())
	{
		scoreAddtion = AppleKillPointsTable[message.NumberKilledTotal];
	}
	else
	{
		scoreAddtion = AppleKillPointsTable[AppleKillPointsTable.size() - 1];
	}
	Score += scoreAddtion;
	RefreshScoreBuffer();
	RemainingEnemies -= message.NumberSignificantKilled;
	if (RemainingEnemies <= 0)
	{
		GameFramework::SendFrameworkMessage<Victory>({VictoryReason::Monsters});
	}
}

void GameState::Draw(SDL_Surface* windowSurface, float scale) const
{
	MyTextRenderer->RenderText({ 0,0 }, ScoreBuffer, windowSurface, scale);
	MyTextRenderer->RenderText(LivesPositionToRender, LivesBuffer, windowSurface, scale);
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
	LivesPositionToRender.y = lvl.NumRows * Config->GetBackgroundConfigData().TileSize - 1;
	LivesPositionToRender.x = 0;
}

void GameState::OnResetAfterDeath(LevelLoadData levelLoadData)
{
	--Lives;
	RefreshLivesBuffer();
	if (Lives < 0)
	{
		GameFramework::SendFrameworkMessage<GameOver>({});
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
