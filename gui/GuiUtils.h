#pragma once
// Helpers shared by the GUI classes: string conversion, colours and
// a single place where exceptions are turned into user-friendly dialogs.
#include <QColor>
#include <QMessageBox>
#include <QString>
#include <QWidget>
#include <string>

#include "model/PatientRecord.h"
#include "utils/Exceptions.h"

namespace scapms::gui {

inline QString qs(const std::string& text) { return QString::fromStdString(text); }
inline std::string ss(const QString& text) { return text.toStdString(); }

// Custom presentation: background colour per patient category.
inline QColor categoryBackground(model::PatientCategory category) {
    switch (category) {
    case model::PatientCategory::Emergency: return QColor("#FDE4E2");
    case model::PatientCategory::FollowUp: return QColor("#E4EEFC");
    case model::PatientCategory::General: break;
    }
    return QColor("#E7F5E9");
}

inline QColor categoryAccent(model::PatientCategory category) {
    switch (category) {
    case model::PatientCategory::Emergency: return QColor("#B71C1C");
    case model::PatientCategory::FollowUp: return QColor("#0D47A1");
    case model::PatientCategory::General: break;
    }
    return QColor("#1B5E20");
}

inline QColor statusBackground(model::AppointmentStatus status) {
    switch (status) {
    case model::AppointmentStatus::Pending: return QColor("#FFF3D6");
    case model::AppointmentStatus::Completed: return QColor("#E7F5E9");
    case model::AppointmentStatus::Cancelled: return QColor("#ECECEC");
    }
    return QColor(Qt::white);
}

// Runs an operation and shows a meaningful dialog for every kind of error.
// Returns true if the operation succeeded.
template <typename Operation>
bool runGuarded(QWidget* parent, const QString& action, Operation&& operation) {
    try {
        operation();
        return true;
    } catch (const utils::DuplicateIdException& e) {
        QMessageBox::warning(parent, "Duplicate Patient ID", "Error: " + qs(e.what()));
    } catch (const utils::ValidationException& e) {   // also catches InvalidAgeException
        QMessageBox::warning(parent, "Invalid Patient Information", "Error: " + qs(e.what()));
    } catch (const utils::AppointmentException& e) {
        QMessageBox::warning(parent, "Invalid Appointment", "Error: " + qs(e.what()));
    } catch (const utils::SearchException& e) {
        QMessageBox::warning(parent, "Invalid Search", "Error: " + qs(e.what()));
    } catch (const utils::FileException& e) {
        QMessageBox::critical(parent, "File Error", "Error: " + qs(e.what()));
    } catch (const utils::ClinicException& e) {
        QMessageBox::warning(parent, action, qs(e.what()));
    } catch (const std::exception& e) {
        QMessageBox::critical(parent, action, "Unexpected error: " + qs(e.what()));
    } catch (...) {
        QMessageBox::critical(parent, action, "An unknown error occurred. The operation was cancelled.");
    }
    return false;
}

} // namespace scapms::gui
