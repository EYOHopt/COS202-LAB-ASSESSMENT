#include "gui/PatientPanel.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QShortcut>
#include <QTableWidget>
#include <QVBoxLayout>

#include "controller/SearchEngine.h"
#include "gui/GuiUtils.h"
#include "gui/PatientDialog.h"
#include "utils/StringUtils.h"

namespace scapms::gui {

using controller::SearchEngine;

PatientPanel::PatientPanel(controller::ClinicManager& manager, controller::AppointmentController& appointments,
                           QWidget* parent)
    : BasePanel(manager, appointments, parent) {
    setObjectName("patientPanel");

    filterEdit_ = new QLineEdit;
    filterEdit_->setObjectName("patientFilter");
    filterEdit_->setPlaceholderText("Type a name or patient ID to filter the list...");
    filterEdit_->setClearButtonEnabled(true);
    filterEdit_->setToolTip("The table is filtered as you type");
    categoryFilter_ = new QComboBox;
    categoryFilter_->addItems({"All categories", "General", "Emergency", "Follow-Up"});

    auto* filterRow = new QHBoxLayout;
    filterRow->addWidget(new QLabel("Filter:"));
    filterRow->addWidget(filterEdit_, 1);
    filterRow->addWidget(new QLabel("Category:"));
    filterRow->addWidget(categoryFilter_);

    table_ = new QTableWidget(0, 8);
    table_->setObjectName("patientTable");
    table_->setHorizontalHeaderLabels({"Patient ID", "Name", "Age", "Category", "Priority", "Contact", "Details",
                                       "Appointments"});
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSortingEnabled(false);   // sorting is done by our own algorithms (Search & Sort tab)
    table_->verticalHeader()->setVisible(false);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);

    detailLabel_ = new QLabel("Select a patient to see their details.");
    detailLabel_->setWordWrap(true);
    detailLabel_->setStyleSheet("background:#F5F7FA; border:1px solid #D0D7E2; border-radius:4px; padding:6px;");

    auto* addButton = new QPushButton("Add Patient");
    addButton->setObjectName("addPatientButton");
    addButton->setToolTip("Register a new patient (Ctrl+N in this tab)");
    editButton_ = new QPushButton("Edit Patient");
    editButton_->setToolTip("Edit the selected patient (or double-click a row)");
    deleteButton_ = new QPushButton("Delete Patient");
    deleteButton_->setToolTip("Delete the selected patient (Delete key). Can be undone.");
    queueButton_ = new QPushButton("Add to Waiting Queue");
    queueButton_->setToolTip("Send the selected patient to the waiting queue");
    countLabel_ = new QLabel;

    auto* buttonRow = new QHBoxLayout;
    buttonRow->addWidget(addButton);
    buttonRow->addWidget(editButton_);
    buttonRow->addWidget(deleteButton_);
    buttonRow->addWidget(queueButton_);
    buttonRow->addStretch();
    buttonRow->addWidget(countLabel_);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(filterRow);
    layout->addWidget(table_, 1);
    layout->addWidget(detailLabel_);
    layout->addLayout(buttonRow);

    // Event handling (signals and slots)
    connect(filterEdit_, &QLineEdit::textChanged, this, &PatientPanel::refresh);
    connect(categoryFilter_, &QComboBox::currentIndexChanged, this, &PatientPanel::refresh);
    connect(table_, &QTableWidget::itemSelectionChanged, this, &PatientPanel::onSelectionChanged);
    connect(table_, &QTableWidget::cellDoubleClicked, this, &PatientPanel::editPatient);
    connect(addButton, &QPushButton::clicked, this, &PatientPanel::addPatient);
    connect(editButton_, &QPushButton::clicked, this, &PatientPanel::editPatient);
    connect(deleteButton_, &QPushButton::clicked, this, &PatientPanel::deletePatient);
    connect(queueButton_, &QPushButton::clicked, this, &PatientPanel::addToQueue);
    auto* deleteShortcut = new QShortcut(QKeySequence::Delete, table_);
    connect(deleteShortcut, &QShortcut::activated, this, &PatientPanel::deletePatient);
    auto* addShortcut = new QShortcut(QKeySequence("Ctrl+N"), this, nullptr, nullptr, Qt::WidgetWithChildrenShortcut);
    connect(addShortcut, &QShortcut::activated, this, &PatientPanel::addPatient);

    refresh();
}

