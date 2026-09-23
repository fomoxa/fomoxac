package fomoxa

import (
	"fmt"
	"sort"
)

const MaxFieldCount = 0xFFFF

type Message struct {
	ID          uint32
	Fingerprint uint64
	Prefixes    []uint64
}

func (m Message) FieldCount() uint32 { return uint32(len(m.Prefixes)) }

type Schema struct {
	fingerprint uint64
	messages    []Message
	byID        map[uint32]int
}

func NewSchema(fingerprint uint64, messages []Message) (*Schema, error) {
	s := &Schema{
		fingerprint: fingerprint,
		messages:    make([]Message, len(messages)),
		byID:        make(map[uint32]int, len(messages)),
	}
	copy(s.messages, messages)
	sort.Slice(s.messages, func(i, j int) bool { return s.messages[i].ID < s.messages[j].ID })

	for i, m := range s.messages {
		if _, seen := s.byID[m.ID]; seen {
			return nil, fmt.Errorf("fomoxa: message id 0x%08X is declared twice", m.ID)
		}
		if len(m.Prefixes) > MaxFieldCount {
			return nil, fmt.Errorf("fomoxa: message 0x%08X has %d fields, more than the %d a hello can carry",
				m.ID, len(m.Prefixes), MaxFieldCount)
		}
		if n := len(m.Prefixes); n > 0 && m.Prefixes[n-1] != m.Fingerprint {
			return nil, fmt.Errorf("fomoxa: message 0x%08X: last prefix 0x%016X is not the message fingerprint 0x%016X",
				m.ID, m.Prefixes[n-1], m.Fingerprint)
		}
		s.byID[m.ID] = i
	}
	return s, nil
}

func (s *Schema) Fingerprint() uint64 { return s.fingerprint }

func (s *Schema) Messages() []Message { return s.messages }

func (s *Schema) message(id uint32) (Message, bool) {
	i, ok := s.byID[id]
	if !ok {
		return Message{}, false
	}
	return s.messages[i], true
}

func (s *Schema) prefix(id uint32, fieldCount uint32) (uint64, bool) {
	m, ok := s.message(id)
	if !ok || fieldCount == 0 || int(fieldCount) > len(m.Prefixes) {
		return 0, false
	}
	return m.Prefixes[fieldCount-1], true
}

type messageCheck int

const (
	checkMatch messageCheck = iota
	checkReject
	checkNeedPrefix
)

func (s *Schema) checkMessage(id uint32, peerFieldCount uint32, peerFingerprint uint64) (messageCheck, uint32) {
	local, ok := s.message(id)
	if !ok {
		return checkMatch, 0
	}
	localFieldCount := local.FieldCount()

	if peerFingerprint == local.Fingerprint {
		return checkMatch, 0
	}
	if peerFieldCount == 0 || localFieldCount == 0 {
		return checkMatch, 0
	}
	if peerFieldCount == localFieldCount {
		return checkReject, 0
	}
	if peerFieldCount < localFieldCount {
		if local.Prefixes[peerFieldCount-1] == peerFingerprint {
			return checkMatch, 0
		}
		return checkReject, 0
	}
	return checkNeedPrefix, localFieldCount
}
