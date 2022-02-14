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

#include <stdexcept>

#include "WasmHostPlugin.hpp"
#include "extra/Path.hpp"

#define WASM_BINARY_PATH "/dsp/optimized.wasm"

USE_NAMESPACE_DISTRHO

WasmHostPlugin::WasmHostPlugin(uint32_t parameterCount, uint32_t programCount, uint32_t stateCount,
                                std::shared_ptr<WasmRuntime> runtime)
    : PluginEx(parameterCount, programCount, stateCount)
    , fActive(false)
{   
    if (runtime != nullptr) {
        fRuntime = runtime;
        return; // caller initializes runtime
    }

    fRuntime.reset(new WasmRuntime());

    try {
        const String path = Path::getPluginLibrary() + WASM_BINARY_PATH;
        fRuntime->load(path);
        onModuleLoad();
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());
    }
}

#define ERROR_STR "Error"
#define CHECK_INSTANCE() checkInstance(__FUNCTION__)
#define SCOPED_RUNTIME_LOCK() ScopedSpinLock lock(fRuntimeLock)

const char* WasmHostPlugin::getLabel() const
{
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        return fRuntime->callFunctionReturnCString("_get_label");
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());

        return ERROR_STR;
    }
}

const char* WasmHostPlugin::getMaker() const
{
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        return fRuntime->callFunctionReturnCString("_get_maker");
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());

        return ERROR_STR;
    }
}

const char* WasmHostPlugin::getLicense() const
{
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        return fRuntime->callFunctionReturnCString("_get_license");
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());

        return ERROR_STR;
    }
}

uint32_t WasmHostPlugin::getVersion() const
{
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        return fRuntime->callFunctionReturnSingleValue("_get_version").of.i32;
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());

        return 0;
    }
}

int64_t WasmHostPlugin::getUniqueId() const
{
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        return fRuntime->callFunctionReturnSingleValue("_get_unique_id").of.i64;
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());

        return 0;
    }
}

void WasmHostPlugin::initParameter(uint32_t index, Parameter& parameter)
{
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        fRuntime->callFunction("_init_parameter", { MakeI32(index) });
        parameter.hints      = fRuntime->getGlobal("_rw_int32_1").of.i32;
        parameter.name       = fRuntime->getGlobalAsCString("_ro_string_1");
        parameter.ranges.def = fRuntime->getGlobal("_rw_float32_1").of.f32;
        parameter.ranges.min = fRuntime->getGlobal("_rw_float32_2").of.f32;
        parameter.ranges.max = fRuntime->getGlobal("_rw_float32_3").of.f32;
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());
    }
}

float WasmHostPlugin::getParameterValue(uint32_t index) const
{
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        return fRuntime->callFunctionReturnSingleValue("_get_parameter_value",
            { MakeI32(index) }).of.f32;
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());

        return 0;
    }
}

void WasmHostPlugin::setParameterValue(uint32_t index, float value)
{
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        fRuntime->callFunction("_set_parameter_value", { MakeI32(index), MakeF32(value) });
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());
    }
}

#if DISTRHO_PLUGIN_WANT_PROGRAMS
void WasmHostPlugin::initProgramName(uint32_t index, String& programName)
{
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        programName = fRuntime->callFunctionReturnCString("_init_program_name", { MakeI32(index) });
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());
    }
}

void WasmHostPlugin::loadProgram(uint32_t index)
{
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        fRuntime->callFunction("_load_program", { MakeI32(index) });
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());
    }
}
#endif // DISTRHO_PLUGIN_WANT_PROGRAMS

#if DISTRHO_PLUGIN_WANT_STATE
void WasmHostPlugin::initState(uint32_t index, String& stateKey, String& defaultStateValue)
{
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        fRuntime->callFunction("_init_state", { MakeI32(index) });
        stateKey = fRuntime->getGlobalAsCString("_ro_string_1");
        defaultStateValue = fRuntime->getGlobalAsCString("_ro_string_2");
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());
    }
}

void WasmHostPlugin::setState(const char* key, const char* value)
{
    PluginEx::setState(key, value);

    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        const WasmValue wkey = fRuntime->getGlobal("_rw_string_1");
        fRuntime->copyCStringToMemory(wkey, key);
        const WasmValue wval = fRuntime->getGlobal("_rw_string_2");
        fRuntime->copyCStringToMemory(wval, value);
        fRuntime->callFunction("_set_state", { wkey, wval });
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());
    }
}

#if DISTRHO_PLUGIN_WANT_FULL_STATE
String WasmHostPlugin::getState(const char* key) const
{
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        const WasmValue wkey = fRuntime->getGlobal("_rw_string_1");
        fRuntime->copyCStringToMemory(wkey, key);
        const char* val = fRuntime->callFunctionReturnCString("_get_state", { wkey });

        return String(val);
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());

        return String();
    }
}
#endif // DISTRHO_PLUGIN_WANT_FULL_STATE

#endif // DISTRHO_PLUGIN_WANT_STATE

void WasmHostPlugin::activate()
{
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        fRuntime->callFunction("_activate");
        fActive = true;
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());
    }
}

