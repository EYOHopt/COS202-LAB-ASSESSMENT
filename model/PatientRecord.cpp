#include "model/PatientRecord.h"

#include <cctype>

#include "model/EmergencyPatient.h"
#include "model/FollowUpPatient.h"
#include "model/GeneralPatient.h"
#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

namespace scapms::model {

using utils::AppointmentException;
using utils::ValidationException;

std::string categoryToString(PatientCategory category) {
    switch (category) {
    case PatientCategory::General: return "General";
    case PatientCategory::Emergency: return "Emergency";
    case PatientCategory::FollowUp: return "Follow-Up";
    }
    return "General";
}

PatientCategory categoryFromString(const std::string& text) {
    std::string value = utils::toLower(utils::trim(text));
    if (value == "general") return PatientCategory::General;
    if (value == "emergency") return PatientCategory::Emergency;
    if (value == "follow-up" || value == "followup" || value == "follow up") return PatientCategory::FollowUp;
    throw ValidationException("Unknown patient category '" + text + "'. Use General, Emergency or Follow-Up.");
}

PatientRecord::PatientRecord(const std::string& id, const std::string& name, int age, const std::string& contact)
    : id_(validateId(id)), name_(validateName(name)), age_(validateAge(age)), contact_(validateContact(contact)) {}

void PatientRecord::setName(const std::string& name) { name_ = validateName(name); }
void PatientRecord::setAge(int age) { age_ = validateAge(age); }
void PatientRecord::setContact(const std::string& contact) { contact_ = validateContact(contact); }

std::string PatientRecord::validateId(const std::string& id) {
    std::string value = utils::toUpper(utils::trim(id));
    bool valid = value.size() >= 2 && value.size() <= 10 && std::isalpha(static_cast<unsigned char>(value[0]));
    for (char c : value) {
        if (!std::isalnum(static_cast<unsigned char>(c))) valid = false;
    }
    if (!valid) {
        throw ValidationException("Patient ID must be 2-10 letters or digits and start with a letter (e.g. P0001).");
    }
    return value;
}

std::string PatientRecord::validateName(const std::string& name) {
    std::string value = utils::trim(name);
    if (value.empty()) throw ValidationException("Patient name is required.");
    if (value.size() > 60) throw ValidationException("Patient name must be at most 60 characters.");
    for (char c : value) {
        bool allowed = std::isalpha(static_cast<unsigned char>(c)) || c == ' ' || c == '-' || c == '\'' || c == '.';
        if (!allowed) {
            throw ValidationException("Patient name may only contain letters, spaces, hyphens, apostrophes and dots.");
        }
    }
    return value;
}

int PatientRecord::validateAge(int age) {
    if (age < 0 || age > 120) throw utils::InvalidAgeException(age);
    return age;
}

std::string PatientRecord::validateContact(const std::string& contact) {
    std::string value = utils::trim(contact);
    if (value.empty()) throw ValidationException("Contact information is required (phone number or email).");

    int atCount = 0;
    std::size_t atPos = 0;
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '@') { ++atCount; atPos = i; }
    }
    if (atCount > 0) {   // email address
        bool hasDot = false;
        for (std::size_t i = atPos + 2; i + 1 < value.size(); ++i) {
            if (value[i] == '.') hasDot = true;
        }
        bool hasSpace = false;
        for (char c : value) {
            if (c == ' ') hasSpace = true;
        }
        if (atCount != 1 || atPos == 0 || !hasDot || hasSpace) {
            throw ValidationException("'" + value + "' is not a valid email address.");
        }
        return value;
    }

    int digits = 0;      // phone number
    for (char c : value) {
        if (std::isdigit(static_cast<unsigned char>(c))) ++digits;
        else if (c != ' ' && c != '+' && c != '-' && c != '(' && c != ')') digits = -100;
    }
    if (digits < 7 || digits > 15) {
        throw ValidationException("Contact must be a phone number (7-15 digits) or an email address.");
    }
    return value;
}

Appointment* PatientRecord::findAppointment(const std::string& appointmentId) {
    for (Appointment& a : appointments_) {
        if (a.id() == appointmentId) return &a;
    }
    return nullptr;
}

void PatientRecord::attachAppointment(const Appointment& appointment) {
    appointment.validate();
    appointments_.push_back(appointment);
}

bool PatientRecord::removeAppointment(const std::string& appointmentId) {
    for (std::size_t i = 0; i < appointments_.size(); ++i) {
        if (appointments_[i].id() == appointmentId) {
            appointments_.erase(appointments_.begin() + static_cast<long>(i));
            return true;
        }
    }
    return false;
}

int PatientRecord::countAppointments(AppointmentStatus status) const {
    int count = 0;
    for (const Appointment& a : appointments_) {
        if (a.status() == status) ++count;
    }
    return count;
}

std::string PatientRecord::nextPendingDateTime() const {
    std::string earliest;
    for (const Appointment& a : appointments_) {
        if (a.isPending() && (earliest.empty() || a.dateTime() < earliest)) earliest = a.dateTime();
    }
    return earliest;
}

void PatientRecord::addAppointmentChecked(const Appointment& appointment, int maxPending) {
    appointment.validate();
    if (appointment.patientId() != id_) throw AppointmentException("This appointment belongs to a different patient.");
    int pending = countAppointments(AppointmentStatus::Pending);
    if (pending >= maxPending) {
        throw AppointmentException(name_ + " already has " + std::to_string(pending) +
                                   " pending appointment(s). The limit for " + categoryToString(category()) +
                                   " patients is " + std::to_string(maxPending) + ".");
    }
    for (const Appointment& existing : appointments_) {
        if (existing.isPending() && existing.dateTime() == appointment.dateTime()) {
            throw AppointmentException(name_ + " already has an appointment at " + appointment.dateTime() + ".");
        }
    }
    appointments_.push_back(appointment);
}

void PatientRecord::cancelPendingAppointment(const std::string& appointmentId) {
    Appointment* appointment = findAppointment(appointmentId);
    if (!appointment) throw utils::NotFoundException("Appointment " + appointmentId + " was not found for " + name_ + ".");
    if (!appointment->isPending()) {
        throw AppointmentException("Only pending appointments can be cancelled (" + appointmentId + " is " +
                                   statusToString(appointment->status()) + ").");
    }
    appointment->setStatus(AppointmentStatus::Cancelled);
}

std::unique_ptr<PatientRecord> createPatient(PatientCategory category, const std::string& id,
                                             const std::string& name, int age, const std::string& contact) {
    switch (category) {
    case PatientCategory::Emergency: return std::make_unique<EmergencyPatient>(id, name, age, contact);
    case PatientCategory::FollowUp: return std::make_unique<FollowUpPatient>(id, name, age, contact);
    case PatientCategory::General: break;
    }
    return std::make_unique<GeneralPatient>(id, name, age, contact);
}

} // namespace scapms::model
