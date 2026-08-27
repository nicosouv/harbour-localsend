.pragma library

// Gives every device a colour of its own, derived from its fingerprint.
//
// The point is recognition rather than decoration: two laptops called
// "Secret Banana" and "Secret Blueberry" are one glance apart in a list, and
// a stable colour makes them tell apart at a distance. The fingerprint is
// what the colour hangs off because it is the one field that never changes -
// an alias can be edited, an address moves with the network.
//
// Colours are computed rather than looked up so a network of twenty devices
// still gets twenty distinguishable ones, and returned as hex because a
// pragma library should not depend on the Qt object being in scope.

function hash(text) {
    var value = 0
    if (!text)
        return 0
    for (var i = 0; i < text.length; i++) {
        value = ((value << 5) - value) + text.charCodeAt(i)
        value = value & value   // keep it a 32-bit int
    }
    return Math.abs(value)
}

function accent(fingerprint, dark) {
    // Golden-angle steps around the wheel: consecutive hashes land far apart
    // rather than in the same corner of the spectrum.
    var hue = (hash(fingerprint) * 137.508) % 360
    // Blues and purples read as "system" on this platform, so the band is
    // kept wide but the extremes are toned down.
    var saturation = 0.52
    var lightness = dark ? 0.62 : 0.46
    return hslToHex(hue / 360, saturation, lightness)
}

function hslToHex(h, s, l) {
    var r, g, b

    if (s === 0) {
        r = g = b = l
    } else {
        var q = l < 0.5 ? l * (1 + s) : l + s - l * s
        var p = 2 * l - q
        r = hueToChannel(p, q, h + 1 / 3)
        g = hueToChannel(p, q, h)
        b = hueToChannel(p, q, h - 1 / 3)
    }

    return "#" + byteToHex(r) + byteToHex(g) + byteToHex(b)
}

function hueToChannel(p, q, t) {
    if (t < 0)
        t += 1
    if (t > 1)
        t -= 1
    if (t < 1 / 6)
        return p + (q - p) * 6 * t
    if (t < 1 / 2)
        return q
    if (t < 2 / 3)
        return p + (q - p) * (2 / 3 - t) * 6
    return p
}

function byteToHex(value) {
    var byte = Math.round(Math.max(0, Math.min(1, value)) * 255)
    var hex = byte.toString(16)
    return hex.length === 1 ? "0" + hex : hex
}

// The two letters shown inside a device's badge, from its alias.
function initials(alias) {
    if (!alias)
        return "?"

    var words = alias.trim().split(/\s+/)
    if (words.length === 1)
        return words[0].substring(0, 2).toUpperCase()
    return (words[0].charAt(0) + words[1].charAt(0)).toUpperCase()
}
