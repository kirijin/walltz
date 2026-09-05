// SPDX-FileCopyrightText: 2026 kirijin <avel.ronin@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

public class WalltzWindow : Gtk.ApplicationWindow {
    private WalltzApp app;
    private Controls controls;
    private Gtk.HeaderBar header_bar;
    private Gtk.Label status_label;

    private WtzImage* current_image = null;
    private WtzRenderParams render_params;

    public WalltzWindow (WalltzApp app) {
        this.app = app;
        this.default_width = 900;
        this.default_height = 700;
        this.title = "Walltz";
        reset_render_params ();
        build_ui ();
    }

    private void build_ui () {
        header_bar = new Gtk.HeaderBar ();
        header_bar.show_title_buttons = true;
        this.titlebar = header_bar;

        var save_button = new Gtk.Button.from_icon_name ("document-save");
        save_button.tooltip_text = "Save Wallpaper";
        header_bar.pack_end (save_button);

        var main_box = new Gtk.Box (Gtk.Orientation.VERTICAL, 0);
        this.child = main_box;

        var paned = new Gtk.Paned (Gtk.Orientation.HORIZONTAL);
        paned.position = 250;
        paned.wide_handle = true;
        main_box.append (paned);

        controls = new Controls ();
        controls.vexpand = true;
        paned.start_child = controls;

        var preview_box = new Gtk.Box (Gtk.Orientation.VERTICAL, 0);
        preview_box.vexpand = true;
        preview_box.hexpand = true;
        paned.end_child = preview_box;

        var preview_label = new Gtk.Label ("Drop an image to begin");
        preview_label.vexpand = true;
        preview_label.hexpand = true;
        preview_box.append (preview_label);

        status_label = new Gtk.Label ("Ready");
        status_label.margin_top = 6;
        status_label.margin_bottom = 6;
        status_label.margin_start = 12;
        main_box.append (status_label);

        controls.param_changed.connect (() => {
            status_label.label = "Params updated";
        });
    }

    private void reset_render_params () {
        render_params = WtzRenderParams ();
        render_params.target_width = 1920;
        render_params.target_height = 1080;
        render_params.blur_mode = 1;
        render_params.blur_radius = 90;
        render_params.saturation_factor = 1.8;
        render_params.fg_zoom = 0.8;
        render_params.pip_zoom = 1.0;
    }
}
