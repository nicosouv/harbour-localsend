#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QString>

// Constants and small helpers shared by every layer that speaks LocalSend.
// Nothing here holds state: the wire format lives in one place so a protocol
// bump is a single edit rather than a grep.
namespace Protocol {

// The version we advertise. Peers negotiate nothing, they just read it, so it
// has to name a version whose endpoints we actually implement.
extern const char *Version;

extern const char *ApiPrefix;          // "/api/localsend/v2"
extern const char *MulticastAddress;   // "224.0.0.167"

const int DefaultPort = 53317;

// A prepare-upload request parks a live connection until the user decides.
// Senders wait, but not forever, and neither do we.
const int AcceptTimeoutMs = 90 * 1000;

// Endpoint paths, relative to ApiPrefix.
extern const char *PathInfo;
extern const char *PathRegister;
extern const char *PathPrepareUpload;
extern const char *PathUpload;
extern const char *PathCancel;

// Both spellings of the info endpoint are in the wild: LocalSend moved it under
// the versioned prefix but older clients still probe the unversioned one.
extern const char *PathLegacyInfo;

// deviceType values the protocol defines. Anything else is shown as unknown.
bool isKnownDeviceType(const QString &type);

// A LocalSend-style "Nice Orange": human readable, unique enough on a LAN.
QString generateAlias();

// 64 hex characters, generated once per install and persisted. In HTTPS mode
// this would be the certificate hash; we run plain HTTP, so a random value is
// exactly what the spec asks for.
QString generateFingerprint();

// URL-safe opaque identifiers for sessions and per-file upload tokens.
QString generateToken();

} // namespace Protocol

#endif // PROTOCOL_H
