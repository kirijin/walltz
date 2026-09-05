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
        label.add_context_class ("dim-label");
        append (label);

        // Make it visually distinct
        get_style_context ().add_class ("drop-area");

        // Drag and drop
        var drop_target = new Gtk.DropTarget (typeof (Gdk.FileList), Gdk.DragAction.COPY);
        drop_target.on_value_received.connect (on_drop);
        add_controller (drop_target);

        var drop_target_uri = new Gtk.DropTarget (typeof (string), Gdk.DragAction.COPY);
        drop_target_uri.on_value_received.connect (on_drop_uri);
        add_controller (drop_target_uri);
    }

    private void on_drop (Value value) {
        var file_list = (Gdk.FileList) value;
        if (file_list != null && file_list.get_n_items () > 0) {
            var file = file_list.get_item (0) as File;
            if (file != null) {
                image_dropped (file.get_path ());
            }
        }
    }

    private void on_drop_uri (Value value) {
        var uri = (string) value;
        if (uri != null && uri.has_prefix ("file://")) {
            var file = File.new_for_uri (uri);
            image_dropped (file.get_path ());
        }
    }
}
