#pragma once
// Encapsulates the collection of registered patients (stored in a std::vector).
// The vector owns the patients through std::unique_ptr; other classes only get raw pointers.
#include <memory>
#include <string>
#include <vector>

#include "model/PatientRecord.h"

namespace scapms::model {

class ClinicDatabase {
public:
    void add(std::unique_ptr<PatientRecord> patient);                     // throws DuplicateIdException
    void insertAt(std::size_t index, std::unique_ptr<PatientRecord> patient);
    std::unique_ptr<PatientRecord> removeById(const std::string& id, std::size_t* oldIndex = nullptr);
    std::unique_ptr<PatientRecord> replace(std::unique_ptr<PatientRecord> patient);   // returns the old record
    void reorder(const std::vector<PatientRecord*>& newOrder);           // apply a sorted order
    void clear() { patients_.clear(); }

    PatientRecord* at(std::size_t index) const { return patients_.at(index).get(); }
    std::size_t size() const { return patients_.size(); }
    bool empty() const { return patients_.empty(); }
    std::vector<PatientRecord*> view() const;                             // non-owning list

private:
    int indexOf(const std::string& id) const;

    std::vector<std::unique_ptr<PatientRecord>> patients_;
};

} // namespace scapms::model
