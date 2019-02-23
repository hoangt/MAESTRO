/******************************************************************************
Copyright (c) 2018 Georgia Instititue of Technology
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*******************************************************************************/

#ifndef MAESTRO_MAESTRO_HPP_
#define MAESTRO_MAESTRO_HPP_

#include <string>
#include <iostream>
#include <list>
#include <memory>

#include "analysis-structure.hpp"
#include "mapping-analysis.hpp"
#include "cost-analysis.hpp"

namespace maestro {

  void SetNumPEs(int np);
  void SetupNoC(int bw, int hops, int hop_latency, bool mc);
  void SetupInputTensors(std::list<std::string>& in_tensors);
  void SetupOutputTensors(std::list<std::string>& out_tensors);
  void ParseInputs(std::string dataflow_file_name, std::string layer_file_name);
  void ConfigureProblem();
  void AnalyzeHardware();
  void AnalyzeMapping();
  void AnalyzeReuse();
  void AnalyzeBuffer(bool silent = false);
  void AnalyzeRuntime(int num_alus_per_pe = 1, bool do_reduction = true, bool do_implicit_reduction = true, bool fg_sync = false, bool latency_hiding = true);

  double AnalyzeL1BuffReq_DSE();
  double AnalyzeL2BuffReq_DSE();
  double AnalyzeEnergyDSE();
  long AnalyzeRuntime_DSE(int num_alus_per_pe = 1, bool do_reduction = true, bool do_implicit_reduction = true, bool fg_sync = false, bool latency_hiding = true);

  std::list<std::string> GetTensors();
  std::list<std::string> GetInputTensors();
  std::list<std::string> GetOutputTensors();

  std::shared_ptr<maestro::BufferAnalysis> GetBufferAnalysis();
  std::shared_ptr<maestro::NetworkOnChipModel> GetNoCModel();
  std::shared_ptr<maestro::PerformanceAnalysis> GetPerfAnalysis();
  std::shared_ptr<maestro::MappingAnalysis> GetMapAnalysis();

}; //End of namespace maestro

#endif
