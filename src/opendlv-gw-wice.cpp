/*
 * Copyright (C) 2019 Ola Benderius
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <string>

#include "cluon-complete.hpp"
#include "opendlv-standard-message-set.hpp"

int32_t main(int32_t argc, char **argv) {
  int32_t retCode{0};
  auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
  if (0 == commandlineArguments.count("cid")
      || 0 == commandlineArguments.count("ip")
      || 0 == commandlineArguments.count("port")) {
    std::cerr << argv[0] 
      << " interfaces to the WICE cloud service." 
      << std::endl;
    std::cerr << "Usage:   " << argv[0] << " " 
      << "--ip=<address to WICE> "
      << "--port=<TCP port to WICE> "
      << "--cid=<OpenDLV session> "
      << "[--verbose]" 
      << std::endl;
    std::cerr << "Example: " << argv[0] << " --ip=10.10.10.10 --port=10000 "
      << "--cid=111 --verbose" << std::endl;
    retCode = 1;
  } else {
    bool const verbose{commandlineArguments.count("verbose") != 0};
    uint16_t const cid = std::stoi(commandlineArguments["cid"]);
    uint16_t const port = std::stoi(commandlineArguments["port"]);
    std::string const ip{commandlineArguments["ip"]};

    cluon::TCPConnection tcpSender(ip, port);

    auto onGeodeticWgs84Reading{[&tcpSender, &verbose](
        cluon::data::Envelope &&envelope)
      {
        int64_t time = cluon::time::toMicroseconds(
            envelope.sampleTimeStamp());

        auto msg = cluon::extractMessage<opendlv::proxy::GeodeticWgs84Reading>(
            std::move(envelope));
        double latitude = msg.latitude();
        double longitude = msg.longitude();

        if (tcpSender.isRunning()) {
          tcpSender.send(std::to_string(time) + ",Latitude," 
              + std::to_string(latitude) + "\n");
          tcpSender.send(std::to_string(time) + ",Longitude," 
              + std::to_string(longitude) + "\n");

          if (verbose) {
            std::cout << "Sending GPS data " << latitude << "," << longitude
              << " at time " << time << std::endl;
          }
        }
      }};

    cluon::OD4Session od4{cid};
    od4.dataTrigger(opendlv::proxy::GeodeticWgs84Reading::ID(),
        onGeodeticWgs84Reading);

    while (od4.isRunning()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    retCode = 0;
  }
  return retCode;
}
