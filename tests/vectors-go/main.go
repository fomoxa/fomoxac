package main

import (
	"bytes"
	"encoding/hex"
	"encoding/json"
	"errors"
	"fmt"
	"os"
	"path/filepath"
	"strconv"
	"strings"
	"testing"

	fomoxa "github.com/fomoxa/go"

	"fomoxac-vectors-go/src/generated"
	"fomoxac-vectors-go/src/models"
)

type vector struct {
	Name     string `json:"name"`
	Message  string `json:"message"`
	Hex      string `json:"hex"`
	Reencode string `json:"reencode"`
	Error    string `json:"error"`
}

type message struct {
	ID          string   `json:"id"`
	Fingerprint string   `json:"fingerprint"`
	Prefixes    []string `json:"prefixes"`
}

type document struct {
	Messages map[string]message `json:"messages"`
	Accept   []vector           `json:"accept"`
	Reject   []vector           `json:"reject"`
}

type identity struct {
	id          uint32
	fingerprint uint64
}

var roundTrips = map[string]func([]byte) ([]byte, error){
	"Player.edge": func(payload []byte) ([]byte, error) {
		var value models.Player
		if err := (generated.PlayerEdgeCodec{}).Decode(generated.NewReader(payload), &value); err != nil {
			return nil, err
		}
		writer := generated.NewWriter()
		generated.PlayerEdgeCodec{}.Encode(writer, &value)
		return writer.Bytes(), nil
	},
	"DeviceState.edge": func(payload []byte) ([]byte, error) {
		var value models.DeviceState
		if err := (generated.DeviceStateEdgeCodec{}).Decode(generated.NewReader(payload), &value); err != nil {
			return nil, err
		}
		writer := generated.NewWriter()
		generated.DeviceStateEdgeCodec{}.Encode(writer, &value)
		return writer.Bytes(), nil
	},
	"DeviceState.unity": func(payload []byte) ([]byte, error) {
		var value models.DeviceState
		if err := (generated.DeviceStateUnityCodec{}).Decode(generated.NewReader(payload), &value); err != nil {
			return nil, err
		}
		writer := generated.NewWriter()
		generated.DeviceStateUnityCodec{}.Encode(writer, &value)
		return writer.Bytes(), nil
	},
	"EveryPrimitive.edge": func(payload []byte) ([]byte, error) {
		var value models.EveryPrimitive
		if err := (generated.EveryPrimitiveEdgeCodec{}).Decode(generated.NewReader(payload), &value); err != nil {
			return nil, err
		}
		writer := generated.NewWriter()
		generated.EveryPrimitiveEdgeCodec{}.Encode(writer, &value)
		return writer.Bytes(), nil
	},
	"Team.edge": func(payload []byte) ([]byte, error) {
		var value models.Team
		if err := (generated.TeamEdgeCodec{}).Decode(generated.NewReader(payload), &value); err != nil {
			return nil, err
		}
		writer := generated.NewWriter()
		generated.TeamEdgeCodec{}.Encode(writer, &value)
		return writer.Bytes(), nil
	},
}

var sharedWriter = generated.NewWriterSize(1)

