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

#ifndef MAESTRO_MAPPING_ANALYSIS_HPP_
#define MAESTRO_MAPPING_ANALYSIS_HPP_



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

namespace maestro{


  class MappingAnalysis {


    public:
      MappingAnalysis(std::shared_ptr<PragmaTable> prag_tbl, std::shared_ptr<LoopInfoTable> loop_tbl) :
        pragma_table_(prag_tbl),
        loop_info_table_(loop_tbl),
        num_edge_tiles_(0)
      {
      }

      void PreProcess(int num_pes) {
        AnalyzeSpatialMapPoints();
        AnalyzeNumTiles(num_pes);
        AnalyzeTemporalIterations();
        AnalyzeUnrollMerge();
        AnalyzeMapSizes(); // Need to call AnalyzeUnrollMerge frist
        AnalyzeSpatialFoldings();
      }

      void Reset() {
      	spatial_map_points_.clear();
      	spatial_foldings_.clear();
      }

      void FullReset() {
      	Reset();

      	num_tiles_.clear();
      	num_temporal_iterations_.clear();

      	is_unrolled_.clear();
      	is_merged_.clear();

      	mapped_elements_.clear();
      	sp_mapped_unique_elements_.clear();
      	tp_mapped_unique_elements_.clear();

      	sp_mapped_reused_elements_.clear();
      	tp_mapped_reused_elements_.clear();
      }

      void AddTensor(std::string tensor_name, std::list<std::string> variable_list) {
        tensor_variables_[tensor_name] = variable_list;
      }

      long GetMappedSize(std::string tensor_name, bool temporal_reuse, bool spatial_reuse) {
        long ret = 1;

        auto sp_map_var= this->GetSpMapVariable();
        int sp_map_size = this->GetSpVarMapSz();

        for(auto& var : tensor_variables_[tensor_name]) {
          int mult = 1;
          bool is_var_correlated_sp_map = this->HasVariable(tensor_name, sp_map_var) && this->HasVariable(tensor_name, var);
          auto cls = pragma_table_->FindPragma(var)->front()->GetClass();

          if(temporal_reuse && spatial_reuse) {
            switch(cls) {
              case PragmaClass::TEMPORAL_MAP : {
                mult = tp_mapped_unique_elements_[var];
                break;
              }
              case PragmaClass::SPATIAL_MAP : {
                mult = sp_mapped_unique_elements_[var];
                break;
              }
              case PragmaClass::UNROLL : {
                mult = tp_mapped_unique_elements_[var];
                break;
              }
              default:
                break;
            }
          }
          else if (temporal_reuse && !spatial_reuse) {
            mult = tp_mapped_unique_elements_[var];
          }
          else if (!temporal_reuse && spatial_reuse) {
            mult = sp_mapped_unique_elements_[var];
          }
          else {
            mult = mapped_elements_[var];
          }

//          if(mult == 0) mult = 1;

          ret *= mult;
        }

        return ret;
      }


      int GetSpMappedSize(std::string tensor_name, bool enable_spatial_reuse = false) {
        int ret = 1;

        auto var_list = tensor_variables_[tensor_name];
        for(auto& var : var_list) {
          int mult = enable_spatial_reuse? sp_mapped_unique_elements_[var] : mapped_elements_[var];
          ret *= mult;
        }

        return ret;
      }

      int GetTpMappedSize(std::string tensor_name, bool enable_temporal_reuse = false) {
        int ret = 1;

        auto var_list = tensor_variables_[tensor_name];
        for(auto& var : var_list) {
          int mult = enable_temporal_reuse? tp_mapped_unique_elements_[var] : mapped_elements_[var];
          ret *= mult;
        }

        return ret;
      }

      bool HasVariable(std::string tensor_name, std::string var_name) {
        bool ret = false;

        auto var_list = tensor_variables_[tensor_name];
        for(auto& var : var_list) {
          if(var_name == var) ret =true;
        }

        return ret;
      }

      std::string GetSpMapVariable() {
        std::string ret = "";

        for(auto& pragma : *pragma_table_) {
          if(pragma->GetClass() == PragmaClass::SPATIAL_MAP) {
            ret = pragma->GetVarName();
          }
        }

        return ret;
      }

      int GetSpVarMapSz() {
        int ret = 1;

        for(auto& pragma : *pragma_table_) {
          if(pragma->GetClass() == PragmaClass::SPATIAL_MAP) {
            ret = pragma->GetSize();
          }
        }

        return ret;
      }

