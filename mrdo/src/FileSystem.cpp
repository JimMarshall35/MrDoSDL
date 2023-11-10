#include "FileSystem.h"

FileSystem::FileSystem(const std::string& exePath)
	:ExePath(GetFolderOfFile(exePath)),
	AssetFolderPath(ExePath + "assets"),
	SpriteFolderPath(ExePath + "assets\\sprites"),
	ConfigFilePath(ExePath + "assets\\json\\config.json"),
	EnemyAIFilePath(ExePath + "assets\\forth\\EnemyAi.fs"),
	ReplaysFolderPath(AssetFolderPath + "\\replays\\")
{

}

const std::string& FileSystem::GetAssetFolderPath() const
{
	return AssetFolderPath;
}

const std::string& FileSystem::GetSpritesFolderPath() const
{
	return SpriteFolderPath;
}

const std::string& FileSystem::GetConfigFilePath() const
{
	return ConfigFilePath;
}

const std::string& FileSystem::GetEnemyAIFilePath() const
{
	return EnemyAIFilePath;
}

const std::string& FileSystem::GetReplaysFolderPath() const
{
	return ReplaysFolderPath;
}

std::string FileSystem::GetFolderOfFile(const std::string& filePath)
{
	std::string rVal = filePath;
	while (rVal[rVal.length() - 1] != '\\')
	{
		rVal.erase(rVal.end() - 1);
	}
	return rVal;
}
