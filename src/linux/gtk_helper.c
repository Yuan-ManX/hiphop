/*
 * Hip-Hop / High Performance Hybrid Audio Plugins
 * Copyright (C) 2021 Luciano Iam <oss@lucianoiam.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <string.h>

#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <X11/Xlib.h>

#include "ipc.h"
#include "ipc_message.h"
#include "macro.h"

// WKGTKRESIZEBUG : webview created with a fixed maximum size, see comprehensive
// explanation in realize(). Plugins that do not change their UI size during
// runtime might want to set these values in DistrhoPluginInfo.h to ensure CSS
// viewport dimensions (vw/vw/vmin/vmax) are relative to some known fixed values.
#if !defined(HIPHOP_MAX_BASE_WIDTH) || !defined(HIPHOP_MAX_BASE_HEIGHT)
#define HIPHOP_MAX_BASE_WIDTH 1536
#define HIPHOP_MAX_BASE_HEIGHT 1536
#endif

// CSS touch-action based approach seems to be failing for WebKitGTK. Looks like a bug.
#define JS_DISABLE_PINCH_ZOOM_WORKAROUND "if (document.body.children.length > 0) document.body.children[0].addEventListener('touchstart', (ev) => { ev.preventDefault(); });"

#define JS_POST_MESSAGE_SHIM "window.webviewHost.postMessage = (args) => window.webkit.messageHandlers.host.postMessage(args);"

typedef struct {
    ipc_t*         ipc;
    float          scaleFactor;
    msg_win_size_t size;
    Display*       display;
    Window         container;
    GtkWindow*     window;
    WebKitWebView* webView;
    gboolean       focus;
    Window         focusXWin;
    pthread_t      watchdog;
    char           injectedJs[65536];
} context_t;

static float get_gtk_scale_factor();
static void realize(context_t *ctx, const msg_win_cfg_t *config);
static void navigate(context_t *ctx, const char *url);
static void run_script(const context_t *ctx, const char *js);
static void inject_script(context_t *ctx, const char *js);
static void set_size(context_t *ctx, const msg_win_size_t *size);
static void apply_size(const context_t *ctx);
static void set_keyboard_focus(context_t *ctx, gboolean focus);
static void* focus_watchdog_worker(void *arg);
static gboolean release_focus(gpointer data);
static void web_view_load_changed_cb(WebKitWebView *view, WebKitLoadEvent event, gpointer data);
static void web_view_script_message_cb(WebKitUserContentManager *manager, WebKitJavascriptResult *res, gpointer data);
static gboolean web_view_keypress_cb(GtkWidget *widget, GdkEventKey *event, gpointer data);
static gboolean ipc_read_cb(GIOChannel *source, GIOCondition condition, gpointer data);
static int ipc_write_simple(const context_t *ctx, msg_opcode_t opcode, const void *payload, int payload_sz);

int main(int argc, char* argv[])
{
    context_t ctx;
    ipc_conf_t conf;
    GIOChannel* channel;

    memset(&ctx, 0, sizeof(ctx));

    if (argc < 3) {
        DBG("Invalid argument count");
        return -1;
    }

    if ((sscanf(argv[1], "%d", &conf.fd_r) == 0) || (sscanf(argv[2], "%d", &conf.fd_w) == 0)) {
        DBG("Invalid file descriptor");
        return -1;
    }

    ctx.ipc = ipc_init(&conf);

    gdk_set_allowed_backends("x11");
    gtk_init(0, NULL);

    ctx.display = XOpenDisplay(NULL);

    if (ctx.display == NULL) {
        DBG("Cannot open display");
        return -1;
    }

    channel = g_io_channel_unix_new(conf.fd_r);    
    g_io_add_watch(channel, G_IO_IN|G_IO_ERR|G_IO_HUP, ipc_read_cb, &ctx);

    ctx.scaleFactor = get_gtk_scale_factor();
    ipc_write_simple(&ctx, OP_HANDLE_INIT, &ctx.scaleFactor, sizeof(ctx.scaleFactor));

    gtk_main();

    set_keyboard_focus(&ctx, FALSE);
    g_io_channel_shutdown(channel, TRUE, NULL);
    ipc_destroy(ctx.ipc);

    if (ctx.container != 0) {
        XDestroyWindow(ctx.display, ctx.container);
    }

    XCloseDisplay(ctx.display);

    return 0;
}

static float get_gtk_scale_factor()
{
    // Favor system-wide setting, this is set for example by updating display
    // scale in Gnome Shell's settings and possibly by other window managers.
    // It can be also controlled by setting the environment variable GDK_SCALE.
    GtkWidget *dummy = gtk_widget_new(GTK_TYPE_WINDOW, NULL);
    float k = (float)gtk_widget_get_scale_factor(dummy);
    gtk_widget_destroy(dummy);

    if (k != 1.f) {
        return k;
    }

    // Also check for an environment variable that allows non-fractional scaling
    const char* dpi = getenv("GDK_DPI_SCALE");

    if ((dpi != 0) && (sscanf(dpi, "%f", &k) == 1)) {
        return k;
    }

    return 1.f;
}

static void realize(context_t *ctx, const msg_win_cfg_t *config)
{
    // Create a native container window of arbitrary maximum size
    int max_width = ctx->scaleFactor * HIPHOP_MAX_BASE_WIDTH;
    int max_height = ctx->scaleFactor * HIPHOP_MAX_BASE_HEIGHT;

    ctx->container = XCreateSimpleWindow(ctx->display, (Window)config->parent, 0, 0,
                                        max_width, max_height, 0, 0, 0);
    XSync(ctx->display, False);

    // Wrap container in a GDK window. Web view text input colored focus boxes
    // do not show in wrapped windows but show correctly in regular windows.
    GdkWindow* gdkWindow = gdk_x11_window_foreign_new_for_display(gdk_display_get_default(),
        ctx->container);
    ctx->window = GTK_WINDOW(gtk_widget_new(GTK_TYPE_WINDOW, NULL));
    g_signal_connect(ctx->window, "realize", G_CALLBACK(gtk_widget_set_window), gdkWindow);

    // WKGTKRESIZEBUG : After the web view becomes visible, gtk_window_resize()
    // will not cause its contents to resize anymore. The issue is probably
    // related to the GdkWindow wrapping a X11 window and not emitting Glib
    // events like configure-event. The workaround consists in creating the
    // window with a predetermined max size and using JavaScript to resize the
    // DOM instead of resizing the window natively. It is an ugly solution that
    // works. Note this renders viewport based units useless (vw/vh/vmin/vmax). 
    gtk_window_resize(ctx->window, max_width, max_height);

    ctx->webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_signal_connect(ctx->webView, "load-changed", G_CALLBACK(web_view_load_changed_cb), ctx);
    g_signal_connect(ctx->webView, "key-press-event", G_CALLBACK(web_view_keypress_cb), ctx);
    WebKitUserContentManager *manager = webkit_web_view_get_user_content_manager(ctx->webView);
    g_signal_connect(manager, "script-message-received::host", G_CALLBACK(web_view_script_message_cb), ctx);
    webkit_user_content_manager_register_script_message_handler(manager, "host");

    gtk_container_add(GTK_CONTAINER(ctx->window), GTK_WIDGET(ctx->webView));  
}

static void navigate(context_t *ctx, const char *url)
{
    // Inject queued scripts
    WebKitUserScript *script = webkit_user_script_new(ctx->injectedJs, WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
        WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START, NULL, NULL);
    WebKitUserContentManager *manager = webkit_web_view_get_user_content_manager(ctx->webView);
    webkit_user_content_manager_add_script(manager, script);
    webkit_user_script_unref(script);
    ctx->injectedJs[0] = '\0'; // already injected on next navigate() call

    webkit_web_view_load_uri(ctx->webView, url);
}

static void run_script(const context_t *ctx, const char *js)
{
    if (ctx->webView != NULL) {
        webkit_web_view_run_javascript(ctx->webView, js, NULL, NULL, NULL);
    }
}

static void inject_script(context_t *ctx, const char *js)
{
    strcat(ctx->injectedJs, (const char *)js);
}

static void set_size(context_t *ctx, const msg_win_size_t *size)
{
    ctx->size = *size;

    if (ctx->webView != NULL) {
        apply_size(ctx);
    }
}

static void apply_size(const context_t *ctx)
{
    unsigned width = ctx->size.width;
    unsigned height = ctx->size.height;

    if ((width == 0) || (height == 0)) {
        return;
    }
 
    // WebKitGTK uses this value for setting window.devicePixelRatio
    gint k = gtk_widget_get_scale_factor(GTK_WIDGET(ctx->webView));
    width /= k;
    height /= k;

    // WKGTKRESIZEBUG : does not result in webview contents size update
    //gtk_window_resize(ctx->window, width, height);

    char js[1024];

    sprintf(js, "document.documentElement.style.width  = '%dpx';"
                "document.documentElement.style.height = '%dpx';",
                width, height);
    run_script(ctx, js);
}

static void set_keyboard_focus(context_t *ctx, gboolean focus)
{
    if (ctx->focus == focus) {
        return;
    }

    if (ctx->webView == NULL) {
        return;
    }

    ctx->focusXWin = 0;
    ctx->focus = focus;

    // Some hosts grab focus back from the plugin, avoid that

    GdkWindow *window = gtk_widget_get_window(GTK_WIDGET(ctx->webView));
    GdkSeat *seat = gdk_display_get_default_seat(gdk_display_get_default());

    if (ctx->focus) {
        gdk_seat_grab(seat, window, GDK_SEAT_CAPABILITY_KEYBOARD, FALSE, NULL, NULL, NULL, NULL);
        pthread_create(&ctx->watchdog, NULL, focus_watchdog_worker, ctx);
    } else {
        gdk_seat_ungrab(seat);

        if (ctx->watchdog != 0) {
            pthread_join(ctx->watchdog, NULL);
            ctx->watchdog = 0;
        }
    }
}

static void* focus_watchdog_worker(void *arg)
{
    // GdkWindow instances created with gdk_x11_window_foreign_new_for_display()
    // do not seem to emit focus events like a regular GdkWindow instance. That
    // makes it difficult to detect when the plugin window goes out of focus in
    // order to release the keyboard lock. This solution uses a thread to poll
    // X11 focus, detect changes and release the keyboard lock when needed.

    context_t *ctx = (context_t *)arg;

    while (ctx->focus) {
        if (ctx->focus && (ctx->focusXWin != 0)) {
            Window focus;
            int revert;

            XLockDisplay(ctx->display);
            XGetInputFocus(ctx->display, &focus, &revert);
            XUnlockDisplay(ctx->display);

            if (ctx->focusXWin != focus) {
                g_idle_add(release_focus, ctx);
            }
        }

        usleep(100000L); // 100ms
    }
}

static gboolean release_focus(gpointer data)
{
    set_keyboard_focus((context_t *)data, FALSE);
    return FALSE;
}

static void web_view_load_changed_cb(WebKitWebView *view, WebKitLoadEvent event, gpointer data)
{
    context_t *ctx = (context_t *)data;

    switch (event) {
        case WEBKIT_LOAD_FINISHED:
            // Load completed. All resources are done loading or there was an error during the load operation. 
            run_script(ctx, JS_DISABLE_PINCH_ZOOM_WORKAROUND);
            apply_size(ctx);
            gtk_widget_show_all(GTK_WIDGET(ctx->window));
            ipc_write_simple(ctx, OP_HANDLE_LOAD_FINISHED, NULL, 0);
            usleep(20000); // 20ms -- prevents flicker, why?
            break;
        default:
            break;
    }
}

static void web_view_script_message_cb(WebKitUserContentManager *manager, WebKitJavascriptResult *res, gpointer data)
{
    // Serialize JS values into type;value chunks. Available types are restricted to
    // those defined by msg_js_arg_type_t so there is no need to encode value sizes.
    gint32 numArgs, i;
    JSCValue *jsArg;
    JSCValue *jsArgs = webkit_javascript_result_get_js_value(res);
    char *payload = NULL;
    int offset = 0;

    if (jsc_value_is_array(jsArgs)) {
        numArgs = jsc_value_to_int32(jsc_value_object_get_property(jsArgs, "length"));

        for (i = 0; i < numArgs; i++) {
            jsArg = jsc_value_object_get_property_at_index(jsArgs, i);

            if (jsc_value_is_boolean(jsArg)) {
                payload = (char *)realloc(payload, offset + 1);
                if (jsc_value_to_boolean(jsArg)) {
                    *(payload+offset) = (char)ARG_TYPE_TRUE;
                } else {
                    *(payload+offset) = (char)ARG_TYPE_FALSE;
                }
                offset += 1;

            } else if (jsc_value_is_number(jsArg)) {
                payload = (char *)realloc(payload, offset + 1 + sizeof(double));
                *(payload+offset) = (char)ARG_TYPE_DOUBLE;
                offset += 1;
                *(double *)(payload+offset) = jsc_value_to_double(jsArg);
                offset += sizeof(double);

            } else if (jsc_value_is_string(jsArg)) {
                const char *s = jsc_value_to_string(jsArg);
                int slen = strlen(s) + 1;
                payload = (char *)realloc(payload, offset + 1 + slen);
                *(payload+offset) = (char)ARG_TYPE_STRING;
                offset += 1;
                strcpy(payload+offset, s);
                offset += slen;

            } else {
                payload = (char *)realloc(payload, offset + 1);
                *(payload+offset) = (char)ARG_TYPE_NULL;
                offset += 1;
            }
        }
    }

    webkit_javascript_result_unref(res);

    ipc_write_simple((context_t *)data, OP_HANDLE_SCRIPT_MESSAGE, payload, offset);

    if (payload) {
        free(payload);
    }
}

static gboolean web_view_keypress_cb(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    context_t *ctx = (context_t *)data;

    int revert;
    XGetInputFocus(ctx->display, &ctx->focusXWin, &revert);

    return !ctx->focus;
}

static gboolean ipc_read_cb(GIOChannel *source, GIOCondition condition, gpointer data)
{
    context_t *ctx = (context_t *)data;
    tlv_t packet;

    if ((condition & G_IO_IN) == 0) {
        return TRUE;
    }

    if (ipc_read(ctx->ipc, &packet) == -1) {
        DBG_ERRNO("Could not read from IPC channel");
        return TRUE;
    }

    switch (packet.t) {
        case OP_REALIZE:
            realize(ctx, (const msg_win_cfg_t *)packet.v);
            break;
        case OP_NAVIGATE:
            navigate(ctx, (const char *)packet.v);
            break;
        case OP_RUN_SCRIPT:
            run_script(ctx, (const char *)packet.v);
            break;
        case OP_INJECT_SHIMS:
            inject_script(ctx, JS_POST_MESSAGE_SHIM);
            break;
        case OP_INJECT_SCRIPT:
            inject_script(ctx, (const char *)packet.v);
            break;
        case OP_SET_SIZE:
            set_size(ctx, (const msg_win_size_t *)packet.v);
            break;
        case OP_SET_KEYBOARD_FOCUS:
            set_keyboard_focus(ctx, *((char *)packet.v) == 1 ? TRUE : FALSE);
            break;
        case OP_TERMINATE:
            gtk_main_quit();
            break;
        default:
            break;
    }

    return TRUE;
}

static int ipc_write_simple(const context_t *ctx, msg_opcode_t opcode, const void *payload, int payload_sz)
{
    int retval;
    tlv_t packet;

    packet.t = (short)opcode;
    packet.l = payload_sz;
    packet.v = payload;

    if ((retval = ipc_write(ctx->ipc, &packet)) == -1) {
        DBG_ERRNO("Could not write to IPC channel");
    }

    return retval;
}
