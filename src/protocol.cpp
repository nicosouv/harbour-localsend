#include "protocol.h"

#include <QByteArray>
#include <QStringList>
#include <QUuid>

#include "crypto.h"

namespace Protocol {

const char *Version = "2.1";

const char *ApiPrefix = "/api/localsend/v2";
const char *MulticastAddress = "224.0.0.167";

const char *PathInfo = "/info";
const char *PathRegister = "/register";
const char *PathPrepareUpload = "/prepare-upload";
const char *PathUpload = "/upload";
const char *PathCancel = "/cancel";

const char *PathLegacyInfo = "/api/localsend/v1/info";

bool isKnownDeviceType(const QString &type)
{
    return type == QLatin1String("mobile")
        || type == QLatin1String("desktop")
        || type == QLatin1String("web")
        || type == QLatin1String("headless")
        || type == QLatin1String("server");
}

QString sanitizeText(const QString &text, int maxLength)
{
    QString clean;
    clean.reserve(text.length());

    for (int i = 0; i < text.length() && clean.length() < maxLength; ++i) {
        const QChar character = text.at(i);
        const ushort code = character.unicode();

        // C0 and C1 control ranges, and DEL.
        if (code < 0x20 || code == 0x7F || (code >= 0x80 && code <= 0x9F))
            continue;

        // Bidirectional overrides and isolates: LRE, RLE, PDF, LRO, RLO and
        // the U+2066..U+2069 family. Nothing legitimate names a device or a
        // file with these, and they are the whole mechanism behind an
        // extension that displays backwards.
        if (code >= 0x202A && code <= 0x202E)
            continue;
        if (code >= 0x2066 && code <= 0x2069)
            continue;

        // Zero-width joiners and the byte-order mark, which render as nothing
        // and let two different names look identical.
        if (code == 0x200B || code == 0x200C || code == 0x200D || code == 0xFEFF)
            continue;

        // Angle brackets, because of how Qt decides what a string is. A Text
        // element defaults to AutoText: it sniffs the string, and one that
        // looks like markup is rendered as rich text - which draws tags and
        // will fetch an <img src="http://..."> from the network. An alias is
        // put on screen by several components whose internals are not ours to
        // configure, PageHeader among them, so the only place this can be
        // stopped for all of them is here, on the way in.
        if (code == '<' || code == '>')
            continue;

        clean.append(character);
    }

    return clean.trimmed();
}

QString generateAlias()
{
    // Deliberately the same shape as LocalSend's own generator, so a Sailfish
    // device does not stand out in someone else's device list.
    static const char *adjectives[] = {
        "Nice", "Secret", "Happy", "Brave", "Calm", "Swift", "Quiet", "Bright",
        "Clever", "Gentle", "Jolly", "Kind", "Lucky", "Merry", "Noble", "Proud",
        "Silent", "Smooth", "Sunny", "Warm", "Wise", "Bold", "Cosmic", "Curious"
    };
    static const char *nouns[] = {
        "Orange", "Banana", "Cherry", "Lemon", "Mango", "Melon", "Peach", "Pear",
        "Plum", "Apple", "Grape", "Kiwi", "Papaya", "Apricot", "Coconut", "Fig",
        "Lime", "Olive", "Walnut", "Almond", "Ginger", "Pepper", "Cocoa", "Vanilla"
    };

    const int adjectiveCount = int(sizeof(adjectives) / sizeof(adjectives[0]));
    const int nounCount = int(sizeof(nouns) / sizeof(nouns[0]));

    // Not a secret, so a fallback is acceptable here in a way it is not for
    // tokens: the worst case is two devices picking the same friendly name.
    QByteArray entropy = Crypto::randomBytes(2);
    if (entropy.size() < 2)
        entropy = QUuid::createUuid().toRfc4122();

    const int adjective = uchar(entropy.at(0)) % adjectiveCount;
    const int noun = uchar(entropy.at(1)) % nounCount;

    return QString::fromLatin1("%1 %2")
            .arg(QLatin1String(adjectives[adjective]))
            .arg(QLatin1String(nouns[noun]));
}

QString generateFingerprint()
{
    // Only used in plain-HTTP mode. Under HTTPS the fingerprint is the
    // certificate hash and comes from Certificate, because the protocol
    // defines it that way and a peer verifies it against what we present.
    // Same width and same casing either way, so nothing downstream can tell
    // the difference.
    return Crypto::randomHex(32).toUpper();
}

QString generateToken()
{
    // 192 bits from the CSPRNG. A token is a bearer capability - whoever
    // holds it may write a file to this device for the length of a session -
    // so it must not come from anything a peer could predict.
    return Crypto::randomHex(24);
}

} // namespace Protocol
