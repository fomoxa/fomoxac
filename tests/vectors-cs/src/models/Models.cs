using System.Collections.Generic;

namespace Models;

[Network]
[Codec("edge", "unity")]
public class DeviceState
{
    [Network("u32")]
    [Codec("edge", "unity")]
    public uint Id { get; set; }

    [Network("f32")]
    [Codec("edge")]
    public float Temperature { get; set; }

    [Network("string")]
    [Codec("unity")]
    public string DisplayName { get; set; } = "";

    [Network("u32")]
    public uint Unrouted { get; set; }

    public string Cache { get; set; } = "";
}

[Network]
[Codec("edge")]
public class EveryPrimitive
{
    [Network("bool")]
    [Codec("edge")]
    public bool Flag { get; set; }

    [Network("i8")]
    [Codec("edge")]
    public sbyte Tiny { get; set; }

    [Network("u8")]
    [Codec("edge")]
    public byte Byte { get; set; }

    [Network("i16")]
    [Codec("edge")]
    public short Small { get; set; }

    [Network("u16")]
    [Codec("edge")]
    public ushort Port { get; set; }

    [Network("i32")]
    [Codec("edge")]
    public int Offset { get; set; }

    [Network("u32")]
    [Codec("edge")]
    public uint Count { get; set; }

    [Network("i64")]
    [Codec("edge")]
    public long Delta { get; set; }

    [Network("u64")]
    [Codec("edge")]
    public ulong Sequence { get; set; }

    [Network("f32")]
    [Codec("edge")]
    public float Ratio { get; set; }

    [Network("f64")]
    [Codec("edge")]
    public double Precise { get; set; }

    [Network("string")]
    [Codec("edge")]
    public string Label { get; set; } = "";

    [Network("bytes")]
    [Codec("edge")]
    public byte[] Blob { get; set; } = new byte[0];
}

[Network]
[Codec("edge")]
public class PlayerInfo
{
    [Network("u32")]
    [Codec("edge")]
    public uint Level { get; set; }
}

[Network]
[Codec("edge")]
public class Player
{
    [Network("u32")]
    [Codec("edge")]
    public uint Id { get; set; }

    [Network("f32")]
    [Codec("edge")]
    public float X { get; set; }

    [Network("f32")]
    [Codec("edge")]
    public float Y { get; set; }
}

[Network]
[Codec("edge")]
public class Team
{
    [Network("PlayerInfo")]
    [Codec("edge")]
    public PlayerInfo Captain { get; set; } = new PlayerInfo();

    [Network("Array<string>")]
    [Codec("edge")]
    public List<string> Tags { get; set; } = new List<string>();

    [Network("Array<u32>")]
    [Codec("edge")]
    public List<uint> Scores { get; set; } = new List<uint>();

    [Network("Array<PlayerInfo>")]
    [Codec("edge")]
    public List<PlayerInfo> Roster { get; set; } = new List<PlayerInfo>();
}