var sharedRoundTrips = map[string]func([]byte) ([]byte, error){
	"Player.edge": func(payload []byte) ([]byte, error) {
		var value models.Player
		if err := (generated.PlayerEdgeCodec{}).Decode(generated.NewReader(payload), &value); err != nil {
			return nil, err
		}
		sharedWriter.Reset()
		generated.PlayerEdgeCodec{}.Encode(sharedWriter, &value)
		return sharedWriter.Bytes(), nil
	},
	"DeviceState.edge": func(payload []byte) ([]byte, error) {
		var value models.DeviceState
		if err := (generated.DeviceStateEdgeCodec{}).Decode(generated.NewReader(payload), &value); err != nil {
			return nil, err
		}
		sharedWriter.Reset()
		generated.DeviceStateEdgeCodec{}.Encode(sharedWriter, &value)
		return sharedWriter.Bytes(), nil
	},
	"DeviceState.unity": func(payload []byte) ([]byte, error) {
		var value models.DeviceState
		if err := (generated.DeviceStateUnityCodec{}).Decode(generated.NewReader(payload), &value); err != nil {
			return nil, err
		}
		sharedWriter.Reset()
		generated.DeviceStateUnityCodec{}.Encode(sharedWriter, &value)
		return sharedWriter.Bytes(), nil
	},
	"EveryPrimitive.edge": func(payload []byte) ([]byte, error) {
		var value models.EveryPrimitive
		if err := (generated.EveryPrimitiveEdgeCodec{}).Decode(generated.NewReader(payload), &value); err != nil {
			return nil, err
		}
		sharedWriter.Reset()
		generated.EveryPrimitiveEdgeCodec{}.Encode(sharedWriter, &value)
		return sharedWriter.Bytes(), nil
	},
	"Team.edge": func(payload []byte) ([]byte, error) {
		var value models.Team
		if err := (generated.TeamEdgeCodec{}).Decode(generated.NewReader(payload), &value); err != nil {
			return nil, err
		}
		sharedWriter.Reset()
		generated.TeamEdgeCodec{}.Encode(sharedWriter, &value)
		return sharedWriter.Bytes(), nil
	},
}

type reusableCodec struct {
	create func() any
	decode func(*generated.Reader, any) error
	encode func(*generated.Writer, any)
}

var reusableCodecs = map[string]reusableCodec{
	"Player.edge": {
		create: func() any { return &models.Player{} },
		decode: func(r *generated.Reader, value any) error {
			return generated.PlayerEdgeCodec{}.Decode(r, value.(*models.Player))
		},
		encode: func(w *generated.Writer, value any) { generated.PlayerEdgeCodec{}.Encode(w, value.(*models.Player)) },
	},
	"DeviceState.edge": {
		create: func() any { return &models.DeviceState{} },
		decode: func(r *generated.Reader, value any) error {
			return generated.DeviceStateEdgeCodec{}.Decode(r, value.(*models.DeviceState))
		},
		encode: func(w *generated.Writer, value any) {
			generated.DeviceStateEdgeCodec{}.Encode(w, value.(*models.DeviceState))
		},
	},
	"DeviceState.unity": {
		create: func() any { return &models.DeviceState{} },
		decode: func(r *generated.Reader, value any) error {
			return generated.DeviceStateUnityCodec{}.Decode(r, value.(*models.DeviceState))
		},
		encode: func(w *generated.Writer, value any) {
			generated.DeviceStateUnityCodec{}.Encode(w, value.(*models.DeviceState))
		},
	},
	"EveryPrimitive.edge": {
		create: func() any { return &models.EveryPrimitive{} },
		decode: func(r *generated.Reader, value any) error {
			return generated.EveryPrimitiveEdgeCodec{}.Decode(r, value.(*models.EveryPrimitive))
		},
		encode: func(w *generated.Writer, value any) {
			generated.EveryPrimitiveEdgeCodec{}.Encode(w, value.(*models.EveryPrimitive))
		},
	},
	"Team.edge": {
		create: func() any { return &models.Team{} },
		decode: func(r *generated.Reader, value any) error {
			return generated.TeamEdgeCodec{}.Decode(r, value.(*models.Team))
		},
		encode: func(w *generated.Writer, value any) { generated.TeamEdgeCodec{}.Encode(w, value.(*models.Team)) },
	},
}

