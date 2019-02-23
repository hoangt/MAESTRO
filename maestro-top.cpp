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

#include <iostream>
#include <memory>

#include<boost/program_options.hpp>

#include "analysis-structure.hpp"
#include "option.hpp"
#include "parser.hpp"

#include "maestro.hpp"

using namespace std;

int main(int argc, char** argv)
{

  maestro::Options option;
  bool success = option.parse(argc, argv);

  if(!success) {
    std::cout << "[MAESTRO] Failed to parse program options" << std::endl;
  }

  std::list<std::string> in_tensors = {"weight", "input"};
  std::list<std::string> out_tensors = {"output"};


  maestro::SetNumPEs(option.np);
  maestro::SetupNoC(option.bw, option.hops, option.hop_latency, option.mc);
  maestro::SetupInputTensors(in_tensors);
  maestro::SetupOutputTensors(out_tensors);
  maestro::ParseInputs(option.dataflow_file_name, option.layer_file_name);
  maestro::ConfigureProblem();

  maestro::AnalyzeHardware();
  maestro::AnalyzeBuffer(true);
  maestro::AnalyzeReuse();
  maestro::AnalyzeRuntime(option.num_alus_per_pe, option.do_reduction, option.do_implicit_reduction, option.fg_sync);

  return 0;
}
