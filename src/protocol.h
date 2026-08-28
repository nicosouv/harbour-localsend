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

// Makes a string from the network safe to show and to use as a file name.
//
// Strips the characters that let a string lie about what it is. Control
// characters can blank a line or overwrite what came before it in a log; the
// Unicode bidirectional overrides are worse, because they reverse how the
// rest of the text is drawn. A name holding U+202E followed by "gpj.sh"
// renders as though it ended in ".jpg" while remaining a shell script on
// disk. The trick is decades old and costs nothing to close.
//
// Written out by codepoint rather than shown: a source file carrying one of
// these is doing the same thing to whoever reads the code, and the compiler
// warns about it (-Wbidi-chars).
//
// Also caps the length: an alias is a label, and a peer sending a megabyte of
// one is not naming a device.
QString sanitizeText(const QString &text, int maxLength);

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
