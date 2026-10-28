#include "Application.h"
#include "FrameQueue.h"
#include "ProtocolConstants.h"
#include "SpinelParser.h"
#include "SpinelProtocolManager.h"
#include "UartLayer.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <limits>
#include <string>
#include <thread>
#include <vector>

enum class RunMode { MONITOR, DEBUG };

struct RuntimeConfig {
    std::string portName = "";
    uint32_t baudRate = 0;
    RunMode mode = RunMode::MONITOR;
    uint8_t targetAddr = '0';
    bool useAscii = false;
    bool showHelp = false;
    bool isValid = true;
};

void printHelp(const char* progName) {
    std::cout << "Spinel PC Control Application\n";
    std::cout << "Usage: " << progName << " <baudrate> <port> [debug]\n\n";
    std::cout << "Arguments:\n";
    std::cout << "  -h, --help      Show this help\n";
    std::cout << "  baudrate        UART speed (e.g., 9600, 115200)\n";
    std::cout << "  port            Serial port device (e.g., /dev/ttyUSB0 or COM3)\n";
    std::cout << "  debug           Optional: Run in interactive debug mode\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << progName << " 115200 /dev/ttyUSB0        # Monitor mode\n";
    std::cout << "  " << progName << " 9600 /dev/ttyAMA0 debug    # Debug mode\n";
}

void printMenu(uint8_t targetAddr, bool useAscii) {
    std::cout << "\n--- Spinel Test Menu (DEBUG) ---\n";
    std::cout << "Target: " << (useAscii ? "ASCII " : "BINARY ");
    if(useAscii) {
        std::cout << (char)targetAddr << "\n";
    } else {
        std::cout << (int)targetAddr << "\n";
    }
    std::cout << "------------------------\n";
    std::cout << "m.  Toggle Mode (Bin/Asc)\n";
    std::cout << "t.  Change Target Address\n";
    std::cout << "b.  Change local Baud rate\n";
    std::cout << "------------------------\n";
    std::cout << "1.  Get Temp\n";
    std::cout << "2.  Get Raw (Bin Only)\n";
    std::cout << "3.  Read Status\n";
    std::cout << "4.  Set Status\n";
    std::cout << "5.  Read Info\n";
    std::cout << "6.  Read User Data\n";
    std::cout << "7.  Write User Data\n";
    std::cout << "8.  Enable Config\n";
    std::cout << "9.  Set Comms / Address\n";
    std::cout << "0.  Read Comms Params (Bin Only)\n";
    std::cout << "c.  Read Checksum (Bin Only)\n";
    std::cout << "d.  Set Checksum (Bin Only)\n";
    std::cout << "e.  Read Errors (Bin Only)\n";
    std::cout << "p.  Read Prod Info (Bin Only)\n";
    std::cout << "r.  Reset\n";
    std::cout << "q.  Quit\n";
    std::cout << "Choice: \n" << std::flush;
}

