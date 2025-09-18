#pragma once
#include "Log.h"
#include <fstream>

/*
===============================================================================
 Sinks.h
 ------------------------------------------------------------------------------
 Purpose
   A "sink" is a destination where a formatted log line is written to.
   The Log system can broadcast each line to one or more sinks. This header
   declares two concrete sinks you can use immediately:

	 1) ConsoleSink  -> prints to stderr, and optionally mirrors to
						Visual Studio's Output window (Windows only).
	 2) FileSink     -> appends each line to a text file on disk.

 How it fits together
   - The Log facade formats a final line (with timestamp, level, tag, message).
   - Then Log calls ILogSink::write(...) on each registered sink.
   - Sinks are kept simple: they only render the already-formatted line.

 Thread-safety
   - The Log facade holds a mutex when calling sinks.
   - Each sink implementation here performs simple I/O; no extra locking needed.

 Notes
   - You can add your own sinks by subclassing ILogSink (e.g., network sink).
   - See Sinks.cpp for the actual implementations.

===============================================================================
*/
namespace eng::debug {

	// ConsoleSink
		// -------------------------------------------------------------------------
		// Writes a final log line to:
		//   - stderr (console)
		//   - and optionally the Visual Studio Output window using OutputDebugString
		//     when running on Windows and m_usePlatformOutput == true.
		//
		// Why stderr?
		//   - stderr is unbuffered by default on many systems, so lines appear
		//     immediately in the console, which is useful for debugging.
	class ConsoleSink final : public ILogSink {
	public:

		// If usePlatformOutput is true (default), and we are on Windows,
		// we will also mirror each line to OutputDebugString so it shows up
		// in the VS "Output" pane while debugging.
		explicit ConsoleSink(bool usePlatformOutput = true) : m_usePlatformOutput(usePlatformOutput) {}

		// Render one fully formatted line.
		void write(LogLevel lvl, const char* tag, const char* msg) override;
	private:
		bool m_usePlatformOutput = true;
	};

	// FileSink
		// -------------------------------------------------------------------------
		// Appends log lines to a text file. The file is opened in append mode so
		// existing logs are preserved across runs.
		//
		// Lifetime:
		//   - Construct with a file path (e.g., "engine.log").
		//   - The destructor flushes the stream so you do not lose data
	class FileSink final : public ILogSink {
	public:
		explicit FileSink(const std::string& path);
		~FileSink();

		// Render one fully formatted line to the file (plus newline).
		void write(LogLevel lvl, const char* tag, const char* msg) override;
	private:
		std::ofstream m_out;	// owned file stream
	};

} // namespace eng::debug
