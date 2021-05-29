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

#include "RuntimePath.hpp"
#include "log.h"
#include "macro.h"

USE_NAMESPACE_DISTRHO

#if defined(DISTRHO_OS_LINUX) || defined(DISTRHO_OS_MAC)

#include <cstring>
#include <dlfcn.h>
#include <libgen.h>
#include <unistd.h>

#ifdef DISTRHO_OS_LINUX

#include <linux/limits.h>

String rtpath::getTemporaryPath()
{
	// Currently not needed
	return String();
}

String rtpath::getExecutablePath()
{
    char path[PATH_MAX];
    ssize_t len = ::readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len == -1) {
        LOG_STDERR_ERRNO("Could not determine executable path");
        return String();
    }
    return String(path);
}

#elif DISTRHO_OS_MAC

#include <sys/syslimits.h>

String rtpath::getExecutablePath()
{
    return getSharedLibraryPath();  // does the trick on macOS
}

#endif

String rtpath::getSharedLibraryPath()
{
    Dl_info dl_info;
    if (::dladdr((void *)&__PRETTY_FUNCTION__, &dl_info) == 0) {
        LOG_STDERR(::dlerror());
        return String();
    }
    return String(dl_info.dli_fname);
}

String rtpath::getBinaryPath()
{
    // DISTRHO_PLUGIN_TARGET_* macros are not available here
    // Is there a better way to differentiate we are being called from library or executable?
    String libPath = getSharedLibraryPath();     // path never empty even if running standalone
    void *handle = ::dlopen(libPath, RTLD_LAZY); // returns non-null on macOS also for standalone
    if (handle) {
        ::dlclose(handle);
        return libPath;
    }
    return getExecutablePath();
}

String rtpath::getBinaryDirectoryPath()
{
    char path[PATH_MAX];
    ::strcpy(path, getBinaryPath());
    return String(::dirname(path));
}

#endif // DISTRHO_OS_LINUX || DISTRHO_OS_MAC


#ifdef DISTRHO_OS_WINDOWS

#include <cstring>
#include <errhandlingapi.h>
#include <libloaderapi.h>
#include <shlobj.h>
#include <shlwapi.h>

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

String rtpath::getTemporaryPath()
{
    // Get temp path inside user files folder: C:\Users\< USERNAME >\AppData\Local\DPFTemp
    char tempPath[MAX_PATH];
    HRESULT result = ::SHGetFolderPath(0, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_DEFAULT, tempPath);
    if (FAILED(result)) {
        LOG_STDERR_INT("Could not determine user app data folder", result);
        return String();
    }

    // Append host executable name to the temp path otherwise WebView2 controller initialization
    // fails with HRESULT 0x8007139f when trying to load plugin into more than a single host
    // simultaneously due to permissions. C:\Users\< USERNAME >\AppData\Local\DPFTemp\< HOST_BIN >
    char exePath[MAX_PATH];
    if (::GetModuleFileName(NULL, exePath, sizeof(exePath)) == 0) {
        LOG_STDERR_INT("Could not determine host executable path", ::GetLastError());
        return String();
    }

    LPSTR exeFilename = ::PathFindFileName(exePath);
    // The following call relies on a further Windows library called Pathcch, which is implemented
    // in in api-ms-win-core-path-l1-1-0.dll and requires Windows 8.
    // Since the minimum plugin target is Windows 7 it is acceptable to use a deprecated function.
    //::PathCchRemoveExtension(exeFilename, sizeof(exeFilename));
    ::PathRemoveExtension(exeFilename);

    ::strcat(tempPath, "\\DPFTemp\\");
    ::strcat(tempPath, exeFilename);
    
    return String(tempPath);
}

String rtpath::getExecutablePath()
{
    // Standalone JACK app on Windows is not currently implemented
    return String();
}

String rtpath::getSharedLibraryPath()
{
    char path[MAX_PATH];
    if (::GetModuleFileName((HINSTANCE)&__ImageBase, path, sizeof(path)) == 0) {
        LOG_STDERR_INT("Could not determine DLL path", ::GetLastError());
        return String();
    }
    return String(path);
}

String rtpath::getBinaryPath()
{
    return getSharedLibraryPath();
}

String rtpath::getBinaryDirectoryPath()
{
    char path[MAX_PATH];
    ::strcpy(path, getBinaryPath());
    ::PathRemoveFileSpec(path);
    return String(path);
}

#endif // DISTRHO_OS_WINDOWS


String rtpath::getResourcePath()
{
#ifdef DISTRHO_OS_MAC
    // There is no DISTRHO method for querying plugin format during runtime
    // Anyways the ideal solution is to modify the Makefile and rely on macros
    // Mac VST is the only special case
    char path[PATH_MAX];
    ::strcpy(path, getSharedLibraryPath());
    void *handle = ::dlopen(path, RTLD_NOLOAD);
    if (handle != NULL) {
        void *addr = ::dlsym(handle, "VSTPluginMain");
        ::dlclose(handle);
        if (addr != NULL) {
            return String(::dirname(path)) + "/../Resources";
        }
    }
#endif
    String binPath = getBinaryDirectoryPath();
#ifdef DISTRHO_OS_WINDOWS
	binPath.replace('\\', '/');
#endif
    return binPath + "/" XSTR(BIN_BASENAME) "_resources";
}