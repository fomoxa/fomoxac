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

    private delegate void DecodeInto(ref Reader reader, object value);

    private sealed class Codec
    {
        public Func<object> Create;
        public DecodeInto Decode;
        public Action<Writer, object> Encode;
    }

    private static readonly Dictionary<string, Codec> Codecs = new Dictionary<string, Codec>
    {
        ["Player.edge"] = new Codec
        {
            Create = () => new Player(),
            Decode = (ref Reader reader, object value) => { var typed = (Player)value; PlayerEdgeCodec.Decode(ref reader, ref typed); },
            Encode = (writer, value) => PlayerEdgeCodec.Encode(writer, (Player)value),
        },
        ["DeviceState.edge"] = new Codec
        {
            Create = () => new DeviceState(),
            Decode = (ref Reader reader, object value) => { var typed = (DeviceState)value; DeviceStateEdgeCodec.Decode(ref reader, ref typed); },
            Encode = (writer, value) => DeviceStateEdgeCodec.Encode(writer, (DeviceState)value),
        },
        ["DeviceState.unity"] = new Codec
        {
            Create = () => new DeviceState(),
            Decode = (ref Reader reader, object value) => { var typed = (DeviceState)value; DeviceStateUnityCodec.Decode(ref reader, ref typed); },
            Encode = (writer, value) => DeviceStateUnityCodec.Encode(writer, (DeviceState)value),
        },
        ["EveryPrimitive.edge"] = new Codec
        {
            Create = () => new EveryPrimitive(),
            Decode = (ref Reader reader, object value) => { var typed = (EveryPrimitive)value; EveryPrimitiveEdgeCodec.Decode(ref reader, ref typed); },
            Encode = (writer, value) => EveryPrimitiveEdgeCodec.Encode(writer, (EveryPrimitive)value),
        },
        ["Team.edge"] = new Codec
        {
            Create = () => new Team(),
            Decode = (ref Reader reader, object value) => { var typed = (Team)value; TeamEdgeCodec.Decode(ref reader, ref typed); },
            Encode = (writer, value) => TeamEdgeCodec.Encode(writer, (Team)value),
        },
    };

    private static readonly Writer SharedWriter = new Writer(1);

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

        foreach (JsonElement vector in root.GetProperty("accept").EnumerateArray())
        {
            CheckSharedWriter(vector);
        }

        CheckReusedTargets(root.GetProperty("accept"));
        CheckRuntimeViews();

        CheckNetSchema(root.GetProperty("messages"));

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

    private static void CheckNetSchema(JsonElement messages)
    {
        Fomoxa.Net.Schema schema;
        try
        {
            schema = NetSchema.Build();
        }
        catch (Exception error)
        {
            Check(false, $"net schema: {error.GetType().Name}: {error.Message}");
            return;
        }

        Check(schema.Fingerprint == Handshake.FomoxaSchemaFingerprint, $"net schema: fingerprint 0x{schema.Fingerprint:X16}, handshake says 0x{Handshake.FomoxaSchemaFingerprint:X16}");
        Check(schema.Messages.Count == Handshake.FomoxaMessages.Length, $"net schema: {schema.Messages.Count} messages, handshake declares {Handshake.FomoxaMessages.Length}");

        foreach (string name in Identities.Keys)
        {
            JsonElement entry = messages.GetProperty(name);
            uint id = Convert.ToUInt32(entry.GetProperty("id").GetString().Substring(2), 16);
            Fomoxa.Net.MessageSchema found = schema.Message(id);
            if (found == null)
            {
                Check(false, $"net schema: {name} (0x{id:X8}) is missing");
                continue;
            }
            ulong[] expected = entry.GetProperty("prefixes").EnumerateArray().Select(prefix => Fingerprint64(prefix.GetString())).ToArray();
            bool matches = found.Fingerprint == Fingerprint64(entry.GetProperty("fingerprint").GetString()) && found.PrefixFingerprints.SequenceEqual(expected);
            Check(matches, $"net schema: {name} does not match the vectors");
        }
    }

    private static ulong Fingerprint64(string tagged) => Convert.ToUInt64(tagged.Substring("sha256:".Length, 16), 16);

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

    private static void CheckSharedWriter(JsonElement vector)
    {
        string name = vector.GetProperty("name").GetString();
        string message = vector.GetProperty("message").GetString();
        byte[] payload = Hex(vector.GetProperty("hex").GetString());
        byte[] expected = RoundTrips[message](payload);
        SharedWriter.Clear();
        var reader = new Reader(payload);
        switch (message)
        {
            case "Player.edge":
            {
                var value = new Player();
                PlayerEdgeCodec.Decode(ref reader, ref value);
                PlayerEdgeCodec.Encode(SharedWriter, value);
                break;
            }
            case "DeviceState.edge":
            {
                var value = new DeviceState();
                DeviceStateEdgeCodec.Decode(ref reader, ref value);
                DeviceStateEdgeCodec.Encode(SharedWriter, value);
                break;
            }
            case "DeviceState.unity":
            {
                var value = new DeviceState();
                DeviceStateUnityCodec.Decode(ref reader, ref value);
                DeviceStateUnityCodec.Encode(SharedWriter, value);
                break;
            }
            case "EveryPrimitive.edge":
            {
                var value = new EveryPrimitive();
                EveryPrimitiveEdgeCodec.Decode(ref reader, ref value);
                EveryPrimitiveEdgeCodec.Encode(SharedWriter, value);
                break;
            }
            case "Team.edge":
            {
                var value = new Team();
                TeamEdgeCodec.Decode(ref reader, ref value);
                TeamEdgeCodec.Encode(SharedWriter, value);
                break;
            }
        }
        Check(SharedWriter.WrittenSpan.SequenceEqual(expected), $"shared writer {name}: {ToHex(SharedWriter.ToArray())}, expected {ToHex(expected)}");
    }

    private static void CheckReusedTargets(JsonElement accept)
    {
        List<JsonElement> vectors = accept.EnumerateArray().ToList();
        var targets = new Dictionary<string, object>();
        for (int pass = 0; pass < 2; pass++)
        {
            IEnumerable<JsonElement> order = pass == 0 ? vectors : Enumerable.Reverse(vectors);
            foreach (JsonElement vector in order)
            {
                string name = vector.GetProperty("name").GetString();
                string message = vector.GetProperty("message").GetString();
                byte[] payload = Hex(vector.GetProperty("hex").GetString());
                byte[] expected = Hex(vector.GetProperty("reencode").GetString());
                Codec codec = Codecs[message];
                if (!targets.TryGetValue(message, out object target))
                {
                    target = codec.Create();
                    targets[message] = target;
                }

                var reader = new Reader(payload);
                codec.Decode(ref reader, target);
                SharedWriter.Clear();
                codec.Encode(SharedWriter, target);
                Check(SharedWriter.WrittenSpan.SequenceEqual(expected), $"reused target {name}: {ToHex(SharedWriter.ToArray())}, expected {ToHex(expected)}");

                long before = GC.GetAllocatedBytesForCurrentThread();
                var again = new Reader(payload);
                codec.Decode(ref again, target);
                long allocated = GC.GetAllocatedBytesForCurrentThread() - before;
                Check(allocated == 0, $"reused target {name}: decoding the same bytes again allocated {allocated} bytes");
            }
        }
    }

    private static void CheckRuntimeViews()
    {
        byte[] blob = { 1, 2, 3, 4, 5 };
        byte[] expected = { 5, 0, 0, 0, 1, 2, 3, 4, 5 };
        var writer = new Writer(1);
        writer.WriteBytes(new ReadOnlyMemory<byte>(blob));
        Check(writer.WrittenSpan.SequenceEqual(expected), "WriteBytes(ReadOnlyMemory<byte>)");
        writer.Clear();
        writer.WriteBytes(new Memory<byte>(blob));
        Check(writer.WrittenSpan.SequenceEqual(expected), "WriteBytes(Memory<byte>)");
        writer.Clear();
        writer.WriteBytes(new ArraySegment<byte>(new byte[] { 9, 1, 2, 3, 4, 5, 9 }, 1, 5));
        Check(writer.WrittenSpan.SequenceEqual(expected), "WriteBytes(ArraySegment<byte>)");
        writer.Clear();
        writer.WriteBytes(new ReadOnlySpan<byte>(blob));
        Check(writer.WrittenSpan.SequenceEqual(expected), "WriteBytes(ReadOnlySpan<byte>)");

        byte[] framed = new byte[] { 0xEE }.Concat(expected).ToArray();
        var memoryReader = new Reader(new ReadOnlyMemory<byte>(framed, 1, expected.Length));
        ReadOnlyMemory<byte> view = default;
        memoryReader.ReadBytes(ref view);
        bool aliases = System.Runtime.InteropServices.MemoryMarshal.TryGetArray(view, out ArraySegment<byte> viewSegment)
            && ReferenceEquals(viewSegment.Array, framed) && viewSegment.Offset == 5 && viewSegment.Count == 5;
        Check(aliases, "ReadBytes(ref ReadOnlyMemory<byte>) over memory is a slice of the input");

        var spanReader = new Reader(new ReadOnlySpan<byte>(expected));
        ReadOnlyMemory<byte> copied = default;
        spanReader.ReadBytes(ref copied);
        Check(copied.Span.SequenceEqual(blob), "ReadBytes(ref ReadOnlyMemory<byte>) over a span copies");

        byte[] reusedArray = new byte[5];
        byte[] arrayTarget = reusedArray;
        var arrayReader = new Reader(expected);
        arrayReader.ReadBytes(ref arrayTarget);
        Check(ReferenceEquals(arrayTarget, reusedArray) && arrayTarget.SequenceEqual(blob), "ReadBytes(ref byte[]) reuses an array of the same length");

        byte[] segmentStorage = new byte[16];
        var segmentTarget = new ArraySegment<byte>(segmentStorage, 0, 2);
        var segmentReader = new Reader(expected);
        segmentReader.ReadBytes(ref segmentTarget);
        Check(ReferenceEquals(segmentTarget.Array, segmentStorage) && segmentTarget.Count == 5 && segmentTarget.SequenceEqual(blob), "ReadBytes(ref ArraySegment<byte>) reuses a large enough array");

        Memory<byte> memoryTarget = new Memory<byte>(new byte[2]);
        var growReader = new Reader(expected);
        growReader.ReadBytes(ref memoryTarget);
        Check(memoryTarget.Length == 5 && memoryTarget.Span.SequenceEqual(blob), "ReadBytes(ref Memory<byte>) grows a small array");

        CheckStringReuse("ascii", "Captain");
        CheckStringReuse("non-ascii", "Đội trưởng 🚀");
        CheckStringReuse("long", new string('x', 300) + "é");

        string previous = "before";
        string replaced = previous;
        var replacingReader = new Reader(StringPayload("after"));
        replacingReader.ReadString(ref replaced);
        Check(replaced == "after", "ReadString(ref string) replaces a different string");

        byte[] invalid = { 2, 0, 0, 0, 0xC3, 0x28 };
        var invalidReader = new Reader(invalid);
        string untouched = "kept";
        try
        {
            invalidReader.ReadString(ref untouched);
            Check(false, "ReadString(ref string) accepted invalid UTF-8");
        }
        catch (DecodeException raised)
        {
            Check(raised.Message.StartsWith("invalid utf-8", StringComparison.Ordinal) && invalidReader.Position == 0 && untouched == "kept", "ReadString(ref string) rejects invalid UTF-8 and leaves the cursor and the value");
        }
    }

    private static void CheckStringReuse(string label, string text)
    {
        byte[] payload = StringPayload(text);
        string held = new string(text.AsSpan());
        string target = held;
        var reader = new Reader(payload);
        long before = GC.GetAllocatedBytesForCurrentThread();
        reader.ReadString(ref target);
        long allocated = GC.GetAllocatedBytesForCurrentThread() - before;
        Check(ReferenceEquals(target, held) && allocated == 0, $"ReadString(ref string) keeps an equal {label} string without allocating ({allocated} bytes)");

        string empty = null;
        var fresh = new Reader(payload);
        fresh.ReadString(ref empty);
        Check(empty == text, $"ReadString(ref string) decodes a {label} string into null");
    }

    private static byte[] StringPayload(string text)
    {
        var writer = new Writer();
        writer.WriteString(text);
        return writer.ToArray();
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
