// SPDX-FileCopyrightText: 2026 kirijin <avel.ronin@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

public class Controls : Gtk.Box {
    public signal void param_changed ();

    private Gtk.Entry width_entry;
    private Gtk.Entry height_entry;
    private Gtk.Scale blur_radius_scale;
    private Gtk.Scale saturation_scale;

    public Controls () {
        orientation = Gtk.Orientation.VERTICAL;
        spacing = 12;
        margin_top = 12;
        margin_bottom = 12;
        margin_start = 12;
        margin_end = 12;

        build_ui ();
        connect_signals ();
    }

    private Gtk.Label make_header(string text) {
        var label = new Gtk.Label (text);
        label.halign = Gtk.Align.START;
        label.add_css_class ("heading");
        return label;
    }

    private void build_ui () {
        var res_header = make_header ("Resolution");
        append (res_header);

        var res_box = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 6);
        append (res_box);

        width_entry = new Gtk.Entry ();
        width_entry.placeholder_text = "Width";
        width_entry.text = "1920";
        width_entry.input_purpose = Gtk.InputPurpose.DIGITS;
        width_entry.width_chars = 6;
        res_box.append (width_entry);

        var x_label = new Gtk.Label ("×");
        res_box.append (x_label);

        height_entry = new Gtk.Entry ();
        height_entry.placeholder_text = "Height";
        height_entry.text = "1080";
        height_entry.input_purpose = Gtk.InputPurpose.DIGITS;
        height_entry.width_chars = 6;
        res_box.append (height_entry);

        var blur_label = new Gtk.Label ("Blur Radius");
        blur_label.halign = Gtk.Align.START;
        append (blur_label);

        blur_radius_scale = new Gtk.Scale.with_range (Gtk.Orientation.HORIZONTAL, 0, 120, 5);
        blur_radius_scale.draw_value = true;
        blur_radius_scale.value_pos = Gtk.PositionType.RIGHT;
        blur_radius_scale.hexpand = true;
        blur_radius_scale.set_value (90);
        append (blur_radius_scale);

        var sat_label = new Gtk.Label ("Saturation");
        sat_label.halign = Gtk.Align.START;
        append (sat_label);

        saturation_scale = new Gtk.Scale.with_range (Gtk.Orientation.HORIZONTAL, 0, 3, 0.1);
        saturation_scale.draw_value = true;
        saturation_scale.value_pos = Gtk.PositionType.RIGHT;
        saturation_scale.hexpand = true;
        saturation_scale.set_value (1.8);
        append (saturation_scale);
    }

    private void connect_signals () {
        width_entry.changed.connect (() => param_changed ());
        height_entry.changed.connect (() => param_changed ());
        blur_radius_scale.value_changed.connect (() => param_changed ());
        saturation_scale.value_changed.connect (() => param_changed ());
    }

    public void apply_to_params (ref WtzRenderParams params) {
        params.target_width = int.parse (width_entry.text);
        params.target_height = int.parse (height_entry.text);
        params.blur_radius = (int) blur_radius_scale.get_value ();
        params.saturation_factor = saturation_scale.get_value ();
    }
}
