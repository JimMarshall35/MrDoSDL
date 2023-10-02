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
	PopulateLevelsConfigData();
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
}

void ConfigFile::PopulateSpriteSheetBase(SpriteSheetConfigData& spriteSheet, const std::string& objectName)
{
	spriteSheet.NumCols = ConfigFileJSON[objectName]["NumCols"];
	spriteSheet.NumRows = ConfigFileJSON[objectName]["NumRows"];
	spriteSheet.TileSize = ConfigFileJSON[objectName]["TileSize"];

	spriteSheet.SpriteSheetAssetPath = Filesystem->GetSpritesFolderPath() + "\\" + ConfigFileJSON[objectName]["AssetPath"].template get<std::string>();

}

void ConfigFile::PopulateLevelsConfigData()
{
	json levelsArray = ConfigFileJSON["Levels"];
	assert(levelsArray.is_array());
	LevelsConfigData.clear();
	for (const json& level : levelsArray)
	{
		LevelConfigData data;
		data.PlayerSpawnLocation = uvec2{ level["PlayerSpawnLocation"]["x"], level["PlayerSpawnLocation"]["y"] };
		data.PlayerSpawnFacing = level["PlayerSpawnFacing"];
		data.NumCols = level["NumCols"];
		data.NumRows = level["NumRows"];
		data.BackgroundTileset = level["BackgroundTileset"];
		json tileData = level["TileData"];
		assert(levelsArray.is_array());
		for (const json& tile : tileData)
		{
			assert(tile.is_number_unsigned());
			assert(tile < 256);
			data.TileData.push_back(tile);
		}
		LevelsConfigData.push_back(data);
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
