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

#ifndef MAESTRO_COST_ANALYSIS_HPP_
#define MAESTRO_COST_ANALYSIS_HPP_



#include <string>
#include <iostream>
#include <list>
#include <vector>
#include <map>
#include <tuple>
#include <algorithm>

#include "mapping-syntax.hpp"
#include "program-syntax.hpp"
#include "noc-model.hpp"
#include "analysis-structure.hpp"
#include "mapping-analysis.hpp"


namespace maestro {

class BufferAnalysis {
    protected:
      std::shared_ptr<MappingAnalysis> map_analysis_;
      std::shared_ptr<NetworkOnChipModel> noc_model_;

      long num_pes_;
      long num_sp_tiles_;
      long num_sp_edge_tiles_;
      long sp_tile_size_;
      long num_tp_foldings_;
      long num_sp_foldings_;

    public:
      BufferAnalysis(std::shared_ptr<MappingAnalysis> map_analysis, std::shared_ptr<NetworkOnChipModel> noc_model, long num_pes) :
        map_analysis_(map_analysis),
        noc_model_(noc_model),
        num_pes_(num_pes)
      {
        auto sp_tile_info = map_analysis->GetNumSpatialTiles();
        num_sp_tiles_ = static_cast<long>(std::get<1>(sp_tile_info.front()));
        num_tp_foldings_ = static_cast<long>(map_analysis->GetNumTemporalIterations());
        num_sp_foldings_ = static_cast<long>(map_analysis->GetNumSpatialFoldings());
        num_sp_edge_tiles_ = static_cast<long>(map_analysis->GetNumEdgeTiles());
        sp_tile_size_ = static_cast<long>(num_pes_) / num_sp_tiles_;
      }

      int GetL1BufferRequiredSize(std::list<std::string> tensors, bool enable_double_buffering = true) {
        int buff_size = 0;
        for(auto& tensor_name : tensors) {
          buff_size += map_analysis_->GetMappedSize(tensor_name, false, false);
        }
        buff_size = enable_double_buffering? 2 * buff_size : buff_size; // Double buffering
        return buff_size;
      }

      int GetL2BufferRequiredSize(std::list<std::string> tensors) {
        int buff_size = 0;

        for(auto& tensor_name : tensors) {
          int first_pe_sp_data = map_analysis_->GetMappedSize(tensor_name, false, false);
          int other_pe_sp_data = map_analysis_->GetMappedSize(tensor_name, false, true); // Consider spatial reuse
          int num_max_pes = (num_sp_foldings_ == 1)? num_sp_edge_tiles_ : num_sp_tiles_;
          buff_size += first_pe_sp_data + (num_max_pes-1) * other_pe_sp_data;
        }

        return buff_size;
      }

      long GetSpatialL1ToL2Traffic(std::string tensor_name, bool sp_iteration_edge = false, bool enable_temporal_reuse = true, bool enable_spatial_reuse = true) {
        long L1ToL2Traffic;

        long unique_volume = static_cast<long>(map_analysis_->GetMappedSize(tensor_name, enable_temporal_reuse, enable_spatial_reuse));
        L1ToL2Traffic = sp_iteration_edge? num_sp_edge_tiles_ * unique_volume : num_sp_tiles_ * unique_volume;

        return L1ToL2Traffic;
      }

