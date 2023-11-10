#pragma once
#include <string>
#include <memory>
#include "IFileSystem.h"

class FileSystem : public IFileSystem
{
public:
	FileSystem(const std::string& exePath);
	// Inherited via IFileSystem
	virtual const std::string& GetAssetFolderPath() const override;
	virtual const std::string& GetSpritesFolderPath() const override;
	virtual const std::string& GetConfigFilePath() const override;
	virtual const std::string& GetEnemyAIFilePath() const override;
	virtual const std::string& GetReplaysFolderPath() const override;

private:
	static std::string GetFolderOfFile(const std::string& filePath);
private:
	std::string ExePath;
	std::string AssetFolderPath;
	std::string SpriteFolderPath;
	std::string ConfigFilePath;
	std::string EnemyAIFilePath;
	std::string ReplaysFolderPath;
};