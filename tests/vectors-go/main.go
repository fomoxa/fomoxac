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

	checkNetSchema(vectors)

	fmt.Printf("%d/%d checks passed\n", checks-failures, checks)
	if failures > 0 {
		os.Exit(1)
	}
}
