#include "gui/QueuePanel.h"

#include <QComboBox>
#include <QDateTime>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <ctime>

#include "gui/GuiUtils.h"

namespace scapms::gui {

namespace {
QTableWidget* makeQueueTable(const QString& name) {
    auto* table = new QTableWidget(0, 5);
    table->setObjectName(name);
    table->setHorizontalHeaderLabels({"Position", "Patient ID", "Name", "Joined", "Waiting"});
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    return table;
}
} // namespace

QueuePanel::QueuePanel(controller::ClinicManager& manager, controller::AppointmentController& appointments,
                       QWidget* parent)
    : BasePanel(manager, appointments, parent) {
    setObjectName("queuePanel");

    patientCombo_ = new QComboBox;
    patientCombo_->setObjectName("queuePatientCombo");
    patientCombo_->setMinimumWidth(320);
    auto* addButton = new QPushButton("Add to Queue");
    addButton->setToolTip("Emergency patients join the priority queue, everyone else the regular queue");
    auto* callButton = new QPushButton("Call Next Patient");
    callButton->setObjectName("callNextButton");
    callButton->setToolTip("Serve the next patient: priority queue first, then first-come first-served");
    callButton->setStyleSheet("QPushButton { background:#1565C0; color:white; font-weight:bold; padding:6px 14px; }");

    auto* topRow = new QHBoxLayout;
    topRow->addWidget(new QLabel("Patient:"));
    topRow->addWidget(patientCombo_);
    topRow->addWidget(addButton);
    topRow->addStretch();
    topRow->addWidget(callButton);

    nowServing_ = new QLabel("Now serving: -");
    nowServing_->setObjectName("nowServing");
    nowServing_->setStyleSheet("font-size:16px; font-weight:bold; color:#0D47A1; padding:6px;");
    alertLabel_ = new QLabel;
    alertLabel_->setStyleSheet("color:#B71C1C; font-weight:bold;");

    priorityTable_ = makeQueueTable("priorityQueueTable");
    regularTable_ = makeQueueTable("regularQueueTable");
    auto* priorityBox = new QGroupBox("Priority queue (Emergency patients)");
    priorityBox->setStyleSheet("QGroupBox { color:#B71C1C; font-weight:bold; }");
    (new QVBoxLayout(priorityBox))->addWidget(priorityTable_);
    auto* regularBox = new QGroupBox("Regular queue (first come, first served)");
    (new QVBoxLayout(regularBox))->addWidget(regularTable_);

    servedList_ = new QListWidget;
    servedList_->setObjectName("servedList");
    auto* servedBox = new QGroupBox("Recently served");
    servedBox->setMaximumWidth(280);
    (new QVBoxLayout(servedBox))->addWidget(servedList_);

    auto* queues = new QVBoxLayout;
    queues->addWidget(priorityBox);
    queues->addWidget(regularBox, 1);
    auto* middle = new QHBoxLayout;
    middle->addLayout(queues, 1);
    middle->addWidget(servedBox);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(topRow);
    layout->addWidget(nowServing_);
    layout->addLayout(middle, 1);
    layout->addWidget(alertLabel_);

    connect(addButton, &QPushButton::clicked, this, &QueuePanel::addToQueue);
    connect(callButton, &QPushButton::clicked, this, &QueuePanel::callNext);

    // Timer event: update waiting times and long-wait alerts every 15 seconds.
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &QueuePanel::updateWaitingTimes);
    timer_->start(15000);

    refresh();
}

void QueuePanel::refresh() {
    QString current = patientCombo_->currentData().toString();
    patientCombo_->clear();
    for (model::PatientRecord* p : manager_.patients()) {
        patientCombo_->addItem(QString("%1 - %2 (%3)").arg(qs(p->id()), qs(p->name()),
                                                          qs(model::categoryToString(p->category()))),
                               qs(p->id()));
    }
    int index = patientCombo_->findData(current);
    if (index >= 0) patientCombo_->setCurrentIndex(index);
    updateWaitingTimes();
}

void QueuePanel::fillTable(QTableWidget* table, const std::vector<controller::QueueEntry>& entries) {
    table->setRowCount(0);
    std::time_t now = std::time(nullptr);
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const controller::QueueEntry& e = entries[i];
        long minutes = static_cast<long>(std::difftime(now, e.joinedAt) / 60);
        int row = table->rowCount();
        table->insertRow(row);
        QStringList values = {QString::number(i + 1), qs(e.patientId), qs(e.patientName),
                              QDateTime::fromSecsSinceEpoch(e.joinedAt).toString("HH:mm"),
                              QString("%1 min").arg(minutes)};
        for (int c = 0; c < values.size(); ++c) {
            auto* item = new QTableWidgetItem(values[c]);
            if (minutes >= kLongWaitMinutes) {   // highlight long waits
                item->setForeground(QColor("#B71C1C"));
                QFont bold = item->font();
                bold.setBold(true);
                item->setFont(bold);
            }
            table->setItem(row, c, item);
        }
    }
}

void QueuePanel::updateWaitingTimes() {
    std::vector<controller::QueueEntry> priority, regular;
    int longWaits = 0;
    std::time_t now = std::time(nullptr);
    for (const controller::QueueEntry& e : manager_.waitingList()) {
        (e.priority ? priority : regular).push_back(e);
        if (std::difftime(now, e.joinedAt) / 60 >= kLongWaitMinutes) ++longWaits;
    }
    fillTable(priorityTable_, priority);
    fillTable(regularTable_, regular);
    alertLabel_->setText(longWaits > 0 ? QString("⚠ %1 patient(s) have been waiting for more than %2 minutes.")
                                             .arg(longWaits).arg(kLongWaitMinutes)
                                       : QString());
}

void QueuePanel::addToQueue() {
    QString id = patientCombo_->currentData().toString();
    if (runGuarded(this, "Waiting Queue", [&] { manager_.enqueue(ss(id)); })) {
        emit statusMessage("Added " + id + " to the waiting queue");
        emit dataChanged();
    }
}

void QueuePanel::callNext() {
    controller::QueueEntry next;
    if (!runGuarded(this, "Call Next Patient", [&] { next = manager_.callNext(); })) return;
    long minutes = static_cast<long>(std::difftime(std::time(nullptr), next.joinedAt) / 60);
    QString text = QString("%1 (%2)").arg(qs(next.patientName), qs(next.patientId));
    nowServing_->setText("Now serving: " + text + (next.priority ? "  [EMERGENCY]" : ""));
    servedList_->insertItem(0, QTime::currentTime().toString("HH:mm") + "  " + text +
                                   QString(" - waited %1 min").arg(minutes));
    emit statusMessage("Please call " + text + " to the consultation room");
    emit dataChanged();
}

} // namespace scapms::gui
