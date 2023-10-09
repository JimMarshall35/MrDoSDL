#pragma once
#include <stack>
#include <string>
#include <map>
#include <thread>
#include <atomic>
#include "AutoList.h"

struct GameInputState;
struct SDL_Surface;

enum class GameLayerType : unsigned int {
	Draw = 1,
	Update = 2,
	Input = 4
};

inline GameLayerType operator|(GameLayerType a, GameLayerType b)
{
	return static_cast<GameLayerType>(static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
}
inline unsigned int operator&(GameLayerType a, GameLayerType b)
{
	return static_cast<unsigned int>(a) & static_cast<unsigned int>(b);
}


class RecieveInputLayerBase
	:public AutoList<RecieveInputLayerBase> {
public:
	virtual void ReceiveInput(const GameInputState& input) = 0;
	virtual bool MasksPreviousInputLayer() const = 0;
	virtual const std::string& GetInputLayerName() const = 0;
	virtual void OnInputPush(void* data) = 0;
	virtual void OnInputPop() = 0;
};

class DrawableLayerBase
	:public AutoList<DrawableLayerBase> {
public:
	virtual void Draw(SDL_Surface* windowSurface, float scale) const = 0;
	virtual bool MasksPreviousDrawableLayer() const = 0;
	virtual const std::string& GetDrawableLayerName() const = 0;
	virtual void OnDrawablePush(void* data) = 0;
	virtual void OnDrawablePop() = 0;
};

class UpdateableLayerBase
	:public AutoList<UpdateableLayerBase> {
public:
	virtual void Update(float deltaT) = 0;
	virtual bool MasksPreviousUpdateableLayer() const = 0;
	virtual const std::string& GetUpdateableLayerName() const = 0;
	virtual void OnUpdatePush(void* data) = 0;
	virtual void OnUpdatePop() = 0;
};

template<typename MessageT>
class GameFrameworkMessageRecipientBase
	:public AutoList<GameFrameworkMessageRecipientBase<MessageT>> {
public:
	virtual void RecieveMessage(const MessageT& message) = 0;
};


#define FRAMEWORK_STACKS_SIZE 100
static class GameFramework {
public:
	GameFramework();
	static void Update(double deltaT);
	static void Draw(SDL_Surface* windowSurface, float scale);
	static void RecieveInput(const GameInputState& input);

	static bool PushLayers(std::string name, GameLayerType whichLayers, void* data = nullptr);

	static bool PopLayers(GameLayerType whichLayers);

	template<typename MessageT>
	static void SendFrameworkMessage(const MessageT& message);
	static RecieveInputLayerBase** GetInputLayers();
	static DrawableLayerBase** GetDrawableLayers();
	static UpdateableLayerBase** GetUpdatableLayers();
	static size_t GetInputLayersSize();
	static size_t GetUpdatableLayersSize();
	static size_t GetDrawableLayersSize();
	static const bool NewDataToReport();
	static void AcknowledgeNewData();
private:
	static void PushInputLayer(RecieveInputLayerBase* input, void* data);
	static void PushDrawableLayer(DrawableLayerBase* drawable, void* data);
	static void PushUpdatableLayer(UpdateableLayerBase* updatable, void* data);

	static RecieveInputLayerBase* PopInputLayer();
	static const DrawableLayerBase* PopDrawableLayer();
	static UpdateableLayerBase* PopUpdatableLayer();
private:
	static size_t m_inputStackSize;
	static size_t m_drawableStackSize;
	static size_t m_updateableStackSize;

	static RecieveInputLayerBase* m_inputStack[FRAMEWORK_STACKS_SIZE];
	static DrawableLayerBase* m_drawableStack[FRAMEWORK_STACKS_SIZE];
	static UpdateableLayerBase* m_updateableStack[FRAMEWORK_STACKS_SIZE];

	static std::atomic<bool> m_newDataToReport;
};

template<typename MessageT>
inline void GameFramework::SendFrameworkMessage(const MessageT& message)
{
	for (auto ptr : AutoList<GameFrameworkMessageRecipientBase<MessageT>>::GetList()) {
		ptr->RecieveMessage(message);
	}
}