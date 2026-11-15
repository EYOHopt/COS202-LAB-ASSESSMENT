#pragma once
// Student-implemented sorting algorithms (no std::sort is used anywhere).
// Each function sorts the vector in place and returns the number of comparisons made.
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "model/PatientRecord.h"

namespace scapms::controller {

enum class SortAlgorithm { Selection, Insertion, Merge, Quick };
enum class SortKey { Name, Id, Age, AppointmentDate };

std::string algorithmName(SortAlgorithm algorithm);
std::string sortKeyName(SortKey key);
std::string algorithmRationale(SortAlgorithm algorithm);   // why/when the algorithm is appropriate

using PatientLess = std::function<bool(const model::PatientRecord*, const model::PatientRecord*)>;
PatientLess makeComparator(SortKey key, bool ascending);

namespace sorting {

// Selection sort: repeatedly select the smallest remaining element. O(n^2), not stable.
template <typename T, typename Less>
long selectionSort(std::vector<T>& items, Less less) {
    long comparisons = 0;
    for (std::size_t i = 0; i + 1 < items.size(); ++i) {
        std::size_t smallest = i;
        for (std::size_t j = i + 1; j < items.size(); ++j) {
            ++comparisons;
            if (less(items[j], items[smallest])) smallest = j;
        }
        if (smallest != i) std::swap(items[i], items[smallest]);
    }
    return comparisons;
}

// Insertion sort: insert each element into the sorted part on its left. O(n^2) worst, O(n) if nearly sorted, stable.
template <typename T, typename Less>
long insertionSort(std::vector<T>& items, Less less) {
    long comparisons = 0;
    for (std::size_t i = 1; i < items.size(); ++i) {
        T key = items[i];
        std::size_t j = i;
        while (j > 0) {
            ++comparisons;
            if (!less(key, items[j - 1])) break;
            items[j] = items[j - 1];
            --j;
        }
        items[j] = key;
    }
    return comparisons;
}

namespace detail {

// Recursive merge sort of the half-open range [low, high).
template <typename T, typename Less>
void mergeSortRange(std::vector<T>& items, std::vector<T>& buffer, std::size_t low, std::size_t high,
                    Less& less, long& comparisons) {
    if (high - low < 2) return;                       // base case: 0 or 1 element
    std::size_t mid = low + (high - low) / 2;
    mergeSortRange(items, buffer, low, mid, less, comparisons);    // sort left half
    mergeSortRange(items, buffer, mid, high, less, comparisons);   // sort right half
    std::size_t i = low, j = mid, k = low;
    while (i < mid && j < high) {                     // merge the two sorted halves
        ++comparisons;
        if (less(items[j], items[i])) buffer[k++] = items[j++];
        else buffer[k++] = items[i++];               // take left on ties => stable
    }
    while (i < mid) buffer[k++] = items[i++];
    while (j < high) buffer[k++] = items[j++];
    for (std::size_t t = low; t < high; ++t) items[t] = buffer[t];
}

// Recursive quick sort of the closed range [low, high] using the middle element as pivot.
template <typename T, typename Less>
void quickSortRange(std::vector<T>& items, long low, long high, Less& less, long& comparisons) {
    if (low >= high) return;
    long mid = low + (high - low) / 2;
    std::swap(items[static_cast<std::size_t>(mid)], items[static_cast<std::size_t>(high)]);
    T pivot = items[static_cast<std::size_t>(high)];
    long store = low;
    for (long i = low; i < high; ++i) {
        ++comparisons;
        if (less(items[static_cast<std::size_t>(i)], pivot)) {
            std::swap(items[static_cast<std::size_t>(i)], items[static_cast<std::size_t>(store)]);
            ++store;
        }
    }
    std::swap(items[static_cast<std::size_t>(store)], items[static_cast<std::size_t>(high)]);
    quickSortRange(items, low, store - 1, less, comparisons);
    quickSortRange(items, store + 1, high, less, comparisons);
}

} // namespace detail

// Merge sort: divide in halves, sort each recursively, merge. O(n log n) always, stable.
template <typename T, typename Less>
long mergeSort(std::vector<T>& items, Less less) {
    long comparisons = 0;
    std::vector<T> buffer(items.size());
    detail::mergeSortRange(items, buffer, 0, items.size(), less, comparisons);
    return comparisons;
}

// Quick sort: partition around a pivot, sort each side recursively. O(n log n) average, not stable.
template <typename T, typename Less>
long quickSort(std::vector<T>& items, Less less) {
    long comparisons = 0;
    if (!items.empty()) detail::quickSortRange(items, 0, static_cast<long>(items.size()) - 1, less, comparisons);
    return comparisons;
}

template <typename T, typename Less>
long runSort(SortAlgorithm algorithm, std::vector<T>& items, Less less) {
    switch (algorithm) {
    case SortAlgorithm::Selection: return selectionSort(items, less);
    case SortAlgorithm::Insertion: return insertionSort(items, less);
    case SortAlgorithm::Merge: return mergeSort(items, less);
    case SortAlgorithm::Quick: return quickSort(items, less);
    }
    return 0;
}

} // namespace sorting
} // namespace scapms::controller
