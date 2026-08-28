# LocalSend for Sailfish OS

An unofficial [LocalSend](https://localsend.org) client for Sailfish OS. Send
and receive files with any phone or computer on the same network — no account,
no cloud, no Internet connection involved.

It speaks the LocalSend v2 protocol, so it talks to the official apps on
Android, iOS, Windows, macOS and Linux without either side knowing or caring
what the other is running.

![icon](icons/172x172/harbour-localsend.png)

## What it does

- **Finds devices by itself.** Multicast announcements on 224.0.0.167, the
  same as everyone else. When a network blocks that — guest Wi-Fi almost
  always does — a manual sweep of the local subnet finds them instead, and the
  app says so rather than just showing an empty list.
- **Sends anything.** Photos, documents, archives, several files at once,
  picked in either order: choose a device and then the files, or stage files
  first and pick a device after.
- **Encrypts by default.** TLS between the two devices, with a certificate the
  phone generates for itself and a fingerprint check in place of the
  certificate authority a local network does not have. See below.
- **Shows you what is arriving before it lands.** Sender, file names and total
  size, with Accept and Decline, unless you have turned that off.
- **Optional PIN**, so a sender has to know a code before you are even asked.
- **Per-file progress**, speed and time remaining, on the page and on the
  cover.
- **Keeps going with the screen off**, which is the difference between a 2 GB
  transfer finishing and stalling.

## Installing

Download the RPM for your device from the
[releases page](https://github.com/nicosouv/harbour-localsend/releases):

| Architecture | Devices |
| --- | --- |
| `aarch64` | Xperia 10 II / III / IV / V |
| `armv7hl` | Jolla 1, Jolla C, Xperia X, Xperia XA2, Xperia 10 |
| `i486` | Sailfish OS emulator |

Then, on the device:

```bash
devel-su
pkcon install-local harbour-localsend-*.rpm
```

Or just open the file from the file manager.

## Using it

Open the app on two devices on the same network and they find each other
within a few seconds. Tap a device to send it something; wait to be sent
something.

If nothing appears, the network is almost certainly eating multicast. Pull
down and choose **Scan network**: it registers with every address on the local
/24 in turn, which is slower but goes through anything that lets ordinary
traffic past.

## Security, plainly

Nothing ever leaves your network. There is no server, no relay and no account:
one device opens a TCP connection to the other's local address and pushes the
bytes. "HTTP" here is the message format, not the web. Even discovery is
multicast on 224.0.0.167, an address routers do not forward by construction.

**Transfers are encrypted by default.** On first launch the app generates its
own EC P-256 certificate and serves TLS 1.2+ from it.

There is no certificate authority on a home network and there never will be,
so what identifies a device is its **fingerprint** — the SHA-256 of its
certificate, which the protocol carries in every announcement. Before sending
anything, the app checks that the certificate the peer presents hashes to the
fingerprint that peer announced. If it does not, the connection is dropped
before a single byte of file data goes out. That is the whole model, and it is
the same one the official apps use.

What that does and does not buy you:

- A passive eavesdropper on the Wi-Fi sees nothing but ciphertext.
- Someone who redirects your traffic — ARP spoofing, a rogue access point —
  cannot produce a certificate matching the announced fingerprint, so the
  transfer **fails instead of leaking**.
- **Discovery is not authenticated, and pinning alone cannot fix that.** The
  announcement carrying a device's name is a plain multicast packet; anyone on
  the network can send one claiming any name, with their own fingerprint.
  Pinning proves you reached the device that announced itself, not that the
  announcement was honest.

  What narrows it is trust on first use: once a transfer with a device
  completes, its key is recorded against its name. A name you have used
  before turning up under a different key is flagged in red and requires an
  explicit confirmation, with both fingerprints shown side by side. The first
  contact is still unverified, which is why the full fingerprint is on the
  device details page and in About — compare them out of band once and you
  are anchored.
- The receiving side checks that a sender's certificate matches the
  fingerprint it claims, so the name on the accept prompt is not simply a
  string the sender chose.
- Encryption can be turned off in Settings for interoperability. The main page
  says **Not encrypted** in red the whole time it is off.

Other hardening, all of it covered by tests:

- **File names from a peer are attacker-controlled.** Any directory component
  is stripped before anything is written, so `../../.ssh/authorized_keys`
  lands as a file in your download folder and nowhere else.
- **The PIN is never stored.** Only a salted PBKDF2-SHA256 hash of it, at
  120 000 iterations, which is why the app can change it but never show it.
- **PIN guessing is rate limited.** A four-digit PIN is ten thousand guesses
  and a phone will answer them as fast as it can; after three failures an
  address gets exponential backoff and a `429` with `Retry-After`.
- **Session tokens come from the CSPRNG**, 192 bits each, and are compared in
  constant time. They are bearer capabilities, not identifiers.
- **Limits that stop a peer exhausting the device**: concurrent connections,
  header size, buffered body size, declared file count, and idle timeouts on
  both the socket and the session.

The PIN and the accept prompt protect against *unwanted* transfers; the
encryption protects against *watched* ones. They are different problems.

### At rest

Sailfish keeps `/home` under LUKS, unlocked by the security code at boot, so
everything below is already encrypted on a powered-off device — and no
app-level encryption could add to that, because an app that receives files
while the phone is locked must be able to read its own keys unattended.
Anything it can decrypt without you, an attacker with the same file access can
decrypt too.

What the app does on top of that:

| | |
| --- | --- |
| PIN | never stored — salted PBKDF2-SHA256, 120 000 iterations |
| TLS private key, settings, history, known devices | owner-only (`0600`), inside the Sailjail sandbox, so no other app can read them |
| Received files | your download folder, ordinary permissions — they are yours to open |

The history holds file names, peer names and destination paths. If that
metadata is unwelcome, it can be turned off in Settings.

## Building

Nothing here needs a device, and only the RPM needs the Sailfish SDK.

```bash
docker compose run --rm tests     # unit tests, including a loopback transfer
docker compose run --rm syntax    # compile-check every C++ file
docker compose run --rm qmllint   # parse every QML file
docker compose run --rm checks    # Qt 5.6 compatibility, QML traps, translations
docker compose run --rm icons     # redraw the app icon
```

The RPM is built by GitHub Actions for all three architectures. Tag a version
to publish a release:

```bash
git tag v1.0.0
git push origin v1.0.0
```

For a local build with the SDK:

```bash
sfdk config target=SailfishOS-4.5.0.18-armv7hl
sfdk build
```

## Sandbox permissions

Sailjail's `Base` profile starts an app with `net none` and every XDG
directory blacklisted, so each entry below buys back exactly one thing. They
are justified line by line in `harbour-localsend.desktop`.

| Permission | Why |
| --- | --- |
| `Internet` | Base allows only unix sockets. This grants `inet`/`inet6` — the socket family, not a destination. Nothing here reaches the Internet. |
| `Downloads` | Where received files are written by default. |
| `Documents`, `Pictures`, `Videos`, `Music` | Un-blacklists each directory so the picker can read from it. |
| `RemovableMedia` | Base sets `disable-mnt`, which hides the SD card. |
| `MediaIndexing` | Talk to Tracker. The picker's Documents/Images/Videos/Music/Downloads tabs are all tracker-backed and come up empty without it. |

Deliberately not requested: `Sharing`, and `UserDirs` — the latter would
bundle `PublicDir` in with the five directories above.

## Translations

English, French, German, Spanish, Finnish, Italian and Norwegian Bokmål.

The catalogues are generated from a single table so a string is translated
once and cannot drift between the files that use it:

```bash
docker compose run --rm checks               # says what is missing
python3 scripts/make_translations.py         # rewrites translations/*.ts
```

Corrections from native speakers are very welcome — Finnish and Norwegian in
particular have had no native review.

## Licence

MIT. Not affiliated with the LocalSend project.
