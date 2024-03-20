#pragma once
#include <memory>
#include "LevelLoadData.h"
#include "EventListener.h"
#include "GameFramework.h"
#include "GameFrameworkMessages.h"
#include "SDL.h"

#include <vector>

#define ScoreBufferSize 12
#define LivesBufferSize 4

class IConfigFile;
struct LevelConfigData;
struct SDL_Surface;

template<typename T>
class Event;
class TextRenderer;
class IAnimationAssetManager;
class Game;

class ExtraState
{
public:
	enum LetterState
	{
		NotGot,
		Got,
		NotGotSelected,
		GotSelected,
		InPlay
	};
	
#ifdef ReplayValidator
	ExtraState(const std::shared_ptr<IConfigFile>& configFile);
#else
	ExtraState(const std::shared_ptr<IAnimationAssetManager>& animAssetManager,
		const std::shared_ptr<IConfigFile>& configFile,
		const std::shared_ptr<TextRenderer>& textRenderer);
#endif
	void AdvanceSelection();
	void Draw(SDL_Surface* windowSurface, float scale) const;
	void ObtainLetter(char letter);
	void Update(float deltaT);
private:
	SDL_Rect ExtraMenLit[5];
	SDL_Rect ExtraMenUnlit[5];
	const char* UnlitFont = "Grey_BlackBackground";
	const char* LitFont = "Yellow_BlackBackground";
	char Letters[5]{ 'e','x','t','r','a' };
	char* LettersS[5]{ "e","x","t","r","a" };

	LetterState LetterStates[5];
	int Selected = 0;
	std::shared_ptr<IConfigFile> pConfigFile;
	float Timer = 0.0f;
	int LetterChangeIntervalMs;
#ifndef ReplayValidator
	std::shared_ptr<IAnimationAssetManager> pAnimAssetManager;
	std::shared_ptr<TextRenderer> pTextRenderer;
#endif
};

class GameState : public GameFrameworkMessageRecipientBase<CherryEaten>,
	GameFrameworkMessageRecipientBase<EnemyDeath>
{
public:
#ifdef ReplayValidator
	GameState(const std::shared_ptr<IConfigFile>& configFile,
		Event<LevelLoadData>& NewLevelBegun,
		Event<LevelLoadData>& ResetAfterDeath, 
		Game* Game);
	u32 GetScore() const { return Score; }
#else
	GameState(const std::shared_ptr<IConfigFile>& configFile,
		const std::shared_ptr<TextRenderer>& textRenderer,
		const std::shared_ptr<IAnimationAssetManager>& animAssetManager,
		Event<LevelLoadData>& NewLevelBegun,
		Event<LevelLoadData>& ResetAfterDeath,
		Game* Game);
#endif
	virtual void RecieveMessage(const CherryEaten& message) override;
	virtual void RecieveMessage(const EnemyDeath& message) override;
	void Draw(SDL_Surface* windowSurface, float scale) const;
	void Update(float deltaT);

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
#ifndef ReplayValidator
	std::shared_ptr<TextRenderer> MyTextRenderer;
#endif
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
	ExtraState MyExtraState;
	Game* pGame;
private:
	LISTENER(GameState, OnLevelLoad, LevelLoadData);
	LISTENER(GameState, OnResetAfterDeath, LevelLoadData);
};