#pragma once
#include "IConfigFile.h"
#include <string>
#include <memory>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

class IFileSystem;
class ConfigFile : public IConfigFile
{
public:
	ConfigFile(const std::shared_ptr<IFileSystem>& fileSystem);
	// Inherited via IConfigFile
	virtual const BackgroundTileConfigData& GetBackgroundConfigData() const override;
	virtual const AnimationsConfigData& GetAnimationsConfigData() const override;
	virtual const std::vector<LevelConfigData>& GetLevelsConfigData() const override;
	virtual const FontConfigData& GetFontConfigData() const override;
	virtual const u32 GetUIntValue(const std::string& key) const override;
	virtual const i32 GetIntValue(const std::string& key) const override;
	virtual const float GetFloatValue(const std::string& key) const override;
	virtual const std::string& GetStringValue(const std::string& key) const override;
	virtual bool GetBoolValue(const std::string& key) const override;
private:
	void PopulateBackgroundConfigDataStruct();
	void PopulateAnimationsConfigDataStruct();
	void PopulateSpriteSheetBase(SpriteSheetConfigData& spriteSheet, const std::string& objectName);
	void PopulateLevelsConfigData();
	void PopulateFontConfigDataStruct();
private:
	json ConfigFileJSON;
	BackgroundTileConfigData BackgroundConfigData;
	AnimationsConfigData AnimationConfigData;
	std::vector<LevelConfigData> LevelsConfigData;
	std::shared_ptr<IFileSystem> Filesystem;
	FontConfigData FontConfigData;
};