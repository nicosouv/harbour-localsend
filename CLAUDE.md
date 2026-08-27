# CLAUDE.md

Guidance for Claude Code (claude.ai/code) when working in this repository.

## Project overview

An unofficial LocalSend client for Sailfish OS. Qt/C++ backend, QML/Silica
front end. It implements the LocalSend v2 protocol from scratch — discovery,
an HTTP server and an HTTP client — because Qt 5.6 provides none of it.

## Build commands

Nothing needs a device. The Docker lanes mirror CI exactly:

```bash
docker compose run --rm tests     # unit tests + the loopback transfer test
docker compose run --rm syntax    # compile-check every file in src/
docker compose run --rm qmllint   # parse every QML file
docker compose run --rm checks    # Qt 5.6 compat, QML traps, translation drift
docker compose run --rm icons     # redraw icons/*/harbour-localsend.png
```

Release:

```bash
git tag v1.0.1 && git push origin v1.0.1
```

GitHub Actions builds armv7hl, aarch64 and i486 and publishes the release. The
version comes from the tag and is written into `rpm/harbour-localsend.spec`
and, through `%qmake5 VERSION=`, into `APP_VERSION`.

## Architecture

### Backend (Qt/C++)

Eight objects are exposed to QML as context properties by
`src/harbour-localsend.cpp`.

1. **AppSettings** (`appsettings.*`) — every preference, plus this device's
   identity. `fingerprint` is generated once and must survive forever: peers
   recognise us by it and filter our own announcements with it. `self()`
   builds the `DeviceInfo` that goes into every payload.

2. **DeviceModel** (`devicemodel.*`) — the peers we can see, keyed by
   fingerprint (not address: a phone moving between networks keeps its
   identity) and sorted by alias (not arrival: a list that reorders under a
   finger sends files to the wrong machine).

3. **Discovery** (`discovery.*`) — UDP multicast on 224.0.0.167:53317, plus a
   manual `/24` sweep for networks that swallow multicast. Announces every 20
   seconds; peers answer, which is what keeps their entries fresh.

4. **HttpServer / HttpConnection** (`httpserver.*`) — the whole of our HTTP.
   Request line, headers, and a body that is either buffered (small JSON) or
   streamed to disk (an upload, which can be gigabytes).

5. **ReceiveService** (`receiveservice.*`) — serves every endpoint and owns
   the incoming session.

6. **SendService** (`sendservice.*`) — the outgoing handshake, then one file
   at a time.

7. **TransferModel** (`transfermodel.*`) — the one in-flight transfer, as a
   list model plus aggregates. Shared by both services.

8. **SelectionModel** (`selectionmodel.*`) and **HistoryModel**
   (`historymodel.*`) — the staging tray and the log.

`protocol.*` and `deviceinfo.*` hold the wire format; nothing else formats a
payload.

### Protocol

```
POST /api/localsend/v2/prepare-upload?pin=…   → { sessionId, files: {id: token} }
POST /api/localsend/v2/upload?sessionId=&fileId=&token=
POST /api/localsend/v2/cancel?sessionId=
POST /api/localsend/v2/register                → our info
GET  /api/localsend/v2/info                    → our info
```

Download mode (`prepare-download` / `download`) is **not** implemented.

### Data flow, receiving

```
prepare-upload arrives → ReceiveService parks the connection unanswered
  → TransferModel.begin(Receiving, …) + state Pending
  → requestArrived() → root window pushes ReceiveRequestPage
  → accept() writes { sessionId, tokens } into the parked connection
  → state Active, transferStarted() → TransferPage
  → upload arrives → headersReady validates token + peer address
  → HttpConnection streams the body into <name>.part
  → requestReady → rename to <name>, file marked done
  → last file → finishSession() → history + transferFinished()
```

## Security model

Every device is its own certificate authority, so X.509 chain validation has
nothing to say. What replaces it:

- `Certificate` generates an EC P-256 self-signed pair once and keeps it. The
  fingerprint is `sha256(cert.toDer())`, which is what the protocol means by
  `fingerprint` in HTTPS mode.
