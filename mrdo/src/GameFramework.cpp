#include <functional>
#include "GameFramework.h"
#include <iostream>
static size_t m_inputStackSize = 0;
static size_t m_drawableStackSize = 0;
static size_t m_updateableStackSize = 0;

RecieveInputLayerBase* GameFramework::m_inputStack[FRAMEWORK_STACKS_SIZE];
DrawableLayerBase* GameFramework::m_drawableStack[FRAMEWORK_STACKS_SIZE];
UpdateableLayerBase* GameFramework::m_updateableStack[FRAMEWORK_STACKS_SIZE];

size_t GameFramework::m_inputStackSize;
size_t GameFramework::m_drawableStackSize;
size_t GameFramework::m_updateableStackSize;
std::atomic<bool> GameFramework::m_newDataToReport;

bool GameFramework::m_shouldPushNewLayerAtFrameEnd = false;
std::string GameFramework::m_layerToPushAtFrameEnd = "";
GameLayerType GameFramework::m_layerTypesToPushAtFrameEnd = GameLayerType::None;


void* GameFramework::m_layerTypeDataToPushAtFrameEnd = nullptr;

GameFramework::GameFramework()
{
	m_newDataToReport = false;
}

void GameFramework::Update(double deltaT)
{
	if (m_updateableStackSize == 0) {
		return;
	}
	auto topOfStackIndex = m_updateableStackSize - 1;
	do {
		m_updateableStack[topOfStackIndex]->Update(deltaT);
	} while (!m_updateableStack[topOfStackIndex--]->MasksPreviousUpdateableLayer());
}

void GameFramework::Draw(SDL_Surface* windowSurface, float scale)
{
	if (m_drawableStackSize == 0) {
		return;
	}
	auto topOfStackIndex = m_drawableStackSize - 1;
	do {
		m_drawableStack[topOfStackIndex]->Draw(windowSurface, scale);
	} while (!m_drawableStack[topOfStackIndex--]->MasksPreviousDrawableLayer());
}

void GameFramework::RecieveInput(const GameInputState& input)
{
	if (m_inputStackSize == 0) {
		return;
	}
	auto topOfStackIndex = m_inputStackSize - 1;
	do {
		m_inputStack[topOfStackIndex]->ReceiveInput(input);
	} while (!m_inputStack[topOfStackIndex--]->MasksPreviousInputLayer());
}

void GameFramework::EndFrame()
{
	if (m_shouldPushNewLayerAtFrameEnd)
	{
		m_shouldPushNewLayerAtFrameEnd = false;
		PushLayers(m_layerToPushAtFrameEnd, m_layerTypesToPushAtFrameEnd, m_layerTypeDataToPushAtFrameEnd);
	}
}




bool GameFramework::PushLayers(std::string name, GameLayerType whichLayers, void* data)
{
	// TODO: refactor to reduce duplication
	if (whichLayers & GameLayerType::Input) {
		bool hasFoundName = false;
		const auto& list = AutoList<RecieveInputLayerBase>::GetList();
		for (const auto layer : list) {
			if (layer->GetInputLayerName() == name) {
				if (!hasFoundName) {
					hasFoundName = true;
					PushInputLayer(layer, data);
				}
				else {
					std::cerr << "[GameFramework] Requested input layer " << name << " was found more than once in the auto list - this isn't allowed" << std::endl;
					return false;
				}
			}
		}
		if (!hasFoundName) {
			std::cerr << "[GameFramework] Requested input layer " << name << " was not found" << std::endl;
			return false;
		}
	}
	if (whichLayers & GameLayerType::Draw) {
		bool hasFoundName = false;
		const auto& list = AutoList<DrawableLayerBase>::GetList();
		for (const auto layer : list) {
			if (layer->GetDrawableLayerName() == name) {
				if (!hasFoundName) {
					hasFoundName = true;
					PushDrawableLayer(layer, data);
				}
				else {
					std::cerr << "[GameFramework] Requested draw layer " << name << " was found more than once in the auto list - this isn't allowed" << std::endl;
					return false;
				}
			}
		}
		if (!hasFoundName) {
			std::cerr << "[GameFramework] Requested draw layer " << name << " was not found" << std::endl;
			return false;
		}
	}
	if (whichLayers & GameLayerType::Update) {
		bool hasFoundName = false;
		const auto& list = AutoList<UpdateableLayerBase>::GetList();
		for (const auto layer : list) {
			if (layer->GetUpdateableLayerName() == name) {
				if (!hasFoundName) {
					hasFoundName = true;
					PushUpdatableLayer(layer, data);
				}
				else {
					std::cerr << "[GameFramework] Requested update layer " << name << " was found more than once in the auto list - this isn't allowed" << std::endl;
					return false;
				}
			}
		}
		if (!hasFoundName) {
			std::cerr << "[GameFramework] Requested update layer " << name << " was not found" << std::endl;
			return false;
		}
	}
	m_newDataToReport = true;
	return true;
}

