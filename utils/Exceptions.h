#pragma once
// Custom exception hierarchy used throughout SCAPMS.
// Every error the user can cause is reported with one of these classes,
// so the GUI can show a meaningful message instead of crashing.
#include <stdexcept>
#include <string>

namespace scapms::utils {

// Base class for all clinic-specific errors.
class ClinicException : public std::runtime_error {
public:
    explicit ClinicException(const std::string& message) : std::runtime_error(message) {}
};

// Invalid patient information (name, contact, ID format, ...).
class ValidationException : public ClinicException {
public:
    using ClinicException::ClinicException;
};

// Age outside the allowed range 0 - 120.
class InvalidAgeException : public ValidationException {
public:
    explicit InvalidAgeException(int age)
        : ValidationException("Patient age must be between 0 and 120 (you entered "
                              + std::to_string(age) + ").") {}
};

// A patient with the same ID is already registered.
class DuplicateIdException : public ClinicException {
public:
    explicit DuplicateIdException(const std::string& id)
        : ClinicException("Patient ID '" + id + "' already exists.") {}
};

// Invalid appointment data (date, time, doctor, booking rules).
class AppointmentException : public ClinicException {
public:
    using ClinicException::ClinicException;
};

// File could not be opened, read or written.
class FileException : public ClinicException {
public:
    using ClinicException::ClinicException;
};

// Invalid search input or search not possible in the current state.
class SearchException : public ClinicException {
public:
    using ClinicException::ClinicException;
};

// A requested patient or appointment does not exist.
class NotFoundException : public ClinicException {
public:
    using ClinicException::ClinicException;
};

} // namespace scapms::utils
