// SPDX-FileCopyrightText: 2026 kirijin <avel.ronin@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

void test_image_new() {
    WtzImage* img = wtz_image_new(100, 50);
    assert(img != null);
    assert(img->width == 100);
    assert(img->height == 50);
    assert(img->pixels != null);
    wtz_image_free(img);
}

void test_image_save_png_null() {
    // Should not crash
    int result = wtz_image_save_png(null, "/tmp/test.png");
    assert(result == 0);
}

void test_blur_preset_count() {
    int count = wtz_blur_preset_count();
    assert(count == 16);  // Default + 15 presets
}

void test_blur_preset_default() {
    const WtzBlurPreset* preset = wtz_blur_preset(0);
    assert(preset != null);
    assert(strcmp(preset->id, "default") == 0);
}

void test_gradient_preset_count() {
    int count = wtz_gradient_preset_count();
    assert(count == 12);
}

void test_mood_palettes() {
    WtzMoodPalettes* palettes = wtz_extract_mood_palettes(null);
    assert(palettes != null);
    wtz_mood_palettes_free(palettes);
}

void test_smart_auto() {
    WtzSmartAutoParams params = wtz_compute_smart_auto(null);
    assert(params.sigma > 0);
    assert(params.sat_boost > 0);
}

public static int main (string[] args) {
    Test.init (ref args);
    
    Test.add_func ("/engine/image_new", test_image_new);
    Test.add_func ("/engine/image_save_png_null", test_image_save_png_null);
    Test.add_func ("/engine/blur_preset_count", test_blur_preset_count);
    Test.add_func ("/engine/blur_preset_default", test_blur_preset_default);
    Test.add_func ("/engine/gradient_preset_count", test_gradient_preset_count);
    Test.add_func ("/engine/mood_palettes", test_mood_palettes);
    Test.add_func ("/engine/smart_auto", test_smart_auto);
    
    return Test.run ();
}
