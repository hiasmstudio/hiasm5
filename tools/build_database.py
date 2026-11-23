"""Build a staged SQLite database without modifying the source catalog."""
import shutil
import sqlite3
import sys
import tempfile
from contextlib import closing
from pathlib import Path


def build_database(output, sources):
    output.parent.mkdir(parents=True, exist_ok=True)
    # A file in the destination directory inherits its Windows access permissions.
    with tempfile.NamedTemporaryFile(dir=output.parent, delete=False) as temp:
        staged = Path(temp.name)
    try:
        if sources[0].suffix == ".db":
            shutil.copyfile(sources[0], staged)
            sources = sources[1:]
        with closing(sqlite3.connect(staged)) as db:
            for script in sources:
                db.executescript(script.read_text(encoding="utf-8-sig"))
            if db.execute("PRAGMA integrity_check").fetchall() != [("ok",)]:
                raise ValueError(f"Invalid database: {output}")
        staged.replace(output)
    finally:
        staged.unlink(missing_ok=True)


if __name__ == "__main__":
    build_database(Path(sys.argv[1]), list(map(Path, sys.argv[2:])))
