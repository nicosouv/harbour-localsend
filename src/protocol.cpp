#include "protocol.h"

#include <QByteArray>
#include <QCryptographicHash>
#include <QDateTime>
#include <QStringList>
#include <QUuid>

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

    // QUuid seeds this better than qrand(), which we would have to seed by hand
    // and which is shared with everything else in the process.
    const QByteArray entropy = QUuid::createUuid().toRfc4122();
    const int adjective = uchar(entropy.at(0)) % adjectiveCount;
    const int noun = uchar(entropy.at(1)) % nounCount;

    return QString::fromLatin1("%1 %2")
            .arg(QLatin1String(adjectives[adjective]))
            .arg(QLatin1String(nouns[noun]));
}

QString generateFingerprint()
{
    QByteArray entropy;
    entropy += QUuid::createUuid().toRfc4122();
    entropy += QUuid::createUuid().toRfc4122();
    entropy += QByteArray::number(QDateTime::currentMSecsSinceEpoch());
    return QString::fromLatin1(
        QCryptographicHash::hash(entropy, QCryptographicHash::Sha256).toHex());
}

QString generateToken()
{
    return QString::fromLatin1(QUuid::createUuid().toRfc4122().toHex());
}

} // namespace Protocol
