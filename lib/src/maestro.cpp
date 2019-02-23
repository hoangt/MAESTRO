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

#include <string>
#include <iostream>
#include <list>
#include <memory>

#include "parser.hpp"
#include "analysis-structure.hpp"
#include "mapping-analysis.hpp"
#include "cost-analysis.hpp"


namespace maestro {

  std::shared_ptr<maestro::PragmaParser> prag_parser;
  std::shared_ptr<maestro::PragmaTable> prag_table;
  std::shared_ptr<maestro::LoopInfoTable> loop_info_table;
  std::shared_ptr<maestro::MappingAnalysis> map_analysis;
  std::shared_ptr<maestro::NetworkOnChipModel> noc_model;

  std::shared_ptr<maestro::BufferAnalysis> buff_analysis;
  std::shared_ptr<maestro::PerformanceAnalysis> perf_analysis;

  int num_pes = 1;

  std::list<std::string> input_tensors = {"weight", "input"};
  std::list<std::string> output_tensors = {"output"};
  std::list<std::string> all_tensors;


  std::list<std::string> GetTensors() {
	  return all_tensors;
  }

  std::list<std::string> GetInputTensors() {
	  return input_tensors;
  }

  std::list<std::string> GetOutputTensors() {
	  return output_tensors;
  }


  std::shared_ptr<maestro::BufferAnalysis> GetBufferAnalysis() {
  	return buff_analysis;
  }

  std::shared_ptr<maestro::NetworkOnChipModel> GetNoCModel() {
  	return noc_model;
  }

  std::shared_ptr<maestro::PerformanceAnalysis> GetPerfAnalysis() {
  	return perf_analysis;
  }

  std::shared_ptr<maestro::MappingAnalysis> GetMapAnalysis() {
  	return map_analysis;
  }


  void SetNumPEs(int np) {
    num_pes = np;
  }

  void SetupNoC(int bw, int hops, int hop_latency, bool mc) {
    noc_model = std::make_shared<maestro::NetworkOnChipModel>(bw, hops, hop_latency, mc);
  }

  void SetupInputTensors(std::list<std::string>& in_tensors) {
    input_tensors = in_tensors;

    for(auto& tensor : in_tensors) {
      all_tensors.push_back(tensor);
    }
  }

  void SetupOutputTensors(std::list<std::string>& out_tensors) {
    output_tensors = out_tensors;

    for(auto& tensor : out_tensors) {
      all_tensors.push_back(tensor);
    }
  }

  void ParseInputs(std::string dataflow_file_name, std::string layer_file_name) {
    maestro::PragmaParser prag_parser(dataflow_file_name);
    prag_table = prag_parser.ParsePragmas();
    std::cout<<"\n------[MAESTRO]: Dataflow Information------\n";
    std::cout << prag_table->ToString() << std::endl;

    maestro::ProblemParser prob_parser(layer_file_name);
    std::cout<<"\n------[MAESTRO]: Layer Information------\n";
    loop_info_table = prob_parser.ParseProblem();
    std::cout << loop_info_table->ToString() << std::endl;
  }

  void ConfigureProblem() {
    map_analysis = std::make_shared<maestro::MappingAnalysis>(prag_table, loop_info_table);
    map_analysis->PreProcess(num_pes);

    std::list<std::string> weight_vars = {"K","C","R","S"} ;
    map_analysis->AddTensor("weight", weight_vars);

    std::list<std::string> input_vars = {"C","Y","X"} ;
    map_analysis->AddTensor("input", input_vars);

    std::list<std::string> output_vars = {"K","Y","X"} ;
    map_analysis->AddTensor("output", output_vars);

  }

  void AnalyzeHardware() {
    std::cout<<"------[MAESTRO]: Hardware Information------" << std::endl;;
    std::cout<<"Number of PEs: " << num_pes << std::endl;
    std::cout<<"NoC Bandwidth: " << noc_model->GetBandwidth() << std::endl;
    std::cout << std::endl;
  }

