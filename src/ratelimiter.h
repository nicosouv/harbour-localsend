#ifndef RATELIMITER_H
#define RATELIMITER_H

#include <QElapsedTimer>
#include <QHash>
#include <QString>

// Throttles repeated failures from one address.
//
// It exists because of the PIN. A four-digit PIN is ten thousand guesses, and
// a peer on the same network can make them as fast as the phone will answer -
// which, without this, is thousands per minute. The work factor in the hash
// slows a stolen database down; only this slows down the network path, and
// the network path is the one an attacker actually has.
//
// The policy is a free allowance and then exponential backoff, which leaves a
// person who mistypes their own PIN twice completely unaffected while making
// an exhaustive search take longer than anybody will wait.
class RateLimiter
{
public:
    RateLimiter();

    // False when `key` is currently backed off and must be refused. Does not
    // itself count as an attempt.
    bool allow(const QString &key);

    // Seconds until `key` may try again; 0 when it may try now.
    int retryAfter(const QString &key);

    void recordFailure(const QString &key);
    void recordSuccess(const QString &key);

    // Test seam. Real time otherwise.
    void setClockOffset(qint64 milliseconds);

    void clear();

private:
    struct Entry
    {
        int failures;
        qint64 blockedUntil;   // ms on the same clock as now()

        Entry();
    };

    qint64 now() const;
    void prune();

    QHash<QString, Entry> m_entries;
    QElapsedTimer m_clock;
    qint64 m_offset;
};

#endif // RATELIMITER_H
