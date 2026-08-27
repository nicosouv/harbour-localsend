# Changelog

## 1.0.0

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

### Notes

- Transfers use plain HTTP. The encrypted transport is not implemented; see
  the About page and the README.
- Download mode (`prepare-download` / `download`) is not implemented.
