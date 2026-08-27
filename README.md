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

Transfers use **plain HTTP** on port 53317. The encrypted transport that the
desktop and mobile apps offer is not implemented here yet, because generating
a self-signed certificate needs machinery Qt 5.6 does not provide. In
practice:

- The protocol interoperates fine — LocalSend reads each peer's advertised
  transport and uses it.
- On a home network or your own hotspot, nobody is watching.
- On café, hotel or office Wi-Fi, treat a transfer as visible.

The PIN and the accept prompt protect against *unwanted* transfers, not
against eavesdropping. File names arriving from a peer are stripped of any
directory component before anything is written, so a hostile sender cannot
write outside your download folder — there is a test that proves it.

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
