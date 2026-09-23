package models

//fomoxa:model codec=edge,unity
type DeviceState struct {
	ID          uint32  `fomoxa:"u32" codec:"edge,unity"`
	Temperature float32 `fomoxa:"f32" codec:"edge"`
	DisplayName string  `fomoxa:"string" codec:"unity"`
	Unrouted    uint32  `fomoxa:"u32"`
	Cache       string
}

//fomoxa:model codec=edge
type EveryPrimitive struct {
	Flag     bool    `fomoxa:"bool" codec:"edge"`
	Tiny     int8    `fomoxa:"i8" codec:"edge"`
	Byte     uint8   `fomoxa:"u8" codec:"edge"`
	Small    int16   `fomoxa:"i16" codec:"edge"`
	Port     uint16  `fomoxa:"u16" codec:"edge"`
	Offset   int32   `fomoxa:"i32" codec:"edge"`
	Count    uint32  `fomoxa:"u32" codec:"edge"`
	Delta    int64   `fomoxa:"i64" codec:"edge"`
	Sequence uint64  `fomoxa:"u64" codec:"edge"`
	Ratio    float32 `fomoxa:"f32" codec:"edge"`
	Precise  float64 `fomoxa:"f64" codec:"edge"`
	Label    string  `fomoxa:"string" codec:"edge"`
	Blob     []byte  `fomoxa:"bytes" codec:"edge"`
}

//fomoxa:model codec=edge
type PlayerInfo struct {
	Level uint32 `fomoxa:"u32" codec:"edge"`
}

//fomoxa:model codec=edge
type Player struct {
	ID uint32  `fomoxa:"u32" codec:"edge"`
	X  float32 `fomoxa:"f32" codec:"edge"`
	Y  float32 `fomoxa:"f32" codec:"edge"`
}

//fomoxa:model codec=edge
type Team struct {
	Captain PlayerInfo   `fomoxa:"PlayerInfo" codec:"edge"`
	Tags    []string     `fomoxa:"Array<string>" codec:"edge"`
	Scores  []uint32     `fomoxa:"Array<u32>" codec:"edge"`
	Roster  []PlayerInfo `fomoxa:"Array<PlayerInfo>" codec:"edge"`
}
