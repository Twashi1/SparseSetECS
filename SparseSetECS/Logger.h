// Just ripped off TwashiUtils
#pragma once

#include <chrono>
#include <format>
#include <iostream>
#include <string>
#include <regex>

#define TWASHI_LOG_TRACE 0
#define TWASHI_LOG_INFO  1
#define TWASHI_LOG_WARN  2
#define TWASHI_LOG_ERROR 3
#define TWASHI_LOG_FATAL 4

#define LogFatal(msg, ...)	Logger::__LogBase(std::format(msg, __VA_ARGS__), __LINE__, __FUNCSIG__, "Fatal", TWASHI_LOG_FATAL)
#define LogError(msg, ...)	Logger::__LogBase(std::format(msg, __VA_ARGS__), __LINE__, __FUNCSIG__, "Error", TWASHI_LOG_ERROR)
#define LogWarn(msg, ...)	Logger::__LogBase(std::format(msg, __VA_ARGS__), __LINE__, __FUNCSIG__, "Warn",  TWASHI_LOG_WARN )
#define LogInfo(msg, ...)	Logger::__LogBase(std::format(msg, __VA_ARGS__), __LINE__, __FUNCSIG__, "Info",  TWASHI_LOG_INFO )
#define LogTrace(msg, ...)	Logger::__LogBase(std::format(msg, __VA_ARGS__), __LINE__, __FUNCSIG__, "Trace", TWASHI_LOG_TRACE)

namespace Logger {
	static constexpr int LOG_LEVEL = TWASHI_LOG_TRACE;

	void __LogBase(const std::string msg, int line, const char* func_sig, const char* severity_text, int severity)
#ifdef TWASHI_LOGGER_IMPLEMENTATION
		// Definition
	{
		if (severity < LOG_LEVEL) return;

		const std::regex cleaner("");
		std::string function_cleaned = std::regex_replace(func_sig, cleaner, "");

		std::cout << std::format(
			"[{}] {}: {}",
			std::format("{:%H:%M:%OS}", std::chrono::system_clock::now()),
			severity_text,
			msg
		) << std::endl;

		if (severity == TWASHI_LOG_FATAL) { exit(EXIT_FAILURE); }
	}
#else
		// Forward decl (pretty cheaty)
		;
#endif
}