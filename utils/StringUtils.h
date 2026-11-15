#pragma once
// Small string helpers shared by all layers.
#include <cctype>
#include <string>

namespace scapms::utils {

inline std::string trim(const std::string& text) {
    std::size_t start = 0;
    std::size_t end = text.size();
    while (start < end && std::isspace(static_cast<unsigned char>(text[start]))) ++start;
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) --end;
    return text.substr(start, end - start);
}

inline std::string toLower(std::string text) {
    for (char& c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return text;
}

inline std::string toUpper(std::string text) {
    for (char& c : text) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return text;
}

// Parses a whole number without throwing. Returns false if the text is not a valid integer.
inline bool parseInt(const std::string& rawText, int& result) {
    std::string text = trim(rawText);
    if (text.empty() || text.size() > 9) return false;
    std::size_t i = 0;
    bool negative = false;
    if (text[0] == '-') { negative = true; i = 1; }
    if (i == text.size()) return false;
    int value = 0;
    for (; i < text.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(text[i]))) return false;
        value = value * 10 + (text[i] - '0');
    }
    result = negative ? -value : value;
    return true;
}

} // namespace scapms::utils
