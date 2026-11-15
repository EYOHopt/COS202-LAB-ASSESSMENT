#include "gui/MainWindow.h"

#include <QAction>
#include <QCloseEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolBar>

#include "gui/AdminPanel.h"
#include "gui/AppointmentPanel.h"
#include "gui/DashboardPanel.h"
#include "gui/GuiUtils.h"
#include "gui/PatientPanel.h"
#include "gui/QueuePanel.h"
#include "gui/ReportPanel.h"
#include "gui/SearchPanel.h"

namespace scapms::gui {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), appointments_(manager_) {
    setObjectName("mainWindow");
    resize(1280, 800);

    tabs_ = new QTabWidget;
    tabs_->setObjectName("mainTabs");
    searchPanel_ = new SearchPanel(manager_, appointments_);
    adminPanel_ = new AdminPanel(manager_, appointments_);
    panels_ = {new DashboardPanel(manager_, appointments_), new PatientPanel(manager_, appointments_),
               new AppointmentPanel(manager_, appointments_), new QueuePanel(manager_, appointments_),
               searchPanel_, new ReportPanel(manager_, appointments_), adminPanel_};
    const QStringList titles = {"Dashboard", "Patient Records", "Appointments", "Waiting Queue", "Search && Sort",
                                "Reports", "Admin / Undo"};
    for (int i = 0; i < panels_.size(); ++i) {
        tabs_->addTab(panels_[i], titles[i]);
        connect(panels_[i], &BasePanel::dataChanged, this, &MainWindow::onDataChanged);
        connect(panels_[i], &BasePanel::statusMessage, this, &MainWindow::showStatus);
    }
    setCentralWidget(tabs_);
    connect(tabs_, &QTabWidget::currentChanged, this, [this](int index) { panels_[index]->refresh(); });

    createMenus();
    createStatusBar();
    updateTitleAndStatus();
}

void MainWindow::createMenus() {
    // ----- File menu -----
    QMenu* fileMenu = menuBar()->addMenu("&File");
    QAction* newAction = fileMenu->addAction("&New", QKeySequence("Ctrl+Shift+N"), this, &MainWindow::newFile);
    newAction->setToolTip("Start a new, empty clinic database");
    QAction* openAction = fileMenu->addAction("&Load...", QKeySequence("Ctrl+O"), this, &MainWindow::openFile);
    QAction* saveAction = fileMenu->addAction("&Save", QKeySequence("Ctrl+S"), this, &MainWindow::saveFile);
    fileMenu->addAction("Save &As...", QKeySequence("Ctrl+Shift+S"), this, &MainWindow::saveFileAs);
    fileMenu->addAction("&Export Patients (CSV)...", QKeySequence("Ctrl+E"), this, &MainWindow::exportPatients);
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", QKeySequence("Ctrl+Q"), this, &QWidget::close);

    // ----- Edit menu -----
    QMenu* editMenu = menuBar()->addMenu("&Edit");
    undoAction_ = editMenu->addAction("&Undo Last Administrative Operation", QKeySequence("Ctrl+Z"), adminPanel_,
                                      &AdminPanel::undoLast);
    editMenu->addAction("&Find Patient...", QKeySequence("Ctrl+F"), this, &MainWindow::focusSearch);

    // ----- View menu: Ctrl+1 ... Ctrl+7 switch tabs -----
    QMenu* viewMenu = menuBar()->addMenu("&View");
    for (int i = 0; i < tabs_->count(); ++i) {
        viewMenu->addAction(tabs_->tabText(i).replace("&&", "&"), QKeySequence(QString("Ctrl+%1").arg(i + 1)), this,
                            [this, i] { tabs_->setCurrentIndex(i); });
    }

    // ----- Help menu -----
    QMenu* helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("&Keyboard Shortcuts", this, &MainWindow::showShortcuts);
    helpMenu->addAction("&About SCAPMS", this, &MainWindow::showAbout);

    QToolBar* toolbar = addToolBar("Main");
    toolbar->setMovable(false);
    toolbar->addAction(newAction);
    toolbar->addAction(openAction);
    toolbar->addAction(saveAction);
    toolbar->addSeparator();
    toolbar->addAction(undoAction_);
    undoAction_->setText("Undo");
    undoAction_->setToolTip("Undo the last administrative operation (Ctrl+Z)");
    saveAction->setToolTip("Save patients and appointments (Ctrl+S)");
    openAction->setToolTip("Load patients and appointments from a file (Ctrl+O)");
}

void MainWindow::createStatusBar() {
    countsLabel_ = new QLabel;
    fileLabel_ = new QLabel;
    statusBar()->addPermanentWidget(countsLabel_);
    statusBar()->addPermanentWidget(fileLabel_);
}

