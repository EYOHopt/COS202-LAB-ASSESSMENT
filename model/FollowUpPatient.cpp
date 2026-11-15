#include "model/FollowUpPatient.h"

#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

namespace scapms::model {

using utils::ValidationException;

FollowUpPatient::FollowUpPatient(const std::string& id, const std::string& name, int age,
                                 const std::string& contact, const std::string& previousDiagnosis, int intervalDays)
    : PatientRecord(id, name, age, contact) {
    setPreviousDiagnosis(previousDiagnosis);
    setIntervalDays(intervalDays);
}

void FollowUpPatient::setPreviousDiagnosis(const std::string& diagnosis) {
    std::string value = utils::trim(diagnosis);
    if (value.empty()) throw ValidationException("Please enter the previous diagnosis for the follow-up.");
    if (value.size() > 100) throw ValidationException("The diagnosis must be at most 100 characters.");
    previousDiagnosis_ = value;
}

void FollowUpPatient::setIntervalDays(int days) {
    if (days < 1 || days > 365) throw ValidationException("Follow-up interval must be between 1 and 365 days.");
    intervalDays_ = days;
}

std::string FollowUpPatient::details() const {
    return "Review of " + previousDiagnosis_ + " every " + std::to_string(intervalDays_) + " days";
}

std::vector<std::string> FollowUpPatient::extraFields() const {
    return {previousDiagnosis_, std::to_string(intervalDays_)};
}

void FollowUpPatient::setExtraFields(const std::vector<std::string>& fields) {
    setPreviousDiagnosis(fields.empty() ? "" : fields[0]);
    int days = 0;
    if (fields.size() < 2 || !utils::parseInt(fields[1], days)) {
        throw ValidationException("Follow-up interval must be a whole number of days.");
    }
    setIntervalDays(days);
}

void FollowUpPatient::scheduleAppointment(const Appointment& appointment) {
    Appointment booking = appointment;
    if (booking.reason().empty()) booking.setReason("Follow-up review: " + previousDiagnosis_);
    addAppointmentChecked(booking, maxPendingAppointments());
}

void FollowUpPatient::cancelAppointment(const std::string& appointmentId) {
    cancelPendingAppointment(appointmentId);
}

} // namespace scapms::model
