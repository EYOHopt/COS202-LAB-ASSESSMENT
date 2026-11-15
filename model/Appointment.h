#pragma once
// An appointment between a patient and a doctor.
#include <string>

namespace scapms::model {

enum class AppointmentStatus { Pending, Completed, Cancelled };

std::string statusToString(AppointmentStatus status);
AppointmentStatus statusFromString(const std::string& text);

class Appointment {
public:
    Appointment() = default;
    Appointment(std::string id, std::string patientId, std::string date, std::string time,
                std::string doctor, std::string reason,
                AppointmentStatus status = AppointmentStatus::Pending);

    const std::string& id() const { return id_; }
    const std::string& patientId() const { return patientId_; }
    const std::string& date() const { return date_; }   // YYYY-MM-DD
    const std::string& time() const { return time_; }   // HH:MM (24-hour)
    const std::string& doctor() const { return doctor_; }
    const std::string& reason() const { return reason_; }
    AppointmentStatus status() const { return status_; }

    void setStatus(AppointmentStatus status) { status_ = status; }
    void setReason(const std::string& reason) { reason_ = reason; }

    std::string dateTime() const { return date_ + " " + time_; }
    bool isPending() const { return status_ == AppointmentStatus::Pending; }

    // Throws AppointmentException if any field is invalid.
    void validate() const;

    static bool isValidDate(const std::string& date);
    static bool isValidTime(const std::string& time);

private:
    std::string id_;
    std::string patientId_;
    std::string date_;
    std::string time_;
    std::string doctor_;
    std::string reason_;
    AppointmentStatus status_ = AppointmentStatus::Pending;
};

} // namespace scapms::model
