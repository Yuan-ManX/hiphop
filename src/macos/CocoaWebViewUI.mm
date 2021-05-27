/*
 * dpf-webui
 * Copyright (C) 2021 Luciano Iam <lucianoiam@protonmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

#include <dlfcn.h>
#include <libgen.h>
#include "CocoaWebViewUI.hpp"

#include "../log.h"

static char _dummy; // for dladdr()

USE_NAMESPACE_DISTRHO

UI* DISTRHO::createUI()
{
    return new CocoaWebViewUI;
}

CocoaWebViewUI::CocoaWebViewUI()
    : fView(0)
{
    // ARC is off
    NSString *urlStr = [[NSString alloc] initWithCString:getContentUrl()
                        encoding:NSUTF8StringEncoding];
    NSURL *url = [[NSURL alloc] initWithString:urlStr];
    fView = [[WKWebView alloc] initWithFrame:CGRectZero];
    [fView loadRequest:[NSURLRequest requestWithURL:url]];
    [url release];
    [urlStr release];
}

CocoaWebViewUI::~CocoaWebViewUI()
{
    [fView removeFromSuperview];
    [fView release];
}

void CocoaWebViewUI::parameterChanged(uint32_t index, float value)
{
    // unused
    (void)index;
    (void)value;
}

void CocoaWebViewUI::reparent(uintptr_t windowId)
{
    // windowId is either a PuglCairoView* or PuglOpenGLViewDGL* depending
    // on the value of UI_TYPE in the Makefile. Both are NSView subclasses.
    NSView *parentView = (NSView *)windowId;
    CGSize parentSize = parentView.frame.size;
    [fView removeFromSuperview];
    fView.frame = CGRectMake(0, 0, parentSize.width, parentSize.height);
    [parentView addSubview:fView];
}

String CocoaWebViewUI::getSharedLibraryDirectoryPath()
{
    Dl_info dl_info;
    if (dladdr((void *)&_dummy, &dl_info) == 0) {
        LOG_STDERR("Failed dladdr() call");
        return String();
    }
    char path[::strlen(dl_info.dli_fname) + 1];
    ::strcpy(path, dl_info.dli_fname);
    return String(dirname(path));
}

String CocoaWebViewUI::getPluginResourcePath()
{
    // There is no DISTRHO method for querying the current plugin format
    Dl_info dl_info;
    if (dladdr((void *)&_dummy, &dl_info) != 0) {
        char path[::strlen(dl_info.dli_fname) + 1];
        ::strcpy(path, dl_info.dli_fname);
        void *handle = dlopen(path, RTLD_NOLOAD);
        if (handle != NULL) {
            void *addr = dlsym(handle, "VSTPluginMain");
            dlclose(handle);
            if (addr != NULL) {
                return String(dirname(path)) + "/../Resources";
            } else {
                LOG_STDERR("Failed dlsym() call");
            }
        } else {
            LOG_STDERR("Failed dlopen() call");
        }
    } else {
        LOG_STDERR("Failed dladdr() call");
    }
    return WebUI::getPluginResourcePath();
}