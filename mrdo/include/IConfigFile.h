#pragma once
#include <string>
#include <vector>
#include <map>
#include "VectorTypes.h"

struct SpriteSheetConfigData
{
	std::string SpriteSheetAssetPath;
	int NumCols;
	int NumRows;
	int TileSize;
};

struct BackgroundTileConfigData : public SpriteSheetConfigData
{
	std::vector<unsigned char> RowPattern;
};

typedef std::map<std::string, std::vector<uvec2>> AnimationMap;
struct AnimationsConfigData : public SpriteSheetConfigData
{
	u8 ColourKeyR;
	u8 ColourKeyG;
	u8 ColourKeyB;
	AnimationMap Animations;
};

struct LevelConfigData
{
	std::vector<u8> TileData;
	int NumRows;
	int NumCols;
	int BackgroundTileset;
	uvec2 PlayerSpawnLocation;
	u32 PlayerSpawnFacing;
};

class IConfigFile
{
public:
	virtual const BackgroundTileConfigData& GetBackgroundConfigData() const = 0;
	virtual const AnimationsConfigData& GetAnimationsConfigData() const = 0;
	virtual const std::vector<LevelConfigData>& GetLevelsConfigData() const = 0;
	virtual const u32 GetUIntValue(const std::string& key) const = 0;
	virtual const i32 GetIntValue(const std::string& key) const = 0;
	virtual const float GetFloatValue(const std::string& key) const = 0;
	virtual const std::string& GetStringValue(const std::string& key) const = 0;
	virtual bool GetBoolValue(const std::string& key) const = 0;


};