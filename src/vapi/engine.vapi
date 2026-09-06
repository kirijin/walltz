// SPDX-FileCopyrightText: 2026 kirijin <avel.ronin@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// VAPI bridge for the C engine library

[CCode (cname = "WtzImage", cheader_filename = "engine.h", free_function = "wtz_image_free")]
public struct WtzImage {
    public uint8 *pixels;
    public int width;
    public int height;
    public int stride;
}

[CCode (cname = "WtzRenderParams", cheader_filename = "engine.h", has_copy_function = false)]
public struct WtzRenderParams {
    public int target_width;
    public int target_height;
    public int blur_mode;
    public int bg_gradient_style;
    public double bg_zoom;
    public double bg_blur_angle;
    public int blur_radius;
    public double saturation_factor;
    public double overlay_opacity;
    public uint32 overlay_color;
    public double blur_brightness;
    public int auto_color;
    public uint32 bg_color;
    public int bg_gradient_preset;
    public double gradient_angle;
    public uint32 mood_color_a;
    public uint32 mood_color_b;
    public int bg_pattern_enabled;
    public int bg_pattern_type;
    public uint32 bg_pattern_color;
    public double bg_pattern_scale;
    public double bg_pattern_rotation;
    public double bg_pattern_spacing;
    public int bg_pattern_random_rotate;
    public int bg_pattern_jitter;
    public double bg_pattern_grid_amplitude;
    public int bg_pattern_mix_enabled;
    public int *bg_pattern_mix_motifs;
    public int bg_pattern_mix_count;
    public double vignette_strength;
    public double grain_strength;
    public double ca_strength;
    public double color_gamma;
    public double color_warmth;
    public double color_black_lift;
    public double texture_opacity;
    public int texture_blend_mode;
    public int texture_over_photo;
    public int texture_kind;
    public uint32 texture_color;
    public int photo_frame;
    public int photo_frame_width;
    public double fg_zoom;
    public double pip_zoom;
    public int photo_grade;
    public int auto_mood;
    public int use_v2;

    public WtzRenderParams () {
        this.target_width = 1920;
        this.target_height = 1080;
        this.blur_mode = 1;
        this.blur_radius = 90;
        this.saturation_factor = 1.8;
        this.bg_zoom = 1.0;
        this.bg_blur_angle = 0.0;
        this.blur_brightness = 1.0;
        this.auto_color = 1;
        this.bg_color = 0xFFFFFFFFU;
        this.bg_gradient_preset = 0;
        this.gradient_angle = 45.0;
        this.mood_color_a = 0xFF808080U;
        this.mood_color_b = 0xFFB4B4B4U;
    }
}

[CCode (cname = "WtzMoodPair", cheader_filename = "engine.h")]
public struct WtzMoodPair {
    public uint32 color_a;
    public uint32 color_b;
}

[CCode (cname = "WtzMoodPalettes", cheader_filename = "engine.h")]
public struct WtzMoodPalettes {
    public WtzMoodPair moods[6];
    public WtzMoodPair v2_moods[6];
}

[CCode (cname = "WtzSmartAutoParams", cheader_filename = "engine.h")]
public struct WtzSmartAutoParams {
    public double sigma;
    public double sat_boost;
    public double brightness;
    public double overlay_opacity;
    public uint32 overlay_color;
    public double vignette;
    public double grain;
}

[CCode (cname = "WtzBlurPreset", cheader_filename = "engine.h")]
public struct WtzBlurPreset {
    public weak string id;
    public weak string display_name;
    public double sigma;
    public double sat_boost;
    public double brightness;
    public double overlay_opacity;
    public uint32 overlay_color;
    public double vignette;
    public double grain;
    public bool frame_enabled;
    public int frame_width_pct;
    public double gamma;
    public double warmth;
    public double black_lift;
    public int texture_kind;
    public double texture_opacity;
    public int texture_blend_mode;
    public bool texture_over_photo;
    public uint32 texture_color;
    public bool photo_grade;
}

[CCode (cname = "WtzGradientPreset", cheader_filename = "engine.h")]
public struct WtzGradientPreset {
    public weak string name;
    public uint32 color1;
    public uint32 color2;
}

[CCode (cheader_filename = "engine.h")]
public static extern WtzImage* wtz_image_load(string path);

[CCode (cheader_filename = "engine.h")]
public static extern WtzImage* wtz_image_new(int width, int height);

[CCode (cheader_filename = "engine.h")]
public static extern void wtz_image_free(WtzImage* img);

[CCode (cheader_filename = "engine.h")]
public static extern int wtz_image_save_png(WtzImage* img, string path);

[CCode (cheader_filename = "engine.h")]
public static extern WtzImage* wtz_render(WtzImage* src, WtzRenderParams* params);

[CCode (cheader_filename = "engine.h")]
public static extern WtzMoodPalettes* wtz_extract_mood_palettes(WtzImage* img);

[CCode (cheader_filename = "engine.h")]
public static extern void wtz_mood_palettes_free(WtzMoodPalettes* palettes);

[CCode (cheader_filename = "engine.h")]
public static extern void wtz_compute_smart_auto(WtzImage* img, out WtzSmartAutoParams result);

[CCode (cheader_filename = "engine.h")]
public static extern WtzImage* wtz_generate_pattern_tile(int kind, int index, int tile_size, uint32 fg_color, double scale);

[CCode (cheader_filename = "engine.h")]
public static extern int wtz_blur_preset_count();

[CCode (cheader_filename = "engine.h")]
public static extern WtzBlurPreset* wtz_blur_preset(int index);

[CCode (cheader_filename = "engine.h")]
public static extern WtzBlurPreset* wtz_blur_preset_for_id(string id);

[CCode (cheader_filename = "engine.h")]
public static extern int wtz_gradient_preset_count();

[CCode (cheader_filename = "engine.h")]
public static extern WtzGradientPreset* wtz_gradient_preset(int index);

// D-Bus portal (set wallpaper)
[CCode (cheader_filename = "engine.h")]
public static extern int wtz_set_as_wallpaper(string path, int target, out string? error_message);

[CCode (cheader_filename = "engine.h")]
public static extern int wtz_save_to_pictures(string path, out string? dest_path);
