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

#ifndef MAESTRO_OPTION_HPP_
#define MAESTRO_OPTION_HPP_

#include <iostream>
#include <list>
#include <string>

#include <boost/program_options.hpp>

namespace maestro {


  namespace po = boost::program_options;
  class Options {
    public:
      /* Default values : Models MAERI with VGG16 and 64 multiplier switches*/
      int np = 7;
      int bw = 32;
      int hops = 1;
      int hop_latency = 1;
      bool mc = true;
      std::list<std::string> in_tensors = {"weight", "input"};
      std::list<std::string> out_tensors = {"output"};

      std::string dataflow_file_name = "data/dataflow/maeri.m";
      std::string layer_file_name = "data/layer/vgg16_conv1.m";

      int num_alus_per_pe = 9;
      bool do_reduction = true;
      bool do_implicit_reduction = true;
      bool fg_sync = false;


      bool parse(int argc, char** argv)
      {
          std::vector<std::string> config_fnames;

          po::options_description desc("General Options");
          desc.add_options()
              ("help", "Display help message")
          ;

          po::options_description io("File IO options");
          io.add_options()
            ("dataflow_file", po::value<std::string>(&dataflow_file_name) ,"the name of dataflow description file")
            ("layer_file", po::value<std::string>(&layer_file_name) ,"the name of layer dimension description file")
          ;

          po::options_description nocs("Network on chip options");
          nocs.add_options()
            ("noc_bw", po::value<int>(&bw), "the bandwidth of NoC")
            ("noc_hops", po::value<int>(&hops), "the average number of NoC hops")
            ("noc_hop_latency", po::value<int>(&hop_latency), "the latency for each of NoC hop")
            ("noc_mc_support", po::value<bool>(&mc), "the multicasting capability of NoC")
          ;

          po::options_description pe_array("Processing element options");
          pe_array.add_options()
            ("num_pes", po::value<int>(&np), "the number of PEs")
            ("num_pe_alus", po::value<int>(&num_alus_per_pe), "the number of ALUs in each PE")
            ("do_implicit_reduction", po::value<bool>(&do_implicit_reduction), "If PEs reduce items as soon as they generate partial results; if set as true, reductions do not require additional cycles.")
            ("do_fg_sync", po::value<bool>(&fg_sync), "Fine-grained synchronization is performed (future work)")
          ;

          po::options_description problem("Problem description options");
          problem.add_options()
            ("do_reduction_op", po::value<bool>(&do_reduction), "If the problem requires reduction or not")
              //TODO: Add correlated variables here
          ;

          po::options_description all_options;
          all_options.add(desc);
          all_options.add(io);
          all_options.add(nocs);
          all_options.add(pe_array);
          all_options.add(problem);


          po::variables_map vm;
          po::store(po::parse_command_line(argc, argv, all_options), vm);
          po::notify(vm);

          return true;
      }
  }; //End of class Options
}; //End of namespace maestro
#endif
