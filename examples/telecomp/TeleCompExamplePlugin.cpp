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

#include "ZamCompX2Plugin.hpp"

START_NAMESPACE_DISTRHO

class TeleCompExamplePlugin : public ZamCompX2Plugin
{
public:
    TeleCompExamplePlugin()
        : ZamCompX2Plugin()
    {}

    const char* getLabel() const noexcept override
    {
        return "TeleComp";
    }

    const char* getDescription() const noexcept override
    {
        return "ZamCompX2 stereo compressor with alternate network-enabled web UI.";
    }

    const char* getMaker() const noexcept override
    {
        return "Luciano Iam, Damien Zammit";
    }

    const char* getLicense() const noexcept override
    {
        return "GPLv3";
    }

    uint32_t getVersion() const noexcept override
    {
        return d_version(1, 0, 0);
    }

    int64_t getUniqueId() const noexcept override
    {
        return d_cconst('H', 'H', 't', 'c');
    }

    // VST3
    void initAudioPort(const bool input, uint32_t index, AudioPort& port) override
    {
        port.groupId = kPortGroupStereo;
        ZamCompX2Plugin::initAudioPort(input, index, port);
    }

private:

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TeleCompExamplePlugin)

};

Plugin* createPlugin()
{
    return new TeleCompExamplePlugin;
}

END_NAMESPACE_DISTRHO
