#pragma once
// A walk-in or booked patient with a general complaint. Can be scheduled (Appointable).
#include "model/Appointable.h"
#include "model/PatientRecord.h"

namespace scapms::model {

class GeneralPatient : public PatientRecord, public Appointable {
public:
    GeneralPatient(const std::string& id, const std::string& name, int age, const std::string& contact,
                   const std::string& complaint = "");

    PatientCategory category() const override { return PatientCategory::General; }
    std::string priorityLabel() const override { return "Normal"; }
    std::string details() const override;
    std::vector<std::string> extraFields() const override { return {complaint_}; }
    void setExtraFields(const std::vector<std::string>& fields) override;
    std::unique_ptr<PatientRecord> clone() const override { return std::make_unique<GeneralPatient>(*this); }

    // Appointable
    void scheduleAppointment(const Appointment& appointment) override;
    void cancelAppointment(const std::string& appointmentId) override;
    int maxPendingAppointments() const override { return 3; }

    const std::string& complaint() const { return complaint_; }
    void setComplaint(const std::string& complaint);

private:
    std::string complaint_;
};

} // namespace scapms::model