  void AnalyzeMapping() {
    std::cout << "Per PE mapping size analysis" << std::endl;
    std::cout << "Num mapped weights: " << map_analysis->GetMappedSize("weight",false, false)
              << ", Num spatially mapped unique weights: " << map_analysis->GetMappedSize("weight", false, true)
              << ", Num temporally mapped unique weights " << map_analysis->GetMappedSize("weight", true, false)
              << ", Num temporally and spatially mapped unique weights " << map_analysis->GetMappedSize("weight", true, true)
              << std::endl;
    std::cout << std::endl;

    std::cout << "Num mapped inputs: " << map_analysis->GetMappedSize("input",false, false)
              << ", Num spatially mapped unique inputs: " << map_analysis->GetMappedSize("input", false, true)
              << ", Num temporally mapped unique inputs " << map_analysis->GetMappedSize("input", true, false)
              << ", Num temporally and spatially mapped unique inputs " << map_analysis->GetMappedSize("input", true, true)
              << std::endl;
    std::cout << std::endl;

    std::cout << "Num mapped outputs: " << map_analysis->GetMappedSize("output",false, false)
              << ", Num spatially mapped unique outputs: " << map_analysis->GetMappedSize("output", false, true)
              << ", Num temporally mapped unique outputs " << map_analysis->GetMappedSize("output", true, false)
              << ", Num temporally and spatially mapped unique outputs " << map_analysis->GetMappedSize("output", true, true)
              << std::endl;
    std::cout << std::endl;

    std::cout << "Spatially reduced outputs per PE in the steady state: " << map_analysis->GetMappedSize("output",false, false) - map_analysis->GetMappedSize("output", true, true) <<std::endl;
    std::cout << std::endl;
  }

  void AnalyzeBuffer(bool silent = false) {
    buff_analysis = std::make_shared<maestro::BufferAnalysis>(map_analysis, noc_model, num_pes);

    if(!silent) {
    	std::cout << "L1 Buffer requirement (per PE): " << buff_analysis->GetL1BufferRequiredSize(all_tensors) << " Bytes" << std::endl;
    	std::cout << "L2 Buffer requirement: " << buff_analysis->GetL2BufferRequiredSize(all_tensors) << " Bytes" << std::endl;

    	std::cout << "L1 Buffer Rd Weight: " << buff_analysis->GetL1BufferRead("weight") << std::endl;
    	std::cout << "L1 Buffer Rd Input: " << buff_analysis->GetL1BufferRead("input") << std::endl;
    	std::cout << "L1 Buffer Rd Output: " << buff_analysis->GetL1BufferRead("output") << std::endl;

    	std::cout << std::endl;

    	std::cout << "L1 Buffer Wr Weight: " << buff_analysis->GetL1BufferWrite("weight", true, true) << std::endl;
    	std::cout << "L1 Buffer Wr Input: " << buff_analysis->GetL1BufferWrite("input", true, true) << std::endl;
    	std::cout << "L1 Buffer Wr Output: " << buff_analysis->GetL1BufferWrite("output", true, true) << std::endl;

    	std::cout << std::endl;

    	std::cout << "L2 Buffer Rd Weight: " << buff_analysis->GetL2BufferRead("weight", true, true) << std::endl;
    	std::cout << "L2 Buffer Rd Input: " << buff_analysis->GetL2BufferRead("input", true, true) << std::endl;
    	std::cout << "L2 Buffer Rd Output: " << buff_analysis->GetL2BufferRead("output", true, true) << std::endl;


    	std::cout << std::endl;

    	std::cout << "L2 Buffer Wr Weight: " << buff_analysis->GetL2BufferWrite("weight", true, true) << std::endl;
    	std::cout << "L2 Buffer Wr Input: " << buff_analysis->GetL2BufferWrite("input", true, true) << std::endl;
    	std::cout << "L2 Buffer Wr Sum: " << buff_analysis->GetL2BufferWrite("output", true, true) << std::endl;
    }

    std::cout << std::endl;
  }

