#include <algorithm>
#include <cstddef>
#include <deque>
#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <tabulate/table.hpp>
#include <vector>

#include "arguments_parser.h"
#include "return_codes.h"
#include "simulator.h"
#include "simulator_config.h"
#include "smo_components.h"

void PrintUsage(std::ostream& out);
void PrintReport(std::ostream& out, const smo::Simulator& simulator);
void PrintSourceCalendar(std::ostream& out, const smo::Simulator& simulator);
void PrintDeviceCalendar(std::ostream& out, const smo::Simulator& simulator);
void PrintFakeBuffer(std::ostream& out, const smo::Simulator& simulator);
void PrintRealBuffer(std::ostream& out, const smo::Simulator& simulator);
int main(int argc, char** argv) {
  parse::Arguments args;
  auto parse_result = args.Parse(argc, argv);
  if (parse_result) {
    return parse_result;
  }
  smo::SimulatorConfig config;
  std::cin >> config;
  if (!std::cin) {
    return codes::configError;
  }
  smo::Simulator simulator(std::move(config));
  switch (args.mode) {
    case parse::SimulationMode::runToCompletion:
      simulator.RunToCompletion();
      break;
    case parse::SimulationMode::interactive:
      break;
    case parse::SimulationMode::automatic:
      break;
  }
  if (args.need_output) {
    if (args.reportFile.has_value()) {
      PrintReport(*args.reportFile, simulator);
    } else {
      PrintReport(std::cout, simulator);
    }
  }
}
void PrintUsage(std::ostream& out) {
  out << "Usage: simulator [-i|-a|-r] [-o [outfile]] infile\n";
}

void PrintSourceReport(std::ostream& out, const smo::Simulator& simulator) {
  tabulate::Table table;
  table.add_row({"i", "Request\namount", "Rejection\nprobability", "Time\nfull",
                 "Time\nbuffer", "Time\nprocessing", "Variance\nbuffer",
                 "Variance\nprocessing"});
  const auto& sources = simulator.source_statistics();
  for (std::size_t i = 0; i < sources.size(); ++i) {
    const auto& source = sources[i];
    auto buffer_time = source.AverageBufferTime();
    auto device_time = source.AverageDeviceTime();
    table.add_row(tabulate::RowStream{}
                  << i << source.generated
                  << static_cast<double>(source.generated) / source.rejected
                  << buffer_time + device_time << buffer_time << device_time
                  << source.BufferTimeVariance()
                  << source.DeviceTimeVariance());
  }
  out << table;
}
void PrintDeviceReport(std::ostream& out, const smo::Simulator& simulator) {
  tabulate::Table table;
  table.add_row({"i", "Usage coefficient"});
  const auto& devices = simulator.device_statistics();
  for (std::size_t i = 0; i < devices.size(); ++i) {
    const auto& device = devices[i];
    table.add_row({std::to_string(i),
                   std::to_string(device.time_in_usage /
                                  simulator.current_simulation_time())});
  }
  out << table;
}
void PrintReport(std::ostream& out, const smo::Simulator& simulator) {
  out << "Sources:\n";
  PrintSourceReport(out, simulator);
  out << "Devices:\n";
  PrintDeviceReport(out, simulator);
}
void PrintSourceCalendar(std::ostream& out, const smo::Simulator& simulator) {
  tabulate::Table table;
  table.add_row({"i", "Next event", "Sign"});
  const auto& sources = simulator.source_statistics();
  for (std::size_t i = 0; i < sources.size(); ++i) {
    const auto& source = sources[i];
    table.add_row(tabulate::RowStream{}
                  << i << source.next_request.count()
                  << (source.next_request == smo::Time::max() ? 1 : 0));
  }
  out << table;
}
std::string FormatRequest(const std::optional<smo::Request>& req) {
  if (req.has_value()) {
    return std::to_string(req->source_id) + "." + std::to_string(req->number);
  } else {
    return "None";
  }
}
void PrintDeviceCalendar(std::ostream& out, const smo::Simulator& simulator) {
  tabulate::Table table;
  table.add_row({"i", "Next event", "Sign", "Request"});
  const auto& devices = simulator.device_statistics();
  for (std::size_t i = 0; i < devices.size(); ++i) {
    const auto& device = devices[i];
    table.add_row(tabulate::RowStream{}
                  << i << device.next_request.count()
                  << (device.next_request == smo::Time::max() ? 1 : 0)
                  << FormatRequest(device.current_request));
  }
  out << table;
}
void PrintFakeBuffer(std::ostream& out, const smo::Simulator& simulator) {
  tabulate::Table table;
  tabulate::RowStream index_row;
  tabulate::RowStream value_row;

  index_row << "i:";
  value_row << "Values:";

  auto fake_buffer = simulator.FakeBuffer();
  for (std::size_t i = 0; i < fake_buffer.size(); ++i) {
    index_row << i;
    value_row << FormatRequest(fake_buffer[i]);
  }
  for (std::size_t i = fake_buffer.size(); i < fake_buffer.capacity(); ++i) {
    index_row << i;
    value_row << FormatRequest(std::nullopt);
  }
  table.add_row(index_row);
  table.add_row(value_row);
  out << table;
}
void PrintRealBuffer(std::ostream& out, const smo::Simulator& simulator) {
  const auto& real_buffer = simulator.RealBuffer();
  using Subbuffer = std::deque<smo::Request>;
  std::size_t max_size = 0;
  for (const auto& subbuffer : real_buffer) {
    max_size = std::max(max_size, subbuffer.size());
  }

  tabulate::Table table;
  tabulate::RowStream index_row;
  index_row << "i:";
  for (std::size_t i = 0; i < max_size; ++i) {
    index_row << i;
  }
  table.add_row(index_row);
  for (std::size_t i = 0; i < real_buffer.size(); ++i) {
    tabulate::RowStream value_row;
    value_row << "Packet " + std::to_string(i) + ":";
    for (const auto& req : real_buffer[i]) {
      value_row << FormatRequest(req);
    }
    table.add_row(value_row);
  }
  out << table;
}
