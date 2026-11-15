#pragma once
// Builds the clinic reports from the current data.
#include <string>
#include <vector>

#include "controller/AppointmentController.h"
#include "controller/ClinicManager.h"

namespace scapms::controller {

struct AttendanceRow {
    model::PatientRecord* patient = nullptr;
    int completed = 0;
    int total = 0;
};

struct CategoryCount {
    std::string category;
    int count = 0;
    double percent = 0.0;
};

struct DailySummary {
    std::string date;
    int total = 0;
    int completed = 0;
    int pending = 0;
    int cancelled = 0;
    std::vector<AppointmentRow> rows;
};

class ReportGenerator {
public:
    ReportGenerator(const ClinicManager& manager, const AppointmentController& appointments);

    std::vector<AttendanceRow> mostFrequentlyAttended(std::size_t limit = 10) const;
    std::vector<AppointmentRow> pendingAppointments() const;
    std::vector<CategoryCount> categoryDistribution() const;
    DailySummary dailySummary(const std::string& date) const;

private:
    const ClinicManager& manager_;
    const AppointmentController& appointments_;
};

} // namespace scapms::controller
