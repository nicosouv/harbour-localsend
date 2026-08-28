import QtQuick 2.0
import Sailfish.Share 1.0

// The app's end of the Sailfish share sheet.
//
// This is what puts "LocalSend" in the list when somebody taps Share in
// Gallery or the file manager. The other half is in the .desktop file: the
// method name here must match its X-Share-Methods entry, and the capabilities
// must match the Capabilities line of its [X-Share Method] section, or the
// entry appears and then does nothing.
//
// Loaded through a Loader rather than declared in the window, because a
// device without Sailfish.Share would fail the root QML file - and that is
// the one file whose failure means the app does not start at all.
ShareProvider {
    id: provider

    // Absolute paths, already stripped of any file:// prefix.
    signal filesShared(var paths)

    method: "files"
    // "*" rather than a list: LocalSend sends bytes and does not care what
    // they are, and an enumeration would only be a list of the things it
    // refuses to be useful for.
    capabilities: ["*"]
    // Owns harbour-localsend.harbour-localsend on the session bus, which is
    // the name the share framework calls and the only one Sailjail permits us
    // (it is OrganizationName.ApplicationName from the desktop file).
    registerName: true

    onTriggered: {
        if (!resources)
            return

        var paths = []
        for (var i = 0; i < resources.length; ++i) {
            var resource = resources[i]
            var path = ""

            // The share framework hands over either a plain string or an
            // object, and which one has varied by release. Reading whichever
            // is there costs a line and saves a bug report.
            if (typeof resource === "string") {
                path = resource
            } else if (resource) {
                path = (resource.filePath || resource.url || resource.path
                        || resource.data || "").toString()
            }

            if (path.indexOf("file://") === 0)
                path = path.substring(7)
            if (path.length > 0)
                paths.push(path)
        }

        if (paths.length > 0)
            provider.filesShared(paths)
    }
}
