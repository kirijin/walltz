// SPDX-FileCopyrightText: 2026 kirijin <avel.ronin@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

public class PreviewWidget : Gtk.Box {
    private Gtk.Image image;
    private Gtk.Label placeholder_label;
    private Gtk.Stack stack;

    public PreviewWidget () {
        orientation = Gtk.Orientation.VERTICAL;
        spacing = 0;
        hexpand = true;
        vexpand = true;

        stack = new Gtk.Stack ();
        append (stack);

        // Placeholder
        placeholder_label = new Gtk.Label ("No image loaded") {
            opacity = 0.5
        };
        placeholder_label.add_context_class ("dim-label");
        stack.add_named (placeholder_label, "placeholder");

        // Image display
        image = new Gtk.Image () {
            icon_size = Gtk.IconSize.LARGE,
            pixel_size = 400
        };
        stack.add_named (image, "image");

        stack.visible_child_name = "placeholder";
    }

    public void set_image (WtzImage? img) {
        if (img == null) {
            stack.visible_child_name = "placeholder";
            return;
        }

        // Convert WtzImage to GdkPixbuf for display
        var pixbuf = image_from_wtz (img);
        if (pixbuf != null) {
            image.set_from_pixbuf (pixbuf);
            stack.visible_child_name = "image";
        } else {
            stack.visible_child_name = "placeholder";
        }
    }

    private Gdk.Pixbuf? image_from_wtz (WtzImage *img) {
        if (img == null || img->pixels == null) return null;

        var pixbuf = new Gdk.Pixbuf (Colorspace.RGB, true, 8, img->width, img->height);
        var dest = pixbuf.get_pixels ();
        var src = (uint8_t *) img->pixels;
        var dest_stride = pixbuf.rowstride;
        var src_stride = img->stride;

        for (int y = 0; y < img->height; y++) {
            Memory.copy (dest + y * dest_stride, src + y * src_stride, img->width * 4);
        }

        return pixbuf;
    }
}
