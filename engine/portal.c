// SPDX-FileCopyrightText: 2026 kirijin <avel.ronin@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// D-Bus portal integration for setting wallpaper
// Uses org.freedesktop.portal.Wallpaper (XDG Desktop Portal)

#include "engine.h"
#include <glib.h>
#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    WTZ_WALLPAPER_TARGET_DESKTOP = 0,
    WTZ_WALLPAPER_TARGET_LOCKSCREEN = 1,
    WTZ_WALLPAPER_TARGET_BOTH = 2
} WtzWallpaperTarget;

typedef struct {
    GMainLoop *loop;
    gboolean success;
    gchar *error_message;
    guint signal_subscription;
} PortalCallData;

static void portal_response_received(GDBusConnection *connection,
                                      const gchar *sender_name,
                                      const gchar *object_path,
                                      const gchar *interface_name,
                                      const gchar *signal_name,
                                      GVariant *parameters,
                                      gpointer user_data) {
    PortalCallData *data = (PortalCallData *)user_data;
    guint32 response;
    GVariant *details;

    g_variant_get(parameters, "(u@a{sv})", &response, &details);

    if (response == 0) {
        data->success = TRUE;
    } else if (response == 1) {
        data->error_message = g_strdup("Cancelled by user");
    } else {
        data->error_message = g_strdup("Failed");
    }

    g_variant_unref(details);
    g_main_loop_quit(data->loop);
}

static gchar *portal_set_on(WtzWallpaperTarget target) {
    switch (target) {
    case WTZ_WALLPAPER_TARGET_LOCKSCREEN: return "lockscreen";
    case WTZ_WALLPAPER_TARGET_BOTH: return "both";
    default: return "background";
    }
}

int wtz_set_as_wallpaper(const char *path, int target, char **error_message) {
    if (!path || !g_file_test(path, G_FILE_TEST_EXISTS)) {
        if (error_message) *error_message = g_strdup("File does not exist");
        return FALSE;
    }

    GError *error = NULL;
    GDBusConnection *connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    if (!connection) {
        if (error_message) *error_message = g_strdup(error ? error->message : "No session bus");
        g_clear_error(&error);
        return FALSE;
    }

    // Build options
    GVariantBuilder options;
    g_variant_builder_init(&options, G_VARIANT_TYPE("a{sv}"));

    gchar *token = g_strdup_printf("walltz_%d", getpid());
    g_variant_builder_add(&options, "{sv}", "handle_token", g_variant_new_string(token));
    g_variant_builder_add(&options, "{sv}", "set-on", g_variant_new_string(portal_set_on(target)));
    g_variant_builder_add(&options, "{sv}", "show-preview", g_variant_new_boolean(FALSE));

    // Convert path to URI
    gchar *uri = g_filename_to_uri(path, NULL, NULL);
    if (!uri) {
        if (error_message) *error_message = g_strdup("Failed to convert path to URI");
        g_free(token);
        g_object_unref(connection);
        return FALSE;
    }

    PortalCallData call_data = {
        .loop = g_main_loop_new(NULL, FALSE),
        .success = FALSE,
        .error_message = NULL,
        .signal_subscription = 0
    };

    // Subscribe to response signal
    call_data.signal_subscription = g_dbus_connection_signal_subscribe(
        connection,
        "org.freedesktop.portal.Desktop",
        "org.freedesktop.portal.Request",
        "Response",
        "/org/freedesktop/portal/desktop",
        NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        portal_response_received,
        &call_data,
        NULL
    );

    // Make the portal call
    GVariant *result = g_dbus_connection_call_sync(
        connection,
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.Wallpaper",
        "SetWallpaperURI",
        g_variant_new("(ssa{sv})", "", uri, &options),
        G_VARIANT_TYPE("(o)"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (result) {
        gchar *request_path = NULL;
        g_variant_get(result, "(o)", &request_path);
        g_variant_unref(result);

        if (request_path) {
            // Update signal subscription to match the actual request path
            g_dbus_connection_signal_unsubscribe(connection, call_data.signal_subscription);
            call_data.signal_subscription = g_dbus_connection_signal_subscribe(
                connection,
                "org.freedesktop.portal.Desktop",
                "org.freedesktop.portal.Request",
                "Response",
                request_path,
                NULL,
                G_DBUS_SIGNAL_FLAGS_NONE,
                portal_response_received,
                &call_data,
                NULL
            );
            g_free(request_path);
        }
    } else {
        if (error_message) *error_message = g_strdup(error ? error->message : "Portal call failed");
        g_clear_error(&error);
        g_free(uri);
        g_free(token);
        g_main_loop_unref(call_data.loop);
        g_dbus_connection_signal_unsubscribe(connection, call_data.signal_subscription);
        g_object_unref(connection);
        return FALSE;
    }

    // Wait for response (with 30s timeout)
    g_timeout_add_seconds(30, (GSourceFunc)g_main_loop_quit, call_data.loop);
    g_main_loop_run(call_data.loop);

    // Cleanup
    if (call_data.signal_subscription) {
        g_dbus_connection_signal_unsubscribe(connection, call_data.signal_subscription);
    }
    g_main_loop_unref(call_data.loop);
    g_free(uri);
    g_free(token);
    g_object_unref(connection);

    if (call_data.success) {
        return TRUE;
    } else {
        if (error_message) *error_message = call_data.error_message ? call_data.error_message : g_strdup("Unknown error");
        return FALSE;
    }
}

// Fallback: save to Pictures folder when portal is unavailable
gboolean wtz_save_to_pictures(const char *path, char **dest_path) {
    const char *pictures_dir = g_get_user_special_dir(G_USER_DIRECTORY_PICTURES);
    if (!pictures_dir) pictures_dir = g_get_home_dir();

    GFile *src = g_file_new_for_path(path);
    gchar *basename = g_file_get_basename(src);
    gchar *dest = g_build_filename(pictures_dir, basename, NULL);

    GError *error = NULL;
    GFile *dst = g_file_new_for_path(dest);

    gboolean ok = g_file_copy(src, dst, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, &error);

    if (ok) {
        if (dest_path) *dest_path = g_strdup(dest);
    } else {
        if (dest_path) *dest_path = g_strdup(path);
    }

    g_free(basename);
    g_free(dest);
    g_object_unref(src);
    g_object_unref(dst);
    g_clear_error(&error);

    return ok;
}
