#include "gui/PatientDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "gui/GuiUtils.h"
#include "utils/StringUtils.h"

namespace scapms::gui {

using model::PatientCategory;

PatientDialog::PatientDialog(controller::ClinicManager& manager, const model::PatientRecord* existing, QWidget* parent)
    : QDialog(parent), manager_(manager), editMode_(existing != nullptr) {
    setWindowTitle(editMode_ ? "Edit Patient" : "Register New Patient");
    setObjectName("patientDialog");
    setMinimumWidth(460);

    auto* title = new QLabel(editMode_ ? "<b>Edit patient record</b>" : "<b>Register a new patient</b>");
    title->setStyleSheet("font-size: 15px; padding-bottom: 4px;");

    categoryCombo_ = new QComboBox;
    categoryCombo_->setObjectName("categoryCombo");
    categoryCombo_->addItem("General", static_cast<int>(PatientCategory::General));
    categoryCombo_->addItem("Emergency", static_cast<int>(PatientCategory::Emergency));
    categoryCombo_->addItem("Follow-Up", static_cast<int>(PatientCategory::FollowUp));
    categoryCombo_->setToolTip("The form shows different fields for each patient type");

    idEdit_ = new QLineEdit;
    idEdit_->setObjectName("idEdit");
    idEdit_->setToolTip("Unique patient ID, e.g. P0001");
    idStatus_ = new QLabel;
    idStatus_->setMinimumWidth(130);
    auto* idRow = new QWidget;
    auto* idLayout = new QHBoxLayout(idRow);
    idLayout->setContentsMargins(0, 0, 0, 0);
    idLayout->addWidget(idEdit_, 1);
    idLayout->addWidget(idStatus_);

    nameEdit_ = new QLineEdit;
    nameEdit_->setObjectName("nameEdit");
    nameEdit_->setPlaceholderText("Full name");
    ageEdit_ = new QLineEdit;
    ageEdit_->setObjectName("ageEdit");
    ageEdit_->setPlaceholderText("0 - 120");
    ageEdit_->setToolTip("Age in years (0 to 120)");
    contactEdit_ = new QLineEdit;
    contactEdit_->setObjectName("contactEdit");
    contactEdit_->setPlaceholderText("Phone number or email");

    complaintEdit_ = new QLineEdit;
    complaintEdit_->setObjectName("complaintEdit");
    complaintEdit_->setPlaceholderText("e.g. Fever and headache (optional)");
    severitySpin_ = new QSpinBox;
    severitySpin_->setObjectName("severitySpin");
    severitySpin_->setRange(1, 5);
    severitySpin_->setValue(3);
    severitySpin_->setToolTip("1 = Critical, 2 = Serious, 3 = Moderate, 4 = Minor, 5 = Minimal");
    conditionEdit_ = new QLineEdit;
    conditionEdit_->setObjectName("conditionEdit");
    conditionEdit_->setPlaceholderText("e.g. Severe chest pain");
    diagnosisEdit_ = new QLineEdit;
    diagnosisEdit_->setObjectName("diagnosisEdit");
    diagnosisEdit_->setPlaceholderText("e.g. Hypertension");
    intervalSpin_ = new QSpinBox;
    intervalSpin_->setObjectName("intervalSpin");
    intervalSpin_->setRange(1, 365);
    intervalSpin_->setValue(14);
    intervalSpin_->setSuffix(" days");

    // QFormLayout: label/field pairs aligned in two columns.
    form_ = new QFormLayout;
    form_->addRow("Patient type:", categoryCombo_);
    form_->addRow("Patient ID:", idRow);
    form_->addRow("Full name:", nameEdit_);
    form_->addRow("Age:", ageEdit_);
    form_->addRow("Contact:", contactEdit_);
    form_->addRow("Complaint:", complaintEdit_);
    form_->addRow("Severity (1-5):", severitySpin_);
    form_->addRow("Condition:", conditionEdit_);
    form_->addRow("Previous diagnosis:", diagnosisEdit_);
    form_->addRow("Review interval:", intervalSpin_);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Save)->setObjectName("saveButton");
    buttons->button(QDialogButtonBox::Save)->setText(editMode_ ? "Save Changes" : "Register Patient");

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(title);
    layout->addLayout(form_);
    layout->addWidget(buttons);

