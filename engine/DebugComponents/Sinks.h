#pragma once
#include "Log.h"
#include <fstream>

namespace eng::debug {

	// // Console (stderr) + Windows Output window (optional)
	class ConsoleSink final : public ILogSink {
	public:
		explicit ConsoleSink(bool usePlatformOutput = true) : m_usePlatformOutput(usePlatformOutput) {}
		void write(LogLevel lvl, const char* tag, const char* msg) override;
	private:
		bool m_usePlatformOutput = true;
	};

	// Append to file
	class FileSink final : public ILogSink {
	public:
		explicit FileSink(const std::string& path);
		~FileSink();
		void write(LogLevel lvl, const char* tag, const char* msg) override;
	private:
		std::ofstream m_out;
	};

} // namespace eng::debug
