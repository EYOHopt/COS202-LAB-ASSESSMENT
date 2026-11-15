#pragma once
// Generates sequential IDs such as P0001 (patients) and A0001 (appointments).
#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>

namespace scapms::utils {

class IDGenerator {
public:
    static std::string nextPatientId() { return format("P", ++patientCounter()); }
    static std::string nextAppointmentId() { return format("A", ++appointmentCounter()); }

    // Looks ahead without consuming an ID (used to suggest an ID in the GUI).
    static std::string peekPatientId(int offset = 1) { return format("P", patientCounter() + offset); }

    // Keeps the counters ahead of IDs loaded from file or typed by the user.
    static void observePatientId(const std::string& id) { observe(id, patientCounter()); }
    static void observeAppointmentId(const std::string& id) { observe(id, appointmentCounter()); }

    static void reset() {
        patientCounter() = 0;
        appointmentCounter() = 0;
    }

private:
    static int& patientCounter() { static int counter = 0; return counter; }
    static int& appointmentCounter() { static int counter = 0; return counter; }

    static std::string format(const std::string& prefix, int number) {
        std::ostringstream out;
        out << prefix << std::setw(4) << std::setfill('0') << number;
        return out.str();
    }

    static void observe(const std::string& id, int& counter) {
        std::size_t i = 0;
        while (i < id.size() && std::isalpha(static_cast<unsigned char>(id[i]))) ++i;
        if (i == id.size()) return;
        int value = 0;
        for (; i < id.size(); ++i) {
            if (!std::isdigit(static_cast<unsigned char>(id[i]))) return;
            value = value * 10 + (id[i] - '0');
            if (value > 1000000) return;
        }
        if (value > counter) counter = value;
    }
};

} // namespace scapms::utils
