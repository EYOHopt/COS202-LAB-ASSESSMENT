#pragma once
// Common base class for every tab of the main window.
// MainWindow keeps a list of BasePanel* and calls refresh() polymorphically.
#include <QWidget>

#include "controller/AppointmentController.h"
#include "controller/ClinicManager.h"

namespace scapms::gui {

class BasePanel : public QWidget {
    Q_OBJECT
public:
    BasePanel(controller::ClinicManager& manager, controller::AppointmentController& appointments,
              QWidget* parent = nullptr)
        : QWidget(parent), manager_(manager), appointments_(appointments) {}

public slots:
    virtual void refresh() = 0;   // redraw the panel from the current data

signals:
    void dataChanged();                                            // data was modified by this panel
    void statusMessage(const QString& message, int timeoutMs = 5000);

protected:
    controller::ClinicManager& manager_;
    controller::AppointmentController& appointments_;
};

} // namespace scapms::gui
