import sqlite3
import json
from flask import Flask, request, jsonify
from struct import *
import codecs
import os
import subprocess
import pathlib
import shutil

app = Flask(__name__)

GameJsonConfigPath = "../mrdo/assets/json/config.json"

ReplayValidatorPath = "../build/build_results/bin/Debug/"
f = open(GameJsonConfigPath)
config = json.load(f)
f.close()

Version = config["Version"]
HighScoreTableName = "HighScores"
HighScoreReplayColumnName = "replayFile"
HighScoreIDColumnName = "id"
HighScoreColumnName = config["HighscoreDataHighScoreColumnName"]
NameColumnName = config["HighscoreDataNameColumnName"]#"name"
GetHighScoresRoute = config["BackendServerHighScoresRoute"]
SubmitHighScoreRoute = config["BackendServerSubmitHighScoreRoute"]
ServerMaxHighScores = config["HighScoreTableSize"]
GetReplaysRoute = config["GetReplayRoute"]
ReplayFolder = "Replays"
ValidReplaysSubFolder = "Valid"
InvalidReplaysSubFolder = "Invalid"

ScriptPath = pathlib.Path(__file__).parent.resolve()
ReplayFolderPath = os.path.join(ScriptPath, ReplayFolder)
ValidReplaysFolderPath = os.path.join(ReplayFolderPath, ValidReplaysSubFolder)
InValidReplaysSubFolderPath = os.path.join(ReplayFolder, InvalidReplaysSubFolder)

RepoPath = pathlib.Path(__file__).parent.parent.resolve()
ReplayValidatorPath = os.path.join(RepoPath, "build\\build_results\\bin\\Debug", "ReplayValidator.exe")

print(ScriptPath)
print(ReplayFolderPath)
print(ReplayValidatorPath)

GetHighScoresForClientQuery = f"""
SELECT
 {NameColumnName}, {HighScoreColumnName}, {HighScoreIDColumnName}, {HighScoreReplayColumnName}
FROM
 {HighScoreTableName} 
ORDER BY 
 {HighScoreColumnName} DESC
"""

DeleteLowestHighScoreQuery = f"""
DELETE FROM
  {HighScoreTableName}
WHERE
  {HighScoreColumnName} = (SELECT MIN({HighScoreColumnName}) FROM {HighScoreTableName} LIMIT 1);
"""

def get_highScore_by_ID_query_string(id):
	return f"""
	SELECT FROM
		{HighScoreTableName}
	WHERE
		{HighScoreIDColumnName} = {id} LIMIT 1
	"""

bCachedHighScoresDirty = True
CachedHighScores = None;

def get_server_replay_file_name(header):
	return header["Name"] + str(header["Score"]) +"serverVersion" + str(Version) + ".replay"

def validate_replay(data, header):
	testFileName = get_server_replay_file_name(header)
	testFilePath = os.path.join(ReplayFolderPath, testFileName)
	with open(testFilePath, "wb+") as binary_file:
		binary_file.write(data)
	result = subprocess.run([ReplayValidatorPath, testFilePath, str(header["Score"])], shell=True, capture_output=True, text=True)
	print(result.stdout)
	print(f"Return code: {result.returncode}")
	replayValid = result.returncode == 0
	destinationFolder = ""
	if replayValid:
		destinationFolder = ValidReplaysFolderPath
	else:
		destinationFolder = InValidReplaysSubFolderPath
	destination = os.path.join(destinationFolder, testFileName)
	shutil.move(testFilePath, destinationFolder)
	header["ServerReplayFilePath"] = destination
	return replayValid

def get_high_scores_internal():
	global GetHighScoresQuery
	global CachedHighScores
	global bCachedHighScoresDirty
	if CachedHighScores != None and (not bCachedHighScoresDirty):
		return CachedHighScores
	else:
		con = sqlite3.connect("Backend.db")
		cur = con.cursor()
		CachedHighScores = [{NameColumnName : row[0], HighScoreColumnName : row[1], HighScoreIDColumnName: row[2], HighScoreReplayColumnName : row[3]} for row in cur.execute(GetHighScoresForClientQuery)]
		bCachedHighScoresDirty = False
		return CachedHighScores

