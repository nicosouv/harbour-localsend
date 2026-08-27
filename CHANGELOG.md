# Changelog

## 0.1.0

First release.

### Added

- Device discovery over multicast (224.0.0.167:53317), with announcement
  responses over both unicast UDP and HTTP `/register`
- A manual subnet sweep for networks that block multicast, with progress, and
  a warning on the main page when multicast turns out to be unavailable
- Receiving: `prepare-upload`, `upload` and `cancel`, with the incoming
  request shown before anything is written to disk
- Sending: multi-file transfers to any LocalSend peer, one file at a time,
  with per-file progress
- Optional PIN on incoming transfers, and a PIN prompt when a peer asks for one
- Quick save, a folder per sender, and a configurable destination
- Transfer history with the folder each receive landed in
- Active cover showing progress, with a cancel action
- Background transfers via a wakelock, so the screen going off does not stall
  a large file
- English, French, German, Spanish, Finnish, Italian and Norwegian Bokmål

### Security

- Transfers are encrypted by default: a self-signed EC P-256 certificate is
  generated on first launch and TLS 1.2+ is served from it
- Peers are authenticated by certificate pinning against the fingerprint they
  announce, which is what stands in for a certificate authority on a local
  network. A mismatch drops the connection before any file data is sent
- The PIN is stored as a salted PBKDF2-SHA256 hash (120 000 iterations) and
  can no longer be read back, only replaced
- PIN guessing is rate limited per address with exponential backoff and a
  `429` carrying `Retry-After`
- Session and file tokens come from the CSPRNG and are compared in constant
  time
- Caps on concurrent connections, header size, buffered body size and declared
  file count, plus idle timeouts on the socket, the handshake and the session
- Sailjail permissions reduced to what each one actually buys back, justified
  individually in the desktop file

### Notes

- Encryption can be turned off in Settings for interoperability; the main page
  says so in red while it is
- Download mode (`prepare-download` / `download`) is not implemented
