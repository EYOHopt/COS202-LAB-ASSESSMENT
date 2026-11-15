#include "gui/ReportPanel.h"

#include <QComboBox>
#include <QDateEdit>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextBrowser>
#include <QTextStream>
#include <QVBoxLayout>

#include "controller/ReportGenerator.h"
#include "gui/GuiUtils.h"

namespace scapms::gui {

namespace {
const char* kTableStyle = "<style>table{border-collapse:collapse;} th{background:#DCE6F2;text-align:left;} "
                          "td,th{padding:4px 10px;border:1px solid #C5CFDB;}</style>";
QString cell(const QString& text) { return "<td>" + text.toHtmlEscaped() + "</td>"; }
} // namespace

ReportPanel::ReportPanel(controller::ClinicManager& manager, controller::AppointmentController& appointments,
                         QWidget* parent)
    : BasePanel(manager, appointments, parent) {
    setObjectName("reportPanel");

    reportCombo_ = new QComboBox;
    reportCombo_->setObjectName("reportCombo");
    reportCombo_->addItems({"Most Frequently Attended Patients", "Patients with Pending Appointments",
                            "Patient Category Distribution", "Daily Appointment Summary"});
    dateEdit_ = new QDateEdit(QDate::currentDate());
    dateEdit_->setObjectName("reportDate");
    dateEdit_->setCalendarPopup(true);
    dateEdit_->setDisplayFormat("yyyy-MM-dd");
    dateEdit_->setToolTip("Date used by the Daily Appointment Summary");
    auto* generateButton = new QPushButton("Generate Report");
    generateButton->setObjectName("generateReportButton");
    auto* saveButton = new QPushButton("Save Report...");
    saveButton->setToolTip("Save the report as an HTML file");

    auto* top = new QHBoxLayout;
    top->addWidget(new QLabel("Report:"));
    top->addWidget(reportCombo_, 1);
    top->addWidget(new QLabel("Date:"));
    top->addWidget(dateEdit_);
    top->addWidget(generateButton);
    top->addWidget(saveButton);

    output_ = new QTextBrowser;
    output_->setObjectName("reportOutput");

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(top);
    layout->addWidget(output_, 1);

    connect(generateButton, &QPushButton::clicked, this, &ReportPanel::generateReport);
    connect(saveButton, &QPushButton::clicked, this, &ReportPanel::saveReport);
    connect(reportCombo_, &QComboBox::currentIndexChanged, this, &ReportPanel::generateReport);
    connect(reportCombo_, &QComboBox::currentIndexChanged, this,
            [this](int index) { dateEdit_->setEnabled(index == 3); });
    connect(dateEdit_, &QDateEdit::dateChanged, this, &ReportPanel::generateReport);
    dateEdit_->setEnabled(false);
    generateReport();
}

void ReportPanel::refresh() { generateReport(); }

void ReportPanel::generateReport() {
    QString html;
    switch (reportCombo_->currentIndex()) {
    case 0: html = attendanceReport(); break;
    case 1: html = pendingReport(); break;
    case 2: html = categoryReport(); break;
    default: html = dailyReport(); break;
    }
    output_->setHtml(kTableStyle + html);
}

QString ReportPanel::attendanceReport() const {
    controller::ReportGenerator reports(manager_, appointments_);
    QString html = "<h2>Most Frequently Attended Patients</h2><p>Patients ranked by completed appointments.</p>"
                   "<table><tr><th>Rank</th><th>Patient ID</th><th>Name</th><th>Category</th>"
                   "<th>Completed</th><th>Total appointments</th></tr>";
    int rank = 1;
    for (const controller::AttendanceRow& row : reports.mostFrequentlyAttended()) {
        html += "<tr>" + cell(QString::number(rank++)) + cell(qs(row.patient->id())) + cell(qs(row.patient->name())) +
                cell(qs(model::categoryToString(row.patient->category()))) + cell(QString::number(row.completed)) +
                cell(QString::number(row.total)) + "</tr>";
    }
    return html + "</table>";
}

QString ReportPanel::pendingReport() const {
    controller::ReportGenerator reports(manager_, appointments_);
    std::vector<controller::AppointmentRow> rows = reports.pendingAppointments();
    QString html = QString("<h2>Patients with Pending Appointments</h2><p>%1 pending appointment(s).</p>"
                           "<table><tr><th>Date</th><th>Time</th><th>Appointment</th><th>Patient</th>"
                           "<th>Doctor</th><th>Reason</th></tr>").arg(rows.size());
    for (const controller::AppointmentRow& row : rows) {
        const model::Appointment& a = row.appointment;
        html += "<tr>" + cell(qs(a.date())) + cell(qs(a.time())) + cell(qs(a.id())) +
                cell(qs(row.patient->name() + " (" + row.patient->id() + ")")) + cell(qs(a.doctor())) +
                cell(qs(a.reason())) + "</tr>";
    }
    return html + "</table>";
}

QString ReportPanel::categoryReport() const {
    controller::ReportGenerator reports(manager_, appointments_);
    static const char* colours[] = {"#2E7D32", "#C62828", "#1565C0"};
    QString html = QString("<h2>Patient Category Distribution</h2><p>Total registered patients: %1</p>"
                           "<table><tr><th>Category</th><th>Patients</th><th>Share</th><th></th></tr>")
                       .arg(manager_.patientCount());
    int i = 0;
    for (const controller::CategoryCount& c : reports.categoryDistribution()) {
        QString bar = QString("<span style='background:%1;color:%1'>%2</span>")
                          .arg(colours[i++], QString(static_cast<int>(c.percent / 2.5) + 1, QChar(0x2588)));
        html += "<tr>" + cell(qs(c.category) + " Patients") + cell(QString::number(c.count)) +
                cell(QString::number(c.percent, 'f', 1) + "%") + "<td>" + bar + "</td></tr>";
    }
    return html + "</table>";
}

QString ReportPanel::dailyReport() const {
    controller::ReportGenerator reports(manager_, appointments_);
    controller::DailySummary s = reports.dailySummary(ss(dateEdit_->date().toString("yyyy-MM-dd")));
    QString html = QString("<h2>Daily Appointment Summary - %1</h2><table>"
                           "<tr><th>Total Appointments</th><td>%2</td></tr><tr><th>Completed</th><td>%3</td></tr>"
                           "<tr><th>Pending</th><td>%4</td></tr><tr><th>Cancelled</th><td>%5</td></tr></table><br>")
                       .arg(qs(s.date)).arg(s.total).arg(s.completed).arg(s.pending).arg(s.cancelled);
    html += "<table><tr><th>Time</th><th>Appointment</th><th>Patient</th><th>Doctor</th><th>Status</th></tr>";
    for (const controller::AppointmentRow& row : s.rows) {
        html += "<tr>" + cell(qs(row.appointment.time())) + cell(qs(row.appointment.id())) +
                cell(qs(row.patient->name())) + cell(qs(row.appointment.doctor())) +
                cell(qs(model::statusToString(row.appointment.status()))) + "</tr>";
    }
    return html + "</table>";
}

void ReportPanel::saveReport() {
    QString path = QFileDialog::getSaveFileName(this, "Save Report", reportCombo_->currentText() + ".html",
                                                "HTML files (*.html)");
    if (path.isEmpty()) return;
    runGuarded(this, "Save Report", [&] {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            throw utils::FileException("Could not write '" + ss(path) + "'.");
        }
        QTextStream(&file) << output_->toHtml();
        emit statusMessage("Report saved to " + path);
    });
}

} // namespace scapms::gui
