#include "BackendClient.h"
#include "IConfigFile.h"
#include <nlohmann/json.hpp>
#include <iostream>
using json = nlohmann::json;


BackendClient::BackendClient(const std::shared_ptr<IConfigFile>& configFIle)
	:ConfigFile(configFIle),
	RecievedBuffer(),
	BaseURL(),
	GetHighScoresRoute(),
	SubmitHighScoreRoute(),
	SubmitHighScoreReplayRoute(),
	Port(configFIle->GetUIntValue("BackendServerConnectionPort")),
	GetHighScoresURL(),
	SubmitHighScoreURL(),
	SubmitHighScoreReplayURL(),
	Curl(curl_easy_init()),
	HighScoreTableSize(configFIle->GetUIntValue("HighScoreTableSize")),
	HighScoreTable(std::make_unique<HighScore[]>(HighScoreTableSize)),
	NumLoadedHighScores(0),
	Response(),
	HighScoreJSONResponseNameField(),
	HighScoreJSONResponseHSField(),
	PlayerName(),
	bCurlInitialised(Curl != nullptr),
	bInitialConnectionSuccessful(false)
{
	BaseURL = configFIle->GetStringValue("BackendServerURL");
	GetHighScoresRoute = configFIle->GetStringValue("BackendServerHighScoresRoute");
	SubmitHighScoreRoute = configFIle->GetStringValue("BackendServerSubmitHighScoreRoute");
	SubmitHighScoreReplayRoute = configFIle->GetStringValue("SubmitReplayRoute"),
	GetHighScoresURL = MakeURL(BaseURL, Port, GetHighScoresRoute);
	SubmitHighScoreURL = MakeURL(BaseURL, Port, SubmitHighScoreRoute);
	SubmitHighScoreReplayURL = MakeURL(BaseURL, Port, SubmitHighScoreReplayRoute);
	HighScoreJSONResponseNameField = configFIle->GetStringValue("HighscoreDataNameColumnName");
	HighScoreJSONResponseHSField = configFIle->GetStringValue("HighscoreDataHighScoreColumnName");
	PlayerName = configFIle->GetStringValue("PlayerName");
}

BackendClient::~BackendClient()
{
	if (bCurlInitialised)
	{
		curl_easy_cleanup(Curl);
	}
}

void BackendClient::PopulateHighScores()
{
	if (!bCurlInitialised)
	{
		std::cout << "[PopulateHighScores] CURL not initialised, loading offline high scores from config file\n";
		ConfigFile->PopulateHighScoreTableWithOfflineScores(HighScoreTable.get(), NumLoadedHighScores);
		return;
	}

	curl_easy_setopt(Curl, CURLOPT_URL, GetHighScoresURL.c_str());
	 
	/* Use HTTP/3 but fallback to earlier HTTP if necessary */
	curl_easy_setopt(Curl, CURLOPT_HTTP_VERSION,
		(long)CURL_HTTP_VERSION_3);

	curl_easy_setopt(Curl, CURLOPT_WRITEFUNCTION, &BackendClient::WriteHighScoresDataCallback);
	curl_easy_setopt(Curl, CURLOPT_WRITEDATA, this);

	/* Perform the request, res will get the return code */
	CURLcode res = curl_easy_perform(Curl);
	/* Check for errors */
	if (res != CURLE_OK)
	{
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res));
		ConfigFile->PopulateHighScoreTableWithOfflineScores(HighScoreTable.get(), NumLoadedHighScores);
		return;
	}

	json parsed = json::parse(Response.str());
	assert(parsed.is_array());
	assert(parsed.size() <= HighScoreTableSize);
	NumLoadedHighScores = 0;
	for (const json& entry : parsed)
	{
		assert(entry[HighScoreJSONResponseNameField].is_string());
		assert(entry[HighScoreJSONResponseHSField].is_number_unsigned());
		HighScoreTable[NumLoadedHighScores++] = HighScore
		{
			entry[HighScoreJSONResponseNameField],
			entry[HighScoreJSONResponseHSField]
		};
	}
	bInitialConnectionSuccessful = true;

}

const HighScore& BackendClient::GetHighScore(int number)
{
	assert(number < NumLoadedHighScores);
	assert(number >= 0);
	return HighScoreTable[number];
}

size_t BackendClient::GetNumHighScores()
{
	return NumLoadedHighScores;
}

void BackendClient::SubmitPossibleHighScore(u32 score, char* replayData, size_t replayDataSize)
{
	for (int i = 0; i < NumLoadedHighScores; i++)
	{
		if (score > HighScoreTable[i].Score)
		{
			if (bCurlInitialised && bInitialConnectionSuccessful)
			{
				//PostPossibleHighScoreToServer(score);
				SubmitPossibleHighScoreReplayToServer(replayData, replayDataSize);
			}
			else
			{
				// todo: update offline high scores in json file and update in memory high score table
			}
			return;
		}
	}
}

void BackendClient::SubmitPossibleHighScoreReplayToServer(char* replayData, size_t replaySize)
{
	curl_easy_setopt(Curl, CURLOPT_URL, SubmitHighScoreReplayURL.c_str());
	struct curl_slist* headers = NULL;
	headers = curl_slist_append(headers, "Content-Type:application/octet-stream");
	curl_easy_setopt(Curl, CURLOPT_HEADER, true);
	curl_easy_setopt(Curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(Curl, CURLOPT_POST, true);
	curl_easy_setopt(Curl, CURLOPT_POSTFIELDSIZE_LARGE, replaySize);
	curl_easy_setopt(Curl, CURLOPT_POSTFIELDS, replayData);
	CURLcode res = curl_easy_perform(Curl);

	if (res != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
	}
}

void BackendClient::PostPossibleHighScoreToServer(u32 score)
{
	curl_easy_setopt(Curl, CURLOPT_URL, SubmitHighScoreURL.c_str());

	/* Use HTTP/3 but fallback to earlier HTTP if necessary */
	curl_easy_setopt(Curl, CURLOPT_HTTP_VERSION,
		(long)CURL_HTTP_VERSION_3);

	static std::string postArgs = HighScoreJSONResponseNameField + "=" + PlayerName + "&" + HighScoreJSONResponseHSField + "=" + std::to_string(score);
	curl_easy_setopt(Curl, CURLOPT_POSTFIELDSIZE, postArgs.size());
	curl_easy_setopt(Curl, CURLOPT_POSTFIELDS, postArgs.c_str());
	/* Perform the request, res will get the return code */
	CURLcode res = curl_easy_perform(Curl);
	/* Check for errors */
	if (res != CURLE_OK)
	{
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res));
	}
}

std::string BackendClient::MakeURL(const std::string& base, u32 port, const std::string& route)
{
	return base + ":" + std::to_string(port) + route;
}

size_t BackendClient::WriteHighScoresDataCallback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
	BackendClient* client = (BackendClient*)userdata;
	for (int i = 0; i < nmemb; i++)
	{
		client->Response << ptr[i];
	}
	return nmemb;
}
