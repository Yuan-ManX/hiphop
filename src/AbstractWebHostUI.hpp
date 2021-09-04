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

#ifndef ABSTRACT_WEB_HOST_UI_HPP
#define ABSTRACT_WEB_HOST_UI_HPP

#include <vector>

#include "DistrhoUI.hpp"

#include "AbstractWebView.hpp"

START_NAMESPACE_DISTRHO

class AbstractWebHostUI : public UI, private WebViewEventHandler
{
public:
    AbstractWebHostUI(uint baseWidth, uint baseHeight, uint32_t backgroundColor);
    virtual ~AbstractWebHostUI() {}
    
    uint getInitWidth() const { return fInitWidth; }
    uint getInitHeight() const { return fInitHeight; }

    uint32_t getBackgroundColor() const { return fBackgroundColor; }

    virtual float getDisplayScaleFactor(uintptr_t window) = 0;
    virtual void  openSystemWebBrowser(String& url) = 0;

protected:
    void sizeChanged(uint width, uint height) override;

    void parameterChanged(uint32_t index, float value) override;
#if DISTRHO_PLUGIN_WANT_PROGRAMS
    void programLoaded(uint32_t index) override;
#endif
#if DISTRHO_PLUGIN_WANT_STATE
    void stateChanged(const char* key, const char* value) override;
#endif

    void uiIdle() override;

    void flushInitMessageQueue();
    void setKeyboardFocus(bool focus);

    void webPostMessage(const JsValueVector& args);

    virtual void onWebContentReady() {}
    virtual void onWebMessageReceived(const JsValueVector& args) { (void)args; }

    virtual uintptr_t createStandaloneWindow() = 0;
    virtual void      processStandaloneEvents() = 0;

    virtual AbstractWebView& getWebView() = 0;

    void initWebView(AbstractWebView& webView);

private:
    // WebViewEventHandler

    virtual void handleWebViewContentLoadFinished() override;
    virtual void handleWebViewScriptMessageReceived(const JsValueVector& args) override;

    typedef std::vector<JsValueVector> InitMessageQueue;

    InitMessageQueue fInitMsgQueue;
    bool             fFlushedInitMsgQueue;
    uint32_t         fBackgroundColor;
    uint             fInitWidth;
    uint             fInitHeight;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AbstractWebHostUI)

};

END_NAMESPACE_DISTRHO

#endif  // ABSTRACT_WEB_HOST_UI_HPP