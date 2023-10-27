#include "FrontEndLayer.h"
#include "VectorTypes.h"
#include "TextRenderer.h"
#include <string>

static const std::string sLayerName = "FrontEnd";


FrontEndLayer::FrontEndLayer(
	const std::shared_ptr<TextRenderer>& textRenderer, 
	u32 screenWidth,
	u32 screenHeight)
	:TextRendererPtr(textRenderer),
	ScreenWidth(screenWidth),
	ScreenHeight(screenHeight),
	SelectedMenuIndex(0)
{
}

void FrontEndLayer::Update(float deltaT)
{
}

bool FrontEndLayer::MasksPreviousUpdateableLayer() const
{
	return true;
}

const std::string& FrontEndLayer::GetUpdateableLayerName() const
{
	return sLayerName;
}

void FrontEndLayer::OnUpdatePush(void* data)
{
}

void FrontEndLayer::OnUpdatePop()
{
}

void FrontEndLayer::Draw(SDL_Surface* windowSurface, float scale) const
{
	vec2 startingPosition = { (ScreenWidth / scale) / 2.0f, (ScreenHeight / scale) / 2.0f };
	float textTileSize = TextRendererPtr->GetTextTileSize();
	TextRendererPtr->SetCurrentFont("White_BlackBackground");
	for (u32 i = 0; i < NumberOfOptions; i++)
	{
		const MenuOption& option = MenuOptions[i];
		float optionStringWidth = TextRendererPtr->GetTextWidthPixels(option.Name.c_str());
		TextRendererPtr->RenderText(
			startingPosition + vec2{ -(optionStringWidth / 2.0f), i * textTileSize },
			option.Name.c_str(),
			windowSurface,
			scale);

		if (i == SelectedMenuIndex)
		{
			if (bBlinkState)
			{
				TextRendererPtr->RenderText(
					startingPosition + vec2{ -(optionStringWidth / 2.0f) - textTileSize, i * textTileSize },
					")",
					windowSurface,
					scale);

				TextRendererPtr->RenderText(
					startingPosition + vec2{ (optionStringWidth / 2.0f), i * textTileSize },
					"(",
					windowSurface,
					scale);
			}
			else
			{

			}
		}
	}
}

bool FrontEndLayer::MasksPreviousDrawableLayer() const
{
	return true;
}

const std::string& FrontEndLayer::GetDrawableLayerName() const
{
	return sLayerName;
}

void FrontEndLayer::OnDrawablePush(void* data)
{
}

void FrontEndLayer::OnDrawablePop()
{
}

void FrontEndLayer::ReceiveInput(const GameInputState& input)
{
	if (!LastFrameInput.Up && input.Up)
	{
		OnUpPress();
	}
	if (!LastFrameInput.Down && input.Down)
	{
		OnDownPress();
	}
	if (!LastFrameInput.CrystalBall && input.CrystalBall)
	{
		OnSelectPress();
	}
	LastFrameInput = input;
}

bool FrontEndLayer::MasksPreviousInputLayer() const
{
	return true;
}

const std::string& FrontEndLayer::GetInputLayerName() const
{
	return sLayerName;
}

void FrontEndLayer::OnInputPush(void* data)
{
}

void FrontEndLayer::OnInputPop()
{
}

void FrontEndLayer::OnUpPress()
{
	++SelectedMenuIndex;
	if (SelectedMenuIndex >= NumberOfOptions)
	{
		SelectedMenuIndex = 0;
	}
}

void FrontEndLayer::OnDownPress()
{
	--SelectedMenuIndex;
	if (SelectedMenuIndex < 0)
	{
		SelectedMenuIndex = NumberOfOptions - 1;
	}
}

void FrontEndLayer::OnSelectPress()
{
	const MenuOption& selectedOption = MenuOptions[SelectedMenuIndex];
	SelectedMenuIndex = 0;
	GameFramework::PushLayers(selectedOption.LayerToPushName, selectedOption.LayerTypesToPushName, selectedOption.PushLayerData);
}
