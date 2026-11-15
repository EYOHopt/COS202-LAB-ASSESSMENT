#pragma once
// Student-implemented searching algorithms (linear, binary and recursive).
#include <string>
#include <vector>

#include "model/PatientRecord.h"

namespace scapms::controller {

struct SearchResult {
    std::vector<model::PatientRecord*> matches;
    long comparisons = 0;
    std::string algorithm;
};

class SearchEngine {
public:
    // Linear search for an exact Patient ID. Works on unsorted data. O(n).
    static int linearSearchById(const std::vector<model::PatientRecord*>& list, const std::string& id, long& comparisons);

    // Binary search for an exact Patient ID. List MUST be sorted by ID (ascending). O(log n).
    static int binarySearchById(const std::vector<model::PatientRecord*>& sortedById, const std::string& id,
                                long& comparisons);

    // Recursive search: collects every patient whose name contains the keyword (case-insensitive).
    static void recursiveSearchByName(const std::vector<model::PatientRecord*>& list, const std::string& keywordLower,
                                      std::size_t index, std::vector<model::PatientRecord*>& results, long& comparisons);

    // Linear (iterative) partial name search, for comparison with the recursive version.
    static std::vector<model::PatientRecord*> linearSearchByName(const std::vector<model::PatientRecord*>& list,
                                                                 const std::string& keywordLower, long& comparisons);

    static std::vector<model::PatientRecord*> linearSearchByCategory(const std::vector<model::PatientRecord*>& list,
                                                                     model::PatientCategory category, long& comparisons);

    // Manual case-insensitive substring test (keyword must already be lower case).
    static bool containsIgnoreCase(const std::string& text, const std::string& keywordLower);
};

} // namespace scapms::controller