def char_buffer_to_string(buf):
	return str(buf.split(b'\0',1)[0].decode('utf-8'))

def parse_replay_file_header(data):
	header = unpack_from("ixxxxQ32sIIQ",data) # I don't know whether this would be portable - works on 32 bit python 3.7 + 64 bit C++ .exe
	return {
		"Version" : header[0],
		"NumSnaps" : header[1],
		"Name" : char_buffer_to_string(header[2]),
		"Score" : header[3]
	}



def add_high_score(header):
	hs = get_high_scores_internal()
	if len(hs) == ServerMaxHighScores:
		print(f"max high scores stored on server ({ServerMaxHighScores}) exceeded, deleting lowest")
		cur.execute(DeleteLowestHighScoreQuery);
	con = sqlite3.connect("Backend.db")
	cur = con.cursor()
	cur.execute(f"""
		INSERT INTO HighScores ({NameColumnName}, {HighScoreColumnName}, {HighScoreReplayColumnName}) VALUES
		("{header["Name"]}", {header["Score"]}, "{header["ServerReplayFilePath"]}")
		""")
	con.commit()
	bCachedHighScoresDirty = True


def should_add_high_score_if_replay_valid(replayHeader):
	highScores = get_high_scores_internal()
	if len(highScores) < ServerMaxHighScores:
		return True
	else:
		for hs in highScores:
			if replayHeader["Score"] >= hs[HighScoreColumnName]:
				return True;
	return False

def delete_high_score_for_client(hs):
	hs.pop(HighScoreReplayColumnName, None)
	return hs


@app.route(GetHighScoresRoute, methods = ['GET'])
def get_high_scores():
	return json.dumps(list(map(delete_high_score_for_client , get_high_scores_internal())))

@app.route("/SubmitReplay", methods=["POST"])
def post_replay():
	if request.content_type == "application/octet-stream":
		data = request.get_data()
		print("length of input data: {}".format(len(data)))
		header = parse_replay_file_header(data)
		print(header)
		# is the replay high enough to make it onto the leaderboard?
		# This is checked on client too before they submit against leaderboard data gotten when the game starts up
		# if the client can't get backend server data on start up it will just never post replay files, until it restarts and can 
		# connect successfully.
		if should_add_high_score_if_replay_valid(header):
			print("should add new high score if replay is valid")
			# validate the replay by playing through a headless stripped down version of the game with uncapped frame rate.
			# If the score from playing the replay matches the claimed one submitted then the replay is valid.
			# I reason that the effort required to craft a replay file and spoof a high score is prohibitively high
			# so as to deter most cheating attempts. Also changes to the config file on clients will cause the replay
			# to desync and the number of scored points to be different. Perhaps another metric such as game ticks until game over
			# would make this more robust. 
			# valid_replay also saves the replay to file and sorts it into a valid or invalid folder, adding the path of the saved
			# file to the header passed in.
			if validate_replay(data, header):
				# commit new high score to database if valid
				add_high_score(header)
				return jsonify({'msg': "success"})
			else:
				return jsonify({'msg': "replay validation failed! CHEAT!"})
		# if we're here it should mean that a player submitted what they thought was a new
		# high score only to find out that new high score(s) have been submitted since they started playing and now 
		# their's is no longer a high score - gutted.
		print("shouldn't add new high score")
		return jsonify({'msg': "Gee kid that's a real tough break - I guess you weren't cut out for the big leagues this time - but keep at it champ"})

	else:
		return jsonify({'msg': "415 Unsupported Media Type ;)"})

@app.route(f'{GetReplaysRoute}/<id>')
def get_replay(id):
	global app
	hs = get_high_scores_internal()
	print(hs)
	first = next(filter(lambda h: h[HighScoreIDColumnName] == int(id), hs), None)
	if first == None:
		return jsonify({'msg' : "high score id not found"})
	print("first")
	print(first)
	replayFileBytes = None
	with open(first[HighScoreReplayColumnName], "rb") as binary_file:
		replayFileBytes = binary_file.read()
	return app.send_file(
    	replayFileBytes,
	    mimetype='application/octet-stream',
	    as_attachment=True,
	)

if __name__ == '__main__':
	app.run(debug=true, port=5000)