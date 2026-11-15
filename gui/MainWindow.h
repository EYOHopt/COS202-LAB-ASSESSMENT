#pragma once
// Main application window: menus, tabs and status bar.
#include <QList>
#include <QMainWindow>

#include "controller/AppointmentController.h"
#include "controller/ClinicManager.h"

class QAction;
class QLabel;
class QTabWidget;

namespace scapms::gui {

class BasePanel;
class AdminPanel;
class SearchPanel;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    bool loadFile(const QString& path, bool quiet = false);
    controller::ClinicManager& manager() { return manager_; }

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void newFile();
    void openFile();
    bool saveFile();
    bool saveFileAs();
    void exportPatients();
    void focusSearch();
    void showShortcuts();
    void showAbout();
    void onDataChanged();
    void showStatus(const QString& message, int timeoutMs);

private:
    void createMenus();
    void createStatusBar();
    void updateTitleAndStatus();
    bool confirmDiscardChanges();

    controller::ClinicManager manager_;
    controller::AppointmentController appointments_;   // declared after manager_ (uses it)
    QString currentFile_;

    QTabWidget* tabs_ = nullptr;
    QList<BasePanel*> panels_;
    AdminPanel* adminPanel_ = nullptr;
    SearchPanel* searchPanel_ = nullptr;
    QLabel* countsLabel_ = nullptr;
    QLabel* fileLabel_ = nullptr;
    QAction* undoAction_ = nullptr;
};

} // namespace scapms::gui
