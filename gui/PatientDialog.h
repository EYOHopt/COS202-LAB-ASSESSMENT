#pragma once
// Dialog for registering or editing a patient. The type-specific fields change
// dynamically when a different patient category is selected.
#include <QDialog>
#include <functional>
#include <memory>

#include "controller/ClinicManager.h"

class QComboBox;
class QFormLayout;
class QLabel;
class QLineEdit;
class QSpinBox;

namespace scapms::gui {

class PatientDialog : public QDialog {
    Q_OBJECT
public:
    using CommitFunction = std::function<void(std::unique_ptr<model::PatientRecord>)>;

    // existing == nullptr  => register a new patient; otherwise edit that patient.
    PatientDialog(controller::ClinicManager& manager, const model::PatientRecord* existing, QWidget* parent = nullptr);
    void setCommitFunction(CommitFunction commit) { commit_ = std::move(commit); }

private slots:
    void onCategoryChanged();
    void onIdChanged(const QString& text);
    void onAccept();

private:
    model::PatientCategory selectedCategory() const;
    std::unique_ptr<model::PatientRecord> buildPatient() const;

    controller::ClinicManager& manager_;
    bool editMode_ = false;
    CommitFunction commit_;

    QFormLayout* form_ = nullptr;
    QComboBox* categoryCombo_ = nullptr;
    QLineEdit* idEdit_ = nullptr;
    QLabel* idStatus_ = nullptr;
    QLineEdit* nameEdit_ = nullptr;
    QLineEdit* ageEdit_ = nullptr;
    QLineEdit* contactEdit_ = nullptr;
    QLineEdit* complaintEdit_ = nullptr;     // General
    QSpinBox* severitySpin_ = nullptr;       // Emergency
    QLineEdit* conditionEdit_ = nullptr;     // Emergency
    QLineEdit* diagnosisEdit_ = nullptr;     // Follow-Up
    QSpinBox* intervalSpin_ = nullptr;       // Follow-Up
};

} // namespace scapms::gui
