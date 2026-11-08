#pragma once

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace history {

struct Record {
    std::uint64_t id = 0;
    std::string result;
    std::string reason;
    std::string moves;
    std::string final_placement;
};

struct Data {
    std::vector<Record> records;
    bool legacy = false;
};

inline constexpr const char* old_header =
    "result\treason\tmoves\tfinal_placement";

inline constexpr const char* new_header =
    "game_id\tresult\treason\tmoves\tfinal_placement";

inline std::vector<std::string> split_tabs(const std::string& line) {
    std::vector<std::string> fields;
    std::size_t start = 0;

    while (true) {
        const auto end = line.find('\t', start);
        fields.push_back(line.substr(start, end - start));

        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }

    return fields;
}

inline std::uint64_t parse_id(const std::string& text) {
    if (text.empty()) {
        throw std::runtime_error("Empty game ID.");
    }

    std::uint64_t value = 0;
    for (char character : text) {
        if (character < '0' || character > '9') {
            throw std::runtime_error("Invalid game ID: " + text);
        }

        const auto digit = static_cast<std::uint64_t>(character - '0');
        if (value >
            (std::numeric_limits<std::uint64_t>::max() - digit) / 10) {
            throw std::runtime_error("Game ID is too large.");
        }

        value = value * 10 + digit;
    }

    if (value == 0) {
        throw std::runtime_error("Game IDs must start at 1.");
    }

    return value;
}

inline Data read(const std::string& filename) {
    Data data;
    const std::filesystem::path path(filename);

    if (!std::filesystem::exists(path)) {
        return data;
    }

    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Could not open history: " + filename);
    }

    std::string line;
    if (!std::getline(input, line)) {
        if (input.bad()) {
            throw std::runtime_error("Could not read history.");
        }
        return data; // Empty file is valid.
    }

    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }

    if (line == old_header) {
        data.legacy = true;
    } else if (line != new_header) {
        throw std::runtime_error("Unrecognised history header.");
    }

    std::set<std::uint64_t> ids;
    std::size_t line_number = 1;

    while (std::getline(input, line)) {
        ++line_number;

        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }

        const auto fields = split_tabs(line);
        const std::size_t expected = data.legacy ? 4 : 5;

        if (fields.size() != expected) {
            throw std::runtime_error(
                "Invalid history row at line " +
                std::to_string(line_number) + ".");
        }

        Record record;
        const std::size_t offset = data.legacy ? 0 : 1;

        record.id = data.legacy
            ? static_cast<std::uint64_t>(data.records.size()) + 1
            : parse_id(fields[0]);

        if (!ids.insert(record.id).second) {
            throw std::runtime_error("Duplicate game ID.");
        }

        record.result = fields[offset];
        record.reason = fields[offset + 1];
        record.moves = fields[offset + 2];
        record.final_placement = fields[offset + 3];
        data.records.push_back(std::move(record));
    }

    if (input.bad()) {
        throw std::runtime_error("Could not finish reading history.");
    }

    return data;
}

// Write a complete replacement beside the original, then rename it.
// On Linux, the rename replaces the original atomically.
inline void write(const std::string& filename, const Data& data) {
    const std::filesystem::path path(filename);

    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path());
    }

    // Preserve the original four-column file before migrating it.
    if (data.legacy && std::filesystem::exists(path)) {
        auto backup = path;
        backup += ".legacy.bak";

        unsigned suffix = 1;
        while (std::filesystem::exists(backup)) {
            backup = path;
            backup += ".legacy.bak." + std::to_string(suffix++);
        }

        std::filesystem::copy_file(path, backup);
    }

    auto temporary = path;
    temporary += ".tmp";

    try {
        std::ofstream output(temporary, std::ios::trunc);
        if (!output) {
            throw std::runtime_error("Could not create history temporary file.");
        }

        output << new_header << '\n';
        for (const auto& record : data.records) {
            output << record.id << '\t'
                   << record.result << '\t'
                   << record.reason << '\t'
                   << record.moves << '\t'
                   << record.final_placement << '\n';
        }

        output.close();
        if (!output) {
            throw std::runtime_error("Could not finish writing history.");
        }

        std::filesystem::rename(temporary, path);
    } catch (...) {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        throw;
    }
}

inline Data load_and_migrate(const std::string& filename) {
    Data data = read(filename);
    if (data.legacy) {
        write(filename, data);
        data.legacy = false;
    }
    return data;
}

inline void append(const std::string& filename, Record record) {
    Data data = read(filename);

    std::uint64_t maximum = 0;
    for (const auto& existing : data.records) {
        maximum = std::max(maximum, existing.id);
    }

    if (maximum == std::numeric_limits<std::uint64_t>::max()) {
        throw std::runtime_error("No more game IDs are available.");
    }

    record.id = maximum + 1;
    data.records.push_back(std::move(record));
    write(filename, data);
}

} // namespace history