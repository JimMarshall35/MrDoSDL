#pragma once
//#include <curl/curl.h>
#include <memory>
#include "CommonTypedefs.h"
#include <string>
#include <curl/curl.h>
#include <vector>
#include <string>
#include <sstream>

class IConfigFile;

struct HighScore
{
	std::string Name;
	u32 Score;
};


class IBackendClient
{
public:
	virtual void PopulateHighScores() = 0; 
	virtual const HighScore& GetHighScore(int number) = 0;
	virtual size_t GetNumHighScores() = 0;
	virtual void SubmitPossibleHighScore(u32 score) = 0;
};

class BackendClient : public IBackendClient
{
public:
	BackendClient(const std::shared_ptr<IConfigFile>& configFIle);
	~BackendClient();
	virtual void PopulateHighScores() override;
	virtual const HighScore& GetHighScore(int number) override;
	virtual size_t GetNumHighScores() override;
	virtual void SubmitPossibleHighScore(u32 score) override;

private:
	void PostPossibleHighScoreToServer(u32 score);

private:
	static std::string MakeURL(const std::string& base, u32 port, const std::string& route);
	static size_t WriteHighScoresDataCallback(char* ptr, size_t size, size_t nmemb, void* userdata);

private:
	std::shared_ptr<IConfigFile> ConfigFile;
	std::vector<char> RecievedBuffer;
	std::string BaseURL;
	std::string GetHighScoresRoute;
	std::string SubmitHighScoreRoute;
	u32 Port;
	std::string GetHighScoresURL;
	std::string SubmitHighScoreURL;
	CURL* Curl;
	size_t HighScoreTableSize;
	std::unique_ptr<HighScore[]> HighScoreTable;
	size_t NumLoadedHighScores;
	std::ostringstream Response;
	std::string HighScoreJSONResponseNameField;
	std::string HighScoreJSONResponseHSField;
	std::string PlayerName;

	u8 bCurlInitialised : 1;
	u8 bInitialConnectionSuccessful : 1;
};