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

#include "drivers/clogger.h"

#include <iostream>
CLogger CLogger::debug_logger("DEBUG", true);
CLogger CLogger::info_logger("INFO");

/**
 * @brief Create a new logger
 *
 * This will create a new logger with a specific name which by default
 * doesn't append functions and filenames.
 * @param[in] name The logger name
 */
CLogger::CLogger(std::string name) {
    this->name = name;
    this->appendFunc = false;
    this->appendFilename = false;
}

/**
 * @brief Create a new logger
 *
 * This will create a new logger with a specific name which by default
 * doesn't append filenames.
 * @param[in] name The logger name
 * @param[in] appendFunc Enable the appending of the function name
 */
CLogger::CLogger(std::string name, bool appendFunc) {
    this->name = name;
    this->appendFunc = appendFunc;
    this->appendFilename = false;
}

/**
 * @brief Create a new logger
 *
 * This will create a new logger with a specific name.
 * @param[in] name The logger name
 * @param[in] appendFunc Enable the appending of the function name
 * @param[in] appendFilename Enable the appending of filename with line number
 */
CLogger::CLogger(std::string name, bool appendFunc, bool appendFilename) {
    this->name = name;
    this->appendFunc = appendFunc;
    this->appendFilename = appendFilename;
}

/**
 * @brief Add an output stream to all loggers
 *
 * This will add an output stream to all available loggers. For example you could add
 * std::cout as a console output stream.
 * @param[in] os The output stream to add
 */
void CLogger::addAllOutput(std::ostream *os) {
    CLogger::debug_logger.addOutput(os);
    CLogger::info_logger.addOutput(os);
}

/**
 * @brief Add an output stream to a logger
 *
 * This will add an output stream to a specific logger. For example you could add the
 * std::cout as a console output stream.
 * @param[in] os The output stream to add
 */
void CLogger::addOutput(std::ostream *os) {
    outputs.push_back(os);
}

/**
 * @brief Format which is added before the line
 *
 * This will output a string based on the settings of the logger. This string contains
 * at least the logger name, but can be extended by the function name and file name.
 * @param[in] func The function name
 * @param[in] file The file name
 * @param[in] line The line number
 * @return Concatenated string based on the settings
 */
std::string CLogger::format(const char *func, const char *file, const int line) {
    std::string str = name + " ";
    if(appendFunc)
        str = str + "[" + func + "]";
    if(appendFilename)
        str = str + "[" + file + ":" + std::to_string(line) + "] ";
    return str;
}
