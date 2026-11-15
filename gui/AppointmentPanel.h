#pragma once
// "Appointments" tab: schedule, cancel, complete and view appointments.
#include "gui/BasePanel.h"

class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QTableWidget;
class QTimeEdit;

namespace scapms::gui {

class AppointmentPanel : public BasePanel {
    Q_OBJECT
public:
    AppointmentPanel(controller::ClinicManager& manager, controller::AppointmentController& appointments,
                     QWidget* parent = nullptr);

public slots:
    void refresh() override;

private slots:
    void scheduleAppointment();
    void cancelAppointment();
    void completeAppointment();
    void removeAppointment();
    void onPatientChanged();
    void onReasonChanged(const QString& text);

private:
    bool selectedAppointment(std::string& patientId, std::string& appointmentId) const;

    QComboBox* patientCombo_ = nullptr;
    QDateEdit* dateEdit_ = nullptr;
    QTimeEdit* timeEdit_ = nullptr;
    QComboBox* doctorCombo_ = nullptr;
    QLineEdit* reasonEdit_ = nullptr;
    QLabel* reasonCounter_ = nullptr;
    QLabel* patientInfo_ = nullptr;
    QComboBox* statusFilter_ = nullptr;
    QTableWidget* table_ = nullptr;
};

} // namespace scapms::gui
