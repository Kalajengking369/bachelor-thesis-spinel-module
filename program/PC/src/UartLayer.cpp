#include "UartLayer.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <poll.h>
#include <stdexcept>
#include <string>
#include <termios.h>
#include <unistd.h>

#ifdef UART_DEBUG
static void printByteAsciiHex(uint8_t byte) {
    char ascii = (byte >= 0x20 && byte <= 0x7E) ? static_cast<char>(byte) : ' ';

    std::cout << ascii << "[0x" << std::hex << std::setw(2) << std::setfill('0')
              << static_cast<int>(byte) << std::dec << "] ";
}
#endif

static speed_t baudRateToSpeedT(uint32_t baudRate) {
    switch(baudRate) {
    case 1200:
        return B1200;
    case 2400:
        return B2400;
    case 4800:
        return B4800;
    case 9600:
        return B9600;
    case 19200:
        return B19200;
    case 38400:
        return B38400;
    case 57600:
        return B57600;
    case 115200:
        return B115200;
    default:
        return B0; // Invalid
    }
}

UartLayer::UartLayer(const std::string& portName, uint32_t baudRate) {
    fd = open(portName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);

    if(fd == -1) {
        throw std::runtime_error("Nelze otevrit port " + portName + ": " + strerror(errno));
    }

    fcntl(fd, F_SETFL, 0);

    struct termios options;

    if(tcgetattr(fd, &options) != 0) {
        close(fd);
        throw std::runtime_error("Chyba tcgetattr");
    }

    speed_t speed = baudRateToSpeedT(baudRate);
    if(speed == B0) {
        close(fd);
        throw std::runtime_error("Nepodporovana rychlost: " + std::to_string(baudRate));
    }

    cfsetispeed(&options, speed);
    cfsetospeed(&options, speed);
    currentBaudRate = baudRate;

    // Configure UART to 8 data bits, no parity, one stop bit and no flow control.
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    options.c_cflag &= ~CRTSCTS;

    // Use raw byte mode without terminal echo or CR/LF conversions.
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    options.c_iflag &= ~(ICRNL | INLCR | IGNCR);

    options.c_oflag &= ~OPOST;

    // Reads are handled by poll(), so read() itself is non-blocking.
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 0;

    if(tcsetattr(fd, TCSANOW, &options) != 0) {
        close(fd);
        throw std::runtime_error("Chyba tcsetattr");
    }

    tcflush(fd, TCIOFLUSH);
}

UartLayer::~UartLayer() {
    if(fd != -1) {
        close(fd);
    }
}

void UartLayer::sendData(const uint8_t* data, size_t length) {
    if(data == nullptr || length == 0)
        return;

#ifdef UART_DEBUG
    std::cout << "[TX]: ";
    for(size_t i = 0; i < length; i++) {
        printByteAsciiHex(data[i]);
    }
    std::cout << std::endl;
#endif

    size_t totalWritten = 0;
    while(totalWritten < length) {
        ssize_t res = write(fd, data + totalWritten, length - totalWritten);
        if(res < 0) {
            if(errno == EINTR)
                continue;
            std::cerr << "Chyba zapisu do UARTu! errno: " << errno << "\n";
            break;
        }
        totalWritten += res;
    }
}

bool UartLayer::readByte(uint8_t& outByte, int timeoutMs) {
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;

    int ret = poll(&pfd, 1, timeoutMs);
    if(ret >= 0) {
        if(pfd.revents & POLLIN) {
            ssize_t n = read(fd, &outByte, 1);
            if(n == 1) {
#ifdef UART_DEBUG
                if(outByte == '*') {
                    std::cout << "[RX]: ";
                }

                printByteAsciiHex(outByte);

                if(outByte == 0x0D) {
                    std::cout << std::endl;
                }
#endif
                return true;
            }
        }
    }

    return false;
}

bool UartLayer::setSpeed(uint32_t baudRate) {
    tcdrain(fd); // Wait until all outgoing data is transmitted.
    struct termios tty;
    if(tcgetattr(fd, &tty) != 0) {
        std::cerr << "Chyba tcgetattr v setSpeed\n";
        return false;
    }

    speed_t speed = baudRateToSpeedT(baudRate);
    if(speed == B0) {
        std::cerr << "Nepodporovana rychlost: " << baudRate << "\n";
        return false;
    }

    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);

    if(tcsetattr(fd, TCSANOW, &tty) != 0) {
        std::cerr << "Chyba tcsetattr v setSpeed\n";
        return false;
    }

    tcflush(fd, TCIOFLUSH);

    currentBaudRate = baudRate;
    return true;
}

const uint32_t* UartLayer::getBaudRatePtr() const {
    return &currentBaudRate;
}