#include "gui/AppointmentPanel.h"

#include <QComboBox>
#include <QDateEdit>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QTableWidget>
#include <QTimeEdit>
#include <QVBoxLayout>

#include "gui/GuiUtils.h"
#include "model/Appointable.h"

namespace scapms::gui {

AppointmentPanel::AppointmentPanel(controller::ClinicManager& manager, controller::AppointmentController& appointments,
                                   QWidget* parent)
    : BasePanel(manager, appointments, parent) {
    setObjectName("appointmentPanel");

    // ----- Left: scheduling form (QFormLayout) -----
    patientCombo_ = new QComboBox;
    patientCombo_->setObjectName("apptPatientCombo");
    patientCombo_->setToolTip("Only General and Follow-Up patients can be booked");
    dateEdit_ = new QDateEdit(QDate::currentDate().addDays(1));
    dateEdit_->setObjectName("apptDate");
    dateEdit_->setCalendarPopup(true);
    dateEdit_->setDisplayFormat("yyyy-MM-dd");
    timeEdit_ = new QTimeEdit(QTime(9, 0));
    timeEdit_->setObjectName("apptTime");
    timeEdit_->setDisplayFormat("HH:mm");
    doctorCombo_ = new QComboBox;
    doctorCombo_->setObjectName("apptDoctor");
    doctorCombo_->setEditable(true);
    doctorCombo_->addItems({"Dr. Adeyemi", "Dr. Bello", "Dr. Okafor", "Dr. Musa"});
    doctorCombo_->setToolTip("Choose a doctor or type a new name");
    reasonEdit_ = new QLineEdit;
    reasonEdit_->setObjectName("apptReason");
    reasonEdit_->setMaxLength(100);
    reasonEdit_->setPlaceholderText("Reason for the visit");
    reasonCounter_ = new QLabel("0/100");
    patientInfo_ = new QLabel;
    patientInfo_->setWordWrap(true);
    patientInfo_->setStyleSheet("color:#455A64;");

    auto* reasonRow = new QHBoxLayout;
    reasonRow->addWidget(reasonEdit_, 1);
    reasonRow->addWidget(reasonCounter_);

    auto* form = new QFormLayout;
    form->addRow("Patient:", patientCombo_);
    form->addRow("", patientInfo_);
    form->addRow("Date:", dateEdit_);
    form->addRow("Time:", timeEdit_);
    form->addRow("Doctor:", doctorCombo_);
    form->addRow("Reason:", reasonRow);

    auto* scheduleButton = new QPushButton("Schedule Appointment");
    scheduleButton->setObjectName("scheduleButton");
    scheduleButton->setToolTip("Book the appointment (checks the date, time and doctor availability)");

    auto* formBox = new QGroupBox("Schedule a new appointment");
    auto* formLayout = new QVBoxLayout(formBox);
    formLayout->addLayout(form);
    formLayout->addWidget(scheduleButton);
    formLayout->addStretch();

    // ----- Right: appointment list -----
    statusFilter_ = new QComboBox;
    statusFilter_->setObjectName("apptStatusFilter");
    statusFilter_->addItems({"All", "Pending", "Completed", "Cancelled"});
    table_ = new QTableWidget(0, 8);
    table_->setObjectName("appointmentTable");
    table_->setHorizontalHeaderLabels({"ID", "Date", "Time", "Patient", "Category", "Doctor", "Reason", "Status"});
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->verticalHeader()->setVisible(false);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);

    auto* completeButton = new QPushButton("Mark Completed");
    completeButton->setToolTip("Record that the patient attended this appointment");
    auto* cancelButton = new QPushButton("Cancel Appointment");
    cancelButton->setToolTip("Cancel the selected pending appointment (can be undone)");
    auto* removeButton = new QPushButton("Remove (Admin)");
    removeButton->setToolTip("Permanently remove the selected appointment record (can be undone)");

    auto* filterRow = new QHBoxLayout;
    filterRow->addWidget(new QLabel("Show:"));
    filterRow->addWidget(statusFilter_);
    filterRow->addStretch();
    auto* actionRow = new QHBoxLayout;
    actionRow->addWidget(completeButton);
    actionRow->addWidget(cancelButton);
    actionRow->addWidget(removeButton);
    actionRow->addStretch();

    auto* listBox = new QGroupBox("Appointments");
    auto* listLayout = new QVBoxLayout(listBox);
    listLayout->addLayout(filterRow);
    listLayout->addWidget(table_, 1);
    listLayout->addLayout(actionRow);

    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(formBox);
    splitter->addWidget(listBox);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({360, 800});

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(splitter);

    connect(scheduleButton, &QPushButton::clicked, this, &AppointmentPanel::scheduleAppointment);
    connect(completeButton, &QPushButton::clicked, this, &AppointmentPanel::completeAppointment);
    connect(cancelButton, &QPushButton::clicked, this, &AppointmentPanel::cancelAppointment);
    connect(removeButton, &QPushButton::clicked, this, &AppointmentPanel::removeAppointment);
    connect(statusFilter_, &QComboBox::currentIndexChanged, this, &AppointmentPanel::refresh);
    connect(patientCombo_, &QComboBox::currentIndexChanged, this, &AppointmentPanel::onPatientChanged);
    connect(reasonEdit_, &QLineEdit::textChanged, this, &AppointmentPanel::onReasonChanged);

