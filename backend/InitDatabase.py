import sqlite3
import os
os.remove("Backend.db")
con = sqlite3.connect("Backend.db")

cur = con.cursor()
cur.execute("CREATE TABLE HighScores(id INTEGER PRIMARY KEY AUTOINCREMENT, name, highscore, replayFile)")

cur.execute("delete from HighScores")
# {"name": "JimmyM", "highscore": 16100}, 
cur.execute("""
INSERT INTO HighScores ( name, highscore, replayFile )
        VALUES ('JimmyM', 16100, 'C:\\Users\\User1\\source\\repos\\MrDoSDL\\backend\\Replays\\Valid\\JimmyM16100serverVersion0.replay')
""")
con.commit()
