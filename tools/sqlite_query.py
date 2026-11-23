import sqlite3
import sys
from pathlib import Path


def main():
    database, sql, mode = sys.argv[1:]
    uri = Path(database).resolve().as_uri() + "?mode=" + mode
    with sqlite3.connect(uri, uri=True) as connection:
        for row in connection.execute(sql):
            print(row[0])


if __name__ == "__main__":
    main()