func checkReusedTargets(accepts []vector) {
	targets := map[string]any{}
	for pass := 0; pass < 2; pass++ {
		for index := range accepts {
			accept := accepts[index]
			if pass == 1 {
				accept = accepts[len(accepts)-1-index]
			}
			codec := reusableCodecs[accept.Message]
			target, found := targets[accept.Message]
			if !found {
				target = codec.create()
				targets[accept.Message] = target
			}
			payload := decodeHex(accept.Hex)
			expected := decodeHex(accept.Reencode)
			err := codec.decode(generated.NewReader(payload), target)
			sharedWriter.Reset()
			codec.encode(sharedWriter, target)
			check(err == nil && bytes.Equal(sharedWriter.Bytes(), expected), "reused target %s: %v %x, expected %x", accept.Name, err, sharedWriter.Bytes(), expected)

			reader := generated.NewReader(nil)
			allocations := testing.AllocsPerRun(10, func() {
				reader.Reset(payload)
				_ = codec.decode(reader, target)
			})
			check(allocations == 0, "reused target %s: decoding the same bytes again allocated %.0f times", accept.Name, allocations)
		}
	}
}

func checkReaderReuse() {
	writer := generated.NewWriter()
	writer.WriteString("Đội trưởng 🚀")
	writer.WriteBytes([]byte{1, 2, 3})
	payload := append([]byte(nil), writer.Bytes()...)

	text := strings.Clone("Đội trưởng 🚀")
	blob := make([]byte, 0, 8)
	reader := generated.NewReader(payload)
	errText := reader.ReadStringInto(&text)
	errBlob := reader.ReadBytesInto(&blob)
	check(errText == nil && errBlob == nil && text == "Đội trưởng 🚀" && bytes.Equal(blob, []byte{1, 2, 3}) && cap(blob) == 8, "ReadStringInto / ReadBytesInto keep equal text and reuse capacity")

	allocations := testing.AllocsPerRun(10, func() {
		reader.Reset(payload)
		_ = reader.ReadStringInto(&text)
		_ = reader.ReadBytesInto(&blob)
	})
	check(allocations == 0, "ReadStringInto / ReadBytesInto allocated %.0f times on unchanged input", allocations)

	invalid := []byte{2, 0, 0, 0, 0xC3, 0x28}
	kept := "kept"
	invalidReader := generated.NewReader(invalid)
	err := invalidReader.ReadStringInto(&kept)
	var decodeError *generated.DecodeError
	check(errors.As(err, &decodeError) && invalidReader.Position() == 0 && kept == "kept", "ReadStringInto rejects invalid UTF-8 and leaves the cursor and the value")
}

var identities = map[string]identity{
	"Player.edge":         {generated.PlayerEdgeCodecMessageID, generated.PlayerEdgeCodecFingerprint},
	"PlayerInfo.edge":     {generated.PlayerInfoEdgeCodecMessageID, generated.PlayerInfoEdgeCodecFingerprint},
	"DeviceState.edge":    {generated.DeviceStateEdgeCodecMessageID, generated.DeviceStateEdgeCodecFingerprint},
	"DeviceState.unity":   {generated.DeviceStateUnityCodecMessageID, generated.DeviceStateUnityCodecFingerprint},
	"EveryPrimitive.edge": {generated.EveryPrimitiveEdgeCodecMessageID, generated.EveryPrimitiveEdgeCodecFingerprint},
	"Team.edge":           {generated.TeamEdgeCodecMessageID, generated.TeamEdgeCodecFingerprint},
}

var errorKinds = map[string]string{
	"UnexpectedEof":  "unexpected_eof",
	"InvalidBool":    "invalid_bool",
	"InvalidUtf8":    "invalid_utf8",
	"LengthOverflow": "length_overflow",
}

var checks, failures int

func check(passed bool, failure string, args ...any) {
	checks++
	if !passed {
		failures++
		fmt.Printf("FAIL "+failure+"\n", args...)
	}
}

func decodeHex(text string) []byte {
	decoded, err := hex.DecodeString(strings.Join(strings.Fields(text), ""))
	if err != nil {
		panic(err)
	}
	return decoded
}

func fingerprint64(tagged string) uint64 {
	value, _ := strconv.ParseUint(strings.TrimPrefix(tagged, "sha256:")[:16], 16, 64)
	return value
}