void GameFramework::QueuePushLayersAtFrameEnd(std::string name, GameLayerType whichLayers, void* data)
{
	m_shouldPushNewLayerAtFrameEnd = true;
	m_layerToPushAtFrameEnd = name;
	m_layerTypesToPushAtFrameEnd = whichLayers;
	m_layerTypeDataToPushAtFrameEnd = data;
}

bool GameFramework::PopLayers(GameLayerType whichLayers)
{
	// TODO: refactor to reduce duplication
	if (whichLayers & GameLayerType::Input) {
		PopInputLayer();
	}
	if (whichLayers & GameLayerType::Draw) {
		PopDrawableLayer();
	}
	if (whichLayers & GameLayerType::Update) {
		PopUpdatableLayer();
	}
	m_newDataToReport = true;

	return true;
}

void GameFramework::PushInputLayer(RecieveInputLayerBase* input, void* data)
{
	if (m_inputStackSize >= FRAMEWORK_STACKS_SIZE) {
		std::cerr << "FRAMEWORK_STACKS_SIZE exceeded - can't push\n";
		return;
	}
	m_inputStack[m_inputStackSize++] = input;
	input->OnInputPush(data);
}

RecieveInputLayerBase* GameFramework::PopInputLayer()
{
	if (m_inputStackSize == 0) {
		std::cerr << "m_inputStackSize is zero - can't pop\n";
		return nullptr;
	}
	auto top = m_inputStack[--m_inputStackSize];
	top->OnInputPop();
	return top;
}

void GameFramework::PushDrawableLayer(DrawableLayerBase* drawable, void* data)
{
	if (m_inputStackSize >= FRAMEWORK_STACKS_SIZE) {
		std::cerr << "FRAMEWORK_STACKS_SIZE exceeded - can't push\n";
		return;
	}
	m_drawableStack[m_drawableStackSize++] = drawable;
	drawable->OnDrawablePush(data);
}

const DrawableLayerBase* GameFramework::PopDrawableLayer()
{
	if (m_drawableStackSize == 0) {
		std::cerr << "m_drawableStackSize is zero - can't pop\n";
		return nullptr;
	}
	auto top = m_drawableStack[--m_drawableStackSize];
	top->OnDrawablePop();
	return top;
}

void GameFramework::PushUpdatableLayer(UpdateableLayerBase* updatable, void* data)
{
	if (m_inputStackSize >= FRAMEWORK_STACKS_SIZE) {
		std::cerr << "FRAMEWORK_STACKS_SIZE exceeded - can't push\n";
		return;
	}
	m_updateableStack[m_updateableStackSize++] = updatable;
	updatable->OnUpdatePush(data);
}

UpdateableLayerBase* GameFramework::PopUpdatableLayer()
{
	if (m_updateableStack == 0) {
		std::cerr << "m_updateableStack is zero - can't pop\n";
		return nullptr;
	}
	auto top = m_updateableStack[--m_updateableStackSize];
	top->OnUpdatePop();
	return top;
}

RecieveInputLayerBase** GameFramework::GetInputLayers() {
	return m_inputStack;
}
DrawableLayerBase** GameFramework::GetDrawableLayers() {
	return m_drawableStack;
}
UpdateableLayerBase** GameFramework::GetUpdatableLayers() {
	return m_updateableStack;
}
size_t GameFramework::GetInputLayersSize() {
	return m_inputStackSize;
}
size_t GameFramework::GetUpdatableLayersSize() {
	return m_updateableStackSize;
}
size_t GameFramework::GetDrawableLayersSize() {
	return m_drawableStackSize;
}
const bool GameFramework::NewDataToReport() {
	return m_newDataToReport;
}
void GameFramework::AcknowledgeNewData() {
	m_newDataToReport = false;
}