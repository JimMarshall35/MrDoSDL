#include "MapMakerCreateNewLevelDialogue.h"
#include <string>
#include <iostream>
#include "SDL.h"
#include "TextRenderer.h"
#include "VectorTypes.h"
#include "InputManager.h"
#include "IConfigFile.h"

static std::string sLayerName = "MapMakerCreateNewLevelDialogue";

MapMakerCreateNewLevelDialogue::MapMakerCreateNewLevelDialogue(const std::shared_ptr<TextRenderer>& textRenderer, const std::shared_ptr<IConfigFile>& config, u32 screenWidth, u32 screenHeight)
	:CachedTextRenderer(textRenderer), ConfigFile(config), ScreenWidth(screenWidth), ScreenHeight(screenHeight)
{
	memset(MapName, 'A', MapMakerNameMaxLen - 1);
	MapName[MapMakerNameMaxLen - 1] = '\0';
}

void MapMakerCreateNewLevelDialogue::ReceiveInput(const GameInputState& input)
{
	if (input.BackPress())
	{
		GameFramework::QueuePopLayersAtFrameEnd(GameLayerType::Draw | GameLayerType::Input);
	}
	if (input.LeftPress())
	{
		--CurrentSelection;
		if (CurrentSelection < 0)
		{
			CurrentSelection = MapMakerNameMaxLen;
		}
	}
	if (input.RightPress())
	{
		++CurrentSelection;
		if (CurrentSelection > MapMakerNameMaxLen)
		{
			CurrentSelection = 0;
		}
	}

	if (input.UpPress())
	{
		if (CurrentSelection < MapMakerNameMaxLen - 1)
		{
			++MapName[CurrentSelection];
		}
	}
	if (input.DownPress())
	{
		if (CurrentSelection < MapMakerNameMaxLen - 1)
		{
			--MapName[CurrentSelection];
		}
	}

	if (input.CrystalBallPress())
	{
		if (CurrentSelection == SaveSelection)
		{
			std::cout << "saving level: " << MapName << "\n";
			LevelConfigData newLevel = ConfigFile->GetBlankLevelTemplate();
			newLevel.Name = MapName;
			std::vector<LevelConfigData>& mapMakerLevels = ConfigFile->GetMapMakerLevelsConfigData();
			mapMakerLevels.push_back(newLevel);
			ConfigFile->SaveAfterMapMakerLevelsChange();
			GameFramework::QueuePopLayersAtFrameEnd(GameLayerType::Draw | GameLayerType::Input);
		}
		if (CurrentSelection == CancelSelection)
		{
			GameFramework::QueuePopLayersAtFrameEnd(GameLayerType::Draw | GameLayerType::Input);
		}
	}
}

bool MapMakerCreateNewLevelDialogue::MasksPreviousInputLayer() const
{
	return true;
}

const std::string& MapMakerCreateNewLevelDialogue::GetInputLayerName() const
{
	return sLayerName;
}

void MapMakerCreateNewLevelDialogue::OnInputPush(void* data)
{
	memset(MapName, 'A', MapMakerNameMaxLen - 1);
	MapName[MapMakerNameMaxLen - 1] = '\0';
	CurrentSelection = 0;
}

void MapMakerCreateNewLevelDialogue::Draw(SDL_Surface* windowSurface, float scale) const
{
	float fontSize = CachedTextRenderer->GetTextTileSize();
	SDL_Rect dialogueWindowRect;
	dialogueWindowRect.w = ScreenWidth / 1.5f;
	dialogueWindowRect.h = ScreenHeight / 2.0f;
	dialogueWindowRect.x = (ScreenWidth / 2.0f) - ScreenWidth / 3.0f;
	dialogueWindowRect.y = (ScreenHeight / 2.0f) - ScreenHeight / 4.0f;
	SDL_FillSurfaceRect(windowSurface, &dialogueWindowRect, SDL_MapRGB(windowSurface->format, 98, 103, 252));
	CachedTextRenderer->SetCurrentFont("Black_BlueBackground");
	vec2 mapNameStartPos = vec2{dialogueWindowRect.x / scale + 8, dialogueWindowRect.y / scale + 32.0f};
	CachedTextRenderer->RenderText(mapNameStartPos, MapName, windowSurface, scale);

	vec2 promptStartPos = vec2{ dialogueWindowRect.x / scale + 8, dialogueWindowRect.y / scale + 16 };
	CachedTextRenderer->RenderText(promptStartPos, "Enter a name", windowSurface, scale);

	const char* saveText = "Save";
	vec2 savePromptStartPos = vec2{
		(dialogueWindowRect.x + dialogueWindowRect.w) / scale - CachedTextRenderer->GetTextWidthPixels(saveText) - fontSize * 2,
		(dialogueWindowRect.y + dialogueWindowRect.h) / scale - fontSize * 2
	};
	CachedTextRenderer->RenderText(savePromptStartPos, saveText, windowSurface, scale);

	const char* cancelText = "Cancel";
	vec2 cancelPromptStartPos = vec2{
		(dialogueWindowRect.x) / scale + fontSize * 2,
		(dialogueWindowRect.y + dialogueWindowRect.h) / scale - fontSize * 2
	};
	CachedTextRenderer->RenderText(cancelPromptStartPos, cancelText, windowSurface, scale);


	if (CurrentSelection < MapMakerNameMaxLen - 1)
	{
		vec2 cursorPos = mapNameStartPos + vec2{ CurrentSelection * fontSize, fontSize };
		CachedTextRenderer->RenderText(cursorPos, "^", windowSurface, scale);
	}
	else
	{
		vec2 cursorPos;
		if (CurrentSelection == MapMakerNameMaxLen - 1)
		{
			cursorPos = cancelPromptStartPos + vec2{ -fontSize, 0 };
		}
		else
		{
			cursorPos = savePromptStartPos + vec2{ -fontSize, 0 };
		}
		CachedTextRenderer->RenderText(cursorPos, "^", windowSurface, scale);
	}
}

bool MapMakerCreateNewLevelDialogue::MasksPreviousDrawableLayer() const
{
	return false;
}

const std::string& MapMakerCreateNewLevelDialogue::GetDrawableLayerName() const
{
	return sLayerName;
}
