#ifndef _IO_H
#define _IO_H

#include <exception>
#include <fstream>
#include <iostream>

class IOException : public std::exception {
  std::string msg;
public:
  IOException(std::string msg): msg(msg) {}
  virtual const char * what() const throw() {
    return msg.c_str();
  }
};

std::string read_file(std::string fname) {
  std::ifstream f(fname, std::ios::binary);
  if (!f.good())
    throw IOException("Could not read file " + fname);
  return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
}

#endif
