# player.gd is the model RFC-0002 §9.1's version-skew tests decode: three
# fields, and a version that appends a fourth - the GDScript counterpart of
# the Player class in tests/fixtures-cs/src/models/Player.cs.
# fomoxa:model codec=edge,unity
class_name Player

# fomoxa:u32 codec=edge,unity
var id: int = 0

# fomoxa:f32 codec=edge
var x: float = 0.0

# fomoxa:f32 codec=edge
var y: float = 0.0

# A network field in no codec: it is written by none of them.
# fomoxa:u32
var unrouted: int = 0

# Not a network field at all. Logic and caches stay off the wire.
var cache: String = ""
