/*
 * This file is part of the XXX distribution (https://github.com/xxxx or http://xxx.github.io).
 * Copyright (c) 2016 Freek van Tienen <freek.v.tienen@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DRIVERS_CLOGGER_H_
#define DRIVERS_CLOGGER_H_

#include <string>
#include <memory>
#include <iostream>
#include <vector>

#if CLOGGER_ENABLE && !CLOGGER_NDEBUG
#define CLOGGER_DEBUG(...) CLogger::debug_logger << CLogger::debug_logger.format(__PRETTY_FUNCTION__, __FILE__, __LINE__) << __VA_ARGS__ << "\r\n"
#else
#define CLOGGER_DEBUG(...)
#endif

#if CLOGGER_ENABLE && !CLOGGER_NINFO
#define CLOGGER_INFO(...) CLogger::info_logger << CLogger::info_logger.format(__PRETTY_FUNCTION__, __FILE__, __LINE__) << __VA_ARGS__ << "\r\n"
#else
#define CLOGGER_INFO(...)
#endif

#if CLOGGER_ENABLE && !CLOGGER_NWARN
#define CLOGGER_WARN(...) CLogger::warn_logger << CLogger::info_logger.format(__PRETTY_FUNCTION__, __FILE__, __LINE__) << __VA_ARGS__ << "\r\n"
#else
#define CLOGGER_WARN(...)
#endif

/**
 * @brief Custom made logger
 *
 * This custom made logger will use targed based functions to log specific messages to console or
 * log files.
 */
class CLogger {
  private:
    std::vector<std::ostream *> outputs;        ///< Logger output streams
    std::string name;                           ///< Logger name (Debug, Info, ...)
    bool appendFunc;                            ///< Append function name to debug string
    bool appendFilename;                        ///< Append filename to debug string

  public:
    static CLogger debug_logger;                ///< Debug logger
    static CLogger info_logger;                 ///< Info logger
    static CLogger warn_logger;                 ///< Warning logger

    static void addAllOutput(std::ostream *os);

    /* Per logger operations */
    CLogger(std::string name, bool appendFunc = false, bool appendFilename = false);
    std::string format(const char* func, const char *file, const int line);
    void addOutput(std::ostream *os);

    /**
     * @brief Implementation for output steam access
     *
     * @param[in] t The second part of the output stream
     * @return The current CLogger
     */
    template<typename T> CLogger &operator<<(const T &t) {
        for(auto os : outputs) {
            *os << t;
        }
        return *this;
    }
};

#endif /* DRIVERS_CLOGGER_H_ */
