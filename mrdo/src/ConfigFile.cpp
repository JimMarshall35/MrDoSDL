#include "ConfigFile.h"
#include "IFileSystem.h"
#include <fstream>
#include <iostream>
#include <cassert>

ConfigFile::ConfigFile(const std::shared_ptr<IFileSystem>& fileSystem)
	:Filesystem(fileSystem)
{
	std::ifstream f(fileSystem->GetConfigFilePath());
	ConfigFileJSON = json::parse(f);
	PopulateBackgroundConfigDataStruct();
	PopulateAnimationsConfigDataStruct();
	PopulateLevelsConfigData(LevelsConfigData, "Levels");
	PopulateLevelsConfigData(MapMakerLevelsConfigData, "MapMakerLevels");
	BlankLevel = ParseLevelConfigData(ConfigFileJSON["BlankLevel"]);
	PopulateFontConfigDataStruct();
}

const BackgroundTileConfigData& ConfigFile::GetBackgroundConfigData() const
{
	return BackgroundConfigData;
}

const AnimationsConfigData& ConfigFile::GetAnimationsConfigData() const
{
	return AnimationConfigData;
}

void ConfigFile::PopulateBackgroundConfigDataStruct()
{
	PopulateSpriteSheetBase(BackgroundConfigData, "BackgroundTiles");
	json rowPattern = ConfigFileJSON["BackgroundTiles"]["RowPattern"];
	assert(rowPattern.is_array());
	for (int i = 0; i < rowPattern.size(); i++)
	{
		assert(rowPattern[i].is_number_unsigned());
		BackgroundConfigData.RowPattern.push_back(rowPattern[i]);
	}
}

void ConfigFile::PopulateAnimationsConfigDataStruct()
{
	PopulateSpriteSheetBase(AnimationConfigData, "Animations");
	AnimationConfigData.ColourKeyR = ConfigFileJSON["Animations"]["ColourKey"]["r"];
	AnimationConfigData.ColourKeyG = ConfigFileJSON["Animations"]["ColourKey"]["g"];
	AnimationConfigData.ColourKeyB = ConfigFileJSON["Animations"]["ColourKey"]["b"];

	for (auto& item : ConfigFileJSON["Animations"]["AnimationFrames"].items()) 
	{
		std::string key = item.key();
		json val = item.value();
		assert(val.is_array());
		for (int i = 0; i < val.size(); i++)
		{
			AnimationConfigData.Animations[key].push_back({ val[i]["x"], val[i]["y"]});
		}
	}
	for (auto& item : ConfigFileJSON["Animations"]["SingleSprites"].items())
	{
		std::string key = item.key();
		json val = item.value();
		assert(val.is_object());
		AnimationConfigData.SingleSprites[key] = { val["x"], val["y"] };
	}
}

void ConfigFile::PopulateSpriteSheetBase(SpriteSheetConfigData& spriteSheet, const std::string& objectName)
{
	spriteSheet.NumCols = ConfigFileJSON[objectName]["NumCols"];
	spriteSheet.NumRows = ConfigFileJSON[objectName]["NumRows"];
	spriteSheet.TileSize = ConfigFileJSON[objectName]["TileSize"];

	spriteSheet.SpriteSheetAssetPath = Filesystem->GetSpritesFolderPath() + "\\" + ConfigFileJSON[objectName]["AssetPath"].template get<std::string>();

}

void ConfigFile::PopulateLevelsConfigData(std::vector<LevelConfigData>& vectorToAddTo, const std::string& configFileArrayName)
{
	json levelsArray = ConfigFileJSON[configFileArrayName];
	assert(levelsArray.is_array());
	vectorToAddTo.clear();
	for (const json& level : levelsArray)
	{
		LevelConfigData data = ParseLevelConfigData(level);
		vectorToAddTo.push_back(data);
	}
}

void ConfigFile::PopulateFontConfigDataStruct()
{
	PopulateSpriteSheetBase(FontConfigData, "Font");
	json font = ConfigFileJSON["Font"];
	json blockDims = font["BlockDimensions"];
	FontConfigData.BlockDims = uvec2{ blockDims["x"], blockDims["y"] };
	json colourKey = font["ColourKey"];
	FontConfigData.ColourKeyR = colourKey["r"];
	FontConfigData.ColourKeyG = colourKey["g"];
	FontConfigData.ColourKeyB = colourKey["b"];

	if (font["AllCaps"])
	{
		assert(!font["AllLowercase"]);
		FontConfigData.LetterAvailability = FontLetterAvailability::AllCaps;
	}

	if (font["AllLowercase"])
	{
		assert(!font["AllCaps"]);
		FontConfigData.LetterAvailability = FontLetterAvailability::AllLowercase;
	}

	json blocks = font["Blocks"];
	for (auto& item : font["Blocks"].items())
	{
		json value = item.value();
		assert(value.is_object());
		FontConfigData.Blocks[item.key()] = uvec2{ value["x"], value["y"] };
	}

	assert(font["BlockMapping"].is_array());
	for (const json& item : font["BlockMapping"])
	{
		assert(item.is_array());
		assert(item.size() >= 2);
		FontConfigData.BlockMapping.push_back(std::pair<u8, u8>(item[0], item[1]));
	}
}

