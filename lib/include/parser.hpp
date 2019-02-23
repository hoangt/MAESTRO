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

#ifndef MAESTRO_PRAGMA_PARSER_HPP_
#define MAESTRO_PRAGMA_PARSER_HPP_

#include <string>
#include <iostream>
#include <fstream>
#include <cstdlib>

#include<boost/tokenizer.hpp>
#include<boost/format.hpp>

#include "analysis-structure.hpp"

namespace maestro {

  class InputParser {
    protected:
      std::string file_name_;
      std::ifstream in_file_;

    public:
      InputParser(std::string file_nm) :
        file_name_(file_nm)
      {
        in_file_.open(file_nm);
        if(!in_file_) {
          std::cout << "Failed to open the input file" << std::endl;
        }
      }
  }; // End of class InputParser

  class PragmaParser : public InputParser {
    protected:
      int num_pes_;

    public:
      PragmaParser(std::string file_nm) :
        InputParser(file_nm),
        num_pes_(1)
      {
      }

      PragmaParser(std::string file_nm, int num_pes) :
        InputParser(file_nm),
        num_pes_(num_pes)
      {
      }

      std::shared_ptr<PragmaTable> ParsePragmas() {
        auto prag_table = std::make_shared<PragmaTable>();
        std::string line;

        //Read a line of the file
        while(std::getline(in_file_, line)) {
          boost::char_separator<char> sep(" ,->()");
          boost::tokenizer<boost::char_separator<char>> tokn(line, sep);

          PragmaClass pragma_cls = PragmaClass::INVALID;
          std::string loop_var = default_loop_var;
          bool saw_size = false;
          bool saw_ofs = false;

          int map_size = 1;
          int tile_size = 1;
          int map_offset = 1;

          for(auto& tok : tokn) {

            switch(pragma_cls) {
              case(PragmaClass::INVALID): {
                if(tok == tkn_temporal_map) {
                  pragma_cls = PragmaClass::TEMPORAL_MAP;
                }
                else if (tok == tkn_spatial_map) {
                  pragma_cls = PragmaClass::SPATIAL_MAP;
                }
                else if (tok == tkn_unroll) {
                  pragma_cls = PragmaClass::UNROLL;
                }
                else if (tok == tkn_merge) {
                  pragma_cls = PragmaClass::MERGE;
                }
                else if (tok == tkn_tile) {
                  pragma_cls = PragmaClass::TILE;
                }
                break;
              }
              case(PragmaClass::TEMPORAL_MAP):
              case(PragmaClass::SPATIAL_MAP): {
                if(!saw_size) {
                  map_size = std::atoi(tok.c_str());
                  saw_size = true;
                }
                else if(!saw_ofs) {
                  map_offset = std::atoi(tok.c_str());
                  saw_ofs = true;
                }
                else {
                  loop_var = tok;
                }
                break;
              }
              case(PragmaClass::TILE): {
                if(!saw_size) {
                  tile_size = std::atoi(tok.c_str());
                  saw_size = true;
                }
                else {
                  loop_var = tok;
                }
                break;
              }
              default: { //Merge and unroll
                loop_var = tok;
                break;
              }
            }
          } // End of for that tokenizes and lexes a line

          switch(pragma_cls) {
            case(PragmaClass::TEMPORAL_MAP):{
              std::shared_ptr<Pragma> new_pragma = std::make_shared<TemporalMap>(loop_var, map_size, map_offset);
              prag_table->AddPragma(new_pragma);
              break;
            }
            case(PragmaClass::SPATIAL_MAP): {
              std::shared_ptr<Pragma> new_pragma = std::make_shared<SpatialMap>(loop_var, map_size, map_offset, num_pes_);
              prag_table->AddPragma(new_pragma);
              break;
            }
            case(PragmaClass::TILE): {
              //std::cout << " TILE" << std::endl;
              std::shared_ptr<Pragma> new_pragma = std::make_shared<Tile>(loop_var, tile_size);
              prag_table->AddPragma(new_pragma);
              break;
            }
            case(PragmaClass::UNROLL): {
              std::shared_ptr<Pragma> new_pragma = std::make_shared<Unroll>(loop_var);
              prag_table->AddPragma(new_pragma);
              break;
            }
            case(PragmaClass::MERGE): {
              std::shared_ptr<Pragma> new_pragma = std::make_shared<Merge>(loop_var);
              prag_table->AddPragma(new_pragma);
              break;
            }
            default: //Parse error -> do not add any item
              break;
          }

        }

        return prag_table;
      }
  }; // End of class PragmaParser

  class ProgramParser : public InputParser {

    public:
      ProgramParser(std::string file_nm) :
        InputParser(file_nm) {
      }

      std::shared_ptr<LoopInfoTable> ParseProgram() {
        auto prob_table = std::make_shared<LoopInfoTable>();
        std::string line;

        //Read a line of the file
        while(std::getline(in_file_, line)) {
          boost::char_separator<char> sep(" ,->()");
          boost::tokenizer<boost::char_separator<char>> tokn(line, sep);

          std::string loop_var = default_loop_var;

          bool saw_size = false;
          int size = 0;

          for(auto& tok : tokn) {
            if(loop_var == default_loop_var) {
              loop_var = tok;
            }
            else if (!saw_size){
              size = std::atoi(tok.c_str());
              saw_size = true;
            }
            else {
              //TODO: Update error message with a better version
              std::cout << "[ProblemParser]Warning: Located extra arguments in problem dimension description. Ignoring extra arguments " << std::endl;
            }
          }

          auto loop_info = std::make_shared<LoopInformation>(loop_var, 0, size);
          prob_table->AddLoop(loop_info);

        }
        return prob_table;
      }


  }; // End of class ProgramParser


  class ProblemParser : public InputParser {
    protected:

    public:
      ProblemParser(std::string file_nm) :
        InputParser(file_nm)
      {
      }

      std::shared_ptr<LoopInfoTable> ParseProblem() {
        auto prob_table = std::make_shared<LoopInfoTable>();

        auto prag_table = std::make_shared<PragmaTable>();
        std::string line;

        //Read a line of the file
        while(std::getline(in_file_, line)) {
          boost::char_separator<char> sep(" ,->()");
          boost::tokenizer<boost::char_separator<char>> tokn(line, sep);

          std::string loop_var = default_loop_var;

          bool saw_size = false;
          int size = 0;

          for(auto& tok : tokn) {
            if(loop_var == default_loop_var) {
              loop_var = tok;
            }
            else if (!saw_size){
              size = std::atoi(tok.c_str());
              saw_size = true;
            }
            else {
              //TODO: Update error message with a better version
              std::cout << "[ProblemParser]Warning: Located extra arguments in problem dimension description. Ignoring extra arguments " << std::endl;
            }
          }

          auto loop_info = std::make_shared<LoopInformation>(loop_var, 0, size);
          prob_table->AddLoop(loop_info);

        }
        return prob_table;
      }

  }; // End of class ProblemParser
}; // End of namespace maestro

#endif