    refresh();
}

void AppointmentPanel::refresh() {
    // Rebuild the patient list, keeping the current choice.
    QString current = patientCombo_->currentData().toString();
    patientCombo_->blockSignals(true);
    patientCombo_->clear();
    for (model::PatientRecord* p : appointments_.bookablePatients()) {
        patientCombo_->addItem(QString("%1 - %2 (%3)").arg(qs(p->id()), qs(p->name()),
                                                          qs(model::categoryToString(p->category()))),
                               qs(p->id()));
    }
    int index = patientCombo_->findData(current);
    patientCombo_->setCurrentIndex(index >= 0 ? index : 0);
    patientCombo_->blockSignals(false);
    onPatientChanged();

    // Rebuild the appointment table.
    int filter = statusFilter_->currentIndex();   // 0 = all, then Pending/Completed/Cancelled
    table_->setRowCount(0);
    for (const controller::AppointmentRow& row : appointments_.allAppointments()) {
        const model::Appointment& a = row.appointment;
        if (filter > 0 && static_cast<int>(a.status()) != filter - 1) continue;
        int r = table_->rowCount();
        table_->insertRow(r);
        QStringList values = {qs(a.id()), qs(a.date()), qs(a.time()), qs(row.patient->name()),
                              qs(model::categoryToString(row.patient->category())), qs(a.doctor()), qs(a.reason()),
                              qs(model::statusToString(a.status()))};
        for (int c = 0; c < values.size(); ++c) {
            auto* item = new QTableWidgetItem(values[c]);
            item->setBackground(statusBackground(a.status()));   // custom presentation per status
            table_->setItem(r, c, item);
        }
        table_->item(r, 0)->setData(Qt::UserRole, qs(row.patient->id()));
    }
}

void AppointmentPanel::onPatientChanged() {
    model::PatientRecord* patient = manager_.findPatient(ss(patientCombo_->currentData().toString()));
    auto* bookable = dynamic_cast<model::Appointable*>(patient);
    if (!patient || !bookable) {
        patientInfo_->setText("No bookable patients registered.");
        return;
    }
    patientInfo_->setText(QString("%1 has %2 of %3 allowed pending appointment(s).")
                              .arg(qs(patient->name()))
                              .arg(patient->countAppointments(model::AppointmentStatus::Pending))
                              .arg(bookable->maxPendingAppointments()));
}

void AppointmentPanel::onReasonChanged(const QString& text) {
    reasonCounter_->setText(QString("%1/100").arg(text.size()));
}

void AppointmentPanel::scheduleAppointment() {
    if (patientCombo_->currentIndex() < 0) {
        QMessageBox::information(this, "Schedule Appointment", "Please register a General or Follow-Up patient first.");
        return;
    }
    std::string newId;
    bool ok = runGuarded(this, "Schedule Appointment", [&] {
        newId = appointments_.schedule(ss(patientCombo_->currentData().toString()),
                                       ss(dateEdit_->date().toString("yyyy-MM-dd")),
                                       ss(timeEdit_->time().toString("HH:mm")), ss(doctorCombo_->currentText()),
                                       ss(reasonEdit_->text()));
    });
    if (ok) {
        reasonEdit_->clear();
        emit statusMessage("Appointment " + qs(newId) + " scheduled");
        emit dataChanged();
    }
}

bool AppointmentPanel::selectedAppointment(std::string& patientId, std::string& appointmentId) const {
    int row = table_->currentRow();
    if (row < 0 || !table_->item(row, 0)->isSelected()) {
        QMessageBox::information(const_cast<AppointmentPanel*>(this), "Appointments", "Please select an appointment first.");
        return false;
    }
    patientId = ss(table_->item(row, 0)->data(Qt::UserRole).toString());
    appointmentId = ss(table_->item(row, 0)->text());
    return true;
}

void AppointmentPanel::cancelAppointment() {
    std::string patientId, appointmentId;
    if (!selectedAppointment(patientId, appointmentId)) return;
    if (runGuarded(this, "Cancel Appointment", [&] { appointments_.cancel(patientId, appointmentId); })) {
        emit statusMessage("Appointment " + qs(appointmentId) + " cancelled. Press Ctrl+Z to undo.");
        emit dataChanged();
    }
}

void AppointmentPanel::completeAppointment() {
    std::string patientId, appointmentId;
    if (!selectedAppointment(patientId, appointmentId)) return;
    if (runGuarded(this, "Complete Appointment", [&] { appointments_.complete(patientId, appointmentId); })) {
        emit statusMessage("Appointment " + qs(appointmentId) + " marked as completed");
        emit dataChanged();
    }
}

void AppointmentPanel::removeAppointment() {
    std::string patientId, appointmentId;
    if (!selectedAppointment(patientId, appointmentId)) return;
    if (QMessageBox::question(this, "Remove Appointment", "Remove appointment " + qs(appointmentId) +
                              " permanently?\n\nYou can undo this with Ctrl+Z.") != QMessageBox::Yes) {
        return;
    }
    if (runGuarded(this, "Remove Appointment", [&] { appointments_.remove(patientId, appointmentId); })) {
        emit statusMessage("Appointment " + qs(appointmentId) + " removed. Press Ctrl+Z to undo.");
        emit dataChanged();
    }
}

} // namespace scapms::gui
