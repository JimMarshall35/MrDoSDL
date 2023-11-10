import sqlite3
import os
os.remove("Backend.db")
con = sqlite3.connect("Backend.db")

cur = con.cursor()
cur.execute("CREATE TABLE HighScores(name, highscore)")

cur.execute("delete from HighScores")
cur.execute("""
INSERT INTO HighScores VALUES
        ('Jim Marshall', 60000),
        ('James Marshall', 50000),
        ('Jimmy Marshall', 40000),
        ('James Nicholas Marshall', 30000),
        ('JNM', 20000),
        ('Jim M Marshal', 10000)
""")
con.commit()
