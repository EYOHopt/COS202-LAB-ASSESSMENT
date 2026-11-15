#include "controller/Sorter.h"

#include "utils/StringUtils.h"

namespace scapms::controller {

std::string algorithmName(SortAlgorithm algorithm) {
    switch (algorithm) {
    case SortAlgorithm::Selection: return "Selection Sort";
    case SortAlgorithm::Insertion: return "Insertion Sort";
    case SortAlgorithm::Merge: return "Merge Sort";
    case SortAlgorithm::Quick: return "Quick Sort";
    }
    return "";
}

std::string sortKeyName(SortKey key) {
    switch (key) {
    case SortKey::Name: return "Patient Name";
    case SortKey::Id: return "Patient ID";
    case SortKey::Age: return "Age";
    case SortKey::AppointmentDate: return "Next Appointment Date";
    }
    return "";
}

std::string algorithmRationale(SortAlgorithm algorithm) {
    switch (algorithm) {
    case SortAlgorithm::Selection:
        return "Selection Sort: simple, makes at most n-1 swaps. Suitable for the small patient lists of a "
               "university clinic, but always O(n\xC2\xB2) comparisons and not stable.";
    case SortAlgorithm::Insertion:
        return "Insertion Sort: stable and very fast (close to O(n)) when records are already nearly sorted, "
               "e.g. after adding a few new patients to a sorted list. O(n\xC2\xB2) in the worst case.";
    case SortAlgorithm::Merge:
        return "Merge Sort: guaranteed O(n log n) and stable (patients with the same name/age keep their order). "
               "Best choice for large lists; uses extra memory for merging.";
    case SortAlgorithm::Quick:
        return "Quick Sort: usually the fastest in practice, O(n log n) on average, sorts in place. "
               "Not stable; the middle-element pivot avoids the worst case on already sorted data.";
    }
    return "";
}

PatientLess makeComparator(SortKey key, bool ascending) {
    PatientLess base;
    switch (key) {
    case SortKey::Name:
        base = [](const model::PatientRecord* a, const model::PatientRecord* b) {
            return utils::toLower(a->name()) < utils::toLower(b->name());
        };
        break;
    case SortKey::Id:
        base = [](const model::PatientRecord* a, const model::PatientRecord* b) { return a->id() < b->id(); };
        break;
    case SortKey::Age:
        base = [](const model::PatientRecord* a, const model::PatientRecord* b) { return a->age() < b->age(); };
        break;
    case SortKey::AppointmentDate:
        base = [](const model::PatientRecord* a, const model::PatientRecord* b) {
            std::string da = a->nextPendingDateTime(), db = b->nextPendingDateTime();
            if (da.empty()) da = "9999";   // patients without appointments go last
            if (db.empty()) db = "9999";
            return da < db;
        };
        break;
    }
    if (ascending) return base;
    return [base](const model::PatientRecord* a, const model::PatientRecord* b) { return base(b, a); };
}

} // namespace scapms::controller
