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

typedef std::map<std::string, std::vector<ivec2>> AnimationMap;
typedef std::map<std::string, ivec2> SingleSpriteMap;
typedef std::map<std::string, ivec2> FontBlocks;
typedef std::vector<std::pair<u8, u8>> FontBlockMapping;

struct AnimationsConfigData : public SpriteSheetConfigData
{
	u8 ColourKeyR;
	u8 ColourKeyG;
	u8 ColourKeyB;
	AnimationMap Animations;
	SingleSpriteMap SingleSprites;
};

enum class FontLetterAvailability
{
	AllAvailable,
	AllCaps,
	AllLowercase
};

struct FontConfigData : public SpriteSheetConfigData
{
	u8 ColourKeyR;
	u8 ColourKeyG;
	u8 ColourKeyB;
	ivec2 BlockDims; // a "block" contains the whole alphabet's worth of characters
	FontBlocks Blocks;
	FontBlockMapping BlockMapping; // maps number of letter in block to which ascii code it is - first in pair is the number in the block, second the ascii code
	FontLetterAvailability LetterAvailability = FontLetterAvailability::AllAvailable;
};

struct MonsterSpawnerData
{
	ivec2 TilePosition;
	u32 NumMonsters;
	bool operator==(const MonsterSpawnerData& other) const
	{
		return TilePosition == other.TilePosition;
	}
};

struct LevelConfigData
{
	std::string Name;
	std::vector<ivec2> Apples;
	std::vector<u8> TileData;
	std::vector<MonsterSpawnerData> MonsterSpawners;
	int NumRows;
	int NumCols;
	int BackgroundTileset;
	ivec2 PlayerSpawnLocation;
	u32 PlayerSpawnFacing;
};

class IFileSystem;

class IConfigFile
{
public:
	virtual const BackgroundTileConfigData& GetBackgroundConfigData() const = 0;
	virtual const AnimationsConfigData& GetAnimationsConfigData() const = 0;
	virtual const FontConfigData& GetFontConfigData() const = 0;
	virtual const std::vector<LevelConfigData>& GetLevelsConfigData() const = 0;
	virtual std::vector<LevelConfigData>& GetMapMakerLevelsConfigData() = 0;
	virtual const LevelConfigData& GetBlankLevelTemplate() const = 0;
	virtual const u32 GetUIntValue(const std::string& key) const = 0;
	virtual const i32 GetIntValue(const std::string& key) const = 0;
	virtual const float GetFloatValue(const std::string& key) const = 0;
	virtual const std::string& GetStringValue(const std::string& key) const = 0;
	virtual bool GetBoolValue(const std::string& key) const = 0;
	virtual void SaveAfterMapMakerLevelsChange() = 0;
	virtual int GetArraySize(const std::string& key) const = 0;
	virtual int GetIntArrayValue(const std::string& key, size_t index) const = 0;
	virtual float GetFloatArrayValue(const std::string& key, size_t index) const = 0;
	virtual const IFileSystem* GetFileSystem() const = 0;
};