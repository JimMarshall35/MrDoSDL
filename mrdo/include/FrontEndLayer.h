#pragma once
#include "GameFramework.h"
#include <memory>
#include <string>
#include "CommonTypedefs.h"
#include "InputManager.h"
#include "LevelLoadData.h"

#define FrontEndPlayOptionIndex 0
#define FrontEndMapMakerOptionIndex 1

class TextRenderer;

class FrontEndLayer :
	public UpdateableLayerBase,
	public DrawableLayerBase,
	public RecieveInputLayerBase
{
private:
	struct MenuOption
	{
		std::string Name;
		std::string LayerToPushName;
		GameLayerType LayerTypesToPushName;
		void* PushLayerData;
	};
public:

	FrontEndLayer(
		const std::shared_ptr<TextRenderer>& textRenderer,
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
	std::shared_ptr<TextRenderer> TextRendererPtr;
	const u32 ScreenWidth;
	const u32 ScreenHeight;
	i32 SelectedMenuIndex;
	LevelLoadData LevelLoad = { LevelSource::ArcadeLevels, 0 };

	MenuOption MenuOptions[2] =
	{
		{"Play", "Game", GameLayerType::Draw | GameLayerType::Input | GameLayerType::Update, &LevelLoad},
		{"MapMaker", "MapMakerLevelSelectLayer", GameLayerType::Draw | GameLayerType::Input | GameLayerType::Update, 0}
	};
	u32 NumberOfOptions = 2;
	GameInputState LastFrameInput = {false,false,false,false,false,false};
	bool bBlinkState = true;
};