LevelConfigData ConfigFile::ParseLevelConfigData(const json& levelJson)
{
	assert(levelJson["PlayerSpawnLocation"].is_object());
	assert(levelJson["PlayerSpawnLocation"]["x"].is_number_unsigned());
	assert(levelJson["PlayerSpawnLocation"]["y"].is_number_unsigned());
	assert(levelJson["PlayerSpawnFacing"].is_number_unsigned());
	assert(levelJson["NumCols"].is_number_unsigned());
	assert(levelJson["NumRows"].is_number_unsigned());
	assert(levelJson["BackgroundTileset"].is_number_unsigned());
	assert(levelJson["Name"].is_string());

	LevelConfigData data;
	data.PlayerSpawnLocation = uvec2{ levelJson["PlayerSpawnLocation"]["x"], levelJson["PlayerSpawnLocation"]["y"] };
	data.PlayerSpawnFacing = levelJson["PlayerSpawnFacing"];
	data.NumCols = levelJson["NumCols"];
	data.NumRows = levelJson["NumRows"];
	data.BackgroundTileset = levelJson["BackgroundTileset"];
	data.Name = levelJson["Name"];
	const json& tileData = levelJson["TileData"];
	const json& monsterSpawnerData = levelJson["MonsterSpawners"];
	
	for (const json& monsterSpawner : monsterSpawnerData)
	{
		assert(monsterSpawner.is_object());
		MonsterSpawnerData monsterSpawnerData;
		assert(monsterSpawner["NumMonsters"].is_number_unsigned());
		assert(monsterSpawner["x"].is_number_unsigned());
		assert(monsterSpawner["y"].is_number_unsigned());
		monsterSpawnerData.NumMonsters = monsterSpawner["NumMonsters"];
		monsterSpawnerData.TilePosition = { monsterSpawner["x"], monsterSpawner["y"] };
		data.MonsterSpawners.push_back(monsterSpawnerData);
	}

	for (const json& tile : tileData)
	{
		assert(tile.is_number_unsigned());
		assert(tile < 256);
		data.TileData.push_back(tile);
	}

	json apples = levelJson["Apples"];
	for (const json& apple : apples)
	{
		data.Apples.push_back(uvec2{ apple["x"], apple["y"] });
	}
	return data;
}

void ConfigFile::RecreateMapMakerLevelsJSON()
{
	ConfigFileJSON["MapMakerLevels"].clear();
	for (const LevelConfigData& lvl : MapMakerLevelsConfigData)
	{

		json lvlObject = json::object();
		
		// Name
		lvlObject["Name"] = lvl.Name;

		// PlayerSpawnLocation
		json playerSpawnObject = json::object();
		playerSpawnObject["x"] = lvl.PlayerSpawnLocation.x;
		playerSpawnObject["y"] = lvl.PlayerSpawnLocation.y;
		lvlObject["PlayerSpawnLocation"] = playerSpawnObject;

		// PlayerSpawnFacing
		lvlObject["PlayerSpawnFacing"] = lvl.PlayerSpawnFacing;
		
		// BackgroundTileset
		lvlObject["BackgroundTileset"] = lvl.BackgroundTileset;
		
		// NumCols
		lvlObject["NumCols"] = lvl.NumCols;

		// NumRows
		lvlObject["NumRows"] = lvl.NumRows;

		// TileData
		json tileDataJson = json::array();
		for (u8 tile : lvl.TileData)
		{
			tileDataJson.push_back(tile);
		}
		lvlObject["TileData"] = tileDataJson;

		// Apples
		json applesJSON = json::array();
		for (const uvec2& apple : lvl.Apples)
		{
			json appleObj = json::object();
			appleObj["x"] = apple.x;
			appleObj["y"] = apple.y;
			applesJSON.push_back(appleObj);
		}
		lvlObject["Apples"] = applesJSON;

		// MonsterSpawners
		json monstersJSON = json::array();
		for (const MonsterSpawnerData& monsterSpawner : lvl.MonsterSpawners)
		{
			json monsterSpawnerObj = json::object();
			monsterSpawnerObj["x"] = monsterSpawner.TilePosition.x;
			monsterSpawnerObj["y"] = monsterSpawner.TilePosition.y;
			monsterSpawnerObj["NumMonsters"] = monsterSpawner.NumMonsters;
			monstersJSON.push_back(monsterSpawnerObj);
		}
		lvlObject["MonsterSpawners"] = monstersJSON;

		ConfigFileJSON["MapMakerLevels"].push_back(lvlObject);
	}
}

const std::string& ConfigFile::GetStringValue(const std::string& key) const
{
	assert(ConfigFileJSON[key].is_string());
	return ConfigFileJSON[key];
}

bool ConfigFile::GetBoolValue(const std::string& key) const
{
	assert(ConfigFileJSON[key].is_boolean());
	return ConfigFileJSON[key];
}

const LevelConfigData& ConfigFile::GetBlankLevelTemplate() const
{
	return BlankLevel;
}

void ConfigFile::SaveAfterMapMakerLevelsChange()
{
	RecreateMapMakerLevelsJSON();
	std::ofstream f(Filesystem->GetConfigFilePath());
	f << ConfigFileJSON.dump(4);
}

const u32 ConfigFile::GetUIntValue(const std::string& key) const
{
	assert(ConfigFileJSON[key].is_number_unsigned());
	return ConfigFileJSON[key];
}

const i32 ConfigFile::GetIntValue(const std::string& key) const
{
	assert(ConfigFileJSON[key].is_number_integer());
	return ConfigFileJSON[key];
}

const float ConfigFile::GetFloatValue(const std::string& key) const
{
	assert(ConfigFileJSON[key].is_number_float());
	return ConfigFileJSON[key];
}

const std::vector<LevelConfigData>& ConfigFile::GetLevelsConfigData() const
{
	return LevelsConfigData;
}

std::vector<LevelConfigData>& ConfigFile::GetMapMakerLevelsConfigData()
{
	return MapMakerLevelsConfigData;
}

const FontConfigData& ConfigFile::GetFontConfigData() const
{
	return FontConfigData;
}