void WasmHostPlugin::deactivate()
{
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        fRuntime->callFunction("_deactivate");
        fActive = false;
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());
    }
}

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    void WasmHostPlugin::run(const float** inputs, float** outputs, uint32_t frames,
                                const MidiEvent* midiEvents, uint32_t midiEventCount)
{
#else
    void WasmHostPlugin::run(const float** inputs, float** outputs, uint32_t frames)
{
    const MidiEvent* midiEvents = 0;
    uint32_t midiEventCount = 0;
#endif // DISTRHO_PLUGIN_WANT_MIDI_INPUT
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        float32_t* audioBlock;

        audioBlock = reinterpret_cast<float32_t *>(fRuntime->getMemory(
            fRuntime->getGlobal("_rw_input_block")));

        for (int i = 0; i < DISTRHO_PLUGIN_NUM_INPUTS; i++) {
            memcpy(audioBlock + i * frames, inputs[i], frames * 4);
        }

        byte_t* midiBlock = fRuntime->getMemory(fRuntime->getGlobal("_rw_midi_block"));

        for (uint32_t i = 0; i < midiEventCount; i++) {
            *reinterpret_cast<uint32_t *>(midiBlock) = midiEvents[i].frame;
            midiBlock += 4;
            *reinterpret_cast<uint32_t *>(midiBlock) = midiEvents[i].size;
            midiBlock += 4;
            if (midiEvents[i].size > MidiEvent::kDataSize) {
                memcpy(midiBlock, midiEvents[i].dataExt, midiEvents[i].size);
            } else {
                memcpy(midiBlock, midiEvents[i].data, midiEvents[i].size);
            }
            midiBlock += midiEvents[i].size;
        }

        fRuntime->callFunction("_run", { MakeI32(frames), MakeI32(midiEventCount) });

        audioBlock = reinterpret_cast<float32_t *>(fRuntime->getMemory(
            fRuntime->getGlobal("_rw_output_block")));

        for (int i = 0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; i++) {
            memcpy(outputs[i], audioBlock + i * frames, frames * 4);
        }
    } catch (const std::exception& ex) {
        //d_stderr2(ex.what());
    }
}

#if HIPHOP_ENABLE_SHARED_MEMORY
void WasmHostPlugin::sharedMemoryChanged(const char* metadata, const unsigned char* data, size_t size)
{
    if (std::strcmp(metadata, "_wasm_bin") == 0) {
        try {
            loadWasmBinary(data, size);
        } catch (const std::exception& ex) {
            d_stderr2(ex.what());
        }
    }
}

void WasmHostPlugin::loadWasmBinary(const unsigned char* data, size_t size)
{
    // No need to check if the runtime is running
    SCOPED_RUNTIME_LOCK();

    fRuntime->load(data, size);
    onModuleLoad();

    // This has no effect on the host parameters but might be needed by the
    // plugin code to properly initialize.
    for (uint32_t i = 0; i < 128; ++i) {
        fRuntime->callFunction("_init_parameter", { MakeI32(i) });
    }

    if (fActive) {
        fRuntime->callFunction("_activate");
    }
}
#endif // HIPHOP_ENABLE_SHARED_MEMORY

WasmValueVector WasmHostPlugin::getTimePosition(WasmValueVector params)
{
    (void)params;
#if DISTRHO_PLUGIN_WANT_TIMEPOS
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        const TimePosition& pos = Plugin::getTimePosition();
        fRuntime->setGlobal("_rw_int32_1", MakeI32(pos.playing));
        fRuntime->setGlobal("_rw_int64_1", MakeI64(pos.frame));

        return {};
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());

        return {};
    }
#else
    throw std::runtime_error("Called getTimePosition() without DISTRHO_PLUGIN_WANT_TIMEPOS");
#endif // DISTRHO_PLUGIN_WANT_TIMEPOS
}

WasmValueVector WasmHostPlugin::writeMidiEvent(WasmValueVector params)
{
    (void)params;
#if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
    try {
        CHECK_INSTANCE();
        SCOPED_RUNTIME_LOCK();

        MidiEvent event;
        byte_t* midiBlock = fRuntime->getMemory(fRuntime->getGlobal("_rw_midi_block"));

        event.frame = *reinterpret_cast<uint32_t *>(midiBlock);
        midiBlock += 4;
        event.size = *reinterpret_cast<uint32_t *>(midiBlock);
        midiBlock += 4;

        if (event.size > MidiEvent::kDataSize) {
            event.dataExt = reinterpret_cast<uint8_t *>(midiBlock);
        } else {
            memcpy(event.data, midiBlock, event.size);
            event.dataExt = 0;
        }

        return { MakeI32(writeMidiEvent(event)) };
    } catch (const std::exception& ex) {
        d_stderr2(ex.what());

        return { MakeI32(0) };
    }
#else
    throw std::runtime_error("Called writeMidiEvent() without DISTRHO_PLUGIN_WANT_MIDI_OUTPUT");
#endif // DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
}

void WasmHostPlugin::onModuleLoad()
{
    WasmFunctionMap hostFunc;

    hostFunc["_get_samplerate"] = { {}, { WASM_F32 }, [this](WasmValueVector) -> WasmValueVector {
        return { MakeF32(getSampleRate()) };
    }};

    hostFunc["_get_time_position"] = { {}, {}, 
        std::bind(&WasmHostPlugin::getTimePosition, this, std::placeholders::_1)
    };

    hostFunc["_write_midi_event"] = { {}, { WASM_I32 }, 
        std::bind(&WasmHostPlugin::writeMidiEvent, this, std::placeholders::_1)
    };

    fRuntime->createInstance(hostFunc);

    fRuntime->setGlobal("_rw_num_inputs", MakeI32(DISTRHO_PLUGIN_NUM_INPUTS));
    fRuntime->setGlobal("_rw_num_outputs", MakeI32(DISTRHO_PLUGIN_NUM_OUTPUTS));
}

void WasmHostPlugin::checkInstance(const char* caller) const
{
    if (!fRuntime->hasInstance()) {
        throw std::runtime_error(std::string(caller) + "() : missing wasm instance");
    }
}