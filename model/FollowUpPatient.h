#pragma once
// A patient returning for review of an earlier diagnosis. Can be scheduled (Appointable).
#include "model/Appointable.h"
#include "model/PatientRecord.h"

namespace scapms::model {

class FollowUpPatient : public PatientRecord, public Appointable {
public:
    FollowUpPatient(const std::string& id, const std::string& name, int age, const std::string& contact,
                    const std::string& previousDiagnosis = "Not specified", int intervalDays = 14);

    PatientCategory category() const override { return PatientCategory::FollowUp; }
    std::string priorityLabel() const override { return "Scheduled"; }
    std::string details() const override;
    std::vector<std::string> extraFields() const override;
    void setExtraFields(const std::vector<std::string>& fields) override;
    std::unique_ptr<PatientRecord> clone() const override { return std::make_unique<FollowUpPatient>(*this); }

    // Appointable: only one pending follow-up at a time; a blank reason is filled in automatically.
    void scheduleAppointment(const Appointment& appointment) override;
    void cancelAppointment(const std::string& appointmentId) override;
    int maxPendingAppointments() const override { return 1; }

    const std::string& previousDiagnosis() const { return previousDiagnosis_; }
    int intervalDays() const { return intervalDays_; }
    void setPreviousDiagnosis(const std::string& diagnosis);
    void setIntervalDays(int days);

private:
    std::string previousDiagnosis_;
    int intervalDays_ = 14;
};

} // namespace scapms::model
