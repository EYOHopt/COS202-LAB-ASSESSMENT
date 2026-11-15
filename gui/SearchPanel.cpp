#include "gui/SearchPanel.h"

#include <QComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include "gui/GuiUtils.h"

namespace scapms::gui {

using controller::SearchField;
using controller::SearchMode;
using controller::SortAlgorithm;
using controller::SortKey;

SearchPanel::SearchPanel(controller::ClinicManager& manager, controller::AppointmentController& appointments,
                         QWidget* parent)
    : BasePanel(manager, appointments, parent) {
    setObjectName("searchPanel");

    // ----- Search group (QGridLayout) -----
    fieldCombo_ = new QComboBox;
    fieldCombo_->setObjectName("searchField");
    fieldCombo_->addItems({"Patient ID", "Patient Name", "Category"});
    keywordEdit_ = new QLineEdit;
    keywordEdit_->setObjectName("searchKeyword");
    keywordEdit_->setClearButtonEnabled(true);
    keywordEdit_->setToolTip("Press Enter to search (Ctrl+F jumps here)");
    searchAlgorithmCombo_ = new QComboBox;
    searchAlgorithmCombo_->setObjectName("searchAlgorithm");
    searchAlgorithmCombo_->addItems({"Auto (best for current data)", "Linear Search", "Binary Search",
                                     "Recursive Search"});
    searchAlgorithmCombo_->setToolTip("Auto: Binary search for IDs when sorted by ID, otherwise Linear; "
                                      "Recursive search for names");
    auto* searchButton = new QPushButton("SEARCH");
    searchButton->setObjectName("searchButton");
    searchButton->setToolTip("Run the selected search algorithm");

    auto* searchBox = new QGroupBox("Search patient records");
    auto* grid = new QGridLayout(searchBox);
    grid->addWidget(new QLabel("Search by:"), 0, 0);
    grid->addWidget(fieldCombo_, 0, 1);
    grid->addWidget(new QLabel("Keyword:"), 1, 0);
    grid->addWidget(keywordEdit_, 1, 1);
    grid->addWidget(new QLabel("Search algorithm:"), 2, 0);
    grid->addWidget(searchAlgorithmCombo_, 2, 1);
    grid->addWidget(searchButton, 3, 1);
    grid->setColumnStretch(1, 1);

    // ----- Sort group (QFormLayout) -----
    sortAlgorithmCombo_ = new QComboBox;
    sortAlgorithmCombo_->setObjectName("sortAlgorithm");
    sortAlgorithmCombo_->addItems({"Selection Sort", "Insertion Sort", "Merge Sort", "Quick Sort"});
    sortKeyCombo_ = new QComboBox;
    sortKeyCombo_->setObjectName("sortKey");
    sortKeyCombo_->addItems({"Patient Name", "Patient ID", "Age", "Next Appointment Date"});
    orderCombo_ = new QComboBox;
    orderCombo_->setObjectName("sortOrder");
    orderCombo_->addItems({"Ascending", "Descending"});
    auto* sortButton = new QPushButton("SORT");
    sortButton->setObjectName("sortButton");
    sortButton->setToolTip("Sort all patient records with the selected algorithm");
    rationaleLabel_ = new QLabel;
    rationaleLabel_->setObjectName("sortRationale");
    rationaleLabel_->setWordWrap(true);
    rationaleLabel_->setStyleSheet("color:#37474F; font-style:italic;");

    auto* sortBox = new QGroupBox("Sort patient records");
    auto* form = new QFormLayout(sortBox);
    form->addRow("Sort algorithm:", sortAlgorithmCombo_);
    form->addRow("Sort by:", sortKeyCombo_);
    form->addRow("Order:", orderCombo_);
    form->addRow("", sortButton);
    form->addRow("Why:", rationaleLabel_);

    auto* top = new QHBoxLayout;
    top->addWidget(searchBox, 1);
    top->addWidget(sortBox, 1);

    stateLabel_ = new QLabel;
    stateLabel_->setObjectName("sortState");
    stateLabel_->setStyleSheet("background:#ECEFF1; padding:4px; border-radius:3px;");
    resultLabel_ = new QLabel("Choose an algorithm and press SEARCH or SORT.");
    resultLabel_->setObjectName("resultLabel");
    resultLabel_->setWordWrap(true);
    resultLabel_->setStyleSheet("font-weight:bold; color:#1B5E20;");

    table_ = new QTableWidget(0, 6);
    table_->setObjectName("searchTable");
    table_->setHorizontalHeaderLabels({"Patient ID", "Name", "Age", "Category", "Next Appointment", "Details"});
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSortingEnabled(false);
    table_->verticalHeader()->setVisible(false);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(top);
    layout->addWidget(stateLabel_);
    layout->addWidget(resultLabel_);
    layout->addWidget(table_, 1);

    connect(searchButton, &QPushButton::clicked, this, &SearchPanel::runSearch);
    connect(keywordEdit_, &QLineEdit::returnPressed, this, &SearchPanel::runSearch);
    connect(keywordEdit_, &QLineEdit::textChanged, this, &SearchPanel::onKeywordChanged);
    connect(fieldCombo_, &QComboBox::currentIndexChanged, this, &SearchPanel::onSearchFieldChanged);
    connect(sortButton, &QPushButton::clicked, this, &SearchPanel::runSort);
    connect(sortAlgorithmCombo_, &QComboBox::currentIndexChanged, this, &SearchPanel::onSortAlgorithmChanged);

    onSearchFieldChanged();
    onSortAlgorithmChanged();
    refresh();
}

void SearchPanel::refresh() {
    stateLabel_->setText("Current order of records: " + qs(manager_.sortStateDescription()));
    if (!showingSearchResults_) showPatients(manager_.patients());
}

void SearchPanel::focusSearch() {
    keywordEdit_->setFocus();
    keywordEdit_->selectAll();
}

void SearchPanel::onSearchFieldChanged() {
    static const char* hints[] = {"e.g. P0003", "e.g. ade (part of a name)", "General, Emergency or Follow-Up"};
    keywordEdit_->setPlaceholderText(hints[fieldCombo_->currentIndex()]);
}

void SearchPanel::onSortAlgorithmChanged() {
    rationaleLabel_->setText(qs(controller::algorithmRationale(
        static_cast<SortAlgorithm>(sortAlgorithmCombo_->currentIndex()))));
}

void SearchPanel::onKeywordChanged(const QString& text) {
    if (text.trimmed().isEmpty() && showingSearchResults_) {   // keyword cleared: show everything again
        showingSearchResults_ = false;
        resultLabel_->setText("Showing all patients.");
        refresh();
    }
}

void SearchPanel::showPatients(const std::vector<model::PatientRecord*>& patients) {
    table_->setRowCount(0);
    for (model::PatientRecord* p : patients) {
        int row = table_->rowCount();
        table_->insertRow(row);
        std::string next = p->nextPendingDateTime();
        QStringList values = {qs(p->id()), qs(p->name()), QString::number(p->age()),
                              qs(model::categoryToString(p->category())), next.empty() ? "-" : qs(next),
                              qs(p->details())};
        for (int c = 0; c < values.size(); ++c) {
            auto* item = new QTableWidgetItem(values[c]);
            item->setBackground(categoryBackground(p->category()));
            table_->setItem(row, c, item);
        }
    }
}

void SearchPanel::runSearch() {
    controller::SearchResult result;
    bool ok = runGuarded(this, "Search", [&] {
        result = manager_.search(static_cast<SearchField>(fieldCombo_->currentIndex()), ss(keywordEdit_->text()),
                                 static_cast<SearchMode>(searchAlgorithmCombo_->currentIndex()));
    });
    if (!ok) return;
    showingSearchResults_ = true;
    showPatients(result.matches);
    QString summary = QString("%1 record(s) found for \"%2\" using %3 - %4 comparison(s) on %5 records.")
                          .arg(result.matches.size())
                          .arg(keywordEdit_->text().trimmed(), qs(result.algorithm))
                          .arg(result.comparisons)
                          .arg(manager_.patientCount());
    resultLabel_->setText(summary);
    for (model::PatientRecord* p : result.matches) manager_.touch(p);
    emit statusMessage(summary);
}

void SearchPanel::runSort() {
    controller::SortStats stats;
    bool ok = runGuarded(this, "Sort", [&] {
        stats = manager_.sortPatients(static_cast<SortAlgorithm>(sortAlgorithmCombo_->currentIndex()),
                                      static_cast<SortKey>(sortKeyCombo_->currentIndex()),
                                      orderCombo_->currentIndex() == 0);
    });
    if (!ok) return;
    showingSearchResults_ = false;
    keywordEdit_->blockSignals(true);
    keywordEdit_->clear();
    keywordEdit_->blockSignals(false);
    QString summary = QString("Sorted %1 records by %2 using %3: %4 comparisons in %5 ms.")
                          .arg(stats.records)
                          .arg(sortKeyCombo_->currentText(), qs(stats.algorithm))
                          .arg(stats.comparisons)
                          .arg(stats.milliseconds, 0, 'f', 3);
    resultLabel_->setText(summary);
    emit statusMessage(summary);
    emit dataChanged();
}

} // namespace scapms::gui
