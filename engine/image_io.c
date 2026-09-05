// SPDX-FileCopyrightText: 2026 kirijin <avel.ronin@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// Image I/O via GdkPixbuf

#include "engine.h"
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>

WtzImage* wtz_image_new(int width, int height) {
    WtzImage *img = g_new0(WtzImage, 1);
    img->width = width;
    img->height = height;
    img->stride = width * 4;
    img->pixels = g_new0(uint8_t, img->stride * height);
    return img;
}

void wtz_image_free(WtzImage *img) {
    if (!img) return;
    g_free(img->pixels);
    g_free(img);
}

static WtzImage* from_pixbuf(GdkPixbuf *pixbuf) {
    int w = gdk_pixbuf_get_width(pixbuf);
    int h = gdk_pixbuf_get_height(pixbuf);
    WtzImage *img = wtz_image_new(w, h);
    if (!img) return NULL;

    guchar *src_pixels = gdk_pixbuf_get_pixels(pixbuf);
    int src_stride = gdk_pixbuf_get_rowstride(pixbuf);
    int n_channels = gdk_pixbuf_get_n_channels(pixbuf);
    gboolean has_alpha = gdk_pixbuf_get_has_alpha(pixbuf);

    for (int y = 0; y < h; y++) {
        const guchar *src_row = src_pixels + y * src_stride;
        uint8_t *dst_row = img->pixels + y * img->stride;
        for (int x = 0; x < w; x++) {
            dst_row[x * 4 + 0] = src_row[x * n_channels + 2]; // B
            dst_row[x * 4 + 1] = src_row[x * n_channels + 1]; // G
            dst_row[x * 4 + 2] = src_row[x * n_channels + 0]; // R
            dst_row[x * 4 + 3] = has_alpha ? src_row[x * n_channels + 3] : 255; // A
        }
    }
    return img;
}

WtzImage* wtz_image_load(const char *path) {
    if (!path) return NULL;

    GError *error = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(path, &error);
    if (!pixbuf) {
        g_printerr("Failed to load %s: %s\n", path, error ? error->message : "unknown");
        g_clear_error(&error);
        return NULL;
    }

    // Convert to RGBA if needed
    if (gdk_pixbuf_get_n_channels(pixbuf) != 4 || !gdk_pixbuf_get_has_alpha(pixbuf)) {
        GdkPixbuf *rgba = gdk_pixbuf_add_alpha(pixbuf, FALSE, 0, 0, 0);
        g_object_unref(pixbuf);
        pixbuf = rgba;
    }

    WtzImage *img = from_pixbuf(pixbuf);
    g_object_unref(pixbuf);
    return img;
}

int wtz_image_save_png(const WtzImage *img, const char *path) {
    if (!img || !path || !img->pixels) return 0;

    GdkPixbuf *pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, img->width, img->height);
    if (!pixbuf) return 0;

    guchar *dst_pixels = gdk_pixbuf_get_pixels(pixbuf);
    int dst_stride = gdk_pixbuf_get_rowstride(pixbuf);

    for (int y = 0; y < img->height; y++) {
        const uint8_t *src_row = img->pixels + y * img->stride;
        guchar *dst_row = dst_pixels + y * dst_stride;
        for (int x = 0; x < img->width; x++) {
            dst_row[x * 4 + 0] = src_row[x * 4 + 2]; // R
            dst_row[x * 4 + 1] = src_row[x * 4 + 1]; // G
            dst_row[x * 4 + 2] = src_row[x * 4 + 0]; // B
            dst_row[x * 4 + 3] = src_row[x * 4 + 3]; // A
        }
    }

    GError *error = NULL;
    gboolean ok = gdk_pixbuf_save(pixbuf, path, "png", &error, NULL);
    g_object_unref(pixbuf);

    if (!ok) {
        g_printerr("Failed to save %s: %s\n", path, error ? error->message : "unknown");
        g_clear_error(&error);
        return 0;
    }
    return 1;
}