      long GetSpatialL2ToL1Traffic(std::string target_tensor, bool first_tp_iteration, bool sp_iteration_edge, bool enable_temporal_reuse = true, bool enable_spatial_reuse = true) {

        long L2ToL1Traffic = -1;

        bool is_multcast_supported = noc_model_->IsMulticastSupported();


        long tp_change_freq = static_cast<long> (map_analysis_->GetTemporalChangeFrequency(target_tensor));

        long non_reuse_unit_L2Rd = static_cast<long>(map_analysis_->GetMappedSize(target_tensor, false, false));
        long first_tp_first_sp_unit_L2Rd = non_reuse_unit_L2Rd;
        long first_tp_steady_sp_unit_L2Rd =  static_cast<long>(map_analysis_->GetMappedSize(target_tensor, false, enable_spatial_reuse));
        long steady_tp_first_sp_unit_L2Rd = static_cast<long>(map_analysis_->GetMappedSize(target_tensor, enable_temporal_reuse, false));
        long steady_tp_steady_sp_unit_L2Rd = static_cast<long>(map_analysis_->GetMappedSize(target_tensor, enable_temporal_reuse, enable_spatial_reuse));

        long first_tp_steady_sp_L2Rd = (first_tp_first_sp_unit_L2Rd + (num_sp_tiles_-1) * first_tp_steady_sp_unit_L2Rd);
        long first_tp_edge_sp_L2Rd =  (first_tp_first_sp_unit_L2Rd + (num_sp_edge_tiles_-1) * first_tp_steady_sp_unit_L2Rd);
        long steady_tp_steady_sp_L2Rd = (steady_tp_first_sp_unit_L2Rd + (num_sp_tiles_-1) * steady_tp_steady_sp_unit_L2Rd);
        long steady_tp_edge_sp_L2Rd = (steady_tp_first_sp_unit_L2Rd + (num_sp_edge_tiles_-1) * steady_tp_steady_sp_unit_L2Rd);

        if(is_multcast_supported) {
          if(first_tp_iteration && !sp_iteration_edge) {
            L2ToL1Traffic = first_tp_steady_sp_L2Rd;
          }
          else if(first_tp_iteration && sp_iteration_edge) {
            L2ToL1Traffic = first_tp_edge_sp_L2Rd;
          }
          else if(!first_tp_iteration && !sp_iteration_edge) {
            L2ToL1Traffic = steady_tp_steady_sp_L2Rd/tp_change_freq;
          }
          else {
            L2ToL1Traffic = steady_tp_edge_sp_L2Rd/tp_change_freq;
          }
        }
        else {
          if(!sp_iteration_edge) {
            L2ToL1Traffic = num_sp_tiles_ * non_reuse_unit_L2Rd/tp_change_freq;
          }
          else {
            L2ToL1Traffic = num_sp_edge_tiles_ * non_reuse_unit_L2Rd/tp_change_freq;
          }
        }

         return L2ToL1Traffic;
      } // End of GetSpatialL2ToL1Traffic

      long GetL2BufferRead(std::string target_tensor, bool enable_temporal_reuse = true, bool enable_spatial_reuse = true) {
        long L2Rd ;

        long first_tp_steady_sp_L2Rd = this->GetSpatialL2ToL1Traffic(target_tensor, true, false, enable_temporal_reuse, enable_spatial_reuse);
        long first_tp_edge_sp_L2Rd = this->GetSpatialL2ToL1Traffic(target_tensor, true, true, enable_temporal_reuse, enable_spatial_reuse);
        long steady_tp_steady_sp_L2Rd = this->GetSpatialL2ToL1Traffic(target_tensor, false, false, enable_temporal_reuse, enable_spatial_reuse);
        long steady_tp_edge_sp_L2Rd = this->GetSpatialL2ToL1Traffic(target_tensor, false, true, enable_temporal_reuse, enable_spatial_reuse);

        long first_tp_L2Rd = first_tp_edge_sp_L2Rd + (num_sp_foldings_-1) * first_tp_steady_sp_L2Rd;
        long steady_tp_L2Rd = steady_tp_edge_sp_L2Rd + (num_sp_foldings_-1) * steady_tp_steady_sp_L2Rd;

        L2Rd = first_tp_L2Rd + (num_tp_foldings_-1) * steady_tp_L2Rd;

        return L2Rd;
      } // End of GetL2BufferRead


      long GetL2BufferWrite(std::string target_tensor, bool enable_temporal_reuse = true, bool enable_spatial_reuse = true) {
        //TODO: Extend it to non-reuse cases
        long L2Wr = map_analysis_->GetFullSize(target_tensor); //Currently assumes full reuse
        //TODO: Manage output counts

        return L2Wr;
      }

      long GetL1BufferRead(std::string target_tensor) {
        long L1Rd ;

        long sp_read_volume = static_cast<long>(map_analysis_->GetMappedSize(target_tensor, false, false));

        long steady_sp_iteration_L1Rd = sp_tile_size_ * num_sp_tiles_ * sp_read_volume;
        long edge_sp_iteration_L1Rd = sp_tile_size_ * num_sp_edge_tiles_  * sp_read_volume;

        L1Rd = num_tp_foldings_ * ((num_sp_foldings_-1) * steady_sp_iteration_L1Rd + edge_sp_iteration_L1Rd);

        return L1Rd;
      }


