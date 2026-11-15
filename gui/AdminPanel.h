#pragma once
// "Admin" tab: undo history (stack) and the frequently accessed patients cache (array).
#include "gui/BasePanel.h"

class QLabel;
class QListWidget;
class QPushButton;

namespace scapms::gui {

class AdminPanel : public BasePanel {
    Q_OBJECT
public:
    AdminPanel(controller::ClinicManager& manager, controller::AppointmentController& appointments,
               QWidget* parent = nullptr);

public slots:
    void refresh() override;
    void undoLast();

private:
    QListWidget* undoList_ = nullptr;
    QListWidget* cacheList_ = nullptr;
    QPushButton* undoButton_ = nullptr;
};

} // namespace scapms::gui
