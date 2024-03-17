import sqlite3
import os

query = f"""
DELETE FROM HighScores
WHERE replayFile = 'None';

"""

con = sqlite3.connect("Backend.db")
cur = con.cursor()
cur.execute(query)
con.commit()
