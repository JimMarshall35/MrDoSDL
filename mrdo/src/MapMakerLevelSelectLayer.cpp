#include "MapMakerLevelSelectLayer.h"
#include "TextRenderer.h"
#include "VectorTypes.h"
#include "IConfigFile.h"
#include <string>
#include <cassert>

/* the code in this file sucks */

static const std::string sLayerName = "MapMakerLevelSelectLayer";

MapMakerLevelSelectLayer::MapMakerLevelSelectLayer(
	const std::shared_ptr<TextRenderer>& textRenderer,
	const std::shared_ptr<IConfigFile>& configFile,
	u32 screenWidth,
	u32 screenHeight)
	:TextRendererPtr(textRenderer),
	ConfigFile(configFile),
	ScreenWidth(screenWidth),
	ScreenHeight(screenHeight)
{
	CurrentOnScreenSelection[0] = -1;
	std::vector<LevelConfigData>& mapMakerLevels = ConfigFile->GetMapMakerLevelsConfigData();
	CurrentNumberOnScreen = 1 + mapMakerLevels.size();
	if (CurrentNumberOnScreen > NumberOfMapMakerLevelsToShowAtOnce)
	{
		CurrentNumberOnScreen = NumberOfMapMakerLevelsToShowAtOnce;
	}
	for (int i = 1; i < NumberOfMapMakerLevelsToShowAtOnce; i++)
	{
		CurrentOnScreenSelection[i] = i - 1;
	}
}

void MapMakerLevelSelectLayer::Update(float deltaT)
{
}

bool MapMakerLevelSelectLayer::MasksPreviousUpdateableLayer() const
{
	return true;
}

const std::string& MapMakerLevelSelectLayer::GetUpdateableLayerName() const
{
	return sLayerName;
}

void MapMakerLevelSelectLayer::OnUpdatePush(void* data)
{
}

void MapMakerLevelSelectLayer::OnUpdatePop()
{
}

void MapMakerLevelSelectLayer::Draw(SDL_Surface* windowSurface, float scale) const
{
	float textTileSize = TextRendererPtr->GetTextTileSize();
	TextRendererPtr->SetCurrentFont("White_BlackBackground");
	TextRendererPtr->RenderText(
		{ 0,0 },
		"select level:",
		windowSurface,
		scale);
	std::vector<LevelConfigData>& mapMakerLevels = ConfigFile->GetMapMakerLevelsConfigData();
	


	for (int i = 0; i < CurrentNumberOnScreen; i++)
	{
		
		i32 index = CurrentOnScreenSelection[i];
		if (index < 0)
		{
			vec2 startingPosition = 
			{ 
				((ScreenWidth / scale) / 2.0f) - TextRendererPtr->GetTextWidthPixels("New Level")/2.0f + (i==IndexInOnscreenSelection? textTileSize/2.0f : 0),
				(ScreenHeight / scale) / 2.0f 
			};
			if (i == IndexInOnscreenSelection)
			{
				TextRendererPtr->SetCurrentFont("Red_BlackBackground");
				TextRendererPtr->RenderText(
					startingPosition + vec2{ 0, i * textTileSize },
					")",
					windowSurface,
					scale);
			}
			TextRendererPtr->SetCurrentFont("Green_BlackBackground");
			TextRendererPtr->RenderText(
				startingPosition + ((i == IndexInOnscreenSelection) ? vec2{textTileSize, 0} : vec2{ 0,0 }),
				"New Level", windowSurface, scale);
		}
		else
		{
			const LevelConfigData& level = mapMakerLevels[index];
			vec2 startingPosition =
			{
				((ScreenWidth / scale) / 2.0f) - TextRendererPtr->GetTextWidthPixels(level.Name.c_str()) / 2.0f - (i == IndexInOnscreenSelection ? textTileSize / 2.0f : 0),
				(ScreenHeight / scale) / 2.0f
			};
			if (i == IndexInOnscreenSelection)
			{
				TextRendererPtr->SetCurrentFont("Red_BlackBackground");
				TextRendererPtr->RenderText(
					startingPosition + vec2{ 0, i * textTileSize },
					")",
					windowSurface,
					scale);
			}
			
			TextRendererPtr->SetCurrentFont("White_BlackBackground");
			TextRendererPtr->RenderText(
				startingPosition + ((i == IndexInOnscreenSelection) ? vec2{ textTileSize, i * textTileSize } : vec2{ 0,i * textTileSize }),//+ vec2{ 0, i * textTileSize },
				level.Name.c_str(),
				windowSurface,
				scale);
		}
	}
}

