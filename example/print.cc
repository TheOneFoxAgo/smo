#include <cstddef>
#include <optional>
#include <ostream>
#include <string>
#include <tabulate/font_style.hpp>
#include <tabulate/row.hpp>
#include <tabulate/table.hpp>
#include <vector>

#include "print.h"

void smo::PrintUsage(std::ostream& out) {
  out << "Usage: simulator [-i|-a] [-d] [-o [outfile]] [-m max_requests] "
         "infile\n";
}
void smo::PrintHelp(std::ostream& out) {
  out << "Interactive mode comands:\n";
  out << "\"\" - Simulation step\n";
  out << "\"q\" - Quit\n";
  out << "\"p\" - Print packets in buffer\n";
  out << "\"h\" - Print help\n";
}
void smo::PrintEvent(std::ostream& out, const smo::SpecialEvent& event) {
  out << "Time: " << event.planned_time << '\n';
  switch (event.kind) {
    case smo::SpecialEventKind::generateNewRequest:
      out << "Source " << event.id << " made new request";
      break;
    case smo::SpecialEventKind::deviceRelease:
      out << "Device " << event.id << " is released";
      break;
    case smo::SpecialEventKind::endOfSimulation:
      out << "Simulation ended";
      break;
  }
  out << '\n';
}

static void PrintGeneralReport(std::ostream& out,
                               const smo::Simulator& simulator);
static void PrintSourceReport(std::ostream& out,
                              const smo::Simulator& simulator);
static void PrintDeviceReport(std::ostream& out,
                              const smo::Simulator& simulator);
void smo::PrintReport(std::ostream& out, const smo::Simulator& simulator) {
  out << "Report:\n";
  PrintGeneralReport(out, simulator);
  out << "Sources:\n";
  PrintSourceReport(out, simulator);
  out << "Devices:\n";
  PrintDeviceReport(out, simulator);
}

static tabulate::Table SourceCalendar(const smo::Simulator& simulator);
static tabulate::Table DeviceCalendar(const smo::Simulator& simulator);
static void PrintFakeBuffer(std::ostream& out, const smo::Simulator& simulator);
void smo::PrintSimulationState(std::ostream& out,
                               const smo::Simulator& simulator) {
  tabulate::Table table;
  table.add_row({"Sources calendar:", "Devices calendar:"});
  table.add_row({SourceCalendar(simulator), DeviceCalendar(simulator)});
  table.format().hide_border().padding(0).padding_right(1);
  out << table;
  out << "Buffer:\n";
  PrintFakeBuffer(out, simulator);
}

static std::vector<tabulate::FontStyle> highlight{
    tabulate::FontStyle::bold, tabulate::FontStyle::underline};

static std::string FormatRequest(const std::optional<smo::Request>& req);
void smo::PrintRealBuffer(std::ostream& out, const smo::Simulator& simulator) {
  const auto& real_buffer = simulator.RealBuffer();
  std::size_t max_size = 0;
  for (const auto& subbuffer : real_buffer) {
    max_size = std::max(max_size, subbuffer.size());
  }

  tabulate::Table table;
  tabulate::Table::Row_t index_row;
  index_row.push_back("i:");
  for (std::size_t i = 0; i < max_size; ++i) {
    index_row.push_back(std::to_string(i));
  }
  table.add_row(index_row);
  for (std::size_t i = 0; i < real_buffer.size(); ++i) {
    tabulate::Table::Row_t value_row;
    value_row.push_back("Packet " + std::to_string(i) + ":");
    for (const auto& req : real_buffer[i]) {
      value_row.push_back(FormatRequest(req));
    }
    table.add_row(value_row);
  }
  std::size_t current_packet = simulator.current_packet();
  if (!simulator.RealBuffer()[current_packet].empty()) {
    auto& current_packet_row = table[current_packet + 1];
    for (std::size_t i = 1; i < current_packet_row.size(); ++i) {
      current_packet_row[i].format().font_style(highlight);
    }
  }
  out << table << '\n';
}

template <typename... T>
static tabulate::Table::Row_t Stringify(T&&... args) {
  return {std::to_string(std::forward<T>(args))...};
}

