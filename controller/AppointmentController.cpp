#include "controller/AppointmentController.h"

#include <ctime>

#include "controller/Sorter.h"
#include "utils/Exceptions.h"
#include "utils/IDGenerator.h"
#include "utils/StringUtils.h"

namespace scapms::controller {

using model::Appointment;
using model::AppointmentStatus;
using model::PatientRecord;
using utils::AppointmentException;

AppointmentController::AppointmentController(ClinicManager& manager) : manager_(manager) {}

std::string AppointmentController::today() {
    std::time_t now = std::time(nullptr);
    char buffer[16];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", std::localtime(&now));
    return buffer;
}

std::string AppointmentController::currentTime() {
    std::time_t now = std::time(nullptr);
    char buffer[8];
    std::strftime(buffer, sizeof(buffer), "%H:%M", std::localtime(&now));
    return buffer;
}

// Uses dynamic_cast to check whether this patient type implements the Appointable interface.
model::Appointable* AppointmentController::requireBookable(PatientRecord* patient) const {
    auto* bookable = dynamic_cast<model::Appointable*>(patient);
    if (!bookable) {
        throw AppointmentException(patient->name() + " is an emergency patient. Emergency patients are seen "
                                   "immediately through the priority queue and cannot be booked.");
    }
    return bookable;
}

bool AppointmentController::doctorIsBusy(const std::string& doctor, const std::string& date,
                                         const std::string& time) const {
    std::string wanted = utils::toLower(utils::trim(doctor));
    for (const PatientRecord* patient : manager_.patients()) {
        for (const Appointment& a : patient->appointments()) {
            if (a.isPending() && a.date() == date && a.time() == time && utils::toLower(a.doctor()) == wanted) {
                return true;
            }
        }
    }
    return false;
}

std::string AppointmentController::schedule(const std::string& patientId, const std::string& date,
                                            const std::string& time, const std::string& doctor,
                                            const std::string& reason) {
    PatientRecord* patient = manager_.requirePatient(patientId);
    model::Appointable* bookable = requireBookable(patient);
    if (!Appointment::isValidDate(date)) throw AppointmentException("Invalid appointment date '" + date + "'.");
    if (!Appointment::isValidTime(time)) throw AppointmentException("Invalid appointment time '" + time + "'.");
    if (date < today()) throw AppointmentException("The appointment date cannot be in the past.");
    if (date == today() && time < currentTime()) {
        throw AppointmentException("That time has already passed today. Please choose a later time.");
    }
    if (utils::trim(doctor).empty()) throw AppointmentException("Please enter the doctor's name.");
    if (doctorIsBusy(doctor, date, time)) {
        throw AppointmentException(utils::trim(doctor) + " already has an appointment on " + date + " at " + time + ".");
    }

    Appointment appointment(utils::IDGenerator::nextAppointmentId(), patient->id(), date, time, doctor, reason);
    std::unique_ptr<PatientRecord> before = patient->clone();
    bookable->scheduleAppointment(appointment);   // polymorphic call: General and Follow-Up apply different rules
    manager_.recordSnapshot(*before, "Schedule appointment " + appointment.id() + " for " + patient->name());
    return appointment.id();
}

void AppointmentController::cancel(const std::string& patientId, const std::string& appointmentId) {
    PatientRecord* patient = manager_.requirePatient(patientId);
    model::Appointable* bookable = requireBookable(patient);
    std::unique_ptr<PatientRecord> before = patient->clone();
    bookable->cancelAppointment(appointmentId);
    manager_.recordSnapshot(*before, "Cancel appointment " + appointmentId + " (" + patient->name() + ")");
}

void AppointmentController::complete(const std::string& patientId, const std::string& appointmentId) {
    PatientRecord* patient = manager_.requirePatient(patientId);
    Appointment* appointment = patient->findAppointment(appointmentId);
    if (!appointment) throw utils::NotFoundException("Appointment " + appointmentId + " was not found.");
    if (!appointment->isPending()) throw AppointmentException("Only pending appointments can be marked as completed.");
    std::unique_ptr<PatientRecord> before = patient->clone();
    appointment->setStatus(AppointmentStatus::Completed);
    manager_.recordSnapshot(*before, "Complete appointment " + appointmentId + " (" + patient->name() + ")");
}

void AppointmentController::remove(const std::string& patientId, const std::string& appointmentId) {
    PatientRecord* patient = manager_.requirePatient(patientId);
    std::unique_ptr<PatientRecord> before = patient->clone();
    if (!patient->removeAppointment(appointmentId)) {
        throw utils::NotFoundException("Appointment " + appointmentId + " was not found.");
    }
    manager_.recordSnapshot(*before, "Remove appointment " + appointmentId + " (" + patient->name() + ")");
}

std::vector<AppointmentRow> AppointmentController::allAppointments() const {
    std::vector<AppointmentRow> rows;
    for (PatientRecord* patient : manager_.patients()) {
        for (const Appointment& a : patient->appointments()) rows.push_back({patient, a});
    }
    // Our own insertion sort (stable): appointments ordered by date and time.
    sorting::insertionSort(rows, [](const AppointmentRow& x, const AppointmentRow& y) {
        return x.appointment.dateTime() < y.appointment.dateTime();
    });
    return rows;
}

std::vector<AppointmentRow> AppointmentController::appointmentsOn(const std::string& date) const {
    std::vector<AppointmentRow> rows;
    for (const AppointmentRow& row : allAppointments()) {
        if (row.appointment.date() == date) rows.push_back(row);
    }
    return rows;
}

std::vector<PatientRecord*> AppointmentController::bookablePatients() const {
    std::vector<PatientRecord*> list;
    for (PatientRecord* patient : manager_.patients()) {
        if (dynamic_cast<model::Appointable*>(patient)) list.push_back(patient);
    }
    return list;
}

} // namespace scapms::controller
