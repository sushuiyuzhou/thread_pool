//
// Created by sushuiyuzhou on 10/8/2020.
//

#ifndef TP__LOGGER_H
#define TP__LOGGER_H

#include "pool.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace utils {

namespace fs = std::filesystem;

class LoggerBase {

  class LoggerDevice {
    fs::path _path;
    std::unique_ptr<std::ofstream> _fsPtr;
    std::mutex _m;

  public:
    explicit LoggerDevice(std::string const &path = "")
        : _path{}, _fsPtr{} {
      if (path.empty()) {
        _path = fs::current_path();
      } else {
        _path = fs::path{path};
      }

      auto filePath = _path / "debug.log";
      if (!fs::exists(filePath)) {
        std::ofstream{filePath};
      }

      _fsPtr = std::make_unique<std::ofstream>(filePath, std::ios::app);
    }

    void write(std::string const &ctn) {
      std::lock_guard<std::mutex> lg{_m};
      *_fsPtr << ctn;
    }
  };

protected:
  LoggerDevice _rsc;
  LoggerBase() : _rsc{} {}
};

class Logger
    : public LoggerBase {// to make sure file stream is destructed after thread pool
  ThreadPool _tp;

  void _write(std::string const &ctn) { _rsc.write(ctn); }

  static std::string getTimeStamp() {
    std::time_t now = std::time(nullptr);
    char buffer[70];
    std::strftime(buffer, sizeof(buffer),
                  "[%Y/%m/%d-%H:%M:%S] : ", std::localtime(&now));
    return buffer;
  }

public:
  Logger() : _tp{1, true} { _tp.run(); }

  void info(std::string const &ctn) {
    _tp.submit(std::move([=] {
      this->_write(std::string("[INFO] - ") + getTimeStamp() + ctn + "\n");
    }));
  }

  void warn(std::string const &ctn) {
    _tp.submit(std::move([=] {
      this->_write(std::string("[WARNING] - ") + getTimeStamp() + ctn
                   + "\n");
    }));
  }

  void error(std::string const &ctn) {
    _tp.submit(std::move([=] {
      this->_write(std::string("[ERROR] - ") + getTimeStamp() + ctn
                   + "\n");
    }));
  }
};

}// namespace utils

#endif//TP__LOGGER_H