void MainWindow::updateTitleAndStatus() {
    QString fileName = currentFile_.isEmpty() ? "Untitled" : QFileInfo(currentFile_).fileName();
    setWindowTitle(QString("SCAPMS - Smart Clinic Appointment & Patient Management System - %1%2")
                       .arg(fileName, manager_.isModified() ? " *" : ""));
    countsLabel_->setText(QString("Patients: %1  |  In queue: %2  ").arg(manager_.patientCount()).arg(manager_.queueSize()));
    fileLabel_->setText("File: " + fileName + (manager_.isModified() ? " (unsaved changes)" : ""));
    undoAction_->setEnabled(manager_.canUndo());
}

// Called whenever any panel changes the data: every panel is refreshed through BasePanel*.
void MainWindow::onDataChanged() {
    for (BasePanel* panel : panels_) panel->refresh();
    updateTitleAndStatus();
}

void MainWindow::showStatus(const QString& message, int timeoutMs) {
    statusBar()->showMessage(message, timeoutMs);
    updateTitleAndStatus();
}

bool MainWindow::confirmDiscardChanges() {
    if (!manager_.isModified()) return true;
    auto answer = QMessageBox::question(this, "Unsaved Changes", "Do you want to save your changes first?",
                                        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (answer == QMessageBox::Save) return saveFile();
    return answer == QMessageBox::Discard;
}

void MainWindow::newFile() {
    if (!confirmDiscardChanges()) return;
    manager_.newDatabase();
    currentFile_.clear();
    onDataChanged();
    showStatus("Started a new clinic database", 4000);
}

bool MainWindow::loadFile(const QString& path, bool quiet) {
    int skipped = 0;
    bool ok = runGuarded(this, "Load", [&] { skipped = manager_.loadFromFile(ss(path)); });
    if (!ok) return false;
    currentFile_ = path;
    onDataChanged();
    showStatus(QString("Loaded %1 patients from %2").arg(manager_.patientCount()).arg(QFileInfo(path).fileName()), 6000);
    if (skipped > 0 && !quiet) {
        QMessageBox::warning(this, "Load", QString("%1 invalid line(s) in the file were skipped.").arg(skipped));
    }
    return true;
}

void MainWindow::openFile() {
    if (!confirmDiscardChanges()) return;
    QString path = QFileDialog::getOpenFileName(this, "Load Clinic Data", QString(),
                                                "SCAPMS data (*.csv *.txt);;All files (*)");
    if (!path.isEmpty()) loadFile(path);
}

bool MainWindow::saveFile() {
    if (currentFile_.isEmpty()) return saveFileAs();
    bool ok = runGuarded(this, "Save", [&] { manager_.saveToFile(ss(currentFile_)); });
    if (ok) showStatus("Saved to " + currentFile_, 5000);
    return ok;
}

bool MainWindow::saveFileAs() {
    QString path = QFileDialog::getSaveFileName(this, "Save Clinic Data", "clinic_data.csv",
                                                "SCAPMS data (*.csv);;Text files (*.txt)");
    if (path.isEmpty()) return false;
    currentFile_ = path;
    return saveFile();
}

void MainWindow::exportPatients() {
    QString path = QFileDialog::getSaveFileName(this, "Export Patients", "patients_export.csv", "CSV files (*.csv)");
    if (path.isEmpty()) return;
    if (runGuarded(this, "Export", [&] { manager_.exportPatients(ss(path)); })) {
        showStatus("Exported patients to " + path, 5000);
    }
}

void MainWindow::focusSearch() {
    tabs_->setCurrentWidget(searchPanel_);
    searchPanel_->focusSearch();
}

void MainWindow::showShortcuts() {
    QMessageBox::information(this, "Keyboard Shortcuts",
        "Ctrl+S\tSave\nCtrl+Shift+S\tSave As\nCtrl+O\tLoad\nCtrl+Shift+N\tNew database\nCtrl+E\tExport patients\n"
        "Ctrl+F\tSearch\nCtrl+Z\tUndo last administrative operation\nCtrl+Q\tExit\n"
        "Ctrl+1 ... Ctrl+7\tSwitch tabs\nCtrl+N\tAdd patient (Patient Records tab)\nDelete\tDelete selected patient");
}

void MainWindow::showAbout() {
    QMessageBox::about(this, "About SCAPMS",
        "<b>Smart Clinic Appointment & Patient Management System</b><br>"
        "COS202 Programming II - Lab Assessment Project<br>Built with C++17 and Qt Widgets.");
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (confirmDiscardChanges()) event->accept();
    else event->ignore();
}

} // namespace scapms::gui
