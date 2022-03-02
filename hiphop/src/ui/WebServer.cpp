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

#include <cstring>

#include "extra/Path.hpp"
#include "WebServer.hpp"

#define PROTOCOL_NAME     "lws-dpf"
#define INJECTED_JS_TOKEN "$injectedjs"

USE_NAMESPACE_DISTRHO

WebServer::WebServer(const char* jsInjectionTarget)
{
    lws_set_log_level(LLL_ERR|LLL_WARN|LLL_DEBUG, 0);

    std::memset(fProtocol, 0, sizeof(fProtocol));
    fProtocol[0].name = PROTOCOL_NAME;
    fProtocol[0].callback = WebServer::lwsCallback;

    std::strcpy(fMountOrigin, Path::getPluginLibrary() + "/ui/");

    std::memset(&fMount, 0, sizeof(fMount));
    fMount.mountpoint       = "/";
    fMount.mountpoint_len   = std::strlen(fMount.mountpoint);
    fMount.origin           = fMountOrigin;
    fMount.origin_protocol  = LWSMPRO_FILE;
    fMount.def              = "index.html";

    if (jsInjectionTarget != nullptr) {
        std::memset(&fMountOptions, 0, sizeof(fMountOptions));
        fMountOptions.name      = jsInjectionTarget;
        fMountOptions.value     = PROTOCOL_NAME;
        fMount.interpret        = &fMountOptions;
    }
#ifndef NDEBUG
    // Send caching headers
    fMount.cache_max_age    = 3600;
    fMount.cache_reusable   = 1;
    fMount.cache_revalidate = 1;
#endif

    findAvailablePort();

    std::memset(&fContextInfo, 0, sizeof(fContextInfo));
    fContextInfo.port      = fPort;
    fContextInfo.protocols = fProtocol;
    fContextInfo.mounts    = &fMount;
    fContextInfo.uid       = -1;
    fContextInfo.gid       = -1;
    fContextInfo.user      = this;

    // SSL (WIP)
    // https://github.com/warmcat/libwebsockets/blob/main/READMEs/README.test-apps.md
    // cp -rp ./scripts/client-ca /tmp
    // cd /tmp/client-ca
    // ./create-ca.sh
    // ./create-server-cert.sh server
    // ./create-client-cert.sh client
    //fContextInfo.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    //fContextInfo.ssl_cert_filepath        = "/tmp/client-ca/server.pem";
    //fContextInfo.ssl_private_key_filepath = "/tmp/client-ca/server.key";
    //fContextInfo.ssl_ca_filepath          = "/tmp/client-ca/ca.pem";

    fContext = lws_create_context(&fContextInfo);
}

WebServer::~WebServer()
{
    if (fContext != nullptr) {
        lws_context_destroy(fContext);
        fContext = nullptr;
    }
}

String WebServer::getLocalUrl()
{
    // TODO - HTTPS
    return String("http://localhost:") + String(fPort);
}

String WebServer::getLanUrl()
{
    // TODO - lan addr
    return String("http://192.168.1.1:") + String(fPort);
}

void WebServer::injectScript(String& script)
{
    fInjectedScripts.push_back(script);
}

void WebServer::process()
{
    // Avoid blocking - https://github.com/warmcat/libwebsockets/issues/1735
    lws_service(fContext, -1);
}

void WebServer::findAvailablePort()
{
    fPort = 9090; // FIXME - pick available port from a range
}

int WebServer::lwsCallback(struct lws* wsi, enum lws_callback_reasons reason,
                           void* user, void* in, size_t len)
{
    void* userdata = lws_context_user(lws_get_context(wsi));
    WebServer* server = static_cast<WebServer*>(userdata);
    int rc = 0; /* 0 OK, close connection otherwise */
    
    //d_stderr("LWS callback reason : %d \n", reason);
    
    switch (reason) {
        case LWS_CALLBACK_PROCESS_HTML: {
            lws_process_html_args* args = static_cast<lws_process_html_args*>(in);
            rc = server->injectScripts(args);
            break;
        }
        /*case LWS_CALLBACK_ESTABLISHED:
            rc = server->add_client(wsi);
            break;
        case LWS_CALLBACK_CLOSED:
            rc = server->del_client(wsi);
            break;
        case LWS_CALLBACK_RECEIVE:
            rc = server->recv_client(wsi, in, len);
            break;
        case LWS_CALLBACK_SERVER_WRITEABLE:
            rc = server->write_client(wsi);
            break;*/
        default:
            rc = lws_callback_http_dummy(wsi, reason, user, in, len);
            break;
    }

    return rc;
}

const char* WebServer::lwsReplaceFunc(void* data, int index)
{
    switch (index) {
        case 0:
            return static_cast<const char*>(data);
        default:
            return "";
    }
}

int WebServer::injectScripts(lws_process_html_args* args)
{
    lws_process_html_state phs;
    std::memset(&phs, 0, sizeof(phs));
    
    if (fInjectedScripts.size() == 0) {
        return lws_chunked_html_process(args, &phs) ? -1 : 0;
    }

    const char* vars[1] = {INJECTED_JS_TOKEN};
    phs.vars = vars;
    phs.count_vars = 1;
    phs.replace = WebServer::lwsReplaceFunc;

    const char* s = INJECTED_JS_TOKEN ";\n";
    size_t len = strlen(s);
    for (StringVector::const_iterator it = fInjectedScripts.begin(); it != fInjectedScripts.end(); ++it) {
        len += it->length();
    }

    char* js = new char[len + 1];
    std::strcat(js, s);
    for (StringVector::const_iterator it = fInjectedScripts.begin(); it != fInjectedScripts.end(); ++it) {
        std::strcat(js, *it);
    }

    phs.data = js;

    int rc = lws_chunked_html_process(args, &phs) ? -1 : 0;
    delete[] js;

    return rc;
}
