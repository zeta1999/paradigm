﻿#pragma once

#define __STDC_WANT_LIB_EXT1__ 1 // https://stackoverflow.com/questions/31278172/undefined-reference-to-memcpy-s
#include <stddef.h>
#include <stdarg.h>
#include <cstddef>
#include <vector>
#include <array>

#include "psl/platform_def.h"
#include "psl/ustring.h"

#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <android_native_app_glue.h>
#include <android/configuration.h>
#endif

#include "psl/logging.h"

namespace platform::specifics
{
#if defined(PLATFORM_ANDROID)
	extern android_app* android_application;
#endif

} // namespace platform::specifics


#if !defined(NDEBUG) && defined(NDEBUG) && !defined(DEBUG)
#define NDEBUG 1
#endif

#include "psl/assertions.h"

/*! \namespace memory \brief this namespace contains types and utilities for managing regions of memory
	\details due to the potential constrained environments that this app might run in, there are
	some helper classes provided to track and manage memory (both physically backed and non-physically backed).
	You can find more information in memory::region about this.
*/


#include "psl/template_utils.h"
#include "psl/timer.h"
#include "psl/profiling/profiler.h"

#include "psl/enumerate.h"
#include "psl/array_view.h"
#include "psl/pack_view.h"

#include "psl/literals.h"