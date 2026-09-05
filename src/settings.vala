// SPDX-FileCopyrightText: 2026 kirijin <avel.ronin@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

public class WalltzSettings : GLib.Settings {
    public int target_width { get; set; }
    public int target_height { get; set; }
    public int blur_mode { get; set; }
    public int bg_gradient_style { get; set; }
    public double bg_zoom { get; set; }
    public double bg_blur_angle { get; set; }
    public int blur_radius { get; set; }
    public double saturation_factor { get; set; }
    public double overlay_opacity { get; set; }
    public string overlay_color { get; set; }
    public double blur_brightness { get; set; }
    public int auto_color { get; set; }
    public string bg_color { get; set; }
    public int bg_gradient_preset { get; set; }
    public double gradient_angle { get; set; }
    public int auto_mood { get; set; }
    public int use_v2 { get; set; }
    public int bg_pattern_enabled { get; set; }
    public int bg_pattern_type { get; set; }
    public string bg_pattern_color { get; set; }
    public double bg_pattern_scale { get; set; }
    public double bg_pattern_rotation { get; set; }
    public double bg_pattern_spacing { get; set; }
    public int bg_pattern_random_rotate { get; set; }
    public int bg_pattern_jitter { get; set; }
    public double bg_pattern_grid_amplitude { get; set; }
    public int bg_pattern_mix_enabled { get; set; }
    public double vignette_strength { get; set; }
    public double grain_strength { get; set; }
    public double ca_strength { get; set; }
    public double color_gamma { get; set; }
    public double color_warmth { get; set; }
    public double color_black_lift { get; set; }
    public int photo_frame { get; set; }
    public int photo_frame_width { get; set; }
    public double fg_zoom { get; set; }
    public double pip_zoom { get; set; }
    public int photo_grade { get; set; }
    public string blur_preset_id { get; set; }

    public WalltzSettings () {
        Object (schema_id: "org.walltz.walltz");
    }

    private static WalltzSettings? instance = null;

    public static WalltzSettings get_default () {
        if (instance == null) {
            instance = new WalltzSettings ();
        }
        return instance;
    }
}
