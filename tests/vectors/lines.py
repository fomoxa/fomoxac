import json
import pathlib
import sys

path = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else pathlib.Path(__file__).with_name("fomoxa-vectors.json")
vectors = json.loads(path.read_text(encoding="utf-8"))


def compact(text):
    digits = "".join(text.split())
    return digits if digits else "-"


for name, message in sorted(vectors["messages"].items()):
    fingerprint = message["fingerprint"].removeprefix("sha256:")[:16]
    print("message", name, message["id"].removeprefix("0x"), fingerprint)

for vector in vectors["accept"]:
    print("accept", vector["name"], vector["message"], compact(vector["hex"]), compact(vector["reencode"]))

for vector in vectors["reject"]:
    print("reject", vector["name"], vector["message"], compact(vector["hex"]), vector["error"])
