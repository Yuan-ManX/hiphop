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

class StubWebUI {

    static installIfNeeded() {
        if (typeof(DISTRHO_WebUI) == 'undefined') {
            console.log('DISTRHO_WebUI is not defined, installing stub')
            window.DISTRHO_WebUI = StubWebUI;
        }
    }

    constructor() {
        // no-op
    }

    async getWidth() {
        return document.body.clientWidth;
    }

    async getHeight() {
        return document.body.clientHeight;
    }

    setWidth(width) {
        console.log(`setWidth(${width})`);
    }

    setHeight(height) {
        console.log(`setHeight(${height})`);
    }

    async isResizable() {
        return true;
    }

    setSize(width, height) {
        console.log(`setSize(${width}, ${height})`);
    }

    editParameter(index, started) {
        console.log(`editParameter(${index}, ${started})`);
    }

    setParameterValue(index, value) {
        console.log(`setParameterValue(${index}, ${value})`);
    }

    setState(key, value) {
        console.log(`setState(${key}, ${value})`);
    }

    flushInitMessageQueue() {
        console.log('flushInitMessageQueue()');
    }

    setGrabKeyboardInput(forward) {
        console.log(`setGrabKeyboardInput(${forward}`);
    }

    async getInitWidth() {
        return document.body.clientWidth;
    }

    async getInitHeight() {
        return document.body.clientHeight;
    }

    postMessage(...args) {
        console.log(`postMessage(${args})`);
    }

}

StubWebUI.installIfNeeded();