void cleanInputBuffer() {
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

RuntimeConfig parseArgs(int argc, char* argv[]) {
    RuntimeConfig cfg;

    std::vector<std::string> args;
    for(int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if(arg == "-h" || arg == "--help" || arg == "help") {
            cfg.showHelp = true;
            return cfg;
        }
        args.push_back(arg);
    }

    if(args.size() < 2) {
        if(!cfg.showHelp) {
            std::cerr << "Error: Missing required arguments.\n";
        }
        cfg.isValid = false;
        return cfg;
    }

    try {
        cfg.baudRate = std::stoul(args[0]);
    } catch(...) {
        std::cerr << "Invalid baudrate: " << args[0] << "\n";
        cfg.isValid = false;
        return cfg;
    }

    cfg.portName = args[1];

    if(args.size() >= 3 && args[2] == "debug") {
        cfg.mode = RunMode::DEBUG;
    }

    return cfg;
}

void runMonitorMode(std::atomic<bool>& running, Application& app) {
    // Discover connected devices using the universal ASCII address.
    app.clearDetectedAddresses();
    app.readInfoAscii('$');

    auto start = std::chrono::steady_clock::now();
    while(running && app.isRequestActive()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() -
                                                            start)
               .count() > 2)
            break;
    }

    std::vector<uint8_t> devices = app.getDetectedAddresses();

    if(devices.empty()) {
        std::cout << "No devices detected. Exiting Monitor Mode.\n";
        return;
    }

    std::cout << "Found " << devices.size() << " devices. Starting poll loop...\n";

    while(running) {
        for(auto it = devices.begin(); it != devices.end();) {
            if(!running)
                break;

            uint8_t addr = *it;

            app.reqTempBin(addr);

            // Wait until the response handler finishes or timeout clears the request.
            while(running && app.isRequestActive()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }

            if(!app.wasLastResponseReceived()) {
                std::cout << "Device '" << (char)addr
                          << "' stopped responding. Removing from poll list.\n";
                it = devices.erase(it);
            } else {
                ++it;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        if(devices.empty()) {
            std::cout << "All devices lost. Exiting Monitor Mode.\n";
            break;
        }

        for(int i = 0; i < 5 && running; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

void runDebugMode(std::atomic<bool>& running, RuntimeConfig& cfg, Application& app, UartLayer& uart,
                  FrameQueue& rxQueue) {
    char cmd;
    printMenu(cfg.targetAddr, cfg.useAscii);
    while(running) {
        if(!(std::cin >> cmd))
            break;

        switch(cmd) {
        case 'm':
            cfg.useAscii = !cfg.useAscii;
            std::cout << "Mode switched to: " << (cfg.useAscii ? "ASCII" : "BINARY") << "\n";
            break;

        case 't': {
            if(cfg.useAscii) {
                char addr;
                std::cout << "New Ascii Addr: ";
                std::cin >> addr;
                cfg.targetAddr = (uint8_t)addr;
            } else {
                int addr;
                std::cout << "New Bin Addr (0-255): ";
                std::cin >> addr;
                cfg.targetAddr = (uint8_t)addr;
            }
            break;
        }

        case 'b': {
            uint32_t newBaud;
            std::cout << "Enter new local Baud rate: ";
            std::cin >> newBaud;
            if(uart.setSpeed(newBaud)) {
                std::cout << "Baud rate changed to " << newBaud << "\n";
            }
            break;
        }

        case '1': // Get Temp
            if(cfg.useAscii)
                app.reqTempAscii((char)cfg.targetAddr);
            else
                app.reqTempBin(cfg.targetAddr);
            break;

        case '2': // Get Raw
            if(cfg.useAscii)
                std::cout << "Not supported in ASCII mode.\n";
            else
                app.reqRawBin(cfg.targetAddr);
            break;

        case '3': // Read Status
            if(cfg.useAscii)
                app.readStatusAscii((char)cfg.targetAddr);
            else
                app.readStatusBin(cfg.targetAddr);
            break;

        case '4': // Set Status
            if(cfg.useAscii) {
                char statusChar;
                std::cout << "Enter Status Char: ";
                std::cin >> statusChar;
                app.setStatusAscii((char)cfg.targetAddr, statusChar);
            } else {
                int statusVal;
                std::cout << "Enter Status (0-255): ";
                std::cin >> statusVal;
                app.setStatusBin(cfg.targetAddr, (uint8_t)statusVal);
            }
            break;

        case '5': // Read Info
            if(cfg.useAscii)
                app.readInfoAscii((char)cfg.targetAddr);
            else
                app.readInfoBin(cfg.targetAddr);
            break;

        case '6': // Read User Data
            if(cfg.useAscii)
                app.readUserDataAscii((char)cfg.targetAddr);
            else
                app.readUserDataBin(cfg.targetAddr);
            break;

        case '7': { // Write User Data
            cleanInputBuffer();
            std::string data;

            if(cfg.useAscii) {
                char posChar;
                std::cout << "Pos Char (0-9, A-F): ";
                std::cin >> posChar;
                cleanInputBuffer();

                std::cout << "Data (String): ";
                std::getline(std::cin, data);

                app.writeUserDataAscii((char)cfg.targetAddr, posChar, data.c_str());
            } else {
                int pos;
                std::cout << "Pos (0-15): ";
                std::cin >> pos;
                cleanInputBuffer();

                std::cout << "Data (String): ";
                std::getline(std::cin, data);

                app.writeUserDataBin(cfg.targetAddr, (uint8_t)pos,
                                     reinterpret_cast<const uint8_t*>(data.c_str()), data.length());
            }
            break;
        }

        case '8': // Enable Config
            if(cfg.useAscii)
                app.enableConfigAscii((char)cfg.targetAddr);
            else
                app.enableConfigBin(cfg.targetAddr);
            break;

        case '9': // Set Comms/Address
            if(cfg.useAscii) {
                char newAddr;
                std::cout << "New Addr Char: ";
                std::cin >> newAddr;
                app.setAddrAscii((char)cfg.targetAddr, newAddr);
            } else {
                int newAddr, baud;
                std::cout << "New Addr (0-253): ";
                std::cin >> newAddr;
                std::cout << "Baud (e.g. 9600): ";
                std::cin >> baud;
                app.setCommsBin(cfg.targetAddr, (uint8_t)newAddr, (uint32_t)baud);
            }
            break;

        case '0': // Read Comms
            if(cfg.useAscii)
                std::cout << "Not supported in ASCII (Protocol limit).\n";
            else
                app.readCommsParams(cfg.targetAddr);
            break;

        case 'e': // Read Errors
            if(cfg.useAscii)
                std::cout << "Not supported in ASCII (Protocol limit).\n";
            else
                app.readErrorsBin(cfg.targetAddr);
            break;

        case 'p': // Read Prod Info
            if(cfg.useAscii)
                std::cout << "Not supported in ASCII (Protocol limit).\n";
            else
                app.readProdInfoBin(cfg.targetAddr);
            break;

        case 'r': // Reset
            if(cfg.useAscii)
                app.sendResetAscii((char)cfg.targetAddr);
            else
                app.sendResetBin(cfg.targetAddr);
            break;

        case 'q':
            running = false;
            rxQueue.stop();
            break;

        case 'c': // Read Checksum
            if(cfg.useAscii)
                std::cout << "Not supported in ASCII (Protocol limit).\n";
            else
                app.readChecksum(cfg.targetAddr);
            break;

        case 'd': // Set Checksum
            if(cfg.useAscii)
                std::cout << "Not supported in ASCII (Protocol limit).\n";
            else {
                char choice;
                std::cout << "Enable checksum? (y/n): ";
                std::cin >> choice;
                app.setChecksum(cfg.targetAddr, choice == 'y');
            }
            break;
        default:
            std::cout << "Unknown option.\n";
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

int main(int argc, char* argv[]) {
    RuntimeConfig cfg = parseArgs(argc, argv);

    if(cfg.showHelp || !cfg.isValid) {
        printHelp(argv[0]);
        return cfg.isValid ? 0 : 1;
    }

    std::cout << "Opening port: " << cfg.portName << " at " << cfg.baudRate << " baud...\n";

    try {
        UartLayer uart(cfg.portName, cfg.baudRate);
        FrameQueue rxQueue;
        SpinelParser parser;

        Application app;
        SpinelProtocolManager manager(rxQueue, uart, app);
        app.setManager(manager);

        std::atomic<bool> running(true);

        std::thread t_parser([&]() {
            // Reads UART bytes and assembles complete Spinel frames.
            while(running) {
                uint8_t byte;
                // Short read timeout allows regular parser timeout checks.
                if(uart.readByte(byte, Timeouts::INTER_BYTE_MS)) {
                    if(parser.processByte(byte)) {
                        if(!rxQueue.push(parser.getParsedFrame())) {
                            std::cerr << "RX queue full\n";
                        }
                    }
                } else {
                    parser.handleTimeout();
                }
            }
        });

        std::thread t_manager([&]() {
            // Processes complete frames from the RX queue.
            manager.run();
        });

        std::thread t_timeout([&]() {
            // Periodically checks whether the active request has timed out.
            while(running) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                app.checkResponseTimeouts();
            }
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        if(cfg.mode == RunMode::MONITOR) {
            runMonitorMode(running, app);
        } else {
            runDebugMode(running, cfg, app, uart, rxQueue);
        }

        running = false;
        rxQueue.stop();
        t_parser.join();
        t_manager.join();
        t_timeout.join();
        std::cout << "Exiting.\n";
    } catch(const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
