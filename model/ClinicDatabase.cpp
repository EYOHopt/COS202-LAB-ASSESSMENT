#include "model/ClinicDatabase.h"

#include "utils/Exceptions.h"

namespace scapms::model {

int ClinicDatabase::indexOf(const std::string& id) const {
    for (std::size_t i = 0; i < patients_.size(); ++i) {
        if (patients_[i]->id() == id) return static_cast<int>(i);
    }
    return -1;
}

void ClinicDatabase::add(std::unique_ptr<PatientRecord> patient) {
    if (!patient) return;
    if (indexOf(patient->id()) >= 0) throw utils::DuplicateIdException(patient->id());
    patients_.push_back(std::move(patient));
}

void ClinicDatabase::insertAt(std::size_t index, std::unique_ptr<PatientRecord> patient) {
    if (!patient) return;
    if (indexOf(patient->id()) >= 0) throw utils::DuplicateIdException(patient->id());
    if (index > patients_.size()) index = patients_.size();
    patients_.insert(patients_.begin() + static_cast<long>(index), std::move(patient));
}

std::unique_ptr<PatientRecord> ClinicDatabase::removeById(const std::string& id, std::size_t* oldIndex) {
    int index = indexOf(id);
    if (index < 0) return nullptr;
    std::unique_ptr<PatientRecord> removed = std::move(patients_[static_cast<std::size_t>(index)]);
    patients_.erase(patients_.begin() + index);
    if (oldIndex) *oldIndex = static_cast<std::size_t>(index);
    return removed;
}

std::unique_ptr<PatientRecord> ClinicDatabase::replace(std::unique_ptr<PatientRecord> patient) {
    int index = indexOf(patient->id());
    if (index < 0) throw utils::NotFoundException("Patient " + patient->id() + " was not found.");
    std::swap(patients_[static_cast<std::size_t>(index)], patient);
    return patient;   // now holds the previous record
}

void ClinicDatabase::reorder(const std::vector<PatientRecord*>& newOrder) {
    if (newOrder.size() != patients_.size()) throw utils::ClinicException("Internal error: invalid sort order.");
    std::vector<std::unique_ptr<PatientRecord>> reordered;
    reordered.reserve(patients_.size());
    for (PatientRecord* wanted : newOrder) {
        for (auto& owned : patients_) {
            if (owned && owned.get() == wanted) {
                reordered.push_back(std::move(owned));
                break;
            }
        }
    }
    if (reordered.size() != newOrder.size()) throw utils::ClinicException("Internal error: invalid sort order.");
    patients_ = std::move(reordered);
}

std::vector<PatientRecord*> ClinicDatabase::view() const {
    std::vector<PatientRecord*> list;
    list.reserve(patients_.size());
    for (const auto& p : patients_) list.push_back(p.get());
    return list;
}

} // namespace scapms::model
