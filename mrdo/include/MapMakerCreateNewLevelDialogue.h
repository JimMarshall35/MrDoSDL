#pragma once
#include <memory>
#include "GameFramework.h"
#include "CommonTypedefs.h"

#define MapMakerNameMaxLen 8
#define CancelSelection (MapMakerNameMaxLen - 1)
#define SaveSelection (MapMakerNameMaxLen)

class IConfigFile;
class TextRenderer;

class MapMakerCreateNewLevelDialogue
	:public DrawableLayerBase,
	public RecieveInputLayerBase
{
public:
	MapMakerCreateNewLevelDialogue(const std::shared_ptr<TextRenderer>& textRenderer, const std::shared_ptr<IConfigFile>& config, u32 screenWidth, u32 screenHeight);

	// Inherited via RecieveInputLayerBase
	virtual void ReceiveInput(const GameInputState& input) override;
	virtual bool MasksPreviousInputLayer() const override;
	virtual const std::string& GetInputLayerName() const override;
	virtual void OnInputPush(void* data) override;
	virtual void OnInputPop() override;

	// Inherited via DrawableLayerBase
	virtual void Draw(SDL_Surface* windowSurface, float scale) const override;
	virtual bool MasksPreviousDrawableLayer() const override;
	virtual const std::string& GetDrawableLayerName() const override;
	virtual void OnDrawablePush(void* data) override;
	virtual void OnDrawablePop() override;
private:
	std::shared_ptr<IConfigFile> ConfigFile;
	std::shared_ptr<TextRenderer> CachedTextRenderer;
	const u32 ScreenWidth;
	const u32 ScreenHeight;
	char MapName[MapMakerNameMaxLen];
	i32 CurrentSelection = 0;
};