#ifndef PRINT_H_
#define PRINT_H_

#include <iosfwd>

#include "simulator.h"

namespace smo {
void PrintUsage(std::ostream& out);
void PrintHelp(std::ostream& out);
void PrintEvent(std::ostream& out, const smo::SpecialEvent& event);
void PrintSimulationState(std::ostream& out, const smo::Simulator& simulator);
void PrintReport(std::ostream& out, const smo::Simulator& simulator);
void PrintRealBuffer(std::ostream& out, const smo::Simulator& simulator);
}  // namespace smo

#endif
