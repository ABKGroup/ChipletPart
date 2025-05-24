///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2024, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <iomanip>

namespace Console {

// ANSI color codes
inline const std::string RESET = "\033[0m";
inline const std::string BLACK = "\033[30m";
inline const std::string RED = "\033[31m";
inline const std::string GREEN = "\033[32m";
inline const std::string YELLOW = "\033[33m";
inline const std::string BLUE = "\033[34m";
inline const std::string MAGENTA = "\033[35m";
inline const std::string CYAN = "\033[36m";
inline const std::string WHITE = "\033[37m";
inline const std::string BOLD = "\033[1m";

/**
 * Create a boxed title for major sections.
 * @param title The title text
 * @param width The width of the box
 * @return The formatted title box as a string
 */
inline std::string BoxedTitle(const std::string& title, int width = 60) {
  std::string result;
  std::string top_bottom(width, '=');
  int padding = (width - title.length() - 2) / 2;
  std::string padded_title = std::string(padding, ' ') + title + std::string(padding, ' ');
  
  // Ensure the title fits the box by adding an extra space if needed
  if (padded_title.length() < width - 2) {
    padded_title += " ";
  }
  
  result = top_bottom + "\n";
  result += "|" + padded_title + "|\n";
  result += top_bottom;
  
  return result;
}

/**
 * Create a progress bar visualization.
 * @param progress Value between 0.0 and 1.0
 * @param width The width of the progress bar
 * @return The formatted progress bar as a string
 */
inline std::string ProgressBar(float progress, int width = 50) {
  int pos = static_cast<int>(width * progress);
  std::string bar;
  
  bar = "[";
  for (int i = 0; i < width; ++i) {
    if (i < pos) bar += "=";
    else if (i == pos) bar += ">";
    else bar += " ";
  }
  
  int percent = static_cast<int>(progress * 100.0);
  bar += "] " + std::to_string(percent) + "%";
  
  return bar;
}

/**
 * Print an informational message.
 * @param message The message to print
 */
inline void Info(const std::string& message) {
  std::cout << BLUE << "[INFO] " << RESET << message << std::endl;
}

/**
 * Print a success message.
 * @param message The message to print
 */
inline void Success(const std::string& message) {
  std::cout << GREEN << "[SUCCESS] " << RESET << message << std::endl;
}

/**
 * Print a warning message.
 * @param message The message to print
 */
inline void Warning(const std::string& message) {
  std::cout << YELLOW << "[WARNING] " << RESET << message << std::endl;
}

/**
 * Print an error message.
 * @param message The message to print
 */
inline void Error(const std::string& message) {
  std::cout << RED << "[ERROR] " << RESET << message << std::endl;
}

/**
 * Print a debug message.
 * @param message The message to print
 */
inline void Debug(const std::string& message) {
  std::cout << MAGENTA << "[DEBUG] " << RESET << message << std::endl;
}

/**
 * Print a section header.
 * @param message The header text
 */
inline void Header(const std::string& message) {
  std::cout << std::endl << BOLD << CYAN 
            << BoxedTitle(message) << RESET << std::endl << std::endl;
}

/**
 * Print a sub-section header.
 * @param message The subheader text
 */
inline void Subheader(const std::string& message) {
  std::string line(message.length() + 4, '-');
  std::cout << std::endl << BOLD << CYAN << line << std::endl
            << "| " << message << " |" << std::endl
            << line << RESET << std::endl << std::endl;
}

/**
 * Print a table header with column titles.
 * @param columns The column titles
 * @param widths The widths for each column
 */
inline void TableHeader(const std::vector<std::string>& columns, const std::vector<int>& widths) {
  // Print top border
  std::cout << "+";
  for (size_t i = 0; i < columns.size(); ++i) {
    std::cout << std::string(widths[i] + 2, '-') << "+";
  }
  std::cout << std::endl;
  
  // Print column headers
  std::cout << "|";
  for (size_t i = 0; i < columns.size(); ++i) {
    std::cout << " " << BOLD << std::left << std::setw(widths[i]) 
              << columns[i] << RESET << " |";
  }
  std::cout << std::endl;
  
  // Print separator
  std::cout << "+";
  for (size_t i = 0; i < columns.size(); ++i) {
    std::cout << std::string(widths[i] + 2, '-') << "+";
  }
  std::cout << std::endl;
}

/**
 * Print a table header with column titles from an initializer list.
 * @param columns The column titles as an initializer list
 * @param widths The widths for each column
 */
inline void TableHeader(std::initializer_list<std::string> columns, const std::vector<int>& widths) {
  std::vector<std::string> vec_columns(columns);
  TableHeader(vec_columns, widths);
}

/**
 * Print a table row with values.
 * @param values The values for each column
 * @param widths The widths for each column
 */
inline void TableRow(const std::vector<std::string>& values, const std::vector<int>& widths) {
  std::cout << "|";
  for (size_t i = 0; i < values.size(); ++i) {
    std::cout << " " << std::left << std::setw(widths[i]) << values[i] << " |";
  }
  std::cout << std::endl;
}

/**
 * Print a table row with values from an initializer list.
 * @param values The values for each column as an initializer list
 * @param widths The widths for each column
 */
inline void TableRow(std::initializer_list<std::string> values, const std::vector<int>& widths) {
  std::vector<std::string> vec_values(values);
  TableRow(vec_values, widths);
}

} // namespace Console 