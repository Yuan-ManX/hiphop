/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2019 Filipe Coelho <falktx@falktx.com>
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

#ifndef WEBGAINEXAMPLEPLUGIN_HPP
#define WEBGAINEXAMPLEPLUGIN_HPP

#include "DistrhoPlugin.hpp"

START_NAMESPACE_DISTRHO

class WebGainExamplePlugin : public Plugin
{
public:
    WebGainExamplePlugin();
    ~WebGainExamplePlugin() {};

    const char* getLabel() const override
    {
        return "WebGain";
    }

    const char* getMaker() const override
    {
        return "Luciano Iam";
    }

    const char* getLicense() const override
    {
        return "ISC";
    }

    uint32_t getVersion() const override
    {
        return 0;
    }

    int64_t getUniqueId() const override
    {
        return d_cconst('D', 'P', 'w', 'g');
    }

    void  initParameter(uint32_t index, Parameter& parameter) override;
    float getParameterValue(uint32_t index) const override;
    void  setParameterValue(uint32_t index, float value) override;

#if (DISTRHO_PLUGIN_WANT_STATE == 1)
    void   initState(uint32_t index, String& stateKey, String& defaultStateValue) override;
    void   setState(const char* key, const char* value) override;
#if (DISTRHO_PLUGIN_WANT_FULL_STATE == 1)
    String getState(const char* key) const override;
#endif
#endif

    void run(const float** inputs, float** outputs, uint32_t frames) override;

private:
    float fGain;

};

END_NAMESPACE_DISTRHO

#endif  // WEBGAINEXAMPLEPLUGIN_HPP