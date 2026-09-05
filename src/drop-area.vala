// SPDX-FileCopyrightText: 2026 kirijin <avel.ronin@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

public class DropArea : Gtk.Box {
    public signal void image_dropped (string path);

    private Gtk.Label label;
    private Gtk.Image icon;

    public DropArea () {
        orientation = Gtk.Orientation.VERTICAL;
        spacing = 12;
        halign = Gtk.Align.CENTER;
        valign = Gtk.Align.CENTER;
        margin_top = 24;
        margin_bottom = 24;
        margin_start = 24;
        margin_end = 24;

        icon = new Gtk.Image.from_icon_name ("image-x-generic") {
            pixel_size = 64,
            opacity = 0.5
        };
        append (icon);

        label = new Gtk.Label ("Drop image(s) here") {
            opacity = 0.5
        };
        label.add_css_class ("dim-label");
        append (label);

        // Drag and drop using URI list (simpler, well-supported)
        var drop_target = new Gtk.DropTarget (typeof (string), Gdk.DragAction.COPY);
        drop_target.drop.connect ((value, x, y) => {
            bool ret = false;
            if (value.holds (typeof (string))) {
                var uri = (string) value.get_string ();
                if (uri != null && uri.has_prefix ("file://")) {
                    try {
                        var file = File.new_for_uri (uri);
                        image_dropped (file.get_path ());
                        ret = true;
                    } catch (Error e) {
                        warning ("Drop error: %s", e.message);
                    }
                }
            }
            return ret;
        });
        add_controller (drop_target);
    }
}
