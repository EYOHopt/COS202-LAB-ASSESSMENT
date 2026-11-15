# SCAPMS – Smart Clinic Appointment & Patient Management System

COS202 Programming II – Lab Assessment Project. A C++17 / Qt 6 Widgets desktop application for a university clinic.

## Features
- Register, edit and delete patients (General, Emergency, Follow-Up)
- Schedule, cancel, complete and remove appointments
- Waiting queue (`std::queue`): emergency patients are served first
- Search by ID, name or category: hand-written linear, binary and recursive search
- Sort by name, ID, age or next appointment date: hand-written selection, insertion, merge and quick sort
- Undo last administrative operation (`std::stack`)
- Frequently accessed patients cache (fixed-size array)
- Dashboard with timers and reminders, four reports, save/load/export (CSV)
- Input validation with custom exception classes

## Project structure
| Folder | Namespace | Contents |
|---|---|---|
| `model/` | `scapms::model` | `PatientRecord` (abstract), `GeneralPatient`, `EmergencyPatient`, `FollowUpPatient`, `Appointable` (interface), `Appointment`, `ClinicDatabase` |
| `controller/` | `scapms::controller` | `ClinicManager`, `AppointmentController`, `SearchEngine`, `Sorter`, `ReportGenerator` |
| `gui/` | `scapms::gui` | `MainWindow`, `BasePanel` and the seven tab panels, `PatientDialog` |
| `utils/` | `scapms::utils` | `FileHandler`, `IDGenerator`, `Exceptions`, `StringUtils` |
| `data/` | | `clinic_data.csv` sample data (loaded automatically) |

## Build and run
Requirements: a C++17 compiler, **Qt 6.4 or later** (Widgets) and CMake 3.16+.

**Qt Creator:** File → Open File or Project → select `CMakeLists.txt` → Build → Run.

**Command line:**
```
cmake -S . -B build
cmake --build build
./build/SCAPMS        (Windows: build\SCAPMS.exe)
```
