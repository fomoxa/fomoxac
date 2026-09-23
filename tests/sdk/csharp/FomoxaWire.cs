namespace Fomoxa.Net
{
    public static class FomoxaWire
    {
        public const int MaxHandshakePayload = 1024 * 1024;
        public const int MaxHelloMessages = 1_000_000;
        public const int HelloHeaderSize = 16;
        public const int HelloEntrySize = 14;
    }
}
