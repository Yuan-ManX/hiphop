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

#ifndef WASM_RUNTIME_HPP
#define WASM_RUNTIME_HPP

#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "src/DistrhoDefines.h"
#include "distrho/extra/LeakDetector.hpp"

#ifdef HIPHOP_WASM_RUNTIME_WAMR
# include "wasm_c_api.h"
#elif HIPHOP_WASM_RUNTIME_WASMER
# ifdef DISTRHO_OS_WINDOWS
#  define WASM_API_EXTERN // allow to link against static lib
# endif
# include "wasm.h"
# include "wasmer.h"
#else
# error "Unknown WebAssembly runtime specified"
#endif

#define MakeI32(x) WASM_I32_VAL(static_cast<int32_t>(x))
#define MakeI64(x) WASM_I64_VAL(static_cast<int64_t>(x))
#define MakeF32(x) WASM_F32_VAL(static_cast<float32_t>(x))
#define MakeF64(x) WASM_F64_VAL(static_cast<float64_t>(x))

START_NAMESPACE_DISTRHO

struct WasmFunctionDescriptor;

typedef wasm_val_t WasmValue;
typedef std::vector<WasmValue> WasmValueVector;
typedef std::vector<enum wasm_valkind_enum> WasmValueKindVector;
typedef std::function<WasmValueVector(WasmValueVector)> WasmFunction;
typedef std::vector<WasmFunction> WasmFunctionVector;
typedef std::unordered_map<std::string, WasmFunctionDescriptor> WasmFunctionMap;
typedef std::unordered_map<std::string, wasm_extern_t*> WasmExternMap;

struct WasmFunctionDescriptor
{
    WasmValueKindVector params;
    WasmValueKindVector result;
    WasmFunction        function;
};

class WasmRuntime
{
public:
    WasmRuntime();
    ~WasmRuntime();

    void load(const char* modulePath);
    void load(const unsigned char* moduleData, size_t size);

    bool isStarted() { return fStarted; }
    void start(WasmFunctionMap hostFunctions);

    byte_t* getMemory(const WasmValue& wPtr = MakeI32(0));
    char*   getMemoryAsCString(const WasmValue& wPtr);
    void    copyCStringToMemory(const WasmValue& wPtr, const char* s);

    WasmValue getGlobal(const char* name);
    void      setGlobal(const char* name, const WasmValue& value);
    char*     getGlobalAsCString(const char* name);

    WasmValueVector callFunction(const char* name, WasmValueVector params = {});
    WasmValue       callFunctionReturnSingleValue(const char* name, WasmValueVector params = {});
    const char*     callFunctionReturnCString(const char* name, WasmValueVector params = {});

private:
    void stop();
    void unload();

    static wasm_trap_t* callHostFunction(void *env, const wasm_val_vec_t* paramsVec, wasm_val_vec_t* resultVec);

    static void toCValueTypeVector(WasmValueKindVector kinds, wasm_valtype_vec_t* types);
       
    const char* WTF16ToCString(const WasmValue& wPtr);
    WasmValue   CToWTF16String(const char* s);

    bool               fStarted;
    wasm_engine_t*     fEngine;
    wasm_store_t*      fStore;
    wasm_module_t*     fModule;
    wasm_instance_t*   fInstance;
    wasm_extern_vec_t  fExportsVec;
#ifdef HIPHOP_ENABLE_WASI
    wasi_env_t*        fWasiEnv;
#endif // HIPHOP_ENABLE_WASI
    WasmFunctionVector fHostFunctions;
    WasmExternMap      fModuleExports;

#ifdef HIPHOP_WASM_RUNTIME_WAMR
    static int sWamrRefCount;
#endif // HIPHOP_WASM_RUNTIME_WAMR

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WasmRuntime)

};

struct wasm_module_exception : public virtual std::runtime_error
{
    using std::runtime_error::runtime_error;
};

#ifdef HIPHOP_WASM_RUNTIME_WASMER
struct wasm_runtime_exception : public virtual std::exception
{
    wasm_runtime_exception(std::string what)
        : fWhat(what)
    {
        const int len = wasmer_last_error_length();
        
        if (len > 0) {
            char s[len];
            wasmer_last_error_message(s, len);
            fWhat += std::string(" - wasmer error: ") + s;
        }
    }

    virtual const char* what() const noexcept override
    {
        return fWhat.c_str();
    }

private:
    std::string fWhat;
};
#else
struct wasm_runtime_exception : public virtual std::runtime_error
{
    using std::runtime_error::runtime_error;
};
#endif

END_NAMESPACE_DISTRHO

#endif  // WASM_RUNTIME_HPP