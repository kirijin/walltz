// SPDX-FileCopyrightText: 2026 kirijin <avel.ronin@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

public class Controls : Gtk.Box {
    public signal void param_changed ();

    // Resolution
    private Gtk.Entry width_entry;
    private Gtk.Entry height_entry;
    private Gtk.Button detect_button;

    // Background mode
    private Gtk.ComboBoxText bg_mode_combo;
    private Gtk.ComboBoxText gradient_preset_combo;
    private Gtk.ComboBoxText mood_combo;
    private Gtk.ComboBoxText blur_preset_combo;

    // Blur
    private Gtk.Scale blur_radius_scale;
    private Gtk.Scale saturation_scale;
    private Gtk.Scale bg_zoom_scale;
    private Gtk.Scale gradient_angle_scale;

    // Effects
    private Gtk.Scale vignette_scale;
    private Gtk.Scale grain_scale;
    private Gtk.Scale ca_scale;

    // Photo frame
    private Gtk.Switch frame_switch;
    private Gtk.Scale frame_width_scale;

    // Foreground
    private Gtk.Scale fg_zoom_scale;
    private Gtk.Scale pip_zoom_scale;

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

    private Gtk.Label make_header (string text) {
        var label = new Gtk.Label (text);
        label.halign = Gtk.Align.START;
        label.add_css_class ("heading");
        return label;
    }

    private void build_ui () {
        // ── Resolution ──
        append (make_header ("Resolution"));

        var res_box = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 6);
        append (res_box);

        width_entry = new Gtk.Entry () {
            placeholder_text = "Width",
            text = "1920",
            input_purpose = Gtk.InputPurpose.DIGITS,
            width_chars = 6
        };
        res_box.append (width_entry);

        var x_label = new Gtk.Label ("×");
        res_box.append (x_label);

        height_entry = new Gtk.Entry () {
            placeholder_text = "Height",
            text = "1080",
            input_purpose = Gtk.InputPurpose.DIGITS,
            width_chars = 6
        };
        res_box.append (height_entry);

        detect_button = new Gtk.Button.from_icon_name ("video-display") {
            tooltip_text = "Detect screen resolution"
        };
        res_box.append (detect_button);

        // ── Background mode ──
        append (make_header ("Background"));

        bg_mode_combo = new Gtk.ComboBoxText ();
        bg_mode_combo.append_text ("Blur");
        bg_mode_combo.append_text ("Solid Color");
        bg_mode_combo.append_text ("Gradient Preset");
        bg_mode_combo.append_text ("Mood (Auto-Color)");
        bg_mode_combo.active = 0;
        append (bg_mode_combo);

        // ── Blur preset ──
        append (make_header ("Blur Preset"));

        blur_preset_combo = new Gtk.ComboBoxText ();
        for (int i = 0; i < wtz_blur_preset_count (); i++) {
            blur_preset_combo.append_text (wtz_blur_preset (i).display_name);
        }
        blur_preset_combo.active = 0;
        append (blur_preset_combo);

        // ── Gradient preset ──
        gradient_preset_combo = new Gtk.ComboBoxText ();
        for (int i = 0; i < wtz_gradient_preset_count (); i++) {
            gradient_preset_combo.append_text (wtz_gradient_preset (i).name);
        }
        gradient_preset_combo.active = 0;
        append (gradient_preset_combo);

        // ── Mood ──
        mood_combo = new Gtk.ComboBoxText ();
        mood_combo.append_text ("Auto");
        mood_combo.append_text ("Soft");
        mood_combo.append_text ("Vivid");
        mood_combo.append_text ("Warm");
        mood_combo.append_text ("Cool");
        mood_combo.append_text ("Deep");
        mood_combo.active = 0;
        append (mood_combo);

        // ── Blur radius ──
        append (make_header ("Blur Radius"));

        blur_radius_scale = new Gtk.Scale.with_range (Gtk.Orientation.HORIZONTAL, 0, 120, 5) {
            draw_value = true,
            value_pos = Gtk.PositionType.RIGHT,
            hexpand = true
        };
        blur_radius_scale.set_value (90);
        append (blur_radius_scale);

        // ── Saturation ──
        append (make_header ("Saturation"));

        saturation_scale = new Gtk.Scale.with_range (Gtk.Orientation.HORIZONTAL, 0, 3, 0.1) {
            draw_value = true,
            value_pos = Gtk.PositionType.RIGHT,
            hexpand = true
        };
        saturation_scale.set_value (1.8);
        append (saturation_scale);

