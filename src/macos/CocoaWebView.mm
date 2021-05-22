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

#include "CocoaWebView.hpp"

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <WebKit/WebKit.h>

#define fWkView ((WKWebView *)fView)

USE_NAMESPACE_DISTRHO

CocoaWebView::CocoaWebView()
    : fView(0)
{
    // ARC is off
    NSString *urlStr = [[NSString alloc] initWithCString:getContentUrl()
                        encoding:NSUTF8StringEncoding];
    NSURL *url = [[NSURL alloc] initWithString:urlStr];
    WKWebView *webView = [[WKWebView alloc] initWithFrame:CGRectZero];
    [webView loadRequest:[NSURLRequest requestWithURL:url]];
    [url release];
    [urlStr release];
    fView = (uintptr_t)webView;
}

CocoaWebView::~CocoaWebView()
{
    [fWkView removeFromSuperview];
    [fWkView release];
}

void CocoaWebView::reparent(uintptr_t parentWindowId)
{
    // parentWindowId is either a PuglCairoView* or PuglOpenGLViewDGL* depending
    // on the value of UI_TYPE in the Makefile. Both are NSView subclasses.
    NSView *parentView = (NSView *)parentWindowId;
    CGSize parentSize = parentView.frame.size;
    [fWkView removeFromSuperview];
    fWkView.frame = CGRectMake(0, 0, parentSize.width, parentSize.height);
    [parentView addSubview:fWkView];
}