- `AppSettings::fingerprint()` returns that hash when encrypted and a stored
  random value otherwise. **These must never diverge from what the server
  actually presents**, or every peer refuses us.
- `TlsClient::pin()` accepts a connection only when the presented certificate
  hashes to what the peer announced, and `configure()` uses `VerifyPeer` so
  that an unignored error actually aborts. `QueryPeer` reports errors and
  connects anyway, which silently turns the check into a suggestion — there is
  a test for exactly this.
- `TlsClient::acceptUnknown()` exists only for the subnet sweep, where no
  fingerprint is known yet. Its callers must then verify the claimed
  fingerprint against `observedFingerprint()`. Never use it for a transfer.

## Invariants worth keeping

1. **The prepare-upload connection is answered late, not early.** The sender
   holds it open while a person decides. `accept()` and `decline()` write into
   a connection that may be minutes old, and `onPendingClosed()` handles the
   sender giving up first. Never answer it to "get it out of the way".

2. **An upload body never reaches memory.** `headersReady` is the only place
   `streamBodyTo()` may be called, and the only place to reject a request
   whose body we do not want. By `requestReady` the body has already arrived.

3. **File names from a peer are attacker-controlled.** `sanitizeFileName()`
   drops the directory component before anything is written.
   `tst_transfer::fileNamesCannotEscapeTheDestination` proves it; do not
   weaken it.

3b. **Secrets go through `Crypto`, never through Qt.** `QUuid::createUuid()`
   falls back to `qrand()` when it cannot read the kernel pool, and a session
   token from `qrand()` is guessable. `Crypto::randomBytes()` returns empty
   rather than degrade. Compare tokens and PINs with `Crypto::equals()`.

4. **Progress counts are absolute, never deltas.** `setFileTransferred(row,
   bytes)` takes the total for that file. A repeated signal must not
   double-count, and a dropped one must not lose bytes.

5. **One session at a time.** A second `prepare-upload` gets 409. Without it
   the second sender's tokens overwrite the first's mid-transfer.

6. **Nothing is emitted per socket read.** `TransferModel` folds progress into
   its entries and flushes to QML on a 200 ms timer.

7. **Never name a QML role or list key `model`** — it shadows the delegate's
   own model object. `DeviceModel` exposes the hardware name as `hardware`.

8. **Never give a QML `id` the name of a context property**
   (`sender`, `receiver`, `transfer`, `selection`, `discovery`, `deviceModel`,
   `historyModel`, `appSettings`). `scripts/check_qml.py` enforces this after
   `id: sender` shadowed the SendService for a whole page.

## Constraints

- **Qt 5.6 and an ES5.1 JavaScript engine.** No `let`/`const`, no arrow
  functions, no template literals, no `Array.includes`, no
  `String.startsWith`. `scripts/check_qt56.py` gates this, because every lane
  that compiles off-device runs something far newer and will happily accept
  code the handset rejects at runtime.
- **QtQuick 2.6 maximum.**
- **Harbour rules**: `harbour-` prefix, allowed dependencies only.
- **OpenSSL is linked directly** (`PKGCONFIG += openssl`) because Qt cannot
  generate an X.509 certificate in any version. Only APIs present and
  non-deprecated in both 1.1.1 (the device) and 3.x (the Docker lane) may be
  used; `src/certificate.cpp` is the only file that talks to it, apart from
  `src/crypto.cpp`.

## Translations

`translations/*.ts` are generated, not hand-edited:

```bash
python3 scripts/make_translations.py
```

The table in that script is keyed by English source string, so a string used
in three files is translated once. `scripts/check_translations.py` fails CI
when the QML and the catalogues drift apart — which happens silently
otherwise, most often when a page is renamed, since the QML file's base name
is the translation context.

## File layout

```
src/                     # backend
qml/pages/               # pages
qml/components/          # delegates, drawing, Formatting.js, DeviceLook.js
qml/cover/               # active cover
tests/                   # QtTest suite + the Docker image CI uses
scripts/                 # static checks, icon and translation generators
rpm/harbour-localsend.spec
```