bool MapMakerLevelSelectLayer::MasksPreviousDrawableLayer() const
{
	return true;
}

const std::string& MapMakerLevelSelectLayer::GetDrawableLayerName() const
{
	return sLayerName;
}

void MapMakerLevelSelectLayer::OnDrawablePush(void* data)
{
}

void MapMakerLevelSelectLayer::OnDrawablePop()
{
}

void MapMakerLevelSelectLayer::ReceiveInput(const GameInputState& input)
{
	if (input.UpPress())
	{
		OnUpPress();
	}
	if (input.DownPress())
	{
		OnDownPress();
	}
	if (input.CrystalBallPress())
	{
		OnSelectPress();
	}
	if (input.BackPress())
	{
		GameFramework::PopLayers(GameLayerType::Draw | GameLayerType::Input | GameLayerType::Update);
	}
}

bool MapMakerLevelSelectLayer::MasksPreviousInputLayer() const
{
	return true;
}

const std::string& MapMakerLevelSelectLayer::GetInputLayerName() const
{
	return sLayerName;
}

void MapMakerLevelSelectLayer::OnInputPush(void* data)
{
}

void MapMakerLevelSelectLayer::OnInputPop()
{
}

void MapMakerLevelSelectLayer::OnUpPress()
{
	// I hate this code (and OnDownPress) so much - but it works - it'll do
	std::vector<LevelConfigData>& mapMakerLevels = ConfigFile->GetMapMakerLevelsConfigData();
	--IndexInOnscreenSelection;
	i32 lastIndexOnScreen = CurrentOnScreenSelection[CurrentNumberOnScreen - 1];
	i32 firstIndexOnScreen = CurrentOnScreenSelection[0];
	if (IndexInOnscreenSelection < 0)
	{
		if (firstIndexOnScreen - 1 >= -1)
		{
			for (i32 i = 0; i < CurrentNumberOnScreen; i++)
			{
				--CurrentOnScreenSelection[i];
			}
			IndexInOnscreenSelection = 0;
		}
		else
		{
			if (NumberOfMapMakerLevelsToShowAtOnce == CurrentNumberOnScreen)
			{
				i32 numLevels = mapMakerLevels.size();
				i32 startPoint = numLevels - NumberOfMapMakerLevelsToShowAtOnce;
				i32 j = 0;
				for (i32 i = startPoint; i < numLevels; i++)
				{
					CurrentOnScreenSelection[j++] = i;
				}
				IndexInOnscreenSelection = CurrentNumberOnScreen - 1;
			}
			else
			{
				IndexInOnscreenSelection = CurrentNumberOnScreen - 1;
			}
		}
	}
}

void MapMakerLevelSelectLayer::OnDownPress()
{
	std::vector<LevelConfigData>& mapMakerLevels = ConfigFile->GetMapMakerLevelsConfigData();
	++IndexInOnscreenSelection;
	i32 lastIndexOnScreen = CurrentOnScreenSelection[CurrentNumberOnScreen - 1];
	i32 firstIndexOnScreen = CurrentOnScreenSelection[0];
	if (IndexInOnscreenSelection >= CurrentNumberOnScreen)
	{
		if (lastIndexOnScreen + 1 < mapMakerLevels.size())
		{
			assert(CurrentNumberOnScreen == NumberOfMapMakerLevelsToShowAtOnce);
			for (i32 i = 0; i < CurrentNumberOnScreen; i++)
			{
				++CurrentOnScreenSelection[i];
			}
			IndexInOnscreenSelection = NumberOfMapMakerLevelsToShowAtOnce - 1;
		}
		else
		{
			CurrentOnScreenSelection[0] = -1;
			if (CurrentNumberOnScreen > NumberOfMapMakerLevelsToShowAtOnce)
			{
				CurrentNumberOnScreen = NumberOfMapMakerLevelsToShowAtOnce;
			}
			for (int i = 1; i < NumberOfMapMakerLevelsToShowAtOnce; i++)
			{
				CurrentOnScreenSelection[i] = i - 1;
			}
			IndexInOnscreenSelection = 0;
		}
	}
}

void MapMakerLevelSelectLayer::OnSelectPress()
{
	i32 index = CurrentOnScreenSelection[IndexInOnscreenSelection];
	if (index == -1)
	{
		GameFramework::QueuePushLayersAtFrameEnd("MapMakerCreateNewLevelDialogue", GameLayerType::Draw | GameLayerType::Input);
	}
}
