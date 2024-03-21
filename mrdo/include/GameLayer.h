#pragma once
#include <memory>
#include "Character.h"
#include "AppleManager.h"
#include "GameFramework.h"
#include <string>
#include "InputManager.h"
#include "VectorTypes.h"
#include "Event.h"
#include "GameFrameworkMessages.h"
#include "LevelLoadData.h"
#include "EnemyManager.h"
#include "GameState.h"
#ifdef ReplayValidator
#include <functional>
#endif

class IFileSystem;
class IBackgroundTileAssetManager;
class IAnimationAssetManager;
class TiledWorld;
class TextRenderer;
class IConfigFile;
class IBackendClient;
class InputManager;
class IRNG;

class Game : 
	public UpdateableLayerBase, 
	public DrawableLayerBase, 
	public RecieveInputLayerBase,
	public GameFrameworkMessageRecipientBase<CharacterDied>,
	public GameFrameworkMessageRecipientBase<Victory>,
	public GameFrameworkMessageRecipientBase<GameOver>

{
private:
	enum class GamePhase
	{
		Playing,
		DieAnimationPlaying
	};
public:
#ifdef ReplayValidator
	Game(const std::shared_ptr<IFileSystem>& fileSystem,
		const std::shared_ptr<IConfigFile>& config,
		const std::shared_ptr<IBackgroundTileAssetManager>& backgroundAssetManager,
		const std::shared_ptr<IAnimationAssetManager>& animationManager,
		InputManager* inputManager,
		std::function<void(void)>& gameOverCallback,
		const std::shared_ptr<IRNG>& rng);
#else
	Game(
		const std::shared_ptr<IFileSystem>& fileSystem,
		const std::shared_ptr<IConfigFile>& config,
		const std::shared_ptr<IBackgroundTileAssetManager>& backgroundAssetManager,
		const std::shared_ptr<IAnimationAssetManager>& animationManager,
		const std::shared_ptr<TextRenderer>& textRenderer,
		const std::shared_ptr<IBackendClient>& backendClient,
		InputManager* inputManager,
		const std::shared_ptr<IRNG>& rng);
#endif
	

	// Inherited via UpdateableLayerBase
	virtual void Update(float deltaT) override;
	virtual bool MasksPreviousUpdateableLayer() const override;
	virtual const std::string& GetUpdateableLayerName() const override;
	virtual void OnUpdatePush(void* data) override;
	virtual void OnUpdatePop() override;

	// Inherited via DrawableLayerBase
	virtual void Draw(SDL_Surface* windowSurface, float scale) const override;
	virtual bool MasksPreviousDrawableLayer() const override;
	virtual const std::string& GetDrawableLayerName() const override;
	virtual void OnDrawablePush(void* data) override;
	virtual void OnDrawablePop() override;

	// Inherited via RecieveInputLayerBase
	virtual void ReceiveInput(const GameInputState& input) override;
	virtual bool MasksPreviousInputLayer() const override;
	virtual const std::string& GetInputLayerName() const override;
	virtual void OnInputPush(void* data) override;
	virtual void OnInputPop() override;

	// Inherited via GameFrameworkMessageRecipientBase
	virtual void RecieveMessage(const CharacterDied& message) override;
	virtual void RecieveMessage(const Victory& message) override;
	virtual void RecieveMessage(const GameOver& message) override;

	void SpawnExtraMan(const ivec2& location, int c);

private:
	Event<LevelLoadData> NewLevelBegun;
	Event<LevelLoadData> ResetAfterDeath;
	std::shared_ptr<TiledWorld> MyTiledWorld; // can't name them the same as the class name hence the stupid "My" prefix
	Character MyCharacter;
	AppleManager MyAppleManager;
	EnemyManager MyEnemyManager;
	static std::string LayerName;
	GameInputState InputState = { false, false, false, false, true, false };
	GamePhase Phase;
	LevelLoadData CurrentLevel = { LevelSource::Undefined, -1};

	GameState MyGameState;
	const std::shared_ptr<IConfigFile> Config;
#ifndef ReplayValidator
	const std::shared_ptr<TextRenderer> CachedTextRenderer;
	const std::shared_ptr<IBackendClient> BackendClient;
#else
	std::function<void(void)> OnGameOverCallback;
public:
	const GameState& GetGamestate() { return MyGameState; }
private:
#endif

	InputManager* MyInputManager;
};