        // ── Background zoom ──
        append (make_header ("Background Zoom"));

        bg_zoom_scale = new Gtk.Scale.with_range (Gtk.Orientation.HORIZONTAL, 0.5, 3, 0.1) {
            draw_value = true,
            value_pos = Gtk.PositionType.RIGHT,
            hexpand = true
        };
        bg_zoom_scale.set_value (1.0);
        append (bg_zoom_scale);

        // ── Gradient angle ──
        append (make_header ("Gradient Angle"));

        gradient_angle_scale = new Gtk.Scale.with_range (Gtk.Orientation.HORIZONTAL, 0, 360, 5) {
            draw_value = true,
            value_pos = Gtk.PositionType.RIGHT,
            hexpand = true
        };
        gradient_angle_scale.set_value (45);
        append (gradient_angle_scale);

        // ── Effects ──
        append (make_header ("Effects"));

        // Vignette
        append (new Gtk.Label ("Vignette") { halign = Gtk.Align.START });

        vignette_scale = new Gtk.Scale.with_range (Gtk.Orientation.HORIZONTAL, 0, 1, 0.05) {
            draw_value = true,
            value_pos = Gtk.PositionType.RIGHT,
            hexpand = true
        };
        append (vignette_scale);

        // Grain
        append (new Gtk.Label ("Grain") { halign = Gtk.Align.START });

        grain_scale = new Gtk.Scale.with_range (Gtk.Orientation.HORIZONTAL, 0, 1, 0.05) {
            draw_value = true,
            value_pos = Gtk.PositionType.RIGHT,
            hexpand = true
        };
        append (grain_scale);

        // Chromatic aberration
        append (new Gtk.Label ("Chromatic Aberration") { halign = Gtk.Align.START });

        ca_scale = new Gtk.Scale.with_range (Gtk.Orientation.HORIZONTAL, 0, 1, 0.05) {
            draw_value = true,
            value_pos = Gtk.PositionType.RIGHT,
            hexpand = true
        };
        append (ca_scale);

