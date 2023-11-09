#pragma once
#include <memory>
#include "LevelLoadData.h"
#include "EventListener.h"
#include "GameFramework.h"
#include "GameFrameworkMessages.h"
#include <vector>

#define ScoreBufferSize 12
#define LivesBufferSize 4

class IConfigFile;
struct LevelConfigData;
struct SDL_Surface;

template<typename T>
class Event;
class TextRenderer;

class GameState : public GameFrameworkMessageRecipientBase<CherryEaten>,
	GameFrameworkMessageRecipientBase<EnemyDeath>
{
public:
	GameState(const std::shared_ptr<IConfigFile>& configFile,
		const std::shared_ptr<TextRenderer>& textRenderer,
		Event<LevelLoadData>& NewLevelBegun,
		Event<LevelLoadData>& ResetAfterDeath);
	virtual void RecieveMessage(const CherryEaten& message) override;
	virtual void RecieveMessage(const EnemyDeath& message) override;
	void Draw(SDL_Surface* windowSurface, float scale) const;

private:
	void OnLevelLoad(LevelLoadData gameState);
	void OnResetAfterDeath(LevelLoadData levelLoadData);
	void RefreshScoreBuffer();
	void RefreshLivesBuffer();

private:
	static int GetNumCherries(const LevelConfigData& lvl);
	static int GetNumEnemies(const LevelConfigData& lvl);

private:
	u32 Score;
	i32 Lives;
	i32 RemainingCherries = 0;
	i32 RemainingEnemies = 0;


	std::shared_ptr<IConfigFile> Config;
	std::shared_ptr<TextRenderer> MyTextRenderer;
	u32 CherryPoints;
	u32 EightCherryBonusPoints;
	u32 CrystalBallKillPoints;
	std::vector<u32> AppleKillPointsTable;
	u32 BonusDiamondPoints;
	u32 StartBonusTreetPoints;
	u32 BonusTreetIncrement;
	u32 BonusTreetMax;
	char ScoreBuffer[ScoreBufferSize];
	char LivesBuffer[LivesBufferSize];
	vec2 LivesPositionToRender = { 0,0 };
private:
	LISTENER(GameState, OnLevelLoad, LevelLoadData);
	LISTENER(GameState, OnResetAfterDeath, LevelLoadData);
};