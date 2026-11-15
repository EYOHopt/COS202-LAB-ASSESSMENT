#pragma once
// Abstract base class for every type of patient.
#include <memory>
#include <string>
#include <vector>

#include "model/Appointment.h"

namespace scapms::model {

enum class PatientCategory { General, Emergency, FollowUp };

std::string categoryToString(PatientCategory category);
PatientCategory categoryFromString(const std::string& text);   // throws ValidationException

class PatientRecord {
public:
    PatientRecord(const std::string& id, const std::string& name, int age, const std::string& contact);
    virtual ~PatientRecord() = default;

    // ---- Pure virtual functions: every subclass must implement them ----
    virtual PatientCategory category() const = 0;
    virtual std::string priorityLabel() const = 0;
    virtual std::string details() const = 0;
    virtual std::vector<std::string> extraFields() const = 0;            // type-specific data (for saving)
    virtual void setExtraFields(const std::vector<std::string>& fields) = 0;
    virtual std::unique_ptr<PatientRecord> clone() const = 0;            // deep copy (used by Undo)

    // ---- Virtual function with a default behaviour ----
    virtual bool requiresImmediateAttention() const { return false; }

    const std::string& id() const { return id_; }
    const std::string& name() const { return name_; }
    int age() const { return age_; }
    const std::string& contact() const { return contact_; }

    void setName(const std::string& name);
    void setAge(int age);
    void setContact(const std::string& contact);

    // ---- Composition: a patient owns one or more appointments ----
    const std::vector<Appointment>& appointments() const { return appointments_; }
    Appointment* findAppointment(const std::string& appointmentId);
    void attachAppointment(const Appointment& appointment);   // used when loading from file
    bool removeAppointment(const std::string& appointmentId);
    int countAppointments(AppointmentStatus status) const;
    std::string nextPendingDateTime() const;                  // "" if none

    // Validation helpers: return the cleaned value or throw ValidationException.
    static std::string validateId(const std::string& id);
    static std::string validateName(const std::string& name);
    static int validateAge(int age);
    static std::string validateContact(const std::string& contact);

protected:
    PatientRecord(const PatientRecord&) = default;
    PatientRecord& operator=(const PatientRecord&) = default;

    // Shared helpers for subclasses that implement Appointable.
    void addAppointmentChecked(const Appointment& appointment, int maxPending);
    void cancelPendingAppointment(const std::string& appointmentId);

    std::vector<Appointment> appointments_;

private:
    std::string id_;
    std::string name_;
    int age_ = 0;
    std::string contact_;
};

// Factory: creates the right subclass for a category.
std::unique_ptr<PatientRecord> createPatient(PatientCategory category, const std::string& id,
                                             const std::string& name, int age, const std::string& contact);

} // namespace scapms::model