    connect(categoryCombo_, &QComboBox::currentIndexChanged, this, &PatientDialog::onCategoryChanged);
    connect(idEdit_, &QLineEdit::textChanged, this, &PatientDialog::onIdChanged);
    connect(buttons, &QDialogButtonBox::accepted, this, &PatientDialog::onAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    if (editMode_) {
        categoryCombo_->setCurrentIndex(static_cast<int>(existing->category()));
        categoryCombo_->setEnabled(false);
        idEdit_->setText(qs(existing->id()));
        idEdit_->setEnabled(false);
        nameEdit_->setText(qs(existing->name()));
        ageEdit_->setText(QString::number(existing->age()));
        contactEdit_->setText(qs(existing->contact()));
        std::vector<std::string> extra = existing->extraFields();
        switch (existing->category()) {
        case PatientCategory::General:
            if (!extra.empty()) complaintEdit_->setText(qs(extra[0]));
            break;
        case PatientCategory::Emergency:
            if (extra.size() >= 2) { severitySpin_->setValue(std::stoi(extra[0])); conditionEdit_->setText(qs(extra[1])); }
            break;
        case PatientCategory::FollowUp:
            if (extra.size() >= 2) { diagnosisEdit_->setText(qs(extra[0])); intervalSpin_->setValue(std::stoi(extra[1])); }
            break;
        }
    } else {
        idEdit_->setText(qs(manager_.suggestPatientId()));
    }
    onCategoryChanged();
}

PatientCategory PatientDialog::selectedCategory() const {
    return static_cast<PatientCategory>(categoryCombo_->currentData().toInt());
}

// Dynamic components: show only the fields that belong to the selected patient type.
void PatientDialog::onCategoryChanged() {
    PatientCategory category = selectedCategory();
    form_->setRowVisible(complaintEdit_, category == PatientCategory::General);
    form_->setRowVisible(severitySpin_, category == PatientCategory::Emergency);
    form_->setRowVisible(conditionEdit_, category == PatientCategory::Emergency);
    form_->setRowVisible(diagnosisEdit_, category == PatientCategory::FollowUp);
    form_->setRowVisible(intervalSpin_, category == PatientCategory::FollowUp);
    adjustSize();
}

// Text field event: check the ID while the user types.
void PatientDialog::onIdChanged(const QString& text) {
    if (editMode_) { idStatus_->clear(); return; }
    std::string id;
    try {
        id = model::PatientRecord::validateId(ss(text));
    } catch (const utils::ValidationException&) {
        idStatus_->setText("<span style='color:#B71C1C'>✗ Invalid format</span>");
        return;
    }
    if (manager_.findPatient(id)) idStatus_->setText("<span style='color:#B71C1C'>✗ ID already exists</span>");
    else idStatus_->setText("<span style='color:#1B5E20'>✓ ID available</span>");
}

std::unique_ptr<model::PatientRecord> PatientDialog::buildPatient() const {
    int age = 0;
    if (!utils::parseInt(ss(ageEdit_->text()), age)) {
        throw utils::ValidationException("Patient age must be a whole number between 0 and 120.");
    }
    auto patient = model::createPatient(selectedCategory(), ss(idEdit_->text()), ss(nameEdit_->text()), age,
                                        ss(contactEdit_->text()));
    switch (selectedCategory()) {
    case PatientCategory::General:
        patient->setExtraFields({ss(complaintEdit_->text())});
        break;
    case PatientCategory::Emergency:
        patient->setExtraFields({std::to_string(severitySpin_->value()), ss(conditionEdit_->text())});
        break;
    case PatientCategory::FollowUp:
        patient->setExtraFields({ss(diagnosisEdit_->text()), std::to_string(intervalSpin_->value())});
        break;
    }
    return patient;
}

// The dialog stays open if the data is invalid, so the user can correct it.
void PatientDialog::onAccept() {
    bool saved = runGuarded(this, "Save Patient", [this] {
        auto patient = buildPatient();
        if (commit_) commit_(std::move(patient));
    });
    if (saved) accept();
}

} // namespace scapms::gui
