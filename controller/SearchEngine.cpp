#include "controller/SearchEngine.h"

#include "utils/StringUtils.h"

namespace scapms::controller {

int SearchEngine::linearSearchById(const std::vector<model::PatientRecord*>& list, const std::string& id,
                                   long& comparisons) {
    for (std::size_t i = 0; i < list.size(); ++i) {
        ++comparisons;
        if (list[i]->id() == id) return static_cast<int>(i);
    }
    return -1;
}

int SearchEngine::binarySearchById(const std::vector<model::PatientRecord*>& sortedById, const std::string& id,
                                   long& comparisons) {
    long low = 0;
    long high = static_cast<long>(sortedById.size()) - 1;
    while (low <= high) {
        long mid = low + (high - low) / 2;
        const std::string& midId = sortedById[static_cast<std::size_t>(mid)]->id();
        ++comparisons;
        if (midId == id) return static_cast<int>(mid);
        if (midId < id) low = mid + 1;     // target is in the right half
        else high = mid - 1;               // target is in the left half
    }
    return -1;
}

void SearchEngine::recursiveSearchByName(const std::vector<model::PatientRecord*>& list,
                                         const std::string& keywordLower, std::size_t index,
                                         std::vector<model::PatientRecord*>& results, long& comparisons) {
    if (index >= list.size()) return;                                    // base case: end of list
    ++comparisons;
    if (containsIgnoreCase(list[index]->name(), keywordLower)) results.push_back(list[index]);
    recursiveSearchByName(list, keywordLower, index + 1, results, comparisons);   // recursive case
}

std::vector<model::PatientRecord*> SearchEngine::linearSearchByName(const std::vector<model::PatientRecord*>& list,
                                                                    const std::string& keywordLower,
                                                                    long& comparisons) {
    std::vector<model::PatientRecord*> results;
    for (model::PatientRecord* patient : list) {
        ++comparisons;
        if (containsIgnoreCase(patient->name(), keywordLower)) results.push_back(patient);
    }
    return results;
}

std::vector<model::PatientRecord*> SearchEngine::linearSearchByCategory(const std::vector<model::PatientRecord*>& list,
                                                                        model::PatientCategory category,
                                                                        long& comparisons) {
    std::vector<model::PatientRecord*> results;
    for (model::PatientRecord* patient : list) {
        ++comparisons;
        if (patient->category() == category) results.push_back(patient);
    }
    return results;
}

bool SearchEngine::containsIgnoreCase(const std::string& text, const std::string& keywordLower) {
    if (keywordLower.empty()) return true;
    std::string lower = utils::toLower(text);
    if (keywordLower.size() > lower.size()) return false;
    for (std::size_t start = 0; start + keywordLower.size() <= lower.size(); ++start) {
        std::size_t matched = 0;
        while (matched < keywordLower.size() && lower[start + matched] == keywordLower[matched]) ++matched;
        if (matched == keywordLower.size()) return true;
    }
    return false;
}

} // namespace scapms::controller
