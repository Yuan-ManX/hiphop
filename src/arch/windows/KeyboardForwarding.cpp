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

#include "KeyboardForwarding.hpp"

#include <processthreadsapi.h>

#include "base/macro.h"

USE_NAMESPACE_DISTRHO

KeyboardForwarding::KeyboardForwarding()
    : fRefCount(0)
    , fHostHWnd(0)
    , fKeyboardHook(0)
{
    // Some hosts need key events delivered directly to their main window
    EnumWindows(EnumWindowsProc, (LPARAM)&fHostHWnd);
}

BOOL CALLBACK KeyboardForwarding::EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
    DWORD winProcId = 0;
    GetWindowThreadProcessId(hWnd, &winProcId);

    if (winProcId == GetCurrentProcessId()) {
        char text[256];
        text[0] = '\0';
        GetWindowText(hWnd, (LPSTR)text, sizeof(text));

        if (strstr(text, "Ableton Live") != 0) {
            *((HWND *)lParam) = hWnd;
            return FALSE;
        }
    }

    return TRUE;
}

void KeyboardForwarding::incRefCount()
{
    if (fRefCount++ == 0) {
        // Passing GetCurrentThreadId() to dwThreadId results in the hook never being called
        fKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
    }
}

void KeyboardForwarding::decRefCount()
{
    if (--fRefCount == 0) {
        UnhookWindowsHookEx(fKeyboardHook);
    }
}

LRESULT CALLBACK KeyboardForwarding::KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{    
    // HC_ACTION means wParam & lParam contain info about keystroke message

    if (nCode == HC_ACTION) {
        HWND hWnd = GetFocus();
        int level = 0;

        // Check if focused window belongs to the hierarchy of one of our plugin instances

        while (level++ < 5) {
            HWND hFocusedPluginHelperWnd = 0;
            EnumChildWindows(hWnd, EnumChildProc, (LPARAM)&hFocusedPluginHelperWnd);

            if (hFocusedPluginHelperWnd != 0) {
                // Key events may be delivered to the plugin root window or host main window

                HWND hPluginRootWnd = GetParent(hFocusedPluginHelperWnd);

                KeyboardForwarding::getInstance().handleLowLevelKeyEvent(hPluginRootWnd,
                    (UINT)wParam, (KBDLLHOOKSTRUCT *)lParam);

                break;
            }

            hWnd = GetParent(hWnd);
        }
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

BOOL CALLBACK KeyboardForwarding::EnumChildProc(HWND hWnd, LPARAM lParam)
{
    (void)lParam;

    char className[256];
    GetClassName(hWnd, (LPSTR)className, sizeof(className));

    if ((strstr(className, "EdgeWebWidget") != 0) && (strstr(className, XSTR(PROJECT_ID_HASH)) != 0)) {
        *((HWND *)lParam) = hWnd;
        return FALSE;
    }

    return TRUE;
}

void KeyboardForwarding::handleLowLevelKeyEvent(HWND hPluginRootWnd, UINT message, KBDLLHOOKSTRUCT* lpData)
{    
    // Translate low level keyboard events into a format suitable for SendMessage()

    WPARAM wParam = lpData->vkCode;
    LPARAM lParam = /* scan code */ lpData->scanCode << 16 | /* repeat count */ 0x1;

    switch (message) {
        case WM_KEYDOWN:
            // Basic logic that forwards a-z to allow playing with Live's virtual keyboard.
            routeKeyMessage(hPluginRootWnd, WM_KEYDOWN, wParam, lParam);

            if ((lpData->vkCode >= 'A') && (lpData->vkCode <= 'Z')) {
                wParam |= 0x20; // to lowercase
                routeKeyMessage(hPluginRootWnd, WM_CHAR, wParam, lParam);
            }

            break;
        case WM_KEYUP:
            // bit 30: The previous key state. The value is always 1 for a WM_KEYUP message.
            // bit 31: The transition state. The value is always 1 for a WM_KEYUP message.
            lParam |= 0xC0000000;
            routeKeyMessage(hPluginRootWnd, WM_KEYUP, wParam, lParam);
            
            break;
    }
}

void KeyboardForwarding::routeKeyMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (fHostHWnd == 0) {
        SetFocus(hWnd);
        SendMessage(hWnd, message, wParam, lParam); // e.g.: REAPER
    } else {
        SendMessage(fHostHWnd, message, wParam, lParam); // e.g.: Live
    }
}