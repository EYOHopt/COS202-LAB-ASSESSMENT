#include "utils/FileHandler.h"

#include <fstream>

#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

namespace scapms::utils {

namespace {
const char* kHeader = "#SCAPMS,1";

std::string joinFields(const std::vector<std::string>& fields) {
    std::string line;
    for (std::size_t i = 0; i < fields.size(); ++i) {
        if (i > 0) line += ',';
        line += FileHandler::escapeField(fields[i]);
    }
    return line;
}
} // namespace

std::string FileHandler::escapeField(const std::string& field) {
    bool needsQuotes = false;
    for (char c : field) {
        if (c == ',' || c == '"' || c == '\n' || c == '\r') needsQuotes = true;
    }
    if (!needsQuotes) return field;
    std::string escaped = "\"";
    for (char c : field) {
        if (c == '"') escaped += "\"\"";
        else if (c == '\n' || c == '\r') escaped += ' ';
        else escaped += c;
    }
    return escaped + "\"";
}

std::vector<std::string> FileHandler::splitCsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    bool inQuotes = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (inQuotes) {
            if (c == '"' && i + 1 < line.size() && line[i + 1] == '"') { current += '"'; ++i; }
            else if (c == '"') inQuotes = false;
            else current += c;
        } else if (c == '"') {
            inQuotes = true;
        } else if (c == ',') {
            fields.push_back(current);
            current.clear();
        } else if (c != '\r') {
            current += c;
        }
    }
    fields.push_back(current);
    return fields;
}

void FileHandler::save(const std::string& path, const model::ClinicDatabase& database) {
    std::ofstream out(path);
    if (!out) throw FileException("Could not open '" + path + "' for writing.");

    out << kHeader << '\n';
    for (const model::PatientRecord* patient : database.view()) {
        std::vector<std::string> fields = {"P", patient->id(), model::categoryToString(patient->category()),
                                           patient->name(), std::to_string(patient->age()), patient->contact()};
        for (const std::string& extra : patient->extraFields()) fields.push_back(extra);
        out << joinFields(fields) << '\n';
    }
    for (const model::PatientRecord* patient : database.view()) {
        for (const model::Appointment& a : patient->appointments()) {
            out << joinFields({"A", a.id(), a.patientId(), a.date(), a.time(), a.doctor(), a.reason(),
                               model::statusToString(a.status())}) << '\n';
        }
    }
    if (!out.good()) throw FileException("An error occurred while writing '" + path + "'.");
}

LoadResult FileHandler::load(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw FileException("Could not open '" + path + "'. Check that the file exists and can be read.");
    }
    std::string line;
    if (!std::getline(in, line) || trim(line).rfind("#SCAPMS", 0) != 0) {
        throw FileException("'" + path + "' is not a SCAPMS data file (the #SCAPMS header is missing).");
    }

    LoadResult result;
    auto findLoaded = [&result](const std::string& id) -> model::PatientRecord* {
        for (auto& p : result.patients) {
            if (p->id() == id) return p.get();
        }
        return nullptr;
    };

    while (std::getline(in, line)) {
        if (trim(line).empty() || line[0] == '#') continue;
        std::vector<std::string> f = splitCsvLine(line);
        try {
            if (f[0] == "P" && f.size() >= 6) {
                int age = 0;
                if (!parseInt(f[4], age)) throw ValidationException("Invalid age '" + f[4] + "'.");
                auto patient = model::createPatient(model::categoryFromString(f[2]), f[1], f[3], age, f[5]);
                patient->setExtraFields(std::vector<std::string>(f.begin() + 6, f.end()));
                if (findLoaded(patient->id())) throw DuplicateIdException(patient->id());
                result.patients.push_back(std::move(patient));
            } else if (f[0] == "A" && f.size() >= 8) {
                model::PatientRecord* owner = findLoaded(f[2]);
                if (!owner) throw NotFoundException("Appointment for unknown patient " + f[2]);
                owner->attachAppointment(model::Appointment(f[1], f[2], f[3], f[4], f[5], f[6],
                                                            model::statusFromString(f[7])));
            } else {
                ++result.skippedLines;
            }
        } catch (const ClinicException&) {
            ++result.skippedLines;   // invalid record: skip it and keep loading the rest
        }
    }
    return result;
}

void FileHandler::exportPatientsCsv(const std::string& path, const model::ClinicDatabase& database) {
    std::ofstream out(path);
    if (!out) throw FileException("Could not open '" + path + "' for writing.");
    out << "PatientID,Name,Age,Category,Priority,Contact,Details,Appointments,Completed,Pending\n";
    for (const model::PatientRecord* p : database.view()) {
        out << joinFields({p->id(), p->name(), std::to_string(p->age()), model::categoryToString(p->category()),
                           p->priorityLabel(), p->contact(), p->details(),
                           std::to_string(p->appointments().size()),
                           std::to_string(p->countAppointments(model::AppointmentStatus::Completed)),
                           std::to_string(p->countAppointments(model::AppointmentStatus::Pending))})
            << '\n';
    }
    if (!out.good()) throw FileException("An error occurred while writing '" + path + "'.");
}

} // namespace scapms::utils
