#ifndef _OPENCL_H
#define _OPENCL_H

#include <algorithm>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <cassert>

#define __CL_ENABLE_EXCEPTIONS
#include "cl.hpp"

class OpenCLApp {
  cl::Platform platform;
  cl::Device device;
  cl::Context context;
  cl::CommandQueue queue;

  void select_device() {
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    std::vector<cl::Device> devices;
    for (auto& p : platforms) {
      std::vector<cl::Device> platform_devs;
      p.getDevices(CL_DEVICE_TYPE_ALL, &platform_devs);
      copy(begin(platform_devs), end(platform_devs), back_inserter(devices));
    }
    assert(!devices.empty());
    device = devices[0];
    for (auto& d : devices) {
      cl_device_type type;
      d.getInfo(CL_DEVICE_TYPE, &type);
      if (type & CL_DEVICE_TYPE_GPU) {
        device = d;
        break;
      }
    }
    device.getInfo(CL_DEVICE_PLATFORM, &platform);
  }

  template<typename T>
  void write(cl::Buffer buf, cl_bool blocking, T* ptr, size_t num, size_t offset=0) {
    if (num == 0)
      return;
    queue.enqueueWriteBuffer(
        buf, blocking, offset * sizeof(T), num * sizeof(T), ptr, nullptr, nullptr);
  }

  template<typename T>
  void read(cl::Buffer buf, cl_bool blocking, T* ptr, size_t num, size_t offset=0) {
    if (num == 0)
      return;
    queue.enqueueReadBuffer(
        buf, blocking, offset * sizeof(T), num * sizeof(T), ptr, nullptr, nullptr);
  }

  void print_build_log(cl::Program prog) {
    std::string build_log = prog.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
    if (build_log[0]) {
      std::cout << "BUILD LOG" << std::endl << build_log << std::endl;
    }
  }

public:
  OpenCLApp() {
    select_device();
    std::vector<cl::Device> devices;
    devices.push_back(device);
    context = cl::Context(devices, nullptr, nullptr, nullptr, nullptr);
    queue = cl::CommandQueue(context, device, 0, nullptr);
  }

  void print_cl_info() {
    std::cout << "OPENCL" << std::endl;
    for (auto i: {
            std::make_tuple("name", CL_PLATFORM_NAME),
            std::make_tuple("vendor", CL_PLATFORM_VENDOR),
            std::make_tuple("version", CL_PLATFORM_VERSION),
            std::make_tuple("profile", CL_PLATFORM_PROFILE),
        })
    {
      std::string info;
      platform.getInfo(std::get<1>(i), &info);
      std::cout << "  Platform " << std::get<0>(i) << ": " << info << std::endl;
    }
    for (auto i: {
            std::make_tuple("name", CL_DEVICE_NAME),
            std::make_tuple("vendor", CL_DEVICE_VENDOR),
            std::make_tuple("driver version", CL_DRIVER_VERSION),
        })
    {
      std::string info;
      device.getInfo(std::get<1>(i), &info);
      std::cout << "  Device " << std::get<0>(i) << ": " << info << std::endl;
    }
    cl_ulong local_mem_size;
    device.getInfo(CL_DEVICE_LOCAL_MEM_SIZE, &local_mem_size);
    std::cout << "  Device local memory size: " << local_mem_size << " bytes" << std::endl;
  }

  template <typename T>
  cl::Buffer alloc(size_t num, cl_mem_flags flags = CL_MEM_READ_WRITE) {
    return cl::Buffer(context, flags, std::max(size_t{1}, num * sizeof(T)), nullptr, nullptr);
  }

  cl::Program build_program(const std::vector<std::string>& sources) {
    std::vector<std::pair<const char*, size_t>> sources_;
    for (const auto& s: sources)
      sources_.emplace_back(s.c_str(), s.size());
    cl::Program prog(context, sources_, nullptr);
    std::vector<cl::Device> devices;
    devices.push_back(device);
    try {
      prog.build(devices, nullptr, nullptr, nullptr);
    } catch (cl::Error err) {
      print_build_log(prog);
      throw;
    }
    print_build_log(prog);
    return prog;
  }

  cl::Program build_program(const std::string& source) {
    return build_program(std::vector<std::string>(1, source));
  }

  cl::Kernel get_kernel(const cl::Program& prog, const std::string& name) {
    return cl::Kernel(prog, name.c_str(), nullptr);
  }

  template<typename T>
  void write_async(const cl::Buffer& buf, T* ptr, size_t num, size_t offset=0) {
    write(buf, false, ptr, num, offset);
  }

  template<typename T>
  void write_sync(const cl::Buffer& buf, T* ptr, size_t num, size_t offset=0) {
    write(buf, true, ptr, num, offset);
  }

  template<typename T>
  void read_async(const cl::Buffer& buf, T* ptr, size_t num, size_t offset=0) {
    read(buf, false, ptr, num, offset);
  }

  template<typename T>
  void read_sync(const cl::Buffer& buf, T* ptr, size_t num, size_t offset=0) {
    read(buf, true, ptr, num, offset);
  }

  void finish_queue() {
    queue.finish();
  }

  void run_kernel(const cl::Kernel& kernel, const cl::NDRange& global, const cl::NDRange& local) {
    queue.enqueueNDRangeKernel(kernel, cl::NullRange, global, local, nullptr, nullptr);
  }
};

#endif