  double AnalyzeEnergy() {
    double energy_consumption_ = 0.0;

    double l1_energy = 0.0;

    l1_energy += buff_analysis->GetL1BufferRead("weight");
    l1_energy += buff_analysis->GetL1BufferRead("input");
    l1_energy += buff_analysis->GetL1BufferRead("output");

    l1_energy += buff_analysis->GetL1BufferWrite("weight", true, true);
    l1_energy += buff_analysis->GetL1BufferWrite("input", true, true);
    l1_energy += buff_analysis->GetL1BufferWrite("output", true, true);

    l1_energy *= 2.91;

    double l2_energy = 0.0;

    l2_energy += buff_analysis->GetL2BufferRead("weight");
    l2_energy += buff_analysis->GetL2BufferRead("input");
    l2_energy += buff_analysis->GetL2BufferRead("output");

    l2_energy += buff_analysis->GetL2BufferWrite("weight", true, true);
    l2_energy += buff_analysis->GetL2BufferWrite("input", true, true);
    l2_energy += buff_analysis->GetL2BufferWrite("output", true, true);

    l2_energy *= 32.2;

    return l1_energy + l2_energy;
    std::cout << std::endl;
  }

  void AnalyzeReuse() {
    std::cout<<"------[MAESTRO]: Reuse analysis ------" << std::endl;
    std::cout<<"Total computations (the number of partial sums): " << loop_info_table->GetTotalIterations() << std::endl;
    std::cout<< "" <<std::endl;

    std::cout << "  1. Weight" << std::endl;
    std::cout<<"  Weight: Total number of values " << map_analysis->GetFullSize("weight") << std::endl;
    std::cout<<"  Weight: Spatial reuse factor (multicast factor)" << buff_analysis->GetSpatialReuse("weight") << std::endl;
    std::cout<<"  Weight: Temporal reuse factor (The number of temporal reuse per data point)" << buff_analysis->GetTemporalReuse("weight") << std::endl;
    std::cout<< "" <<std::endl;

    std::cout << "  2. Input" << std::endl;
    std::cout<<"  Input: Total values " << map_analysis->GetFullSize("input") << std::endl;
    std::cout<<"  Input: Spatial reuse factor (multicast factor)" << buff_analysis->GetSpatialReuse("input") << std::endl;
    std::cout<<"  Input: Temporal reuse factor (The number of temporal reuse per data point)" << buff_analysis->GetTemporalReuse("input") << std::endl;
    std::cout<< "" <<std::endl;

    std::cout << "  3. Output" << std::endl;
    std::cout<<"  Output: Total values " << map_analysis->GetFullSize("output") << std::endl;
    std::cout<<"  Output: Spatial reuse factor (Partial sum accumulation via PE-to-PE communication)" << buff_analysis->GetSpatialReuse("output") << std::endl;
    std::cout<<"  Output: Temporal reuse factor (The number of temporal reuse per data point)" << buff_analysis->GetTemporalReuse("output") << std::endl;
    std::cout<< "" <<std::endl;

    std::cout << std::endl;
  }

  void AnalyzeRuntime(int num_alus_per_pe = 1, bool do_reduction = true, bool do_implicit_reduction = true, bool fg_sync = false, bool latency_hiding = true) {
    perf_analysis = std::make_shared<maestro::PerformanceAnalysis> (map_analysis, buff_analysis, noc_model, do_reduction, do_implicit_reduction, fg_sync);

    long runtime = perf_analysis->GetRunTime (input_tensors, output_tensors, num_pes, num_alus_per_pe, latency_hiding);

    std::cout<<"------[MAESTRO]: Runtime and Energy details------" << std::endl;

      std::cout << "L1 Weight Buffer requirement (per PE): " << buff_analysis->GetL1BufferRequiredSize({"weight"}) << " Bytes" << std::endl;
      std::cout << "L1 Input Buffer requirement (per PE): " << buff_analysis->GetL1BufferRequiredSize({"input"}) << " Bytes" << std::endl;
      std::cout << "L1 Output Buffer requirement (per PE): " << buff_analysis->GetL1BufferRequiredSize({"output"}) << " Bytes" << std::endl;
      std::cout << std::endl;

    long temporal_iterations = map_analysis->GetNumTemporalIterations();
    long spatial_foldings = map_analysis->GetNumSpatialFoldings();

    std::cout<< "The number of temporal iterations: " << temporal_iterations << std::endl;
    std::cout<< "The number of spatial foldings: " << spatial_foldings << std::endl;
    std::cout<< "The number of total iterations: " << temporal_iterations * spatial_foldings << std::endl;
    std::cout << "Total Runtime: " << runtime << " cycles" << std::endl;
    std::cout << "Total Energy: " << AnalyzeEnergy()/(float) (1.73) << " times MAC energy" << std::endl;
  }

} //End of namespace maestro
