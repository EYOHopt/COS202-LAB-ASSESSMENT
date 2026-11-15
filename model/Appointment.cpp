#include "model/Appointment.h"

#include <cctype>

#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

namespace scapms::model {

using utils::AppointmentException;

std::string statusToString(AppointmentStatus status) {
    switch (status) {
    case AppointmentStatus::Pending: return "Pending";
    case AppointmentStatus::Completed: return "Completed";
    case AppointmentStatus::Cancelled: return "Cancelled";
    }
    return "Pending";
}

AppointmentStatus statusFromString(const std::string& text) {
    if (text == "Pending") return AppointmentStatus::Pending;
    if (text == "Completed") return AppointmentStatus::Completed;
    if (text == "Cancelled") return AppointmentStatus::Cancelled;
    throw AppointmentException("Unknown appointment status '" + text + "'.");
}

Appointment::Appointment(std::string id, std::string patientId, std::string date, std::string time,
                         std::string doctor, std::string reason, AppointmentStatus status)
    : id_(std::move(id)), patientId_(std::move(patientId)), date_(std::move(date)), time_(std::move(time)),
      doctor_(utils::trim(doctor)), reason_(utils::trim(reason)), status_(status) {}

namespace {
bool digitsAt(const std::string& text, std::size_t from, std::size_t count) {
    for (std::size_t i = from; i < from + count; ++i) {
        if (!std::isdigit(static_cast<unsigned char>(text[i]))) return false;
    }
    return true;
}
int numberAt(const std::string& text, std::size_t from, std::size_t count) {
    int value = 0;
    for (std::size_t i = from; i < from + count; ++i) value = value * 10 + (text[i] - '0');
    return value;
}
} // namespace

bool Appointment::isValidDate(const std::string& date) {
    if (date.size() != 10 || date[4] != '-' || date[7] != '-') return false;
    if (!digitsAt(date, 0, 4) || !digitsAt(date, 5, 2) || !digitsAt(date, 8, 2)) return false;
    int year = numberAt(date, 0, 4), month = numberAt(date, 5, 2), day = numberAt(date, 8, 2);
    if (year < 2000 || year > 2100 || month < 1 || month > 12 || day < 1) return false;
    static const int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int maxDay = daysInMonth[month - 1];
    bool leapYear = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
    if (month == 2 && leapYear) maxDay = 29;
    return day <= maxDay;
}

bool Appointment::isValidTime(const std::string& time) {
    if (time.size() != 5 || time[2] != ':' || !digitsAt(time, 0, 2) || !digitsAt(time, 3, 2)) return false;
    return numberAt(time, 0, 2) < 24 && numberAt(time, 3, 2) < 60;
}

void Appointment::validate() const {
    if (id_.empty()) throw AppointmentException("Appointment ID is missing.");
    if (patientId_.empty()) throw AppointmentException("An appointment must belong to a patient.");
    if (!isValidDate(date_)) throw AppointmentException("Invalid appointment date '" + date_ + "'. Use YYYY-MM-DD.");
    if (!isValidTime(time_)) throw AppointmentException("Invalid appointment time '" + time_ + "'. Use HH:MM (24-hour).");
    if (doctor_.empty()) throw AppointmentException("Please enter the doctor's name.");
    if (reason_.size() > 100) throw AppointmentException("The reason must be at most 100 characters.");
}

} // namespace scapms::model
