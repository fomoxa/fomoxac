# player_info.gd is part of the annotated source the GDScript integration
# tests exercise - the GDScript counterpart of the PlayerInfo class in
# tests/fixtures-cs/src/models/Player.cs, split into its own file since a
# `.gd` file may declare only one `class_name`.

# A field whose network type is another model.
# fomoxa:model codec=edge
class_name PlayerInfo

# fomoxa:u32 codec=edge
var level: int = 0
