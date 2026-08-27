.pragma library

// Numbers turned into something readable. Deliberately free of qsTr(): a
// pragma library has no translation context, so anything here has to be a
// unit symbol that reads the same everywhere. Sentences are assembled in QML,
// where they can be translated.

function fileSize(bytes) {
    if (bytes === undefined || bytes === null || bytes < 0)
        return "0 B"
    if (bytes < 1024)
        return bytes + " B"

    var units = ["kB", "MB", "GB", "TB"]
    var value = bytes / 1024
    var unit = 0

    while (value >= 1024 && unit < units.length - 1) {
        value /= 1024
        unit++
    }

    // One decimal below 100, none above: "9.7 MB" is useful precision,
    // "947.3 MB" is noise on a phone-sized label.
    var decimals = value < 100 ? 1 : 0
    return value.toFixed(decimals) + " " + units[unit]
}

function speed(bytesPerSecond) {
    if (!bytesPerSecond || bytesPerSecond <= 0)
        return ""
    return fileSize(bytesPerSecond) + "/s"
}

// "45s", "2m 14s", "1h 03m". Never returns a bare number of seconds above a
// minute, because "134" tells nobody anything.
function duration(seconds) {
    if (seconds === undefined || seconds === null || seconds < 0)
        return ""

    var whole = Math.round(seconds)
    if (whole < 60)
        return whole + "s"

    var minutes = Math.floor(whole / 60)
    if (minutes < 60)
        return minutes + "m " + pad(whole % 60) + "s"

    var hours = Math.floor(minutes / 60)
    return hours + "h " + pad(minutes % 60) + "m"
}

function pad(value) {
    return value < 10 ? "0" + value : "" + value
}

function percent(fraction) {
    if (!fraction || fraction < 0)
        return "0%"
    return Math.round(Math.min(fraction, 1) * 100) + "%"
}

// A file list summarised for one line: the first name, then how many more.
// The caller supplies the "and %n more" wording so it can be translated.
function firstName(names) {
    if (!names || names.length === 0)
        return ""
    return names[0]
}
