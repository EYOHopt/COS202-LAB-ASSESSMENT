#include "model/GeneralPatient.h"

#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

namespace scapms::model {

GeneralPatient::GeneralPatient(const std::string& id, const std::string& name, int age,
                               const std::string& contact, const std::string& complaint)
    : PatientRecord(id, name, age, contact) {
    setComplaint(complaint);
}

void GeneralPatient::setComplaint(const std::string& complaint) {
    std::string value = utils::trim(complaint);
    if (value.size() > 100) throw utils::ValidationException("The complaint must be at most 100 characters.");
    complaint_ = value;
}

std::string GeneralPatient::details() const {
    return complaint_.empty() ? "General consultation" : "Complaint: " + complaint_;
}

void GeneralPatient::setExtraFields(const std::vector<std::string>& fields) {
    setComplaint(fields.empty() ? "" : fields[0]);
}

// General patients may hold up to 3 pending appointments.
void GeneralPatient::scheduleAppointment(const Appointment& appointment) {
    addAppointmentChecked(appointment, maxPendingAppointments());
}

void GeneralPatient::cancelAppointment(const std::string& appointmentId) {
    cancelPendingAppointment(appointmentId);
}

} // namespace scapms::model
