#pragma once
// "Dashboard" tab: live statistics and today's appointment reminders.
#include <QMap>
#include <QSet>

#include "gui/BasePanel.h"

class QGridLayout;
class QLabel;
class QTableWidget;
class QTimer;

namespace scapms::gui {

class DashboardPanel : public BasePanel {
    Q_OBJECT
public:
    static constexpr int kReminderMinutes = 15;

    DashboardPanel(controller::ClinicManager& manager, controller::AppointmentController& appointments,
                   QWidget* parent = nullptr);

public slots:
    void refresh() override;

private slots:
    void updateClock();      // timer: every second
    void checkReminders();   // timer: every 30 seconds

private:
    QLabel* addCard(QGridLayout* grid, int row, int column, const QString& caption, const QString& colour,
                    const QString& tooltip);

    QLabel* clockLabel_ = nullptr;
    QLabel* reminderBanner_ = nullptr;
    QMap<QString, QLabel*> cards_;
    QTableWidget* todayTable_ = nullptr;
    QTimer* clockTimer_ = nullptr;
    QTimer* refreshTimer_ = nullptr;
    QSet<QString> remindedIds_;
};

} // namespace scapms::gui
