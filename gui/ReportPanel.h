#pragma once
// "Reports" tab: generates the clinic reports as formatted HTML.
#include "gui/BasePanel.h"

class QComboBox;
class QDateEdit;
class QTextBrowser;

namespace scapms::gui {

class ReportPanel : public BasePanel {
    Q_OBJECT
public:
    ReportPanel(controller::ClinicManager& manager, controller::AppointmentController& appointments,
                QWidget* parent = nullptr);

public slots:
    void refresh() override;
    void generateReport();
    void saveReport();

private:
    QString attendanceReport() const;
    QString pendingReport() const;
    QString categoryReport() const;
    QString dailyReport() const;

    QComboBox* reportCombo_ = nullptr;
    QDateEdit* dateEdit_ = nullptr;
    QTextBrowser* output_ = nullptr;
};

} // namespace scapms::gui