static void PrintGeneralReport(std::ostream& out,
                               const smo::Simulator& simulator) {
  tabulate::Table table;
  table.add_row({"Total\nsimulation\ntime", "Requests\nrecieved",
                 "Requests\nprocessed", "Requests\nrejected",
                 "Rejection\nprobability"});
  std::size_t recieved = simulator.current_amount_of_requests();
  std::size_t rejected = simulator.rejected_amount();
  table.add_row(Stringify(simulator.current_simulation_time(), recieved,
                          recieved - rejected, rejected,
                          static_cast<double>(rejected) / recieved));
  out << table << '\n';
}

static void PrintSourceReport(std::ostream& out,
                              const smo::Simulator& simulator) {
  tabulate::Table table;
  table.add_row({"i", "Request\namount", "Rejection\nprobability", "Time\nfull",
                 "Time\nbuffer", "Time\nprocessing", "Variance\nbuffer",
                 "Variance\nprocessing"});
  const auto& sources = simulator.source_statistics();
  for (std::size_t i = 0; i < sources.size(); ++i) {
    const auto& source = sources[i];
    auto buffer_time = source.AverageBufferTime();
    auto device_time = source.AverageDeviceTime();
    table.add_row(
        Stringify(i, source.generated,
                  static_cast<double>(source.rejected) / source.generated,
                  buffer_time + device_time, buffer_time, device_time,
                  source.BufferTimeVariance(), source.DeviceTimeVariance()));
  }
  out << table << '\n';
}

static void PrintDeviceReport(std::ostream& out,
                              const smo::Simulator& simulator) {
  tabulate::Table table;
  table.add_row({"i", "Usage\ncoefficient"});
  const auto& devices = simulator.device_statistics();
  for (std::size_t i = 0; i < devices.size(); ++i) {
    const auto& device = devices[i];
    table.add_row(Stringify(i, static_cast<double>(device.time_in_usage) /
                                   simulator.current_simulation_time()));
  }
  out << table << '\n';
}

static void PushBackTimeAndSign(tabulate::Table::Row_t& row, smo::Time time) {
  if (time == smo::maxTime) {
    row.push_back("");
    row.push_back("1");
  } else {
    row.push_back(std::to_string(time));
    row.emplace_back("0");
  }
}
static tabulate::Table SourceCalendar(const smo::Simulator& simulator) {
  tabulate::Table table;
  table.add_row({"i", "Next event", "Sign"});
  const auto& sources = simulator.source_statistics();
  for (std::size_t i = 0; i < sources.size(); ++i) {
    const auto& source = sources[i];
    tabulate::Table::Row_t row{std::to_string(i)};
    PushBackTimeAndSign(row, source.next_request);
    table.add_row(row);
  }
  return table;
}
static tabulate::Table DeviceCalendar(const smo::Simulator& simulator) {
  tabulate::Table table;
  table.add_row({"i", "Next event", "Sign", "Request"});
  const auto& devices = simulator.device_statistics();
  for (std::size_t i = 0; i < devices.size(); ++i) {
    const auto& device = devices[i];
    tabulate::Table::Row_t row{std::to_string(i)};
    PushBackTimeAndSign(row, device.next_request);
    row.push_back(FormatRequest(device.current_request));
    table.add_row(row);
  }
  return table;
}
static void PrintFakeBuffer(std::ostream& out,
                            const smo::Simulator& simulator) {
  tabulate::Table table;
  tabulate::Table::Row_t index_row;
  tabulate::Table::Row_t value_row;

  std::size_t current_packet = simulator.current_packet();
  std::vector<std::size_t> cells_to_highlight;

  index_row.push_back("i:");
  value_row.push_back("Values:");

  auto fake_buffer = simulator.FakeBuffer();
  for (std::size_t i = 0; i < fake_buffer.size(); ++i) {
    index_row.push_back(std::to_string(i));
    auto& request = fake_buffer[i];
    if (request.source_id == current_packet) {
      cells_to_highlight.push_back(i);
    }
    value_row.push_back(FormatRequest(request));
  }
  for (std::size_t i = fake_buffer.size(); i < fake_buffer.capacity(); ++i) {
    index_row.push_back(std::to_string(i));
    value_row.push_back(FormatRequest(std::nullopt));
  }
  table.add_row(index_row);
  table.add_row(value_row);
  for (std::size_t i : cells_to_highlight) {
    table[1][i + 1].format().font_style(highlight);
  }
  out << table << '\n';
}

static std::string FormatRequest(const std::optional<smo::Request>& req) {
  if (req.has_value()) {
    return std::to_string(req->source_id) + "." + std::to_string(req->number);
  } else {
    return "";
  }
}