      double GetTemporalReuse(std::string target_tensor){
          long L1Rd = this->GetL1BufferRead(target_tensor);
          long total_volume = map_analysis_->GetFullSize(target_tensor);

          double reuse = (L1Rd / (double) (total_volume));
          return reuse;
      }

      double GetSpatialReuse(std::string target_tensor){
          long l1writes = GetL1BufferWrite(target_tensor, true, true);
          long l2reads = GetL2BufferRead(target_tensor, true, true);
          return l1writes/(double) (l2reads);
      }

      long GetL1BufferWrite(std::string target_tensor, bool enable_temporal_reuse = true, bool enable_spatial_reuse = true) {
        long L1Wr = this->GetL2BufferRead(target_tensor, enable_temporal_reuse, false);

        if(noc_model_->IsMulticastSupported()) {
          long multcast_factor = map_analysis_->GetMappedSize(target_tensor, false ,false) / map_analysis_->GetMappedSize(target_tensor, false ,true);
          L1Wr *= multcast_factor;
        }

        return L1Wr;
      }

  }; // End of class BufferAnalysis

  class PerformanceAnalysis {
    protected:
      std::shared_ptr<MappingAnalysis> map_analysis_;
      std::shared_ptr<BufferAnalysis> buffer_analysis_;
      std::shared_ptr<NetworkOnChipModel> noc_model_;
      bool perform_reduction_;
      bool same_cycle_reduction_;
      bool fine_grained_sync_;

    public:
      PerformanceAnalysis(std::shared_ptr<MappingAnalysis> map_analysis, std::shared_ptr<BufferAnalysis> buffer_analysis,
      		                std::shared_ptr<NetworkOnChipModel> noc_model, bool reduction = true, bool same_cycle_reduction = true, bool fg_sync = false) :
        map_analysis_(map_analysis),
        buffer_analysis_(buffer_analysis),
        noc_model_(noc_model),
        perform_reduction_(reduction),
        same_cycle_reduction_(same_cycle_reduction),
        fine_grained_sync_(fg_sync)
      {
      }

      long GetNumOpsPerPE (std::list<std::string> correlated_tensors, bool doCartesianProduct) {
        long num_ops = 1;
        long mult = 1;
        long small_one = 0;

        if(doCartesianProduct) {
          for(auto& tensor_name : correlated_tensors) {
            num_ops *= static_cast<long>(map_analysis_->GetMappedSize(tensor_name, false, false));
          }
        }
        else {
          for(auto& tensor_name : correlated_tensors) {
            long map_size = static_cast<long>(map_analysis_->GetMappedSize(tensor_name, false, false));
            if(map_size > num_ops) {
              mult = num_ops;
              num_ops = map_size;
            }
            else {
              //TODO: Extend it. It is not correct for arbitrary applications;
              mult *= map_size;
            }
          }
        }

        if(perform_reduction_ && !same_cycle_reduction_) {
          num_ops = 2 * num_ops * mult - 1;
        }

        return num_ops;
      }

