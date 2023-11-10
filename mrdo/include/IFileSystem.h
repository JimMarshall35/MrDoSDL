#pragma once 
#include <string>
class IFileSystem
{
public:
	virtual const std::string& GetAssetFolderPath() const = 0;
	virtual const std::string& GetSpritesFolderPath() const = 0;
	virtual const std::string& GetConfigFilePath() const = 0;
	virtual const std::string& GetEnemyAIFilePath() const = 0;
	virtual const std::string& GetReplaysFolderPath() const = 0;
};