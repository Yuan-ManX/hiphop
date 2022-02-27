/*
 * Hip-Hop / High Performance Hybrid Audio Plugins
 * Copyright (C) 2021-2022 Luciano Iam <oss@lucianoiam.com>
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

#include <errno.h>
#include <X11/Xresource.h>

#include "LinuxWebUI.hpp"

USE_NAMESPACE_DISTRHO

float DISTRHO::getDisplayScaleFactor(WebViewUI* ui)
{
    if (ui == nullptr) {
        return 1.f;
    }

    ChildProcessWebView* webview = static_cast<ChildProcessWebView*>(ui->getWebView());

    return webview->getDisplayScaleFactor();
}

LinuxWebUI::LinuxWebUI(uint baseWidth, uint baseHeight,
        uint32_t backgroundColor, bool startLoading)
    : WebViewUI(baseWidth, baseHeight, backgroundColor)
{
    if (!shouldCreateWebView()) {
        return;
    }

    ChildProcessWebView* webview = new ChildProcessWebView();
    setWebView(webview); // base class owns web view

    // Allow JavaScript code to detect some unavailable features
    String js = String(
        // LXDRAGDROPBUG : No drag and drop on both GTK and CEF web views
        "window._webview_env = {};"
        "_webview_env.noDragAndDrop = true;"
#ifdef HIPHOP_LINUX_WEBVIEW_GTK
        // WKGTKRESIZEBUG : Broken vw/vh/vmin/vmax CSS units
        "_webview_env.noCSSViewportUnits = true;"
        // No touch events for <input type="range"> elements
        "_webview_env.noRangeInputTouch = true;"
#endif
    );
    webview->injectScript(js);

    if (startLoading) {
        load();
    }
}

LinuxWebUI::~LinuxWebUI()
{
    // TODO - standalone support
}

void LinuxWebUI::openSystemWebBrowser(String& url)
{
    char buf[256];
    snprintf(buf, sizeof(buf), "xdg-open %s", url.buffer());

    if (system(buf) != 0) {
        d_stderr("Could not open system web browser - %s", strerror(errno));
    }
}

void LinuxWebUI::sizeRequest(const UiBlock& block)
{
    queue(block);   // queue for next uiIdle() call
}

uintptr_t LinuxWebUI::createStandaloneWindow()
{
    // TODO - standalone support
    return 0;
}

void LinuxWebUI::processStandaloneEvents()
{
    // TODO - standalone support
}