#include "model/EmergencyPatient.h"

#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

namespace scapms::model {

using utils::ValidationException;

EmergencyPatient::EmergencyPatient(const std::string& id, const std::string& name, int age,
                                   const std::string& contact, int severity, const std::string& condition)
    : PatientRecord(id, name, age, contact) {
    setSeverity(severity);
    setCondition(condition);
}

void EmergencyPatient::setSeverity(int severity) {
    if (severity < 1 || severity > 5) {
        throw ValidationException("Emergency severity must be between 1 (critical) and 5 (minimal).");
    }
    severity_ = severity;
}

void EmergencyPatient::setCondition(const std::string& condition) {
    std::string value = utils::trim(condition);
    if (value.empty()) throw ValidationException("Please describe the emergency condition.");
    if (value.size() > 100) throw ValidationException("The condition must be at most 100 characters.");
    condition_ = value;
}

std::string EmergencyPatient::severityText() const {
    static const char* names[] = {"Critical", "Serious", "Moderate", "Minor", "Minimal"};
    return names[severity_ - 1];
}

std::string EmergencyPatient::details() const {
    return "Severity " + std::to_string(severity_) + " (" + severityText() + "): " + condition_;
}

std::vector<std::string> EmergencyPatient::extraFields() const {
    return {std::to_string(severity_), condition_};
}

void EmergencyPatient::setExtraFields(const std::vector<std::string>& fields) {
    int severity = 0;
    if (fields.empty() || !utils::parseInt(fields[0], severity)) {
        throw ValidationException("Emergency severity must be a whole number from 1 to 5.");
    }
    setSeverity(severity);
    setCondition(fields.size() > 1 ? fields[1] : "");
}

} // namespace scapms::model
