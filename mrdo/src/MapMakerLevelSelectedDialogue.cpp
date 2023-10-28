#include "MapMakerLevelSelectedDialogue.h"
#include <string>
#include "InputManager.h"
#include "TextRenderer.h"

static const std::string sLayerName = "MapMakerLevelSelectedDialogue";

MapMakerLevelSelectedDialogue::MapMakerLevelSelectedDialogue(
	const std::shared_ptr<IConfigFile>& config, 
	const std::shared_ptr<TextRenderer>& textRenderer,
	u32 screenWidth,
	u32 screenHeight)
	:ConfigFile(config),
	CachedTextRenderer(textRenderer),
	SelectedLevel(nullptr),
	SelectedLevelIndex(0),
	SelectedOption(0),
	ScreenWidth(screenWidth),
	ScreenHeight(screenHeight)
{
	PopulateOptionsArray();
}

void MapMakerLevelSelectedDialogue::Draw(SDL_Surface* windowSurface, float scale) const
{
	CachedTextRenderer->SetCurrentFont("Yellow_BlackBackground");
	for (int i = 0; i < MMNumOptions; i++)
	{
		const DialogueOption& option = Options[i];
		CachedTextRenderer->RenderText(option.Position, option.Text, windowSurface, scale);
	}
	const DialogueOption& option = Options[SelectedOption];
	CachedTextRenderer->RenderText(option.CursorPosition, "^", windowSurface, scale);
}

bool MapMakerLevelSelectedDialogue::MasksPreviousDrawableLayer() const
{
	return false;
}

const std::string& MapMakerLevelSelectedDialogue::GetDrawableLayerName() const
{
	return sLayerName;
}

void MapMakerLevelSelectedDialogue::OnDrawablePush(void* data)
{
}

void MapMakerLevelSelectedDialogue::OnDrawablePop()
{
}

void MapMakerLevelSelectedDialogue::ReceiveInput(const GameInputState& input)
{
	if (input.BackPress())
	{
		GameFramework::QueuePopLayersAtFrameEnd(GameLayerType::Draw | GameLayerType::Input);
	}
	if (input.UpPress() || input.RightPress())
	{
		++SelectedOption;
		if (SelectedOption == MMNumOptions)
		{
			SelectedOption = 0;
		}
	}
	if (input.DownPress() || input.LeftPress())
	{
		--SelectedOption;
		if (SelectedOption < 0)
		{
			SelectedOption = MMNumOptions - 1;
		}
	}
	if (input.CrystalBallPress())
	{
		Options[SelectedOption].OnSelectedFunc(this);
	}
}

bool MapMakerLevelSelectedDialogue::MasksPreviousInputLayer() const
{
	return true;
}

const std::string& MapMakerLevelSelectedDialogue::GetInputLayerName() const
{
	return sLayerName;
}

void MapMakerLevelSelectedDialogue::OnInputPush(void* data)
{
	SelectedOption = MMOptionIndexEdit;
	SelectedLevelIndex = (u32)data;
	LevelLoad.LevelIndex = SelectedLevelIndex;
	std::vector<LevelConfigData>& mapMakerLevels = ConfigFile->GetMapMakerLevelsConfigData();
	SelectedLevel = &mapMakerLevels[SelectedLevelIndex];
}

void MapMakerLevelSelectedDialogue::OnInputPop()
{
}

void MapMakerLevelSelectedDialogue::PopulateOptionsArray()
{
	static const char* playText = "Play";
	static const char* editText = "Edit";
	static const char* cancelText = "Cancel";
	static const char* deleteText = "Delete";

	float textTileSize = CachedTextRenderer->GetTextTileSize();

	Options[MMOptionIndexPlay].Text = playText;
	Options[MMOptionIndexPlay].OnSelectedFunc = &OnPlaySelected;
	Options[MMOptionIndexPlay].CursorPosition = vec2{ 0, textTileSize * 2 };
	Options[MMOptionIndexPlay].Position = vec2{ textTileSize, textTileSize * 2 };

	Options[MMOptionIndexEdit].Text = editText;
	Options[MMOptionIndexEdit].OnSelectedFunc = &OnEditSelected;
	Options[MMOptionIndexEdit].CursorPosition = vec2{ 0, textTileSize * 3 };
	Options[MMOptionIndexEdit].Position = vec2{ textTileSize, textTileSize * 3 };

	Options[MMOptionIndexCancel].Text = cancelText;
	Options[MMOptionIndexCancel].OnSelectedFunc = &OnCancelSelected;
	Options[MMOptionIndexCancel].CursorPosition = vec2{ 0, textTileSize * 4 };
	Options[MMOptionIndexCancel].Position = vec2{ textTileSize, textTileSize * 4 };

	Options[MMOptionIndexDelete].Text = deleteText;
	Options[MMOptionIndexDelete].OnSelectedFunc = &OnDeleteSelected;
	Options[MMOptionIndexDelete].CursorPosition = vec2{ 0, textTileSize * 5 };
	Options[MMOptionIndexDelete].Position = vec2{ textTileSize, textTileSize * 5 };

}

void MapMakerLevelSelectedDialogue::OnPlaySelected(MapMakerLevelSelectedDialogue* layer)
{
	GameFramework::QueuePopLayersAtFrameEnd(GameLayerType::Draw | GameLayerType::Input);
	GameFramework::QueuePushLayersAtFrameEnd("Game", GameLayerType::Draw | GameLayerType::Input | GameLayerType::Update, &layer->LevelLoad);
}

void MapMakerLevelSelectedDialogue::OnEditSelected(MapMakerLevelSelectedDialogue* layer)
{
	// todo: implement
}

void MapMakerLevelSelectedDialogue::OnDeleteSelected(MapMakerLevelSelectedDialogue* layer)
{
	// todo: implement
}

void MapMakerLevelSelectedDialogue::OnCancelSelected(MapMakerLevelSelectedDialogue* layer)
{
	GameFramework::QueuePopLayersAtFrameEnd(GameLayerType::Draw | GameLayerType::Input);
}
