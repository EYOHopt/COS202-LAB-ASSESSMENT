#include "gui/DashboardPanel.h"

#include <QDateTime>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

#include "gui/GuiUtils.h"

namespace scapms::gui {

using model::AppointmentStatus;

DashboardPanel::DashboardPanel(controller::ClinicManager& manager, controller::AppointmentController& appointments,
                               QWidget* parent)
    : BasePanel(manager, appointments, parent) {
    setObjectName("dashboardPanel");

    auto* title = new QLabel("Smart Clinic Dashboard");
    title->setStyleSheet("font-size:20px; font-weight:bold; color:#0D47A1;");
    clockLabel_ = new QLabel;
    clockLabel_->setStyleSheet("font-size:15px; color:#37474F;");
    auto* header = new QHBoxLayout;
    header->addWidget(title);
    header->addStretch();
    header->addWidget(clockLabel_);

    reminderBanner_ = new QLabel;
    reminderBanner_->setObjectName("reminderBanner");
    reminderBanner_->setWordWrap(true);
    reminderBanner_->setStyleSheet("background:#FFF3D6; border:1px solid #F0B429; border-radius:4px; padding:6px;"
                                   "font-weight:bold; color:#7A4F01;");
    reminderBanner_->hide();

    // QGridLayout: statistic cards in a 2 x 4 grid.
    auto* grid = new QGridLayout;
    grid->setSpacing(10);
    cards_["patients"] = addCard(grid, 0, 0, "Registered patients", "#0D47A1", "All patients in the database");
    cards_["general"] = addCard(grid, 0, 1, "General patients", "#2E7D32", "Normal priority");
    cards_["emergency"] = addCard(grid, 0, 2, "Emergency patients", "#C62828", "High priority");
    cards_["followup"] = addCard(grid, 0, 3, "Follow-up patients", "#1565C0", "Scheduled reviews");
    cards_["today"] = addCard(grid, 1, 0, "Appointments today", "#6A1B9A", "All appointments dated today");
    cards_["pending"] = addCard(grid, 1, 1, "Pending appointments", "#EF6C00", "Not yet completed or cancelled");
    cards_["completed"] = addCard(grid, 1, 2, "Completed appointments", "#2E7D32", "All time");
    cards_["queue"] = addCard(grid, 1, 3, "Waiting in queue", "#AD1457", "Patients waiting for consultation");

    todayTable_ = new QTableWidget(0, 6);
    todayTable_->setObjectName("todayTable");
    todayTable_->setHorizontalHeaderLabels({"Time", "Patient", "Category", "Doctor", "Status", "Reminder"});
    todayTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    todayTable_->setSelectionMode(QAbstractItemView::NoSelection);
    todayTable_->verticalHeader()->setVisible(false);
    todayTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    auto* todayBox = new QGroupBox("Today's appointments and reminders");
    (new QVBoxLayout(todayBox))->addWidget(todayTable_);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(header);
    layout->addWidget(reminderBanner_);
    layout->addLayout(grid);
    layout->addWidget(todayBox, 1);

    // Timer events
    clockTimer_ = new QTimer(this);
    connect(clockTimer_, &QTimer::timeout, this, &DashboardPanel::updateClock);
    clockTimer_->start(1000);
    refreshTimer_ = new QTimer(this);
    connect(refreshTimer_, &QTimer::timeout, this, &DashboardPanel::refresh);
    connect(refreshTimer_, &QTimer::timeout, this, &DashboardPanel::checkReminders);
    refreshTimer_->start(30000);

    updateClock();
    refresh();
}

QLabel* DashboardPanel::addCard(QGridLayout* grid, int row, int column, const QString& caption, const QString& colour,
                                const QString& tooltip) {
    auto* card = new QFrame;
    card->setToolTip(tooltip);
    card->setStyleSheet(QString("QFrame { background:white; border:1px solid #D0D7E2; border-left:6px solid %1;"
                                " border-radius:6px; }").arg(colour));
    auto* value = new QLabel("0");
    value->setStyleSheet(QString("border:none; font-size:28px; font-weight:bold; color:%1;").arg(colour));
    auto* label = new QLabel(caption);
    label->setStyleSheet("border:none; color:#546E7A;");
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->addWidget(value);
    cardLayout->addWidget(label);
    grid->addWidget(card, row, column);
    return value;
}

void DashboardPanel::updateClock() {
    clockLabel_->setText(QDateTime::currentDateTime().toString("dddd, d MMMM yyyy  HH:mm:ss"));
}

void DashboardPanel::refresh() {
    int general = 0, emergency = 0, followUp = 0, pending = 0, completed = 0;
    for (const model::PatientRecord* p : manager_.patients()) {
        switch (p->category()) {
        case model::PatientCategory::General: ++general; break;
        case model::PatientCategory::Emergency: ++emergency; break;
        case model::PatientCategory::FollowUp: ++followUp; break;
        }
        pending += p->countAppointments(AppointmentStatus::Pending);
        completed += p->countAppointments(AppointmentStatus::Completed);
    }
    std::vector<controller::AppointmentRow> today = appointments_.appointmentsOn(controller::AppointmentController::today());
    cards_["patients"]->setText(QString::number(manager_.patientCount()));
    cards_["general"]->setText(QString::number(general));
    cards_["emergency"]->setText(QString::number(emergency));
    cards_["followup"]->setText(QString::number(followUp));
    cards_["today"]->setText(QString::number(today.size()));
    cards_["pending"]->setText(QString::number(pending));
    cards_["completed"]->setText(QString::number(completed));
    cards_["queue"]->setText(QString::number(manager_.queueSize()));

    QTime now = QTime::currentTime();
    todayTable_->setRowCount(0);
    for (const controller::AppointmentRow& row : today) {
        const model::Appointment& a = row.appointment;
        QString reminder = "-";
        if (a.isPending()) {
            int minutes = now.secsTo(QTime::fromString(qs(a.time()), "HH:mm")) / 60;
            reminder = minutes >= 0 ? QString("Starts in %1 min").arg(minutes)
                                    : QString("Overdue by %1 min").arg(-minutes);
        }
        int r = todayTable_->rowCount();
        todayTable_->insertRow(r);
        QStringList values = {qs(a.time()), qs(row.patient->name()), qs(model::categoryToString(row.patient->category())),
                              qs(a.doctor()), qs(model::statusToString(a.status())), reminder};
        for (int c = 0; c < values.size(); ++c) {
            auto* item = new QTableWidgetItem(values[c]);
            item->setBackground(statusBackground(a.status()));
            todayTable_->setItem(r, c, item);
        }
    }
    checkReminders();
}

// Shows a reminder banner for appointments starting within the next 15 minutes.
void DashboardPanel::checkReminders() {
    QTime now = QTime::currentTime();
    QStringList messages;
    for (const controller::AppointmentRow& row : appointments_.appointmentsOn(controller::AppointmentController::today())) {
        const model::Appointment& a = row.appointment;
        int minutes = now.secsTo(QTime::fromString(qs(a.time()), "HH:mm")) / 60;
        if (!a.isPending() || minutes < 0 || minutes > kReminderMinutes) continue;
        messages << QString("%1 at %2 with %3 (in %4 min)").arg(qs(row.patient->name()), qs(a.time()), qs(a.doctor()))
                        .arg(minutes);
        if (!remindedIds_.contains(qs(a.id()))) {
            remindedIds_.insert(qs(a.id()));
            emit statusMessage("Reminder: " + messages.last(), 10000);
        }
    }
    reminderBanner_->setVisible(!messages.isEmpty());
    reminderBanner_->setText("⏰ Upcoming: " + messages.join("   |   "));
}

} // namespace scapms::gui
