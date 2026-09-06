// SPDX-FileCopyrightText: 2026 kirijin <avel.ronin@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

public class WalltzWindow : Gtk.ApplicationWindow {
    private WalltzApp app;
    private Gtk.HeaderBar header_bar;
    private Gtk.Label status_label;
    private Gtk.ProgressBar progress_bar;

    // UI panels
    private Controls controls;
    private Gtk.Picture preview_picture;
    private Gtk.Label preview_label;
    private Gtk.Overlay preview_overlay;
    private DropArea drop_area;

    // Engine state
    private WtzImage* current_image = null;
    private WtzImage* current_preview = null;
    private WtzRenderParams* render_params;
    private bool is_busy = false;

    public WalltzWindow (WalltzApp app) {
        this.app = app;
        this.default_width = 1000;
        this.default_height = 700;
        this.title = "Walltz";
        this.render_params = malloc (sizeof (WtzRenderParams));
        reset_render_params ();
        build_ui ();
        connect_signals ();
    }

    ~WalltzWindow () {
        if (render_params != null) {
            free (render_params);
        }
    }

    private void build_ui () {
        header_bar = new Gtk.HeaderBar ();
        header_bar.show_title_buttons = true;
        this.titlebar = header_bar;

        // Save button
        var save_button = new Gtk.Button.from_icon_name ("document-save");
        save_button.tooltip_text = "Save Wallpaper";
        save_button.sensitive = false;
        save_button.clicked.connect (on_save_clicked);
        header_bar.pack_end (save_button);

        // Set wallpaper button
        var set_wallpaper_button = new Gtk.Button.from_icon_name ("preferences-desktop-wallpaper");
        set_wallpaper_button.tooltip_text = "Set as Wallpaper";
        set_wallpaper_button.sensitive = false;
        set_wallpaper_button.clicked.connect (on_set_wallpaper_clicked);
        header_bar.pack_end (set_wallpaper_button);

        // Open button
        var open_button = new Gtk.Button.from_icon_name ("document-open");
        open_button.tooltip_text = "Open Image";
        open_button.clicked.connect (on_open_clicked);
        header_bar.pack_start (open_button);

        var main_box = new Gtk.Box (Gtk.Orientation.VERTICAL, 0);
        this.child = main_box;

        var paned = new Gtk.Paned (Gtk.Orientation.HORIZONTAL);
        paned.position = 280;
        paned.wide_handle = true;
        main_box.append (paned);

        var left_scroll = new Gtk.ScrolledWindow ();
        left_scroll.hscrollbar_policy = Gtk.PolicyType.NEVER;
        left_scroll.vexpand = true;
        controls = new Controls ();
        left_scroll.child = controls;
        paned.start_child = left_scroll;

        var right_box = new Gtk.Box (Gtk.Orientation.VERTICAL, 0);
        paned.end_child = right_box;

        preview_overlay = new Gtk.Overlay ();
        preview_overlay.vexpand = true;
        preview_overlay.hexpand = true;
        right_box.append (preview_overlay);

        preview_picture = new Gtk.Picture ();
        preview_picture.can_shrink = true;
        preview_picture.keep_aspect_ratio = true;
        preview_picture.halign = Gtk.Align.CENTER;
        preview_picture.valign = Gtk.Align.CENTER;
        preview_picture.vexpand = true;
        preview_picture.hexpand = true;
        preview_overlay.set_child (preview_picture);

        preview_label = new Gtk.Label ("Drop an image or click Open");
        preview_label.add_css_class ("dim-label");
        preview_label.vexpand = true;
        preview_label.hexpand = true;
        preview_label.halign = Gtk.Align.CENTER;
        preview_label.valign = Gtk.Align.CENTER;
        preview_overlay.add_overlay (preview_label);

        drop_area = new DropArea ();
        drop_area.vexpand = true;
        drop_area.hexpand = true;
        drop_area.set_halign (Gtk.Align.FILL);
        drop_area.set_valign (Gtk.Align.FILL);
        drop_area.image_dropped.connect (load_image);
        preview_overlay.add_overlay (drop_area);

        var status_box = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 6);
        status_box.margin_top = 6;
        status_box.margin_bottom = 6;
        status_box.margin_start = 12;
        status_box.margin_end = 12;
        main_box.append (status_box);

