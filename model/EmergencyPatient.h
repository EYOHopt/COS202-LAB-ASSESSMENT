#pragma once
// An emergency patient. Seen immediately through the priority queue,
// so this class does NOT implement Appointable (cannot be booked).
#include "model/PatientRecord.h"

namespace scapms::model {

class EmergencyPatient : public PatientRecord {
public:
    EmergencyPatient(const std::string& id, const std::string& name, int age, const std::string& contact,
                     int severity = 3, const std::string& condition = "Not specified");

    PatientCategory category() const override { return PatientCategory::Emergency; }
    std::string priorityLabel() const override { return "High Priority"; }
    std::string details() const override;
    std::vector<std::string> extraFields() const override;
    void setExtraFields(const std::vector<std::string>& fields) override;
    std::unique_ptr<PatientRecord> clone() const override { return std::make_unique<EmergencyPatient>(*this); }
    bool requiresImmediateAttention() const override { return true; }

    int severity() const { return severity_; }            // 1 = critical ... 5 = minimal
    const std::string& condition() const { return condition_; }
    void setSeverity(int severity);
    void setCondition(const std::string& condition);
    std::string severityText() const;

private:
    int severity_ = 3;
    std::string condition_;
};

} // namespace scapms::model