        // ── Photo frame ──
        var frame_box = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 6);
        append (frame_box);

        frame_box.append (new Gtk.Label ("Photo Frame") { halign = Gtk.Align.START, hexpand = true });

        frame_switch = new Gtk.Switch () { halign = Gtk.Align.END };
        frame_box.append (frame_switch);

        frame_width_scale = new Gtk.Scale.with_range (Gtk.Orientation.HORIZONTAL, 0, 25, 1) {
            draw_value = true,
            value_pos = Gtk.PositionType.RIGHT,
            hexpand = true,
            sensitive = false
        };
        append (frame_width_scale);

        // ── Foreground ──
        append (make_header ("Foreground"));

        append (new Gtk.Label ("Foreground Zoom") { halign = Gtk.Align.START });

        fg_zoom_scale = new Gtk.Scale.with_range (Gtk.Orientation.HORIZONTAL, 0.5, 1.0, 0.05) {
            draw_value = true,
            value_pos = Gtk.PositionType.RIGHT,
            hexpand = true
        };
        fg_zoom_scale.set_value (0.8);
        append (fg_zoom_scale);

        append (new Gtk.Label ("PiP Zoom") { halign = Gtk.Align.START });

        pip_zoom_scale = new Gtk.Scale.with_range (Gtk.Orientation.HORIZONTAL, 1.0, 4.0, 0.1) {
            draw_value = true,
            value_pos = Gtk.PositionType.RIGHT,
            hexpand = true
        };
        pip_zoom_scale.set_value (1.0);
        append (pip_zoom_scale);

        // ── Reset button ──
        var reset_button = new Gtk.Button.with_label ("Reset All") { margin_top = 12 };
        reset_button.add_css_class ("destructive-action");
        reset_button.clicked.connect (on_reset_clicked);
        append (reset_button);
    }

    private void connect_signals () {
        width_entry.changed.connect (() => param_changed ());
        height_entry.changed.connect (() => param_changed ());
        detect_button.clicked.connect (() => {
            width_entry.text = "1920";
            height_entry.text = "1080";
        });
        blur_radius_scale.value_changed.connect (() => param_changed ());
        saturation_scale.value_changed.connect (() => param_changed ());
        bg_zoom_scale.value_changed.connect (() => param_changed ());
        gradient_angle_scale.value_changed.connect (() => param_changed ());
        vignette_scale.value_changed.connect (() => param_changed ());
        grain_scale.value_changed.connect (() => param_changed ());
        ca_scale.value_changed.connect (() => param_changed ());
        bg_mode_combo.changed.connect (on_bg_mode_changed);
        gradient_preset_combo.changed.connect (() => param_changed ());
        mood_combo.changed.connect (() => param_changed ());
        blur_preset_combo.changed.connect (on_blur_preset_changed);
        frame_switch.notify["active"].connect (on_frame_toggled);
        frame_width_scale.value_changed.connect (() => param_changed ());
        fg_zoom_scale.value_changed.connect (() => param_changed ());
        pip_zoom_scale.value_changed.connect (() => param_changed ());
    }

    private void on_reset_clicked () {
        blur_radius_scale.set_value (90);
        saturation_scale.set_value (1.8);
        bg_zoom_scale.set_value (1.0);
        gradient_angle_scale.set_value (45);
        vignette_scale.set_value (0);
        grain_scale.set_value (0);
        ca_scale.set_value (0);
        frame_switch.active = false;
        frame_width_scale.set_value (0);
        fg_zoom_scale.set_value (0.8);
        pip_zoom_scale.set_value (1.0);
    }

    private void on_bg_mode_changed () {
        int active = bg_mode_combo.active;
        blur_preset_combo.sensitive = (active == 0);
        gradient_preset_combo.sensitive = (active == 2);
        mood_combo.sensitive = (active == 3);
        param_changed ();
    }

    private void on_blur_preset_changed () {
        int idx = blur_preset_combo.active;
        if (idx < 0 || idx >= wtz_blur_preset_count ()) return;

        var preset = wtz_blur_preset (idx);

        blur_radius_scale.set_value (preset.sigma);
        saturation_scale.set_value (preset.sat_boost);
        bg_zoom_scale.set_value (1.0);
        vignette_scale.set_value (preset.vignette);
        grain_scale.set_value (preset.grain);
        frame_switch.active = preset.frame_enabled;
        frame_width_scale.set_value (preset.frame_width_pct);

        param_changed ();
    }

    private void on_frame_toggled () {
        frame_width_scale.sensitive = frame_switch.active;
        param_changed ();
    }

    public void set_params (WtzRenderParams* p) {
        width_entry.text = p->target_width.to_string ();
        height_entry.text = p->target_height.to_string ();
        blur_radius_scale.set_value (p->blur_radius);
        saturation_scale.set_value (p->saturation_factor);
        bg_zoom_scale.set_value (p->bg_zoom);
        gradient_angle_scale.set_value (p->gradient_angle);
        vignette_scale.set_value (p->vignette_strength);
        grain_scale.set_value (p->grain_strength);
        ca_scale.set_value (p->ca_strength);
        fg_zoom_scale.set_value (p->fg_zoom);
        pip_zoom_scale.set_value (p->pip_zoom);
        frame_switch.active = p->photo_frame != 0;
        frame_width_scale.set_value (p->photo_frame_width);
        bg_mode_combo.active = p->blur_mode != 0 ? 0 : (p->bg_gradient_style + 1);
    }

    public void apply_to_params (WtzRenderParams* p) {
        p->target_width = int.parse (width_entry.text);
        p->target_height = int.parse (height_entry.text);
        p->blur_radius = (int) blur_radius_scale.get_value ();
        p->saturation_factor = saturation_scale.get_value ();
        p->bg_zoom = bg_zoom_scale.get_value ();
        p->gradient_angle = gradient_angle_scale.get_value ();
        p->vignette_strength = vignette_scale.get_value ();
        p->grain_strength = grain_scale.get_value ();
        p->ca_strength = ca_scale.get_value ();
        p->fg_zoom = fg_zoom_scale.get_value ();
        p->pip_zoom = pip_zoom_scale.get_value ();
        p->photo_frame = frame_switch.active ? 1 : 0;
        p->photo_frame_width = (int) frame_width_scale.get_value ();
        p->blur_mode = (bg_mode_combo.active == 0) ? 1 : 0;
        p->bg_gradient_style = bg_mode_combo.active - 1;
        if (p->bg_gradient_style < 0) p->bg_gradient_style = 0;
        if (p->bg_gradient_style > 2) p->bg_gradient_style = 2;
        p->bg_gradient_preset = gradient_preset_combo.active;
        p->auto_mood = mood_combo.active;
    }
}
