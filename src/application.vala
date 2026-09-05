// SPDX-FileCopyrightText: 2026 kirijin <avel.ronin@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

public class WalltzApp : Gtk.Application {
    public WalltzApp () {
        Object (
            application_id: "org.walltz.walltz",
            flags: ApplicationFlags.HANDLES_OPEN
        );
    }

    protected override void activate () {
        var window = new WalltzWindow (this);
        window.show ();
    }

    protected override void open (File[] files, string hint) {
        var window = new WalltzWindow (this);
        window.show ();
    }

    public static int main (string[] args) {
        var app = new WalltzApp ();
        return app.run (args);
    }
}