      long GetFullSize(std::string tensor_name) {
        long full_size = 1;
        auto targ_tensor_vars = tensor_variables_[tensor_name];
        for(auto& var_name : targ_tensor_vars) {
          auto corr_loop_list = loop_info_table_->FindLoops(var_name);
          //TODO: Extend it to general loop nest cases

          full_size *= static_cast<long>(corr_loop_list->front()->GetNumIter());
        }

        return full_size;
      }

      int GetLoopBound(std::string var_name) {

      	int loop_bound = -1;

        for(auto& prag : *pragma_table_) {
          auto loop_var = prag->GetVarName();
          if(loop_var == var_name) {
          	auto corresponding_loops = loop_info_table_->FindLoops(loop_var);
          	auto target_loop = corresponding_loops->front();
          	loop_bound = target_loop->GetNumIter();
          }
        }

        return loop_bound;
      }

      void SetMapSize(std::string var_name, int size, int ofs, PragmaClass targPragmaClass) {

      	int pos = 0;
        for(auto prag : *pragma_table_) {
          auto loop_var = prag->GetVarName();

          if(loop_var == var_name && (prag->GetClass() == PragmaClass::TEMPORAL_MAP || prag->GetClass() == PragmaClass::SPATIAL_MAP)) {
          	switch(targPragmaClass) {
							case PragmaClass::TEMPORAL_MAP: {
								auto new_prag = std::make_shared<TemporalMap>(var_name, size, ofs);
								pragma_table_->SetPragma(new_prag, pos);
								break;
							}
							case PragmaClass::SPATIAL_MAP: {
								auto new_prag = std::make_shared<SpatialMap>(var_name, size, ofs);
								pragma_table_->SetPragma(new_prag, pos);
								break;
							}
							default: {
								// Do nothing
							}
          	}
          }

          pos++;
        }

      }


      long GetTemporalChangeFrequency(std::string target_tensor) {
        long ret;
        std::vector<int> mult2;
        long mult = 1;


        for(auto& sp_map_info : spatial_map_points_) {
//        	std::cout << "SP Map info iteration " << std::endl;
          auto sp_var = std::get<0>(sp_map_info);
          int sp_map_prag_id = std::get<1>(sp_map_info);

          if(this->HasVariable(target_tensor, sp_var)) {
            return 1;
          }

          int prag_id = 0;
          bool saw_related_value = false;

          for(auto& prag : *pragma_table_) {
            auto loopvar = prag->GetVarName();

            if(this->HasVariable(target_tensor, loopvar)) {
              saw_related_value = true;
            }
            else if(prag_id < sp_map_prag_id && saw_related_value ) {
              auto loop_info = loop_info_table_->FindLoops(loopvar)->front();
//                mult2.push_back((prag->GetClass() == PragmaClass::UNROLL)? 1 : loop_info->GetNumIter()/prag->GetOffset());
              int test_zero = loop_info->GetNumIter()/prag->GetSize();
              test_zero = (test_zero == 0)? 1 : test_zero;
                mult *= (prag->GetClass() == PragmaClass::UNROLL)? 1 : test_zero;
//                std::cout << "LoopVar: " << loopvar << ", Num iter: " << loop_info->GetNumIter() << ", Size: " <<  prag->GetSize() << ", Offset: " << prag->GetOffset() << ", mult: " << mult << std::endl;
            }
            prag_id++;
          }
        }
        ret = mult;
//        std::cout << "ret = " << ret << std::endl;

        return ret;
      }


      std::list<std::tuple<std::string, int>> GetNumSpatialTiles() {
        std::list<std::tuple<std::string, int>> num_spatial_tiles;

        for(auto& sMapPoint : spatial_map_points_) {
          auto sMapLoopVar = std::get<0>(sMapPoint);
          num_spatial_tiles.push_back({sMapLoopVar, num_tiles_[sMapLoopVar]});
        }

        return num_spatial_tiles;
      }

      int GetNumEdgeTiles() {
        return num_edge_tiles_;
      }

      int GetNumTemporalIterations() {
        //TODO: Extend it for multi-level spatial mapping cases
        return num_temporal_iterations_.front();
      }

