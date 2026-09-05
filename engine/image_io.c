// SPDX-FileCopyrightText: 2026 kirijin <avel.ronin@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// Image I/O — PNG/JPEG load/save via GdkPixbuf or libpng

#include "engine.h"
#include <glib.h>
#include <stdlib.h>
#include <string.h>

// TODO: implement with GdkPixbuf
// For now, stubs that return NULL

WtzImage* wtz_image_load(const char *path) {
    if (!path) return NULL;
    // TODO: use gdk_pixbuf_new_from_file() or libpng
    return NULL;
}

int wtz_image_save_png(const WtzImage *img, const char *path) {
    if (!img || !path) return 0;
    // TODO: use gdk_pixbuf_save() or libpng
    return 0;
}
