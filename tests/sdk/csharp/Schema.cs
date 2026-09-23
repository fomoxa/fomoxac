#nullable enable

using System;
using System.Collections.Generic;

namespace Fomoxa.Net
{
    public sealed class SchemaException : Exception
    {
        public SchemaException(string message) : base(message)
        {
        }
    }

    public sealed class MessageSchema
    {
        private readonly ulong[] prefixes;

        public MessageSchema(uint id, ulong fingerprint, IReadOnlyList<ulong> prefixFingerprints)
        {
            if (prefixFingerprints == null)
            {
                throw new ArgumentNullException(nameof(prefixFingerprints));
            }
            if (prefixFingerprints.Count > ushort.MaxValue)
            {
                throw new SchemaException(
                    $"message 0x{id:X8} declares {prefixFingerprints.Count} fields, more than a u16 can carry");
            }

            var copy = new ulong[prefixFingerprints.Count];
            for (int index = 0; index < copy.Length; index++)
            {
                copy[index] = prefixFingerprints[index];
            }
            if (copy.Length > 0 && copy[copy.Length - 1] != fingerprint)
            {
                throw new SchemaException(
                    $"message 0x{id:X8}: the last prefix fingerprint is not the message fingerprint");
            }

            Id = id;
            Fingerprint = fingerprint;
            prefixes = copy;
        }

        public uint Id { get; }

        public ulong Fingerprint { get; }

        public ushort FieldCount => (ushort)prefixes.Length;

        public IReadOnlyList<ulong> PrefixFingerprints => prefixes;

        public bool TryPrefix(ushort fieldCount, out ulong fingerprint)
        {
            if (fieldCount == 0 || fieldCount > prefixes.Length)
            {
                fingerprint = 0;
                return false;
            }
            fingerprint = prefixes[fieldCount - 1];
            return true;
        }
    }

    public sealed class Schema
    {
        private readonly MessageSchema[] messages;

        public Schema(ulong fingerprint, IEnumerable<MessageSchema> messages)
        {
            if (messages == null)
            {
                throw new ArgumentNullException(nameof(messages));
            }

            var ordered = new List<MessageSchema>(messages);
            if (ordered.Count > FomoxaWire.MaxHelloMessages)
            {
                throw new SchemaException(
                    $"{ordered.Count} messages exceeds the {FomoxaWire.MaxHelloMessages} a hello may declare");
            }

            long helloSize = FomoxaWire.HelloHeaderSize + (long)FomoxaWire.HelloEntrySize * ordered.Count;
            if (helloSize > FomoxaWire.MaxHandshakePayload)
            {
                throw new SchemaException(
                    $"{ordered.Count} messages need {helloSize} hello bytes, past the " +
                    $"{FomoxaWire.MaxHandshakePayload} a handshake frame carries");
            }

            ordered.Sort((left, right) => left.Id.CompareTo(right.Id));
            for (int index = 1; index < ordered.Count; index++)
            {
                if (ordered[index - 1].Id == ordered[index].Id)
                {
                    throw new SchemaException($"message id 0x{ordered[index].Id:X8} is declared twice");
                }
            }

            Fingerprint = fingerprint;
            this.messages = ordered.ToArray();
        }

        public ulong Fingerprint { get; }

        public IReadOnlyList<MessageSchema> Messages => messages;

        public MessageSchema? Message(uint id)
        {
            int low = 0;
            int high = messages.Length;
            while (low < high)
            {
                int middle = low + ((high - low) / 2);
                if (messages[middle].Id < id)
                {
                    low = middle + 1;
                }
                else
                {
                    high = middle;
                }
            }
            return low < messages.Length && messages[low].Id == id ? messages[low] : null;
        }
    }
}
