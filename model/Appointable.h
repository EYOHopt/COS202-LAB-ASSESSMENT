#pragma once
// Interface-like abstract class (only pure virtual functions, no data).
// Implemented by the patient types that can be booked for appointments.
#include <string>

#include "model/Appointment.h"

namespace scapms::model {

class Appointable {
public:
    virtual ~Appointable() = default;
    virtual void scheduleAppointment(const Appointment& appointment) = 0;
    virtual void cancelAppointment(const std::string& appointmentId) = 0;
    virtual int maxPendingAppointments() const = 0;
};

} // namespace scapms::model