      int GetNumSpatialFoldings () {
        auto sp_fold_info = spatial_foldings_.front();
        return std::get<1>(sp_fold_info);
      }

      long GetTotalIterations () {
      	return loop_info_table_->GetTotalIterations();
      }

    protected:
      std::shared_ptr<PragmaTable> pragma_table_;
      std::shared_ptr<LoopInfoTable> loop_info_table_;

      std::map<std::string, int> num_tiles_;
      std::list<std::tuple<std::string, int>> spatial_map_points_;
      std::list<std::tuple<std::string, int>> spatial_foldings_;

      int num_edge_tiles_;

      std::vector<int> num_temporal_iterations_;
      std::map<std::string, bool> is_unrolled_; //Unroll
      std::map<std::string, bool> is_merged_; //Merge
      std::map<std::string, int> mapped_elements_; //TSz
      std::map<std::string, int> sp_mapped_unique_elements_; //TUSz
      std::map<std::string, int> tp_mapped_unique_elements_; //TUSz

      std::map<std::string, int> sp_mapped_reused_elements_; //
      std::map<std::string, int> tp_mapped_reused_elements_; //

      /* One invaraint
       *
       * mapped_element[v] == mapped_unique_elements[v] + mapped_reused_elements[v]
       *
       * */


      std::map<std::string, std::list<std::string>> tensor_variables_;

    private:

      void AnalyzeSpatialMapPoints() {
        int pragma_id = 0;
        for(auto& pragma : *pragma_table_) {
          if(pragma->GetClass() == PragmaClass::SPATIAL_MAP) {
//          	std::cout << "SMAP size: " << pragma->GetSize() << ", Offset: " << pragma->GetOffset() << std::endl;
            spatial_map_points_.push_back({pragma->GetVarName(), pragma_id});
          }
          pragma_id++;
        }
      }

      void AnalyzeSpatialFoldings() {

        for(auto& pragma : *pragma_table_) {
          if(pragma->GetClass() == PragmaClass::SPATIAL_MAP) {

            auto loop_var = pragma->GetVarName();
            int ofs = pragma->GetOffset();

            auto corresponding_loops = loop_info_table_->FindLoops(loop_var);
            //TODO:Extend it for general cases
            auto loop_info = corresponding_loops->front();
            auto loop_sz = loop_info->GetNumIter();

            auto sp_tile_info = this->GetNumSpatialTiles();
            //TODO: Extend it to multi-level spatial map
            int num_sp_tiles = std::get<1>(sp_tile_info.front());

//            std::cout << "num_sp_tiles: " << num_sp_tiles << std::endl;

            int num_spatial_foldings = loop_sz / ofs / num_sp_tiles;
            if(num_spatial_foldings == 0) {
              num_spatial_foldings = 1;
            }

            num_edge_tiles_ = (loop_sz / ofs) % num_sp_tiles;
            if(num_edge_tiles_ == 0) num_edge_tiles_ = num_sp_tiles;
            spatial_foldings_.push_back({pragma->GetVarName(), num_spatial_foldings});
          }
        }

      }

      void AnalyzeUnrollMerge() {

        for(auto& pragma : *pragma_table_) {
          auto loop_var = pragma->GetVarName();
          is_unrolled_[loop_var] = false;
          is_merged_[loop_var] = false;
        }

        for(auto& pragma : *pragma_table_) {
          auto loop_var = pragma->GetVarName();

          if(pragma->GetClass() == PragmaClass::UNROLL) {
            is_unrolled_[loop_var] = true;
          }
          if(pragma->GetClass() == PragmaClass::MERGE) {
            is_merged_[loop_var] = true;
          }
        }
      }

      void AnalyzeNumTiles(int num_pes) {
        int curr_num_tiles = num_pes;

        for(auto& pragma : *pragma_table_) {
          if(pragma->GetClass() == PragmaClass::TILE) {
            curr_num_tiles = curr_num_tiles / pragma->GetSize();
          }
          num_tiles_[pragma->GetVarName()] = curr_num_tiles;
//            std::cout << "NumTiles[" << pragma->GetVarName() << "] = " << curr_num_tiles << std::endl;
        }
      }

