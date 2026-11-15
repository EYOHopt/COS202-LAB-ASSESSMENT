#pragma once
// Business rules for scheduling, cancelling, completing and removing appointments.
#include <string>
#include <vector>

#include "controller/ClinicManager.h"
#include "model/Appointable.h"

namespace scapms::controller {

struct AppointmentRow {
    model::PatientRecord* patient = nullptr;
    model::Appointment appointment;
};

class AppointmentController {
public:
    explicit AppointmentController(ClinicManager& manager);

    std::string schedule(const std::string& patientId, const std::string& date, const std::string& time,
                         const std::string& doctor, const std::string& reason);   // returns the new appointment ID
    void cancel(const std::string& patientId, const std::string& appointmentId);
    void complete(const std::string& patientId, const std::string& appointmentId);
    void remove(const std::string& patientId, const std::string& appointmentId);   // admin operation

    std::vector<AppointmentRow> allAppointments() const;                     // sorted by date and time
    std::vector<AppointmentRow> appointmentsOn(const std::string& date) const;
    std::vector<model::PatientRecord*> bookablePatients() const;            // patients that implement Appointable

    static std::string today();         // YYYY-MM-DD
    static std::string currentTime();   // HH:MM

private:
    model::Appointable* requireBookable(model::PatientRecord* patient) const;
    bool doctorIsBusy(const std::string& doctor, const std::string& date, const std::string& time) const;

    ClinicManager& manager_;
};

} // namespace scapms::controller