        status_label = new Gtk.Label ("Ready");
        status_label.halign = Gtk.Align.START;
        status_label.hexpand = true;
        status_box.append (status_label);

        progress_bar = new Gtk.ProgressBar ();
        progress_bar.visible = false;
        progress_bar.pulse_step = 0.05;
        status_box.append (progress_bar);
    }

    private void connect_signals () {
        controls.param_changed.connect (on_param_changed);
    }

    private void reset_render_params () {
        render_params.target_width = 1920;
        render_params.target_height = 1080;
        render_params.blur_mode = 1;
        render_params.blur_radius = 90;
        render_params.saturation_factor = 1.8;
        render_params.bg_zoom = 1.0;
        render_params.bg_blur_angle = 0.0;
        render_params.blur_brightness = 1.0;
        render_params.auto_color = 1;
        render_params.bg_color = 0xFFFFFFFFU;
        render_params.bg_gradient_preset = 0;
        render_params.gradient_angle = 45.0;
        render_params.mood_color_a = 0xFF808080U;
        render_params.mood_color_b = 0xFFB4B4B4U;
        render_params.vignette_strength = 0.0;
        render_params.grain_strength = 0.0;
        render_params.ca_strength = 0.0;
        render_params.color_gamma = 1.0;
        render_params.color_warmth = 0.0;
        render_params.color_black_lift = 0.0;
        render_params.photo_frame = 0;
        render_params.photo_frame_width = 0;
        render_params.fg_zoom = 0.8;
        render_params.pip_zoom = 1.0;
        render_params.photo_grade = 0;
        render_params.auto_mood = 0;
        render_params.use_v2 = 0;
    }

    private void on_open_clicked () {
        var dialog = new Gtk.FileDialog ();
        dialog.title = "Open Image";

        var filters = new ListStore (typeof (Gtk.FileFilter));
        var image_filter = new Gtk.FileFilter ();
        image_filter.name = "Image files";
        image_filter.add_mime_type ("image/png");
        image_filter.add_mime_type ("image/jpeg");
        image_filter.add_mime_type ("image/webp");
        image_filter.add_mime_type ("image/svg+xml");
        filters.append (image_filter);
        var all_filter = new Gtk.FileFilter ();
        all_filter.name = "All files";
        all_filter.add_pattern ("*");
        filters.append (all_filter);
        dialog.filters = filters;

        dialog.open.begin (this, null, (obj, res) => {
            try {
                var file = dialog.open.end (res);
                if (file != null) {
                    load_image (file.get_path ());
                }
            } catch (Error e) {
                status_label.label = "Open failed: %s".printf (e.message);
            }
        });
    }

    public void load_image (string path) {
        status_label.label = "Loading: %s".printf (Path.get_basename (path));

        new Thread<void> ("load-image", () => {
            var img = wtz_image_load (path);
            Idle.add (() => {
                if (img != null) {
                    current_image = img;
                    var palettes = wtz_extract_mood_palettes (img);
                    if (palettes != null) {
                        render_params.mood_color_a = palettes.moods[0].color_a;
                        render_params.mood_color_b = palettes.moods[0].color_b;
                        wtz_mood_palettes_free (palettes);
                    }
                    var smart = WtzSmartAutoParams ();
                    wtz_compute_smart_auto (img, out smart);
                    render_params.blur_radius = (int) smart.sigma;
                    render_params.saturation_factor = smart.sat_boost;
                    render_params.blur_brightness = smart.brightness;
                    render_params.overlay_opacity = smart.overlay_opacity;
                    render_params.overlay_color = smart.overlay_color;
                    controls.set_params (render_params);
                    status_label.label = "Loaded: %s".printf (Path.get_basename (path));
                    generate_preview ();
                } else {
                    status_label.label = "Failed to load: %s".printf (Path.get_basename (path));
                }
                return false;
            });
        });
    }

    private void on_param_changed () {
        if (current_image != null) {
            generate_preview ();
        }
    }

    private void generate_preview () {
        if (current_image == null || is_busy) return;

        is_busy = true;
        progress_bar.visible = true;
        progress_bar.pulse ();

        controls.apply_to_params (render_params);

        var src = current_image;

        new Thread<void> ("render-preview", () => {
            var result = wtz_render (src, render_params);
            Idle.add (() => {
                if (result != null) {
                    current_preview = result;
                    set_preview_picture (result);
                    status_label.label = "Preview: %dx%d".printf (result.width, result.height);
                }
                is_busy = false;
                progress_bar.visible = false;
                return false;
            });
        });
    }

    private void set_preview_picture (WtzImage *img) {
        if (img == null) return;

        var pixbuf = image_to_pixbuf (img);
        if (pixbuf != null) {
            preview_picture.set_pixbuf (pixbuf);
            preview_label.visible = false;
        }
    }

    private Gdk.Pixbuf? image_to_pixbuf (WtzImage *img) {
        if (img == null || img->pixels == null) return null;

        var pixbuf = new Gdk.Pixbuf (Gdk.Colorspace.RGB, true, 8, img->width, img->height);
        var dest = pixbuf.get_pixels ();
        var src = img->pixels;
        var dest_stride = pixbuf.rowstride;
        var src_stride = img->stride;

        for (int y = 0; y < img->height; y++) {
            for (int x = 0; x < img->width * 4; x++) {
                dest[y * dest_stride + x] = src[y * src_stride + x];
            }
        }

        return pixbuf;
    }

    private void on_save_clicked () {
        if (current_image == null) return;

        var dialog = new Gtk.FileDialog ();
        dialog.title = "Save Wallpaper";
        dialog.initial_name = "wallpaper.png";

        var filters = new ListStore (typeof (Gtk.FileFilter));
        var png_filter = new Gtk.FileFilter ();
        png_filter.name = "PNG images";
        png_filter.add_mime_type ("image/png");
        filters.append (png_filter);
        dialog.filters = filters;

        dialog.save.begin (this, null, (obj, res) => {
            try {
                var file = dialog.save.end (res);
                if (file != null) {
                    save_wallpaper (file.get_path ());
                }
            } catch (Error e) {
                status_label.label = "Save failed: %s".printf (e.message);
            }
        });
    }

    private void on_set_wallpaper_clicked () {
        if (current_image == null) return;

        status_label.label = "Setting wallpaper...";
        progress_bar.visible = true;

        controls.apply_to_params (render_params);

        var src = current_image;

        new Thread<void> ("set-wallpaper", () => {
            var result = wtz_render (src, render_params);
            Idle.add (() => {
                if (result != null) {
                    var tmp_dir = Environment.get_tmp_dir ();
                    var tmp_name = "walltz_wallpaper_%d.png".printf (Random.int_range (100000, 999999));
                    var tmp_path = tmp_dir + "/" + tmp_name;

                    if (wtz_image_save_png (result, tmp_path) != 0) {
                        string? error_msg = null;
                        if (wtz_set_as_wallpaper (tmp_path, 0, out error_msg) != 0) {
                            status_label.label = "Wallpaper set successfully";
                        } else {
                            string? dest_path = null;
                            if (wtz_save_to_pictures (tmp_path, out dest_path) != 0) {
                                status_label.label = "Saved to Pictures: %s".printf (Path.get_basename (dest_path));
                            } else {
                                status_label.label = "Failed: %s".printf (error_msg ?? "Unknown error");
                            }
                        }
                        FileUtils.unlink (tmp_path);
                    } else {
                        status_label.label = "Failed to render wallpaper";
                    }
                    wtz_image_free (result);
                }
                progress_bar.visible = false;
                return false;
            });
        });
    }

    private void save_wallpaper (string path) {
        if (current_image == null) return;

        status_label.label = "Saving...";
        progress_bar.visible = true;

        controls.apply_to_params (render_params);

        var src = current_image;

        new Thread<void> ("save-wallpaper", () => {
            var result = wtz_render (src, render_params);
            Idle.add (() => {
                if (result != null) {
                    if (wtz_image_save_png (result, path) != 0) {
                        status_label.label = "Saved: %s".printf (Path.get_basename (path));
                    } else {
                        status_label.label = "Failed to save: %s".printf (Path.get_basename (path));
                    }
                    wtz_image_free (result);
                }
                progress_bar.visible = false;
                return false;
            });
        });
    }
}