func checkNetSchema(vectors document) {
	schema, err := generated.FomoxaNetSchema()
	if err != nil {
		check(false, "net schema: %v", err)
		return
	}
	check(schema.Fingerprint() == generated.FomoxaSchemaFingerprint, "net schema: fingerprint %016x, handshake says %016x", schema.Fingerprint(), generated.FomoxaSchemaFingerprint)
	check(len(schema.Messages()) == len(generated.FomoxaMessages), "net schema: %d messages, handshake declares %d", len(schema.Messages()), len(generated.FomoxaMessages))

	declared := make(map[uint32]fomoxa.Message, len(schema.Messages()))
	for _, declaredMessage := range schema.Messages() {
		declared[declaredMessage.ID] = declaredMessage
	}
	for name := range identities {
		entry := vectors.Messages[name]
		id, _ := strconv.ParseUint(strings.TrimPrefix(entry.ID, "0x"), 16, 32)
		found, ok := declared[uint32(id)]
		if !ok {
			check(false, "net schema: %s (%08x) is missing", name, id)
			continue
		}
		matches := found.Fingerprint == fingerprint64(entry.Fingerprint) && len(found.Prefixes) == len(entry.Prefixes)
		for index := 0; matches && index < len(entry.Prefixes); index++ {
			matches = found.Prefixes[index] == fingerprint64(entry.Prefixes[index])
		}
		check(matches, "net schema: %s is %016x %x, vectors say %s %v", name, found.Fingerprint, found.Prefixes, entry.Fingerprint, entry.Prefixes)
	}
}

func main() {
	path := filepath.Join("..", "vectors", "fomoxa-vectors.json")
	if len(os.Args) > 1 {
		path = os.Args[1]
	}
	text, err := os.ReadFile(path)
	if err != nil {
		panic(err)
	}
	var vectors document
	if err := json.Unmarshal(text, &vectors); err != nil {
		panic(err)
	}

	for name, expected := range identities {
		entry := vectors.Messages[name]
		id, _ := strconv.ParseUint(strings.TrimPrefix(entry.ID, "0x"), 16, 32)
		fingerprint, _ := strconv.ParseUint(strings.TrimPrefix(entry.Fingerprint, "sha256:")[:16], 16, 64)
		check(uint32(id) == expected.id, "%s: message id %08x, vectors say %08x", name, expected.id, id)
		check(fingerprint == expected.fingerprint, "%s: fingerprint %016x, vectors say %016x", name, expected.fingerprint, fingerprint)
	}

	for _, accept := range vectors.Accept {
		expected := decodeHex(accept.Reencode)
		actual, err := roundTrips[accept.Message](decodeHex(accept.Hex))
		if err != nil {
			check(false, "accept %s: %v", accept.Name, err)
			continue
		}
		check(bytes.Equal(actual, expected), "accept %s: re-encoded %x, expected %x", accept.Name, actual, expected)
	}

	for _, reject := range vectors.Reject {
		_, err := roundTrips[reject.Message](decodeHex(reject.Hex))
		var decodeError *generated.DecodeError
		matched := errors.As(err, &decodeError) && decodeError.Kind == errorKinds[reject.Error]
		check(matched, "reject %s: %v, expected %s", reject.Name, err, reject.Error)
	}

	for _, accept := range vectors.Accept {
		expected, _ := roundTrips[accept.Message](decodeHex(accept.Hex))
		actual, err := sharedRoundTrips[accept.Message](decodeHex(accept.Hex))
		check(err == nil && bytes.Equal(actual, expected), "shared writer %s: %x, expected %x", accept.Name, actual, expected)
	}

	checkReusedTargets(vectors.Accept)
	checkReaderReuse()

	checkNetSchema(vectors)

	fmt.Printf("%d/%d checks passed\n", checks-failures, checks)
	if failures > 0 {
		os.Exit(1)
	}
}