      long GetRunTime (std::list<std::string> input_tensors, std::list<std::string> output_tensors, int num_pes, int num_alus_per_pe, bool latency_hiding) {

        long runtime = 0;

        int num_tp_foldings = map_analysis_->GetNumTemporalIterations();
        int num_sp_foldings = map_analysis_->GetNumSpatialFoldings();
        int num_sp_edge_tiles = map_analysis_->GetNumEdgeTiles();

        long compute_delay = this->GetNumOpsPerPE(input_tensors, false)/num_alus_per_pe;
        if(compute_delay == 0) compute_delay = 1;

        if(!fine_grained_sync_) {


          long init_traffic_amount = 0;
          for(auto& in_tensor_name : input_tensors) {
            init_traffic_amount += buffer_analysis_->GetSpatialL2ToL1Traffic(in_tensor_name, num_pes, true, true, false);
          }

          long init_noc_delay = noc_model_->GetOutStandingDelay(init_traffic_amount);

          runtime+=init_noc_delay;

          long L2ToL1_traffic = 0;
          long L1ToL2_traffic = 0;

          long L2ToL1_noc_delay = 0;
          long L1ToL2_noc_delay  = 0;

          for(auto& out_tensor_name : output_tensors) {
            L1ToL2_traffic += buffer_analysis_->GetSpatialL1ToL2Traffic(out_tensor_name);
          }
          L1ToL2_noc_delay = noc_model_->GetOutStandingDelay(L1ToL2_traffic);

          long this_iteration_delay = 0;

          /* Analytic model */
          {
            /* 1. Temp iter = 0 */
            // 1-1) Non-edge spatial iterations (steady state)
            if(num_sp_foldings > 2 ) {
              for(auto& in_tensor_name : input_tensors) {
                long tp_change_freq = map_analysis_->GetTemporalChangeFrequency(in_tensor_name);
                L2ToL1_traffic += buffer_analysis_->GetSpatialL2ToL1Traffic(in_tensor_name, num_pes, true, false, false)/tp_change_freq; // tp_iter =0, sp iter is in steady state
              }
              L2ToL1_noc_delay = noc_model_->GetOutStandingDelay(L2ToL1_traffic);
              if(latency_hiding) {
                this_iteration_delay = std::max(L2ToL1_noc_delay, L1ToL2_noc_delay + compute_delay);
              }
              else {
                this_iteration_delay = L2ToL1_noc_delay + compute_delay + L1ToL2_noc_delay;
              }
              runtime += (num_sp_foldings -2) * this_iteration_delay;
            }
            // 1-2) At spatial iteration edge
            L2ToL1_traffic = 0;
            for(auto& in_tensor_name : input_tensors) {
              long tp_change_freq = map_analysis_->GetTemporalChangeFrequency(in_tensor_name);
              L2ToL1_traffic += buffer_analysis_->GetSpatialL2ToL1Traffic(in_tensor_name, num_pes, true, false, true)/tp_change_freq; // tp_iter =0, sp iter is in steady state

            }
            L2ToL1_noc_delay = noc_model_->GetOutStandingDelay(L2ToL1_traffic);

            if(latency_hiding) {
              this_iteration_delay = std::max(L2ToL1_noc_delay, L1ToL2_noc_delay + compute_delay);
            }
            else {
              this_iteration_delay = L2ToL1_noc_delay + compute_delay + L1ToL2_noc_delay;
            }

            runtime += this_iteration_delay;

            /* 2. Temp iter != 0 */
            // 2-1) Non-edge spatial iterations (steady state)
            for(auto& in_tensor_name : input_tensors) {
              long tp_change_freq = static_cast<long> (map_analysis_->GetTemporalChangeFrequency(in_tensor_name));
              L2ToL1_traffic += buffer_analysis_->GetSpatialL2ToL1Traffic(in_tensor_name, num_pes, false, false, false)/tp_change_freq; // tp_iter and sp_iter are in steady states
            }
            L2ToL1_noc_delay = noc_model_->GetOutStandingDelay(L2ToL1_traffic);

            if(latency_hiding) {
              this_iteration_delay = std::max(L2ToL1_noc_delay, L1ToL2_noc_delay + compute_delay);
            }
            else {
              this_iteration_delay = L2ToL1_noc_delay + compute_delay + L1ToL2_noc_delay;
            }

            runtime += (num_tp_foldings-1) * (num_sp_foldings -1) * this_iteration_delay;

            // 2-2) At spatial iteration edge
            L2ToL1_traffic = 0;
            for(auto& in_tensor_name : input_tensors) {
              long tp_change_freq = static_cast<long> (map_analysis_->GetTemporalChangeFrequency(in_tensor_name));
              L2ToL1_traffic += buffer_analysis_->GetSpatialL2ToL1Traffic(in_tensor_name, num_pes, false, false, true)/tp_change_freq; // tp_iter is in steady states (does not distinguish edge), sp_iter is at edge
            }
            L2ToL1_noc_delay = noc_model_->GetOutStandingDelay(L2ToL1_traffic);

            if(latency_hiding) {
              this_iteration_delay = std::max(L2ToL1_noc_delay, L1ToL2_noc_delay + compute_delay);
            }
            else {
              this_iteration_delay = L2ToL1_noc_delay + compute_delay + L1ToL2_noc_delay;
            }

            runtime += (num_tp_foldings-1) * this_iteration_delay;
          }
        } // End of if (!fine_grained_sync_)
        else {
          //TODO: Add fine-grained sync case


        }

        return runtime;
      } // End of GetRunTime
  };


};


#endif
