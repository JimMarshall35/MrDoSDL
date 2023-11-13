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
HighScoreColumnName = config["HighscoreDataHighScoreColumnName"]
NameColumnName = config["HighscoreDataNameColumnName"]#"name"
GetHighScoresRoute = config["BackendServerHighScoresRoute"]
SubmitHighScoreRoute = config["BackendServerSubmitHighScoreRoute"]
ServerMaxHighScores = config["HighScoreTableSize"]
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
 {NameColumnName}, {HighScoreColumnName} 
FROM
 {HighScoreTableName} 
ORDER BY 
 {HighScoreColumnName} DESC
"""

GetAllHighScoresQuery = f"""
SELECT
 {NameColumnName}, {HighScoreColumnName} 
FROM
 {HighScoreTableName} 
ORDER BY 
 {HighScoreColumnName} ASC
"""

DeleteLowestHighScoreQuery = f"""
DELETE FROM
  {HighScoreTableName}
WHERE
  {HighScoreColumnName} = (SELECT MIN({HighScoreColumnName}) FROM {HighScoreTableName} LIMIT 1);
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
		CachedHighScores = [{NameColumnName : row[0], HighScoreColumnName : row[1]} for row in cur.execute(GetHighScoresForClientQuery)]
		bCachedHighScoresDirty = False
		return CachedHighScores

def char_buffer_to_string(buf):
	return str(buf.split(b'\0',1)[0].decode('utf-8'))

def parse_replay_file_header(data):
	header = unpack_from("ixxxxQ32sIIQ",data) # this is a bit fucked up because I have 32 bit python and this refers to a 64 bit struct
	return {
		"Version" : header[0],
		"NumSnaps" : header[1],
		"Name" : char_buffer_to_string(header[2]),#codecs.decode(header[2], 'utf-8'),
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
		INSERT INTO HighScores VALUES
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

@app.route(GetHighScoresRoute, methods = ['GET'])
def get_high_scores():
	return json.dumps(get_high_scores_internal())

@app.route("/SubmitReplay", methods=["POST"])
def post_replay():
	if request.content_type == "application/octet-stream":
		data = request.get_data()
		print("length of input data: {}".format(len(data)))
		header = parse_replay_file_header(data)
		print(header)
		if should_add_high_score_if_replay_valid(header):
			print("should add new high score if replay is valid")
			if validate_replay(data, header):
				add_high_score(header)
				return jsonify({'msg': "success"})
			else:
				return jsonify({'msg': "replay validation failed! CHEAT!"})
		else:
			print("shouldn't add new high score")
		return jsonify({'msg': "you were wrong to think you had a shot at the big leagues"})

	else:
		return jsonify({'msg': "415 Unsupported Media Type ;)"})

if __name__ == '__main__':
	app.run(debug=true, port=5000)