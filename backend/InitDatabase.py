import sqlite3
import os
os.remove("Backend.db")
con = sqlite3.connect("Backend.db")

cur = con.cursor()
cur.execute("CREATE TABLE HighScores(name, highscore, replayFile)")

cur.execute("delete from HighScores")
cur.execute("""
INSERT INTO HighScores VALUES
        ('Jim Marshall', 60000, 'None'),
        ('James Marshall', 50000, 'None'),
        ('Jimmy Marshall', 40000, 'None'),
        ('James Nicholas Marshall', 30000, 'None'),
        ('JNM', 20000, 'None'),
        ('Jim M Marshal', 10000, 'None')
""")
con.commit()
