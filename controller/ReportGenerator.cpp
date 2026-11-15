#include "controller/ReportGenerator.h"

#include "controller/Sorter.h"
#include "utils/StringUtils.h"

namespace scapms::controller {

using model::AppointmentStatus;

ReportGenerator::ReportGenerator(const ClinicManager& manager, const AppointmentController& appointments)
    : manager_(manager), appointments_(appointments) {}

std::vector<AttendanceRow> ReportGenerator::mostFrequentlyAttended(std::size_t limit) const {
    std::vector<AttendanceRow> rows;
    for (model::PatientRecord* patient : manager_.patients()) {
        int completed = patient->countAppointments(AppointmentStatus::Completed);
        if (completed > 0) rows.push_back({patient, completed, static_cast<int>(patient->appointments().size())});
    }
    // Our own merge sort: most completed first, ties broken alphabetically.
    sorting::mergeSort(rows, [](const AttendanceRow& a, const AttendanceRow& b) {
        if (a.completed != b.completed) return a.completed > b.completed;
        return utils::toLower(a.patient->name()) < utils::toLower(b.patient->name());
    });
    if (rows.size() > limit) rows.resize(limit);
    return rows;
}

std::vector<AppointmentRow> ReportGenerator::pendingAppointments() const {
    std::vector<AppointmentRow> rows;
    for (const AppointmentRow& row : appointments_.allAppointments()) {
        if (row.appointment.isPending()) rows.push_back(row);
    }
    return rows;
}

std::vector<CategoryCount> ReportGenerator::categoryDistribution() const {
    std::vector<CategoryCount> counts = {{"General", 0, 0.0}, {"Emergency", 0, 0.0}, {"Follow-Up", 0, 0.0}};
    for (const model::PatientRecord* patient : manager_.patients()) {
        counts[static_cast<std::size_t>(patient->category())].count++;   // enum order matches the list
    }
    std::size_t total = manager_.patientCount();
    for (CategoryCount& c : counts) c.percent = total == 0 ? 0.0 : 100.0 * c.count / static_cast<double>(total);
    return counts;
}

DailySummary ReportGenerator::dailySummary(const std::string& date) const {
    DailySummary summary;
    summary.date = date;
    summary.rows = appointments_.appointmentsOn(date);
    for (const AppointmentRow& row : summary.rows) {
        summary.total++;
        switch (row.appointment.status()) {
        case AppointmentStatus::Completed: summary.completed++; break;
        case AppointmentStatus::Pending: summary.pending++; break;
        case AppointmentStatus::Cancelled: summary.cancelled++; break;
        }
    }
    return summary;
}

} // namespace scapms::controller