void PatientPanel::refresh() {
    QString keepSelected = selectedPatientId();
    std::string keyword = utils::toLower(utils::trim(ss(filterEdit_->text())));
    int categoryIndex = categoryFilter_->currentIndex();   // 0 = all, then General/Emergency/Follow-Up

    table_->setRowCount(0);
    int shown = 0;
    for (model::PatientRecord* p : manager_.patients()) {
        bool matchesText = SearchEngine::containsIgnoreCase(p->name(), keyword) ||
                           SearchEngine::containsIgnoreCase(p->id(), keyword);
        bool matchesCategory = categoryIndex == 0 || static_cast<int>(p->category()) == categoryIndex - 1;
        if (!matchesText || !matchesCategory) continue;

        int row = table_->rowCount();
        table_->insertRow(row);
        int pending = p->countAppointments(model::AppointmentStatus::Pending);
        QStringList values = {qs(p->id()), qs(p->name()), QString::number(p->age()),
                              qs(model::categoryToString(p->category())), qs(p->priorityLabel()), qs(p->contact()),
                              qs(p->details()),
                              QString("%1 (%2 pending)").arg(p->appointments().size()).arg(pending)};
        for (int col = 0; col < values.size(); ++col) {
            auto* item = new QTableWidgetItem(values[col]);
            item->setBackground(categoryBackground(p->category()));   // custom presentation per category
            if (col == 4) {
                QFont bold = item->font();
                bold.setBold(true);
                item->setFont(bold);
                item->setForeground(categoryAccent(p->category()));
            }
            table_->setItem(row, col, item);
        }
        table_->item(row, 0)->setData(Qt::UserRole, qs(p->id()));
        if (qs(p->id()) == keepSelected) table_->selectRow(row);
        ++shown;
    }
    countLabel_->setText(QString("Showing %1 of %2 patients").arg(shown).arg(manager_.patientCount()));
    onSelectionChanged();
}

QString PatientPanel::selectedPatientId() const {
    int row = table_->currentRow();
    if (row < 0 || !table_->item(row, 0) || !table_->item(row, 0)->isSelected()) return {};
    return table_->item(row, 0)->data(Qt::UserRole).toString();
}

void PatientPanel::onSelectionChanged() {
    model::PatientRecord* patient = manager_.findPatient(ss(selectedPatientId()));
    editButton_->setEnabled(patient);
    deleteButton_->setEnabled(patient);
    queueButton_->setEnabled(patient);
    if (!patient) {
        detailLabel_->setText("Select a patient to see their details.");
        return;
    }
    manager_.touch(patient);   // record access in the "frequently accessed" cache
    detailLabel_->setText(qs(controller::ClinicManager::processPatient(*patient)));   // polymorphic description
}

void PatientPanel::addPatient() {
    PatientDialog dialog(manager_, nullptr, this);
    dialog.setCommitFunction([this](std::unique_ptr<model::PatientRecord> patient) {
        model::PatientRecord* added = manager_.registerPatient(std::move(patient));
        emit statusMessage("Registered patient " + qs(added->id()) + " - " + qs(added->name()));
    });
    if (dialog.exec() == QDialog::Accepted) emit dataChanged();
}

void PatientPanel::editPatient() {
    model::PatientRecord* patient = manager_.findPatient(ss(selectedPatientId()));
    if (!patient) {
        QMessageBox::information(this, "Edit Patient", "Please select a patient to edit.");
        return;
    }
    PatientDialog dialog(manager_, patient, this);
    dialog.setCommitFunction([this](std::unique_ptr<model::PatientRecord> updated) {
        model::PatientRecord* saved = manager_.updatePatient(std::move(updated));
        emit statusMessage("Updated patient " + qs(saved->id()));
    });
    if (dialog.exec() == QDialog::Accepted) emit dataChanged();
}

void PatientPanel::deletePatient() {
    model::PatientRecord* patient = manager_.findPatient(ss(selectedPatientId()));
    if (!patient) return;
    QString id = qs(patient->id());
    auto answer = QMessageBox::question(this, "Delete Patient",
        QString("Delete %1 (%2) and all of their appointments?\n\nYou can undo this with Edit > Undo (Ctrl+Z).")
            .arg(qs(patient->name()), id));
    if (answer != QMessageBox::Yes) return;
    if (runGuarded(this, "Delete Patient", [&] { manager_.deletePatient(ss(id)); })) {
        emit statusMessage("Deleted patient " + id + ". Press Ctrl+Z to undo.");
        emit dataChanged();
    }
}

void PatientPanel::addToQueue() {
    QString id = selectedPatientId();
    if (runGuarded(this, "Waiting Queue", [&] { manager_.enqueue(ss(id)); })) {
        emit statusMessage("Added " + id + " to the waiting queue");
        emit dataChanged();
    }
}

} // namespace scapms::gui
