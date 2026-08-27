#include "ratelimiter.h"

namespace {

// Mistyping your own PIN twice should cost nothing, so the first few failures
// are free.
const int FreeAttempts = 3;

// Doubling from one second. The tenth failure already costs about two
// minutes, and the cap keeps a wrong guess from locking a device out for the
// rest of the day.
const qint64 BaseBackoffMs = 1000;
const qint64 MaxBackoffMs = 15 * 60 * 1000;

// An entry is forgotten once it has been quiet for this long, so a neighbour
// who fat-fingered a PIN last week starts clean.
const qint64 ForgetAfterMs = 60 * 60 * 1000;

// A ceiling on how many addresses are tracked at once. Reaching it means
// something is scanning us, and at that point remembering every source
// address is itself the attack.
const int MaxEntries = 512;

} // namespace

RateLimiter::Entry::Entry()
    : failures(0)
    , blockedUntil(0)
{
}

RateLimiter::RateLimiter()
    : m_offset(0)
{
    m_clock.start();
}

qint64 RateLimiter::now() const
{
    return m_clock.elapsed() + m_offset;
}

void RateLimiter::setClockOffset(qint64 milliseconds)
{
    m_offset = milliseconds;
}

bool RateLimiter::allow(const QString &key)
{
    const QHash<QString, Entry>::const_iterator it = m_entries.constFind(key);
    if (it == m_entries.constEnd())
        return true;
    return now() >= it.value().blockedUntil;
}

int RateLimiter::retryAfter(const QString &key)
{
    const QHash<QString, Entry>::const_iterator it = m_entries.constFind(key);
    if (it == m_entries.constEnd())
        return 0;

    const qint64 remaining = it.value().blockedUntil - now();
    if (remaining <= 0)
        return 0;
    // Rounded up: answering "retry in 0 seconds" while still refusing is the
    // kind of thing that sends a client into a tight loop.
    return int((remaining + 999) / 1000);
}

void RateLimiter::recordFailure(const QString &key)
{
    prune();

    if (!m_entries.contains(key) && m_entries.size() >= MaxEntries) {
        // Nothing sensible left to record. Refusing to grow is the point.
        return;
    }

    Entry &entry = m_entries[key];
    ++entry.failures;

    if (entry.failures <= FreeAttempts)
        return;

    qint64 backoff = BaseBackoffMs;
    for (int i = FreeAttempts + 1; i < entry.failures && backoff < MaxBackoffMs; ++i)
        backoff *= 2;
    if (backoff > MaxBackoffMs)
        backoff = MaxBackoffMs;

    entry.blockedUntil = now() + backoff;
}

void RateLimiter::recordSuccess(const QString &key)
{
    m_entries.remove(key);
}

void RateLimiter::prune()
{
    const qint64 current = now();

    QHash<QString, Entry>::iterator it = m_entries.begin();
    while (it != m_entries.end()) {
        // Only entries that are no longer blocked and have been idle past the
        // window are dropped; blockedUntil doubles as the last-seen marker.
        if (it.value().blockedUntil != 0
                && current - it.value().blockedUntil > ForgetAfterMs) {
            it = m_entries.erase(it);
        } else {
            ++it;
        }
    }
}

void RateLimiter::clear()
{
    m_entries.clear();
}
