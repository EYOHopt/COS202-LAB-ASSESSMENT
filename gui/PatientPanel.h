#pragma once
// "Patient Records" tab: view, add, edit, delete patients and send them to the queue.
#include "gui/BasePanel.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

namespace scapms::gui {

class PatientPanel : public BasePanel {
    Q_OBJECT
public:
    PatientPanel(controller::ClinicManager& manager, controller::AppointmentController& appointments,
                 QWidget* parent = nullptr);

public slots:
    void refresh() override;
    void addPatient();
    void editPatient();
    void deletePatient();
    void addToQueue();

private slots:
    void onSelectionChanged();

private:
    QString selectedPatientId() const;

    QLineEdit* filterEdit_ = nullptr;
    QComboBox* categoryFilter_ = nullptr;
    QTableWidget* table_ = nullptr;
    QLabel* detailLabel_ = nullptr;
    QLabel* countLabel_ = nullptr;
    QPushButton* editButton_ = nullptr;
    QPushButton* deleteButton_ = nullptr;
    QPushButton* queueButton_ = nullptr;
};

} // namespace scapms::gui
