using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;
using Generated;
using Models;

internal static class Program
{
    private delegate byte[] RoundTrip(byte[] payload);

    private static readonly Dictionary<string, RoundTrip> RoundTrips = new Dictionary<string, RoundTrip>
    {
        ["Player.edge"] = payload =>
        {
            var reader = new Reader(payload);
            var value = new Player();
            PlayerEdgeCodec.Decode(ref reader, ref value);
            var writer = new Writer();
            PlayerEdgeCodec.Encode(writer, value);
            return writer.ToArray();
        },
        ["DeviceState.edge"] = payload =>
        {
            var reader = new Reader(payload);
            var value = new DeviceState();
            DeviceStateEdgeCodec.Decode(ref reader, ref value);
            var writer = new Writer();
            DeviceStateEdgeCodec.Encode(writer, value);
            return writer.ToArray();
        },
        ["DeviceState.unity"] = payload =>
        {
            var reader = new Reader(payload);
            var value = new DeviceState();
            DeviceStateUnityCodec.Decode(ref reader, ref value);
            var writer = new Writer();
            DeviceStateUnityCodec.Encode(writer, value);
            return writer.ToArray();
        },
        ["EveryPrimitive.edge"] = payload =>
        {
            var reader = new Reader(payload);
            var value = new EveryPrimitive();
            EveryPrimitiveEdgeCodec.Decode(ref reader, ref value);
            var writer = new Writer();
            EveryPrimitiveEdgeCodec.Encode(writer, value);
            return writer.ToArray();
        },
        ["Team.edge"] = payload =>
        {
            var reader = new Reader(payload);
            var value = new Team();
            TeamEdgeCodec.Decode(ref reader, ref value);
            var writer = new Writer();
            TeamEdgeCodec.Encode(writer, value);
            return writer.ToArray();
        },
    };

    private static readonly Dictionary<string, (uint Id, ulong Fingerprint)> Identities = new Dictionary<string, (uint, ulong)>
    {
        ["Player.edge"] = (PlayerEdgeCodec.MessageId, PlayerEdgeCodec.Fingerprint),
        ["PlayerInfo.edge"] = (PlayerInfoEdgeCodec.MessageId, PlayerInfoEdgeCodec.Fingerprint),
        ["DeviceState.edge"] = (DeviceStateEdgeCodec.MessageId, DeviceStateEdgeCodec.Fingerprint),
        ["DeviceState.unity"] = (DeviceStateUnityCodec.MessageId, DeviceStateUnityCodec.Fingerprint),
        ["EveryPrimitive.edge"] = (EveryPrimitiveEdgeCodec.MessageId, EveryPrimitiveEdgeCodec.Fingerprint),
        ["Team.edge"] = (TeamEdgeCodec.MessageId, TeamEdgeCodec.Fingerprint),
    };

    private static readonly Dictionary<string, string> ErrorPrefixes = new Dictionary<string, string>
    {
        ["UnexpectedEof"] = "unexpected eof",
        ["InvalidBool"] = "invalid bool",
        ["InvalidUtf8"] = "invalid utf-8",
        ["LengthOverflow"] = "length overflow",
    };

    private static int failures;
    private static int checks;

    private static int Main(string[] args)
    {
        string path = args.Length > 0 ? args[0] : Path.Combine("..", "vectors", "fomoxa-vectors.json");
        using JsonDocument document = JsonDocument.Parse(File.ReadAllText(path));
        JsonElement root = document.RootElement;

        CheckIdentities(root.GetProperty("messages"));
        foreach (JsonElement vector in root.GetProperty("accept").EnumerateArray())
        {
            CheckAccept(vector);
        }
        foreach (JsonElement vector in root.GetProperty("reject").EnumerateArray())
        {
            CheckReject(vector);
        }

        Console.WriteLine($"{checks - failures}/{checks} checks passed");
        return failures == 0 ? 0 : 1;
    }

    private static void CheckIdentities(JsonElement messages)
    {
        foreach (KeyValuePair<string, (uint Id, ulong Fingerprint)> expected in Identities)
        {
            JsonElement message = messages.GetProperty(expected.Key);
            uint id = Convert.ToUInt32(message.GetProperty("id").GetString().Substring(2), 16);
            ulong fingerprint = Convert.ToUInt64(message.GetProperty("fingerprint").GetString().Substring("sha256:".Length, 16), 16);
            Check(id == expected.Value.Id, $"{expected.Key}: message id 0x{expected.Value.Id:X8}, vectors say 0x{id:X8}");
            Check(fingerprint == expected.Value.Fingerprint, $"{expected.Key}: fingerprint 0x{expected.Value.Fingerprint:X16}, vectors say 0x{fingerprint:X16}");
        }
    }

    private static void CheckAccept(JsonElement vector)
    {
        string name = vector.GetProperty("name").GetString();
        string message = vector.GetProperty("message").GetString();
        byte[] payload = Hex(vector.GetProperty("hex").GetString());
        byte[] expected = Hex(vector.GetProperty("reencode").GetString());
        try
        {
            byte[] actual = RoundTrips[message](payload);
            Check(actual.SequenceEqual(expected), $"accept {name}: re-encoded {ToHex(actual)}, expected {ToHex(expected)}");
        }
        catch (Exception error)
        {
            Check(false, $"accept {name}: {error.GetType().Name}: {error.Message}");
        }
    }

    private static void CheckReject(JsonElement vector)
    {
        string name = vector.GetProperty("name").GetString();
        string message = vector.GetProperty("message").GetString();
        string error = vector.GetProperty("error").GetString();
        byte[] payload = Hex(vector.GetProperty("hex").GetString());
        try
        {
            RoundTrips[message](payload);
            Check(false, $"reject {name}: decoded, expected {error}");
        }
        catch (DecodeException raised)
        {
            Check(raised.Message.StartsWith(ErrorPrefixes[error], StringComparison.Ordinal), $"reject {name}: \"{raised.Message}\", expected {error}");
        }
        catch (Exception raised)
        {
            Check(false, $"reject {name}: {raised.GetType().Name}: {raised.Message}, expected DecodeException {error}");
        }
    }

    private static void Check(bool passed, string failure)
    {
        checks++;
        if (!passed)
        {
            failures++;
            Console.WriteLine($"FAIL {failure}");
        }
    }

    private static byte[] Hex(string text)
    {
        string digits = new string(text.Where(character => !char.IsWhiteSpace(character)).ToArray());
        return Convert.FromHexString(digits);
    }

    private static string ToHex(byte[] bytes) => Convert.ToHexString(bytes).ToLowerInvariant();
}
