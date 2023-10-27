#pragma once
#include <memory>
#include "GameFramework.h"
#include "CommonTypedefs.h"
#include "InputManager.h"

#define NumberOfMapMakerLevelsToShowAtOnce 4

class TextRenderer;
class IConfigFile;

class MapMakerLevelSelectLayer :
	public UpdateableLayerBase,
	public DrawableLayerBase,
	public RecieveInputLayerBase
{
public:
	MapMakerLevelSelectLayer(
		const std::shared_ptr<TextRenderer>& textRenderer,
		const std::shared_ptr<IConfigFile>& configFile,
		u32 screenWidth,
		u32 screenHeight);

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
	void OnUpPress();
	void OnDownPress();
	void OnSelectPress();

private:
	const std::shared_ptr<TextRenderer> TextRendererPtr;
	const u32 ScreenWidth;
	const u32 ScreenHeight;
	const std::shared_ptr<IConfigFile> ConfigFile;
	i32 IndexInOnscreenSelection = 0;
	i32 CurrentOnScreenSelection[NumberOfMapMakerLevelsToShowAtOnce];
	u32 CurrentNumberOnScreen = 1;
	i32 FirstIndexInCurrent = -1;
	GameInputState LastFrameInput = { false,false,false,false,false,false };
};