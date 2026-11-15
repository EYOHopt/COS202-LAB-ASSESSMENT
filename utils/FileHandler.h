#pragma once
// Saves and loads clinic data as a CSV-style text file.
//
// File format (one record per line):
//   #SCAPMS,1
//   P,<id>,<category>,<name>,<age>,<contact>,<extra1>,<extra2>
//   A,<id>,<patientId>,<date>,<time>,<doctor>,<reason>,<status>
#include <memory>
#include <string>
#include <vector>

#include "model/ClinicDatabase.h"

namespace scapms::utils {

struct LoadResult {
    std::vector<std::unique_ptr<model::PatientRecord>> patients;
    int skippedLines = 0;   // malformed or invalid lines that were ignored
};

class FileHandler {
public:
    static void save(const std::string& path, const model::ClinicDatabase& database);
    static LoadResult load(const std::string& path);
    static void exportPatientsCsv(const std::string& path, const model::ClinicDatabase& database);

    static std::string escapeField(const std::string& field);
    static std::vector<std::string> splitCsvLine(const std::string& line);
};

} // namespace scapms::utils
