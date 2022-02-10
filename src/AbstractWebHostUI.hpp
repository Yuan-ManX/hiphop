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

#ifndef ABSTRACT_WEB_HOST_UI_HPP
#define ABSTRACT_WEB_HOST_UI_HPP

#include <functional>
#include <unordered_map>
#include <vector>

#include "UIEx.hpp"
#include "AbstractWebView.hpp"

START_NAMESPACE_DISTRHO

class AbstractWebHostUI;
float getDisplayScaleFactor(AbstractWebHostUI* ui);

class AbstractWebHostUI : public UIEx, private WebViewEventHandler
{
public:
    AbstractWebHostUI(uint widthCssPx, uint heightCssPx, uint32_t backgroundColor, 
        bool startLoading = true);
    virtual ~AbstractWebHostUI();

    typedef std::function<void()> UiBlock;

    void queue(const UiBlock& block);
    
    uint getInitialWidth() const { return fInitialWidth; }
    uint getInitialHeight() const { return fInitialHeight; }

    uintptr_t getPlatformWindow() const { return fPlatformWindow; }

    AbstractWebView* getWebView() { return fWebView; }

    virtual void openSystemWebBrowser(String& url) = 0;

protected:
    bool shouldCreateWebView();
    void setWebView(AbstractWebView* webView);

    void load();
    
    void runScript(String& source);
    void injectScript(String& source);

    void webViewPostMessage(const JsValueVector& args);

    void flushInitMessageQueue();
    void setKeyboardFocus(bool focus);

    void uiIdle() override;

#if HIPHOP_ENABLE_SHARED_MEMORY
    void sharedMemoryChanged(const char* metadata, const unsigned char* data, size_t size) override;
#endif // HIPHOP_ENABLE_SHARED_MEMORY

    void sizeChanged(uint width, uint height) override;

    void parameterChanged(uint32_t index, float value) override;
#if DISTRHO_PLUGIN_WANT_PROGRAMS
    void programLoaded(uint32_t index) override;
#endif // DISTRHO_PLUGIN_WANT_PROGRAMS
#if DISTRHO_PLUGIN_WANT_STATE
    void stateChanged(const char* key, const char* value) override;
#endif // DISTRHO_PLUGIN_WANT_STATE

    virtual void onWebContentReady() {}
    virtual void onWebMessageReceived(const JsValueVector& args) { (void)args; }

    virtual void sizeRequest(const UiBlock& block);

    virtual uintptr_t createStandaloneWindow() = 0;
    virtual void      processStandaloneEvents() = 0;

private:
    // WebViewEventHandler

    virtual void handleWebViewLoadFinished() override;
    virtual void handleWebViewScriptMessage(const JsValueVector& args) override;
    virtual void handleWebViewConsole(const String& tag, const String& text) override;

    typedef std::vector<JsValueVector> InitMessageQueue;

    typedef std::function<void(const JsValueVector& args)> MessageHandler;
    typedef std::pair<int, MessageHandler> ArgumentCountAndMessageHandler;
    typedef std::unordered_map<std::string, ArgumentCountAndMessageHandler> MessageHandlerMap;

    uint              fInitialWidth;
    uint              fInitialHeight;
    uint32_t          fBackgroundColor;
    bool              fMessageQueueReady;
    bool              fUiBlockQueued;
    uintptr_t         fPlatformWindow;
    AbstractWebView*  fWebView;
    UiBlock           fUiBlock;
    InitMessageQueue  fInitMessageQueue;
    MessageHandlerMap fHandler;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AbstractWebHostUI)

};

END_NAMESPACE_DISTRHO

#endif  // ABSTRACT_WEB_HOST_UI_HPP
