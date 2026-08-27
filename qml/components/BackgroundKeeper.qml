import QtQuick 2.0
import Nemo.KeepAlive 1.2

// Holds a wakelock for the duration of a transfer.
//
// Without it the device suspends with the screen and the socket goes quiet
// half way through a large file - which looks exactly like a network fault
// and is the single most common way a phone file transfer "fails".
//
// It lives in its own file so the root window can load it through a Loader:
// the import resolves on every Sailfish release we target, but a failed
// import inside the root would take the whole app down with it, and a
// battery optimisation is not worth that risk.
Item {
    id: keeper

    property bool active: false

    onActiveChanged: {
        if (active)
            activity.run()
        else
            activity.stop()
    }

    BackgroundActivity {
        id: activity
    }
}
