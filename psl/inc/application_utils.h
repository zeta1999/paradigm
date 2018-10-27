﻿#pragma once
#include "stdafx_psl.h"
#include "ustring.h"
#ifdef WIN32
#include "Shlwapi.h"
#endif
#if defined(PLATFORM_LINUX)
#include <unistd.h>
#include <limits.h>
#include <libgen.h>
#endif
/// \brief contains application specific information and utilities
///
/// this namespace contains application specific information and utilities that might help you to find
/// out information of its location, specific datapaths, etc...
namespace utility::application::path
{
	/// \brief gets the path of the current application. This path is platform specific.
	/// \todo check if returning an empty path for android is correct behaviour.
	/// \returns an absolute path where the application is being ran from.
	/// \note even though the path is absolute, don't expect it to look like a windows path on other platforms.
	inline static const psl::string get_path()
	{
#ifdef WIN32
		wchar_t dest[MAX_PATH];
		GetModuleFileName(NULL, dest, MAX_PATH);
		PathRemoveFileSpec(dest);
		return psl::from_platform_string(std::wstring_view{dest}) + "\\";
#elif defined PLATFORM_LINUX
		char result[PATH_MAX];
		ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
		const char *path;
		if(count != -1)
		{
			return psl::string{dirname(result)} + "/";
		}
#elif defined(PLATFORM_ANDROID) // we run in a sandbox where we are root
		return "";
#else
#error not supported
#endif
		// todo platform specific versions
		return {};
	}

	/// \brief location where the application is being ran from, check utility::application::path::get_path() for more info.
	static const psl::string project   = get_path();
	/// \brief data path on this platform.
	static const psl::string data = project + "data/";
	/// \brief library path on this platform.
	static const psl::string library = project + "library/";
	/// \brief settings path on this platform.
	static const psl::string settings = data + "settings/";
	/// \brief textures path on this platform.
	static const psl::string textures = data + "textures/";
	/// \brief materials path on this platform.
	static const psl::string materials = data + "materials/";
	/// \brief models path on this platform.
	static const psl::string models	= data + "models/";
	/// \brief shaders path on this platform.
	static const psl::string shaders   = data + "shaders/";
	/// \brief samplers path on this platform.
	static const psl::string samplers  = data + "samplers/";
	/// \brief worlds path on this platform.
	static const psl::string worlds	= data + "worlds/";
} // namespace utility::application::path
