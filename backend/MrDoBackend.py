import sqlite3
import json
from flask import Flask, request

app = Flask(__name__)

GameJsonConfigPath = "../mrdo/assets/json/config.json"


f = open(GameJsonConfigPath)
config = json.load(f)
f.close()

HighScoreTableName = "HighScores"
HighScoreColumnName = config["HighscoreDataHighScoreColumnName"]
NameColumnName = config["HighscoreDataNameColumnName"]#"name"
ClientHighScoreTableSize = config["ClientHighScoreTableSize"]
GetHighScoresRoute = config["BackendServerHighScoresRoute"]
SubmitHighScoreRoute = config["BackendServerSubmitHighScoreRoute"]
ServerMaxHighScores = config["ServerMaxHighScoresStored"]

GetHighScoresForClientQuery = f"""
SELECT
 {NameColumnName}, {HighScoreColumnName} 
FROM
 {HighScoreTableName} 
ORDER BY 
 {HighScoreColumnName} DESC
LIMIT 
 {ClientHighScoreTableSize}
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



@app.route(GetHighScoresRoute, methods = ['GET'])
def get_high_scores():
	global GetHighScoresQuery
	con = sqlite3.connect("Backend.db")
	cur = con.cursor()
	highScores = [{NameColumnName : row[0], HighScoreColumnName : row[1]} for row in cur.execute(GetHighScoresForClientQuery)]
	return json.dumps(highScores)
		
def add_high_score(submittedScore, cur, highScores):
	if len(highScores) + 1 > ServerMaxHighScores:
		print(f"max high scores stored on server ({ServerMaxHighScores}) exceeded, deleting lowest")
		cur.execute(DeleteLowestHighScoreQuery);
	cur.execute(f"""
		INSERT INTO HighScores VALUES
		("{submittedScore[NameColumnName]}", {submittedScore[HighScoreColumnName]})
		""")

@app.route(f"{SubmitHighScoreRoute}", methods = ['POST'])
def post_high_score():
	submittedScore = { NameColumnName : request.form.get(NameColumnName), HighScoreColumnName : request.form.get(HighScoreColumnName)}
	print("high score submitted")
	print(submittedScore)

	con = sqlite3.connect("Backend.db")
	cur = con.cursor()
	highScores = [{NameColumnName : row[0], HighScoreColumnName : row[1]} for row in cur.execute(GetAllHighScoresQuery)]
	for hs in highScores:
		if int(submittedScore[HighScoreColumnName]) > int(hs[HighScoreColumnName]):
			print(f"adding high score: {submittedScore}")
			add_high_score(submittedScore, cur, highScores)
			con.commit()
			break
	return "OK"

if __name__ == '__main__':
	app.run(debug=true, port=5000)