#include "gui/AdminPanel.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

#include "gui/GuiUtils.h"

namespace scapms::gui {

AdminPanel::AdminPanel(controller::ClinicManager& manager, controller::AppointmentController& appointments,
                       QWidget* parent)
    : BasePanel(manager, appointments, parent) {
    setObjectName("adminPanel");

    undoList_ = new QListWidget;
    undoList_->setObjectName("undoList");
    undoButton_ = new QPushButton("Undo Last Administrative Operation  (Ctrl+Z)");
    undoButton_->setObjectName("undoButton");
    undoButton_->setToolTip("Reverses the operation at the top of the stack");
    auto* undoInfo = new QLabel("Stored on a std::stack: the most recent operation is on top and is undone first "
                                "(Last In, First Out).");
    undoInfo->setWordWrap(true);
    auto* undoBox = new QGroupBox("Undo stack (most recent first)");
    auto* undoLayout = new QVBoxLayout(undoBox);
    undoLayout->addWidget(undoInfo);
    undoLayout->addWidget(undoList_, 1);
    undoLayout->addWidget(undoButton_);

    cacheList_ = new QListWidget;
    cacheList_->setObjectName("cacheList");
    auto* cacheInfo = new QLabel(QString("Fixed-size array PatientRecord* recentPatients[%1]. Viewing or searching a "
                                         "patient moves it to slot 1; the least recently used drops out.")
                                     .arg(controller::ClinicManager::kCacheSize));
    cacheInfo->setWordWrap(true);
    auto* cacheBox = new QGroupBox("Frequently accessed patient records (cache)");
    auto* cacheLayout = new QVBoxLayout(cacheBox);
    cacheLayout->addWidget(cacheInfo);
    cacheLayout->addWidget(cacheList_, 1);

    auto* layout = new QHBoxLayout(this);
    layout->addWidget(undoBox, 3);
    layout->addWidget(cacheBox, 2);

    connect(undoButton_, &QPushButton::clicked, this, &AdminPanel::undoLast);
    refresh();
}

void AdminPanel::refresh() {
    undoList_->clear();
    std::vector<std::string> history = manager_.undoHistory();
    for (std::size_t i = 0; i < history.size(); ++i) {
        undoList_->addItem((i == 0 ? "▶ TOP   " : "         ") + qs(history[i]));
    }
    if (history.empty()) undoList_->addItem("(no operations to undo)");
    undoButton_->setEnabled(manager_.canUndo());

    cacheList_->clear();
    std::vector<model::PatientRecord*> recent = manager_.recentPatients();
    for (std::size_t i = 0; i < recent.size(); ++i) {
        QString text = QString("[%1]  ").arg(i + 1, 2);
        if (recent[i]) {
            text += qs(recent[i]->id() + "  " + recent[i]->name() + "  (" +
                       model::categoryToString(recent[i]->category()) + ")");
        } else {
            text += "- empty -";
        }
        auto* item = new QListWidgetItem(text);
        if (recent[i]) item->setBackground(categoryBackground(recent[i]->category()));
        cacheList_->addItem(item);
    }
}

void AdminPanel::undoLast() {
    std::string description;
    if (runGuarded(this, "Undo", [&] { description = manager_.undoLast(); })) {
        emit statusMessage("Undone: " + qs(description));
        emit dataChanged();
    }
}

} // namespace scapms::gui
