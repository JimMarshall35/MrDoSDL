#pragma once
#include "IConfigFile.h"
#include <string>
#include <memory>
#include <nlohmann/json.hpp>
using json = nlohmann::json;


class ConfigFile : public IConfigFile
{
public:
	ConfigFile(const std::shared_ptr<IFileSystem>& fileSystem);
	// Inherited via IConfigFile
	virtual const BackgroundTileConfigData& GetBackgroundConfigData() const override;
	virtual const AnimationsConfigData& GetAnimationsConfigData() const override;
	virtual const std::vector<LevelConfigData>& GetLevelsConfigData() const override;
	virtual std::vector<LevelConfigData>& GetMapMakerLevelsConfigData() override;
	virtual const FontConfigData& GetFontConfigData() const override;
	virtual const u32 GetUIntValue(const std::string& key) const override;
	virtual const i32 GetIntValue(const std::string& key) const override;
	virtual const float GetFloatValue(const std::string& key) const override;
	virtual std::string GetStringValue(const std::string& key) const override;
	virtual bool GetBoolValue(const std::string& key) const override;
	virtual const LevelConfigData& GetBlankLevelTemplate() const override;
	virtual void SaveAfterMapMakerLevelsChange() override;
	virtual int GetArraySize(const std::string& key) const;
	virtual int GetIntArrayValue(const std::string& key, size_t index) const;
	virtual float GetFloatArrayValue(const std::string& key, size_t index) const;
	virtual const IFileSystem* GetFileSystem() const override { return Filesystem.get(); }
	virtual void PopulateHighScoreTableWithOfflineScores(HighScore* tableToWrite, size_t& outNumWritten) const override;
	virtual vec2 GetVec2Value(const std::string& key) const;

private:
	void PopulateBackgroundConfigDataStruct();
	void PopulateAnimationsConfigDataStruct();
	void PopulateSpriteSheetBase(SpriteSheetConfigData& spriteSheet, const std::string& objectName);
	void PopulateLevelsConfigData(std::vector<LevelConfigData>& vectorToAddTo, const std::string& configFileArrayName);
	void PopulateFontConfigDataStruct();
	static LevelConfigData ParseLevelConfigData(const json& levelJson);
	void RecreateMapMakerLevelsJSON();
private:
	json ConfigFileJSON;
	BackgroundTileConfigData BackgroundConfigData;
	AnimationsConfigData AnimationConfigData;
	std::vector<LevelConfigData> LevelsConfigData;
	std::vector<LevelConfigData> MapMakerLevelsConfigData;
	LevelConfigData BlankLevel;
	std::shared_ptr<IFileSystem> Filesystem;
	FontConfigData FontConfigData;

};