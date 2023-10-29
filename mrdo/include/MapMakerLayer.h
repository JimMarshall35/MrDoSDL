#pragma once

#include <memory>
#include "GameFramework.h"
#include "TiledWorld.h"
#include "LevelLoadData.h"
#include "VectorTypes.h"
#include "SDL.h"

enum MapMakerTool
{
	MM_BreakTile,
	MM_FillTile,
	MM_AddCherry,
	MM_AddApple,
	MM_AddPlayerStart,
	MM_AddMonsterSpawner,
	MM_NumTools
};

struct LevelConfigData;
class IConfigFile;
class IAnimationAssetManager;
class TextRenderer;

class MapMakerLayer
	: public UpdateableLayerBase,
	public DrawableLayerBase,
	public RecieveInputLayerBase
{
public:
	MapMakerLayer(
		const std::shared_ptr<IConfigFile>& config, 
		const std::shared_ptr<IBackgroundTileAssetManager>& bgtam,
		const std::shared_ptr<IAnimationAssetManager>& aam,
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

private:
	std::shared_ptr<IConfigFile> ConfigFile;
	std::shared_ptr<IAnimationAssetManager> AnimationAssetManager;
	std::shared_ptr<TextRenderer> CachedTextRenderer;
	TiledWorld MMTiledWorld;
	LevelConfigData* EditedLevel;
	LevelLoadData MMLevelLoadData;
	ivec2 CursorPos = { 1,1 };
	SDL_Rect CursorSprite;
	int BackgroundTileSize;
	ivec2 LevelDims = { 0,0 };
	int CurrentTool = MM_BreakTile;
	static const char* ToolNames[MM_NumTools];
};