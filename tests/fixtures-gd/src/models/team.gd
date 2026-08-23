# team.gd holds composites: an array of primitives, an array of models, and a
# nested model - the GDScript counterpart of the Team class in
# tests/fixtures-cs/src/models/Player.cs.
# fomoxa:model codec=edge
class_name Team

# fomoxa:PlayerInfo codec=edge
var captain: PlayerInfo = PlayerInfo.new()

# fomoxa:Array<string> codec=edge
var tags: Array = []

# fomoxa:Array<u32> codec=edge
var scores: Array = []

# fomoxa:Array<PlayerInfo> codec=edge
var roster: Array = []
