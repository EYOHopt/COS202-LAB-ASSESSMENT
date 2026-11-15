#pragma once
// "Waiting Queue" tab: add patients to the queue and call the next patient.
#include "gui/BasePanel.h"

class QComboBox;
class QLabel;
class QListWidget;
class QTableWidget;
class QTimer;

namespace scapms::gui {

class QueuePanel : public BasePanel {
    Q_OBJECT
public:
    static constexpr int kLongWaitMinutes = 30;

    QueuePanel(controller::ClinicManager& manager, controller::AppointmentController& appointments,
               QWidget* parent = nullptr);

public slots:
    void refresh() override;

private slots:
    void addToQueue();
    void callNext();
    void updateWaitingTimes();   // timer event

private:
    void fillTable(QTableWidget* table, const std::vector<controller::QueueEntry>& entries);

    QComboBox* patientCombo_ = nullptr;
    QTableWidget* priorityTable_ = nullptr;
    QTableWidget* regularTable_ = nullptr;
    QLabel* nowServing_ = nullptr;
    QLabel* alertLabel_ = nullptr;
    QListWidget* servedList_ = nullptr;
    QTimer* timer_ = nullptr;
};

} // namespace scapms::gui
