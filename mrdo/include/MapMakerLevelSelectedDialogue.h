#pragma once

#include <memory>
#include "GameFramework.h"
#include "VectorTypes.h"
#include "IConfigFile.h"

#define MMOptionIndexEdit 0
#define MMOptionIndexPlay 1
#define MMOptionIndexDelete 2
#define MMOptionIndexCancel 3
#define MMNumOptions 4


class TextRenderer;
class MapMakerLevelSelectedDialogue;

typedef void(*OnOptionSelected)(MapMakerLevelSelectedDialogue*);

class MapMakerLevelSelectedDialogue
	:public DrawableLayerBase,
	public RecieveInputLayerBase
{
private:
	struct DialogueOption
	{
		const char* Text;
		vec2 Position;
		vec2 CursorPosition;
		OnOptionSelected OnSelectedFunc;
	};
public:
	MapMakerLevelSelectedDialogue(
		const std::shared_ptr<IConfigFile>& config,
		const std::shared_ptr<TextRenderer>& textRenderer,
		u32 screenWidth,
		u32 screenHeight);
	
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
	void PopulateOptionsArray();
	static void OnPlaySelected(MapMakerLevelSelectedDialogue* layer);
	static void OnEditSelected(MapMakerLevelSelectedDialogue* layer);
	static void OnDeleteSelected(MapMakerLevelSelectedDialogue* layer);
	static void OnCancelSelected(MapMakerLevelSelectedDialogue* layer);
private:
	LevelLoadData LevelLoad = { LevelSource::MapMaker, 0 };
	std::shared_ptr<IConfigFile> ConfigFile;
	std::shared_ptr<TextRenderer> CachedTextRenderer;
	LevelConfigData* SelectedLevel;
	u32 SelectedLevelIndex;
	i32 SelectedOption;
	DialogueOption Options[MMNumOptions];
	u32 ScreenWidth;
	u32 ScreenHeight;
};
