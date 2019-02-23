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

#ifndef MAESTRO_ANALYSIS_STRUCTURE_HPP_
#define MAESTRO_ANALYSIS_STRUCTURE_HPP_

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

namespace maestro {

  class LoopInformation {
    protected:
      int loop_id_;
      std::string loop_var_;
      int base_;
      int bound_;
      int incr_;
    public:
      LoopInformation(std::string loop_var, int base, int bound) :
        loop_id_(-1),
        incr_(1),
        loop_var_(loop_var),
        base_(base),
        bound_(bound)
      {
        if(bound < base) {
          std::cout << "Warning: invalid loop" << std::endl;
        }
      }

      std::string ToString() {
        std::string ret = boost::str(boost::format("Loop %s, base: %d, bound: %d")
                                        % loop_var_
                                        % base_
                                        % bound_ );
        return ret;
      }

      std::string GetLoopVar() {
        return loop_var_;
      }

      int GetBase() {
        return base_;
      }

      int GetBound() {
        return bound_;
      }

      int GetNumIter() {
        int ret = (bound_ - base_) / incr_;
        return ret;
      }

  }; // End of class LoopInformation

  class LoopInfoTable {
    protected:
      std::shared_ptr<std::vector<std::shared_ptr<LoopInformation>>> info_table_;

    public:
      LoopInfoTable() {
        info_table_ = std::make_shared<std::vector<std::shared_ptr<LoopInformation>>>();
      }

      std::string ToString() {
        std::string ret = "";

        for(auto& loop : *info_table_) {
          ret += loop->ToString() + "\n";
        }

        return ret;
      }


      void AddLoop(std::shared_ptr<LoopInformation> new_loop) {
        info_table_->push_back(new_loop);
      }

      std::shared_ptr<std::list<std::shared_ptr<LoopInformation>>> FindLoops(std::string loop_var) {
         auto ret = std::make_shared<std::list<std::shared_ptr<LoopInformation>>>();

        for(auto& loop_info : *info_table_) {
          if(loop_info->GetLoopVar() == loop_var) {
            ret->push_back(loop_info); // This is not a bug; Just IDE does not understand auto around here
          }
        }

        return ret;
      }

      long GetTotalIterations() {
      	long ret = 1;

      	for(auto& loop: *info_table_) {
      		ret *= loop->GetBound();
      	}

      	return ret;
      }

  }; // End of class LoopInfoTable

  class PragmaTable {
    public:
      PragmaTable() {
        pragma_table_ = std::make_shared<std::vector<std::shared_ptr<Pragma>>>();
      }

      class iterator {
        public:
          iterator() = default;
          iterator(std::shared_ptr<std::vector<std::shared_ptr<Pragma>>> ptr):
            pragma_table_ptr_(ptr.get()),
            pos_(0)
          {
          }

          iterator(std::shared_ptr<std::vector<std::shared_ptr<Pragma>>> ptr, int pos):
            pragma_table_ptr_(ptr.get()),
            pos_(pos)
          {
          }

          iterator operator--() {
            --pos_;
            return *this;
          }

          iterator operator++() {
            ++pos_;
            return *this;
          }

          bool operator!=(const iterator & other) const {
            return pos_ != other.pos_;
          }

          const std::shared_ptr<Pragma>& operator*() const {
            return (*pragma_table_ptr_)[pos_];
          }

        private:
          int pos_ = 0;
          std::vector<std::shared_ptr<Pragma>>* pragma_table_ptr_;

      };

      iterator begin() const {
        return iterator(pragma_table_, 0);
      }

      iterator end() const {
        int num_entries = pragma_table_->size();
        return iterator(pragma_table_, num_entries);
      }

      std::string ToString() {
        std::string ret = "";

        for(auto& pragma : *pragma_table_) {
          ret += pragma->ToString() + "\n";
        }

        return ret;
      }

      void AddPragma(std::shared_ptr<Pragma> prag) {
        pragma_table_->push_back(prag);
      }

      std::shared_ptr<std::list<std::shared_ptr<Pragma>>> FindPragma(std::string var_name) {
        auto ret = std::make_shared<std::list<std::shared_ptr<Pragma>>>();

        for(auto& prg : *pragma_table_) {
          if(prg->GetVarName() == var_name) {
            ret->push_back(prg); // This is not a bug; Just IDE does not understand auto around here
          }
        }
        return ret;
      }

      std::shared_ptr<Pragma> GetPragma(int pos) {
        std::shared_ptr<Pragma> ret = nullptr;
        if(pos < pragma_table_->size()) {
          ret = pragma_table_->at(pos);
        }

        return ret;
      }

      int GetPragmaCounts() {
        return pragma_table_->size();
      }

      void SetPragma(std::shared_ptr<Pragma> new_prag, int pos) {
      	pragma_table_->at(pos) = new_prag;
      }

    protected:
      std::shared_ptr<std::vector<std::shared_ptr<Pragma>>> pragma_table_;


  }; // End of class PragmaTable

}; // End of namespace maestro

#endif
