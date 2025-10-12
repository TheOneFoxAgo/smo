#include <cstddef>
#include <ostream>
#include <tabulate/table.hpp>

#include "print.h"
void smo::PrintUsage(std::ostream& out) {
  out << "Usage: simulator [-i|-a|-r] [-o [outfile]] [-m max_requests] "
         "infile\n";
}

void smo::PrintHelp(std::ostream& out) {
  out << "Interactive mode comands:\n";
  out << "\"\" - Simulation step\n";
  out << "\"q\" - Quit\n";
  out << "\"s\" - Print source calendar\n";
  out << "\"d\" - Print device calendar\n";
  out << "\"b\" - Print buffer\n";
  out << "\"p\" - Print packets in buffer\n";
  out << "\"h\" - Print help\n";
}

void smo::PrintEvent(std::ostream& out, const smo::SpecialEvent& event) {
  out << "Time: " << event.planned_time << ". ";
  switch (event.kind) {
    case smo::SpecialEventKind::generateNewRequest:
      out << "Source " << event.id << " made new request\n";
      break;
    case smo::SpecialEventKind::deviceRelease:
      out << "Device " << event.id << " is released\n";
      break;
    case smo::SpecialEventKind::endOfSimulation:
      out << "Simulation ended\n";
      break;
  }
}
void smo::PrintReport(std::ostream& out, const smo::Simulator& simulator) {
  out << "Report:\n";
  PrintGeneralReport(out, simulator);
  out << "Sources:\n";
  PrintSourceReport(out, simulator);
  out << "Devices:\n";
  PrintDeviceReport(out, simulator);
}
void smo::PrintGeneralReport(std::ostream& out,
                             const smo::Simulator& simulator) {
  tabulate::Table table;
  table.add_row({"Total\nsimulation\ntime", "Requests\nrecieved",
                 "Requests\nprocessed", "Requests\nrejected",
                 "Rejection\nprobability"});
  std::size_t recieved = simulator.current_amount_of_requests();
  std::size_t rejected = simulator.rejected_amount();
  table.add_row(tabulate::RowStream{}
                << simulator.current_simulation_time() << recieved
                << recieved - rejected << rejected
                << static_cast<double>(rejected) / recieved);
  out << table << '\n';
}
void smo::PrintSourceReport(std::ostream& out,
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
    table.add_row(tabulate::RowStream{}
                  << i << source.generated
                  << static_cast<double>(source.rejected) / source.generated
                  << buffer_time + device_time << buffer_time << device_time
                  << source.BufferTimeVariance()
                  << source.DeviceTimeVariance());
  }
  out << table << '\n';
}
void smo::PrintDeviceReport(std::ostream& out,
                            const smo::Simulator& simulator) {
  tabulate::Table table;
  table.add_row({"i", "Usage\ncoefficient"});
  const auto& devices = simulator.device_statistics();
  for (std::size_t i = 0; i < devices.size(); ++i) {
    const auto& device = devices[i];
    table.add_row({std::to_string(i),
                   std::to_string(static_cast<double>(device.time_in_usage) /
                                  simulator.current_simulation_time())});
  }
  out << table << '\n';
}
void smo::PrintSourceCalendar(std::ostream& out,
                              const smo::Simulator& simulator) {
  tabulate::Table table;
  table.add_row({"i", "Next event", "Sign"});
  const auto& sources = simulator.source_statistics();
  for (std::size_t i = 0; i < sources.size(); ++i) {
    const auto& source = sources[i];
    tabulate::RowStream row{};
    row << i;
    if (source.next_request != smo::maxTime) {
      row << source.next_request << 0;
    } else {
      row << "" << 1;
    }
    table.add_row(row);
  }
  out << table << '\n';
}
std::string FormatRequest(const std::optional<smo::Request>& req) {
  if (req.has_value()) {
    return std::to_string(req->source_id) + "." + std::to_string(req->number);
  } else {
    return "";
  }
}
void smo::PrintDeviceCalendar(std::ostream& out,
                              const smo::Simulator& simulator) {
  tabulate::Table table;
  table.add_row({"i", "Next event", "Sign", "Request"});
  const auto& devices = simulator.device_statistics();
  for (std::size_t i = 0; i < devices.size(); ++i) {
    const auto& device = devices[i];
    tabulate::RowStream row{};
    row << i;
    if (device.next_request != smo::maxTime) {
      row << device.next_request << 0;

    } else {
      row << "" << 1;
    }
    row << FormatRequest(device.current_request);
    table.add_row(row);
  }
  out << table << '\n';
}
void smo::PrintFakeBuffer(std::ostream& out, const smo::Simulator& simulator) {
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
  out << table << '\n';
}
void smo::PrintRealBuffer(std::ostream& out, const smo::Simulator& simulator) {
  const auto& real_buffer = simulator.RealBuffer();
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
  out << table << '\n';
}
