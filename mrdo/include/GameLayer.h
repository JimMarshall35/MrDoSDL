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

class IFileSystem;
class IBackgroundTileAssetManager;
class IAnimationAssetManager;
class TiledWorld;
class TextRenderer;
class IConfigFile;

class Game : 
	public UpdateableLayerBase, 
	public DrawableLayerBase, 
	public RecieveInputLayerBase,
	public GameFrameworkMessageRecipientBase<CharacterDied>
{
private:
	enum class GamePhase
	{
		Playing,
		DieAnimationPlaying
	};
public:
	Game(
		const std::shared_ptr<IFileSystem>& fileSystem, 
		const std::shared_ptr<IConfigFile>& config,
		const std::shared_ptr<IBackgroundTileAssetManager>& backgroundAssetManager,
		const std::shared_ptr<IAnimationAssetManager>& animationManager,
		const std::shared_ptr<TextRenderer>& textRenderer);

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
	const std::shared_ptr<TextRenderer> CachedTextRenderer;
};