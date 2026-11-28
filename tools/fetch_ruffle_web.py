"""Download and unpack the pinned Ruffle Self-Hosted runtime."""
import hashlib
from io import BytesIO
from pathlib import Path, PurePosixPath
import shutil
import sys
import tarfile
from urllib.request import urlopen


VERSION = "0.8.0"
SHA256 = "33e36db218a69723bf9263ccaddcabf2a8cf59d4634e28fa89a60d6969357363"


def fetch(destination):
    url = f"https://registry.npmjs.org/@ruffle-rs/ruffle/-/ruffle-{VERSION}.tgz"
    with urlopen(url, timeout=90) as response:
        bundle = response.read()
    if hashlib.sha256(bundle).hexdigest() != SHA256:
        raise ValueError("Ruffle Self-Hosted archive checksum mismatch")
    destination.mkdir(parents=True, exist_ok=True)
    with tarfile.open(fileobj=BytesIO(bundle), mode="r:gz") as archive:
        for entry in archive:
            name = PurePosixPath(entry.name)
            if name.parent != PurePosixPath("package") or not entry.isfile():
                continue
            if name.suffix not in {".js", ".wasm"} and not name.name.startswith("LICENSE"):
                continue
            with archive.extractfile(entry) as source, (destination / name.name).open("wb") as target:
                shutil.copyfileobj(source, target)
    if not (destination / "ruffle.js").is_file() or not any(destination.glob("*.wasm")):
        raise ValueError("Ruffle Self-Hosted archive is incomplete")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("Usage: fetch_ruffle_web.py <destination>")
    fetch(Path(sys.argv[1]))
