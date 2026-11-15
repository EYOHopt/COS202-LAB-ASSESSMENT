#pragma once
// "Search & Sort" tab: choose the searching/sorting algorithm and run it.
#include "gui/BasePanel.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QTableWidget;

namespace scapms::gui {

class SearchPanel : public BasePanel {
    Q_OBJECT
public:
    SearchPanel(controller::ClinicManager& manager, controller::AppointmentController& appointments,
                QWidget* parent = nullptr);

public slots:
    void refresh() override;
    void focusSearch();
    void runSearch();
    void runSort();

private slots:
    void onSearchFieldChanged();
    void onSortAlgorithmChanged();
    void onKeywordChanged(const QString& text);

private:
    void showPatients(const std::vector<model::PatientRecord*>& patients);

    QComboBox* fieldCombo_ = nullptr;
    QLineEdit* keywordEdit_ = nullptr;
    QComboBox* searchAlgorithmCombo_ = nullptr;
    QComboBox* sortAlgorithmCombo_ = nullptr;
    QComboBox* sortKeyCombo_ = nullptr;
    QComboBox* orderCombo_ = nullptr;
    QLabel* rationaleLabel_ = nullptr;
    QLabel* stateLabel_ = nullptr;
    QLabel* resultLabel_ = nullptr;
    QTableWidget* table_ = nullptr;
    bool showingSearchResults_ = false;
};

} // namespace scapms::gui
