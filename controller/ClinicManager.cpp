#include "controller/ClinicManager.h"

#include <chrono>

#include "model/Appointable.h"
#include "utils/Exceptions.h"
#include "utils/FileHandler.h"
#include "utils/IDGenerator.h"
#include "utils/StringUtils.h"

namespace scapms::controller {

using model::PatientRecord;
using utils::ClinicException;
using utils::IDGenerator;
using utils::SearchException;

ClinicManager::ClinicManager() {
    for (auto& slot : recentPatients_) slot = nullptr;
}

// ======================= Patient records =======================

std::string ClinicManager::suggestPatientId() const {
    std::string id;
    int offset = 1;
    do {
        id = IDGenerator::peekPatientId(offset++);
    } while (findPatient(id));
    return id;
}

PatientRecord* ClinicManager::registerPatient(std::unique_ptr<PatientRecord> patient) {
    if (!patient) throw utils::ValidationException("No patient information was supplied.");
    if (findPatient(patient->id())) throw utils::DuplicateIdException(patient->id());
    PatientRecord* raw = patient.get();
    db_.add(std::move(patient));
    IDGenerator::observePatientId(raw->id());
    pushUndo(AdminOperation::Type::Register, raw->id(), nullptr, db_.size() - 1,
             "Register patient " + raw->id() + " (" + raw->name() + ")");
    sortedBy_.reset();   // new record appended at the end: list is no longer sorted
    markModified();
    touch(raw);
    return raw;
}

PatientRecord* ClinicManager::updatePatient(std::unique_ptr<PatientRecord> updated) {
    if (!updated) throw utils::ValidationException("No patient information was supplied.");
    PatientRecord* existing = requirePatient(updated->id());
    if (existing->category() != updated->category()) {
        throw utils::ValidationException("The patient category cannot be changed when editing.");
    }
    for (const model::Appointment& a : existing->appointments()) updated->attachAppointment(a);
    recordSnapshot(*existing, "Modify patient " + existing->id() + " (" + existing->name() + ")");
    PatientRecord* raw = updated.get();
    std::unique_ptr<PatientRecord> old = db_.replace(std::move(updated));
    cacheReplace(old.get(), raw);
    sortedBy_.reset();
    markModified();
    return raw;
}

void ClinicManager::deletePatient(const std::string& patientId) {
    PatientRecord* patient = requirePatient(patientId);
    cacheRemove(patient);
    removeFromQueues(patient->id());
    std::size_t index = 0;
    std::string description = "Delete patient " + patient->id() + " (" + patient->name() + ")";
    std::shared_ptr<const PatientRecord> snapshot(db_.removeById(patient->id(), &index).release());
    pushUndo(AdminOperation::Type::Delete, snapshot->id(), snapshot, index, description);
    markModified();   // deleting keeps the remaining records in sorted order
}

// Uses binary search when the records are sorted by ID, otherwise linear search.
PatientRecord* ClinicManager::findPatient(const std::string& patientId) const {
    std::string id = utils::toUpper(utils::trim(patientId));
    std::vector<PatientRecord*> list = db_.view();
    long comparisons = 0;
    int index = isSortedById() ? SearchEngine::binarySearchById(list, id, comparisons)
                               : SearchEngine::linearSearchById(list, id, comparisons);
    return index >= 0 ? list[static_cast<std::size_t>(index)] : nullptr;
}

PatientRecord* ClinicManager::requirePatient(const std::string& patientId) const {
    PatientRecord* patient = findPatient(patientId);
    if (!patient) throw utils::NotFoundException("No patient with ID '" + patientId + "' was found.");
    return patient;
}

// ======================= Runtime polymorphism =======================

// Works with ANY PatientRecord through a base-class reference. The virtual calls
// (category, priorityLabel, details, requiresImmediateAttention) are resolved at run time.
std::string ClinicManager::processPatient(const PatientRecord& patient) {
    std::string text = "[" + model::categoryToString(patient.category()) + " | " + patient.priorityLabel() + "] " +
                       patient.name() + " (" + patient.id() + ", age " + std::to_string(patient.age()) + ") - " +
                       patient.details();
    if (patient.requiresImmediateAttention()) {
        text += "  ->  goes to the PRIORITY queue; cannot be booked";
    } else if (dynamic_cast<const model::Appointable*>(&patient)) {
        text += "  ->  can be booked for appointments";
    }
    return text;
}

std::vector<std::string> ClinicManager::processAllPatients() const {
    std::vector<std::string> lines;
    for (const PatientRecord* patient : db_.view()) lines.push_back(processPatient(*patient));
    return lines;
}

// ======================= Searching and sorting =======================

SearchResult ClinicManager::search(SearchField field, const std::string& rawQuery, SearchMode mode) const {
    std::string query = utils::trim(rawQuery);
    if (query.empty()) throw SearchException("Please enter a search keyword.");
    if (query.size() > 60) throw SearchException("The search keyword is too long (maximum 60 characters).");

    SearchResult result;
    std::vector<PatientRecord*> list = db_.view();

    switch (field) {
    case SearchField::Id: {
        if (mode == SearchMode::Recursive) {
            throw SearchException("Recursive search is used for patient names. Choose Auto, Linear or Binary for IDs.");
        }
        if (mode == SearchMode::Binary && !isSortedById()) {
            throw SearchException("Binary search needs the records sorted by Patient ID (ascending). "
                                  "Sort them first, or choose Auto/Linear search.");
        }
        bool useBinary = mode == SearchMode::Binary || (mode == SearchMode::Auto && isSortedById());
        std::string id = utils::toUpper(query);
        int index = useBinary ? SearchEngine::binarySearchById(list, id, result.comparisons)
                              : SearchEngine::linearSearchById(list, id, result.comparisons);
        result.algorithm = useBinary ? "Binary Search" : "Linear Search";
        if (index >= 0) result.matches.push_back(list[static_cast<std::size_t>(index)]);
        break;
    }
    case SearchField::Name: {
        if (mode == SearchMode::Binary) {
            throw SearchException("Binary search is only used for exact Patient IDs. Names are matched partially, "
                                  "so use Auto, Linear or Recursive search.");
        }
        std::string keyword = utils::toLower(query);
        if (mode == SearchMode::Linear) {
            result.matches = SearchEngine::linearSearchByName(list, keyword, result.comparisons);
            result.algorithm = "Linear Search";
        } else {
            SearchEngine::recursiveSearchByName(list, keyword, 0, result.matches, result.comparisons);
            result.algorithm = "Recursive Search";
        }
        break;
    }
    case SearchField::Category: {
        if (mode == SearchMode::Binary || mode == SearchMode::Recursive) {
            throw SearchException("Category search uses Linear search. Choose Auto or Linear.");
        }
        model::PatientCategory category;
        try {
            category = model::categoryFromString(query);
        } catch (const utils::ValidationException&) {
            throw SearchException("Unknown category '" + query + "'. Type General, Emergency or Follow-Up.");
        }
        result.matches = SearchEngine::linearSearchByCategory(list, category, result.comparisons);
        result.algorithm = "Linear Search";
        break;
    }
    }
    return result;
}

SortStats ClinicManager::sortPatients(SortAlgorithm algorithm, SortKey key, bool ascending) {
    std::vector<PatientRecord*> list = db_.view();
    PatientLess less = makeComparator(key, ascending);
    auto start = std::chrono::steady_clock::now();
    long comparisons = sorting::runSort(algorithm, list, less);
    auto end = std::chrono::steady_clock::now();
    db_.reorder(list);
    sortedBy_ = std::make_pair(key, ascending);
    markModified();
    SortStats stats;
    stats.algorithm = algorithmName(algorithm);
    stats.comparisons = comparisons;
    stats.milliseconds = std::chrono::duration<double, std::milli>(end - start).count();
    stats.records = list.size();
    return stats;
}

bool ClinicManager::isSortedById() const {
    return sortedBy_ && sortedBy_->first == SortKey::Id && sortedBy_->second;
}

std::string ClinicManager::sortStateDescription() const {
    if (!sortedBy_) return "Unsorted (registration order) - ID searches use Linear Search";
    std::string text = "Sorted by " + sortKeyName(sortedBy_->first) + (sortedBy_->second ? " (ascending)" : " (descending)");
    return text + (isSortedById() ? " - ID searches use Binary Search" : " - ID searches use Linear Search");
}

// ======================= Waiting queue =======================

void ClinicManager::enqueue(const std::string& patientId, std::time_t joinedAt) {
    PatientRecord* patient = requirePatient(patientId);
    if (isQueued(patient->id())) throw ClinicException(patient->name() + " is already in the waiting queue.");
    QueueEntry entry{patient->id(), patient->name(), joinedAt, patient->requiresImmediateAttention()};
    if (entry.priority) priorityQueue_.push(entry);
    else regularQueue_.push(entry);
}

// Emergency patients are served first; within each queue it is first-in, first-out.
QueueEntry ClinicManager::callNext() {
    std::queue<QueueEntry>& source = !priorityQueue_.empty() ? priorityQueue_ : regularQueue_;
    if (source.empty()) throw ClinicException("The waiting queue is empty.");
    QueueEntry next = source.front();
    source.pop();
    return next;
}

std::vector<QueueEntry> ClinicManager::waitingList() const {
    std::vector<QueueEntry> list;
    std::queue<QueueEntry> copy = priorityQueue_;   // std::queue cannot be iterated, so copy and pop
    while (!copy.empty()) { list.push_back(copy.front()); copy.pop(); }
    copy = regularQueue_;
    while (!copy.empty()) { list.push_back(copy.front()); copy.pop(); }
    return list;
}

bool ClinicManager::isQueued(const std::string& patientId) const {
    for (const QueueEntry& entry : waitingList()) {
        if (entry.patientId == patientId) return true;
    }
    return false;
}

void ClinicManager::removeFromQueues(const std::string& patientId) {
    for (std::queue<QueueEntry>* queue : {&priorityQueue_, &regularQueue_}) {
        std::queue<QueueEntry> kept;
        while (!queue->empty()) {
            if (queue->front().patientId != patientId) kept.push(queue->front());
            queue->pop();
        }
        *queue = std::move(kept);
    }
}

// ======================= Undo stack =======================

void ClinicManager::pushUndo(AdminOperation::Type type, const std::string& patientId,
                             std::shared_ptr<const PatientRecord> snapshot, std::size_t index,
                             const std::string& description) {
    AdminOperation op;
    op.type = type;
    op.patientId = patientId;
    op.snapshot = std::move(snapshot);
    op.index = index;
    op.description = description;
    op.when = std::time(nullptr);
    undoStack_.push(op);
}

void ClinicManager::recordSnapshot(const PatientRecord& before, const std::string& description) {
    std::shared_ptr<const PatientRecord> snapshot(before.clone().release());
    pushUndo(AdminOperation::Type::Modify, before.id(), snapshot, 0, description);
    markModified();
}

std::string ClinicManager::undoLast() {
    if (undoStack_.empty()) throw ClinicException("There is no administrative operation to undo.");
    const AdminOperation& op = undoStack_.top();

    switch (op.type) {
    case AdminOperation::Type::Register: {       // undo a registration: remove the patient again
        PatientRecord* patient = findPatient(op.patientId);
        if (patient) {
            cacheRemove(patient);
            removeFromQueues(op.patientId);
            db_.removeById(op.patientId);
        }
        break;
    }
    case AdminOperation::Type::Delete: {         // undo a deletion: put the patient back where it was
        if (findPatient(op.patientId)) {
            throw ClinicException("Cannot undo: patient ID " + op.patientId + " is in use again.");
        }
        db_.insertAt(op.index, op.snapshot->clone());
        break;
    }
    case AdminOperation::Type::Modify: {         // undo an edit/appointment change: restore the old copy
        PatientRecord* current = requirePatient(op.patientId);
        std::unique_ptr<PatientRecord> restored = op.snapshot->clone();
        PatientRecord* raw = restored.get();
        std::unique_ptr<PatientRecord> old = db_.replace(std::move(restored));
        cacheReplace(current, raw);
        break;
    }
    }
    std::string description = op.description;
    undoStack_.pop();
    sortedBy_.reset();
    markModified();
    return description;
}

std::vector<std::string> ClinicManager::undoHistory() const {
    std::vector<std::string> history;
    std::stack<AdminOperation> copy = undoStack_;   // std::stack cannot be iterated, so copy and pop
    while (!copy.empty()) {
        char when[16];
        std::strftime(when, sizeof(when), "%H:%M:%S", std::localtime(&copy.top().when));
        history.push_back(std::string(when) + "  " + copy.top().description);
        copy.pop();
    }
    return history;
}

// ======================= Fixed-size array cache =======================

// Moves the patient to slot 0 (most recently used). The least recently used entry drops out when full.
void ClinicManager::touch(PatientRecord* patient) {
    if (!patient) return;
    int position = -1;
    for (int i = 0; i < recentCount_; ++i) {
        if (recentPatients_[i] == patient) { position = i; break; }
    }
    if (position < 0) {
        position = recentCount_ < kCacheSize ? recentCount_++ : kCacheSize - 1;
    }
    for (int i = position; i > 0; --i) recentPatients_[i] = recentPatients_[i - 1];
    recentPatients_[0] = patient;
}

std::vector<PatientRecord*> ClinicManager::recentPatients() const {
    return std::vector<PatientRecord*>(recentPatients_, recentPatients_ + kCacheSize);
}

void ClinicManager::cacheRemove(const PatientRecord* patient) {
    for (int i = 0; i < recentCount_; ++i) {
        if (recentPatients_[i] == patient) {
            for (int j = i; j + 1 < recentCount_; ++j) recentPatients_[j] = recentPatients_[j + 1];
            recentPatients_[--recentCount_] = nullptr;
            return;
        }
    }
}

void ClinicManager::cacheReplace(const PatientRecord* oldPatient, PatientRecord* newPatient) {
    for (int i = 0; i < recentCount_; ++i) {
        if (recentPatients_[i] == oldPatient) recentPatients_[i] = newPatient;
    }
}

// ======================= Persistence =======================

void ClinicManager::clearTransientState() {
    priorityQueue_ = {};
    regularQueue_ = {};
    undoStack_ = {};
    for (auto& slot : recentPatients_) slot = nullptr;
    recentCount_ = 0;
    sortedBy_.reset();
}

void ClinicManager::newDatabase() {
    db_.clear();
    clearTransientState();
    IDGenerator::reset();
    modified_ = false;
}

void ClinicManager::saveToFile(const std::string& path) {
    utils::FileHandler::save(path, db_);
    modified_ = false;
}

int ClinicManager::loadFromFile(const std::string& path) {
    utils::LoadResult loaded = utils::FileHandler::load(path);   // throws FileException; nothing changes on failure
    db_.clear();
    clearTransientState();
    IDGenerator::reset();
    for (auto& patient : loaded.patients) {
        IDGenerator::observePatientId(patient->id());
        for (const model::Appointment& a : patient->appointments()) IDGenerator::observeAppointmentId(a.id());
        db_.add(std::move(patient));
    }
    modified_ = false;
    return loaded.skippedLines;
}

void ClinicManager::exportPatients(const std::string& path) const {
    utils::FileHandler::exportPatientsCsv(path, db_);
}

} // namespace scapms::controller
