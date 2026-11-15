#pragma once
// Central business-logic class. Owns the patient database, the waiting queue,
// the undo stack and the "frequently accessed patients" cache.
#include <ctime>
#include <memory>
#include <optional>
#include <queue>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "controller/SearchEngine.h"
#include "controller/Sorter.h"
#include "model/ClinicDatabase.h"

namespace scapms::controller {

enum class SearchField { Id, Name, Category };
enum class SearchMode { Auto, Linear, Binary, Recursive };

struct QueueEntry {
    std::string patientId;
    std::string patientName;
    std::time_t joinedAt = 0;
    bool priority = false;   // emergency patients go to the priority queue
};

// One reversible administrative operation stored on the undo stack.
struct AdminOperation {
    enum class Type { Register, Delete, Modify };
    Type type = Type::Modify;
    std::string patientId;
    std::shared_ptr<const model::PatientRecord> snapshot;   // patient state BEFORE the operation
    std::size_t index = 0;                                  // original position (for Delete)
    std::string description;
    std::time_t when = 0;
};

struct SortStats {
    std::string algorithm;
    long comparisons = 0;
    double milliseconds = 0.0;
    std::size_t records = 0;
};

class ClinicManager {
public:
    static constexpr int kCacheSize = 10;

    ClinicManager();

    // ----- Patient records -----
    std::string suggestPatientId() const;
    model::PatientRecord* registerPatient(std::unique_ptr<model::PatientRecord> patient);
    model::PatientRecord* updatePatient(std::unique_ptr<model::PatientRecord> updated);
    void deletePatient(const std::string& patientId);
    model::PatientRecord* findPatient(const std::string& patientId) const;      // nullptr if not found
    model::PatientRecord* requirePatient(const std::string& patientId) const;   // throws NotFoundException
    std::vector<model::PatientRecord*> patients() const { return db_.view(); }
    std::size_t patientCount() const { return db_.size(); }
    const model::ClinicDatabase& database() const { return db_; }

    // ----- Runtime polymorphism -----
    static std::string processPatient(const model::PatientRecord& patient);
    std::vector<std::string> processAllPatients() const;

    // ----- Searching and sorting -----
    SearchResult search(SearchField field, const std::string& query, SearchMode mode) const;
    SortStats sortPatients(SortAlgorithm algorithm, SortKey key, bool ascending);
    bool isSortedById() const;
    std::string sortStateDescription() const;

    // ----- Waiting queue (std::queue) -----
    void enqueue(const std::string& patientId, std::time_t joinedAt = std::time(nullptr));
    QueueEntry callNext();
    std::vector<QueueEntry> waitingList() const;
    bool isQueued(const std::string& patientId) const;
    std::size_t queueSize() const { return priorityQueue_.size() + regularQueue_.size(); }

    // ----- Undo last administrative operation (std::stack) -----
    void recordSnapshot(const model::PatientRecord& before, const std::string& description);
    std::string undoLast();
    bool canUndo() const { return !undoStack_.empty(); }
    std::vector<std::string> undoHistory() const;   // most recent first

    // ----- Frequently accessed patients cache (fixed-size array) -----
    void touch(model::PatientRecord* patient);
    std::vector<model::PatientRecord*> recentPatients() const;   // kCacheSize slots, may contain nullptr

    // ----- Persistence -----
    void newDatabase();
    void saveToFile(const std::string& path);
    int loadFromFile(const std::string& path);   // returns number of skipped lines
    void exportPatients(const std::string& path) const;
    bool isModified() const { return modified_; }
    void markModified() { modified_ = true; }

private:
    void pushUndo(AdminOperation::Type type, const std::string& patientId,
                  std::shared_ptr<const model::PatientRecord> snapshot, std::size_t index,
                  const std::string& description);
    void removeFromQueues(const std::string& patientId);
    void cacheRemove(const model::PatientRecord* patient);
    void cacheReplace(const model::PatientRecord* oldPatient, model::PatientRecord* newPatient);
    void clearTransientState();

    model::ClinicDatabase db_;                           // std::vector of registered patients
    std::queue<QueueEntry> priorityQueue_;               // emergency patients, first come first served
    std::queue<QueueEntry> regularQueue_;                // general / follow-up patients, first come first served
    std::stack<AdminOperation> undoStack_;               // most recent admin operation on top
    model::PatientRecord* recentPatients_[kCacheSize];   // fixed-size array cache (most recent first)
    int recentCount_ = 0;
    std::optional<std::pair<SortKey, bool>> sortedBy_;   // current order of the records, if sorted
    bool modified_ = false;
};

} // namespace scapms::controller
