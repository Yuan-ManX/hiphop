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

class UI {

    constructor() {
        this._resolve = {};

        window.webviewHost.addMessageListener((args) => {
            if (args[0] != 'UI') {
                this.messageReceived(args); // passthrough
                return;
            }

            const method = args[1];
            args = args.slice(2);

            if (method in this._resolve) {
                this._resolve[method][0](...args); // fulfill promise
                delete this._resolve[method];
            } else {
                this[method](...args); // call method
            }
        });

        // Call WebHostUI::flushInitMessageQueue() to receive any UI message
        // generated while the web view was still loading. Since this involves
        // message passing, it will not cause any UI methods to be triggered
        // synchronously and it is safe to indirectly call from super() in
        // subclass constructors.

        this._call('flushInitMessageQueue');
    }

    // uint UI::getWidth()
    async getWidth() {
        return this._callAndExpectReply('getWidth');
    }

    // uint UI::getHeight()
    async getHeight() {
        return this._callAndExpectReply('getHeight');
    }

    // void UI::setWidth(uint width)
    setWidth(width) {
        this._call('setWidth', width);
    }

    // void UI::setHeight(uint height)
    setHeight(height) {
        this._call('setHeight', height);
    }

    // bool UI::isResizable()
    async isResizable() {
        return this._callAndExpectReply('isResizable');
    }

    // void UI::setSize(uint width, uint height)
    setSize(width, height) {
        this._call('setSize', width, height);
    }

    // void UI::sendNote(uint8_t channel, uint8_t note, uint8_t velocity)
    sendNote(channel, note, velocity) {
        this._call('sendNote', channel, note, velocity);
    }

    // void UI::editParameter(uint32_t index, bool started)
    editParameter(index, started) {
        this._call('editParameter', index, started);
    }

    // void UI::setParameterValue(uint32_t index, float value)
    setParameterValue(index, value) {
        this._call('setParameterValue', index, value);
    }

    // void UI::setState(const char* key, const char* value)
    setState(key, value) {
        this._call('setState', key, value);
    }

    // bool UI::requestStateFile(const char* key)
    requestStateFile(key) {
        this._call('requestStateFile', key);
    }

    // void UI::sizeChanged(uint width, uint height)
    sizeChanged(width, height) {
        // default empty implementation
    }

    // void UI::parameterChanged(uint32_t index, float value)
    parameterChanged(index, value) {
        // default empty implementation
    }

    // void UI::programLoaded(uint32_t index)
    programLoaded(index) {
        // default empty implementation
    }

    // void UI::stateChanged(const char* key, const char* value)
    stateChanged(key, value) {
        // default empty implementation
    }
   
    // bool ExternalWindow::isStandalone()
    async isStandalone() {
        return this._callAndExpectReply('isStandalone');
    }

    // Non-DPF method for grabbing or releasing the keyboard focus
    // void AbstractWebHostUI::setKeyboardFocus()
    setKeyboardFocus(focus) {
        this._call('setKeyboardFocus', focus);
    }

    // Non-DPF method for opening the default system browser
    // void AbstractWebHostUI::openSystemWebBrowser(String& url)
    openSystemWebBrowser(url) {
        this._call('openSystemWebBrowser', url);
    }

    // Non-DPF method that returns the UI width at initialization time
    // uint AbstractWebHostUI::getInitialWidth()
    async getInitialWidth() {
        return this._callAndExpectReply('getInitialWidth');
    }

    // Non-DPF method that returns the UI height at initialization time
    // uint AbstractWebHostUI::getInitialHeight()
    async getInitialHeight() {
        return this._callAndExpectReply('getInitialHeight');
    }

    // Non-DPF method for sending a message to the web host
    // void AbstractWebHostUI::webViewPostMessage(const JsValueVector& args)
    postMessage(...args) {
        window.webviewHost.postMessage(args);
    }

    // Non-DPF callback method for receiving messages from the web host
    // void AbstractWebHostUI::webMessageReceived(const JsValueVector& args)
    messageReceived(args) {
        // default empty implementation
    }

    // Non-DPF method that writes memory shared with DISTRHO::PluginEx instance
    // void UIEx::writeSharedMemory(const char* metadata, const unsigned char* data, size_t size)
    writeSharedMemory(metadata /*string*/, data /*Uint8Array*/) {
        const td = new TextDecoder('utf8');
        this._call('writeSharedMemory', metadata, btoa(td.decode(data)));
    }

    // Non-DPF method that loads binary into DISTRHO::WasmHostPlugin instance
    replaceWasmBinary(data /*Uint8Array*/) {
        this.writeSharedMemory('_wasm', data);
    }

    // Helper for calling UI methods
    _call(method, ...args) {
        this.postMessage('UI', method, ...args)
    }

    // Helper for supporting synchronous calls using promises
    _callAndExpectReply(method, ...args) {
        if (method in this._resolve) {
            this._resolve[method][1](); // reject previous
        }
        
        return new Promise((resolve, reject) => {
            this._resolve[method] = [resolve, reject];
            this._call(method, ...args);
        });
    }

}