      void AnalyzeTemporalIterations() {

        int curr_base = 0;
        int curr_bound = 0;

        for(auto& sMapPoint : spatial_map_points_) {
          curr_bound = std::get<1>(sMapPoint);

          curr_bound = pragma_table_->GetPragmaCounts(); //TODO

          int num_temp_iter = 1;

          for(int prag_id = curr_base; prag_id < curr_bound; prag_id++) {
            auto targ_pragma = pragma_table_->GetPragma(prag_id);
            if(targ_pragma->GetClass() != PragmaClass::TILE && targ_pragma->GetClass() != PragmaClass::SPATIAL_MAP) {
              auto match_loop_list = loop_info_table_->FindLoops(pragma_table_->GetPragma(prag_id)->GetVarName());
              int ofs = targ_pragma->GetOffset();
              for(auto& loop : *match_loop_list) {
                //TODO: Extend it to general loop nest cases
                //if(loop_block_id matches)
                if(targ_pragma->GetClass() != PragmaClass::UNROLL) {  //TODO
                  int mult = (loop->GetNumIter()/ofs);
                  mult = (mult == 0)? 1 : mult;
                  num_temp_iter *= mult;
//                  std::cout << "For pragma " << targ_pragma->ToString() << ", temp it size = " << mult <<std::endl;
                }  //TODO
              }
            }
          }

          num_temporal_iterations_.push_back(num_temp_iter);
  //          std::cout << "Temporal iteration count " << num_temp_iter << std::endl;
          curr_base = curr_bound;
        }
      }

      void AnalyzeMapSizes() {
        for(auto& prag : *pragma_table_) {
          auto prag_class = prag->GetClass();
          int map_size = prag->GetSize();
          int offset = prag->GetOffset();

          auto loop_var = prag->GetVarName();
          auto corresponding_loops = loop_info_table_->FindLoops(loop_var);
          //TODO: Extend it to generael case; non-perfectly-nested loop case
          auto target_loop = corresponding_loops->front();
          int loop_size = target_loop->GetNumIter();

          switch(prag_class) {
            case PragmaClass::TEMPORAL_MAP: {
              if(!is_unrolled_[loop_var] && !is_merged_[loop_var]){
//              	std::cout << "Map size (" << loop_var << "): " << map_size << std::endl;
                mapped_elements_[loop_var] = map_size;
                tp_mapped_unique_elements_[loop_var] = (map_size > offset)? offset : map_size;
                sp_mapped_unique_elements_[loop_var] = map_size;
                tp_mapped_reused_elements_[loop_var] = (map_size > offset)? map_size - offset : 0;
                sp_mapped_reused_elements_[loop_var] = 0; // No spatial reuse when temorally mapped
              }
              else {
                std::cout << "[MAESTRO] Error; a loop cannot be unrolled or merged and temporally mapped at the same time" << std::endl;
                exit(-1);
              }
              break;
            } // End of case TEMPORAL_MAP
            case PragmaClass::SPATIAL_MAP: {

              if(!is_unrolled_[loop_var] && !is_merged_[loop_var]){
              	//std::cout << "Map size (" << loop_var << "): " << map_size << std::endl;
                mapped_elements_[loop_var] = map_size;
                tp_mapped_unique_elements_[loop_var] = map_size; //No temporal reuse
                sp_mapped_unique_elements_[loop_var] = (map_size > offset)? offset : map_size;
                tp_mapped_reused_elements_[loop_var] = 0; //No temporal reuse
                sp_mapped_reused_elements_[loop_var] = (map_size > offset)? map_size - offset : 0;
              }
              else {
                std::cout << "[MAESTRO] Error; a loop cannot be unrolled or merged and temporally mapped at the same time" << std::endl;
                exit(-1);
              }
              break;
            } // End of case SPATIAL_MAP
            case PragmaClass::UNROLL: {
              is_unrolled_[loop_var] = true;
              mapped_elements_[loop_var] = loop_size;
              tp_mapped_unique_elements_[loop_var] = loop_size;
              sp_mapped_unique_elements_[loop_var] = 1;
              tp_mapped_reused_elements_[loop_var] = loop_size; //No temporal reuse
              sp_mapped_reused_elements_[loop_var] = loop_size;

              break;
            } // End of case UNROLL
            case PragmaClass::MERGE: {
              is_merged_[loop_var] = true;
              //TODO: Implement
              break;
            }
            default:
              break;
          } // End of switch(prag_class)
        } // End of for(prag : pragma_table)
      } // End of  function AnalyzeMapSizes


  }; // End of class MappingAnalysis
}; // End of namespace maestro

#endif
