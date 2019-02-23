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

#ifndef MAESTRO_PROGRAM_SYNTAX_HPP_
#define MAESTRO_PROGRAM_SYNTAX_HPP_

#include <string>

namespace maestro {

  enum class BinaryOp {
    ADD,
    SUB,
    MUL,
    DIV,
    INVALID
  };

  class Expression {
    protected:

    public:
      virtual ~Expression() {}

      virtual std::string GetName() {
        return "";
      }

  };

  class Container : public Expression {
    protected:
      std::string name_;
      std::vector<int> dimension_size_;
    public:
      Container(std::string name) :
        name_(name),
        dimension_size_(1) {
      }

      Container(std::string name, std::vector<int>& dim_size) :
        name_(name),
        dimension_size_(dim_size) {
      }

      virtual ~Container() {}

      virtual std::string GetName() {return name_;}
  };

  class Variable : public Container {
    protected:

    public:
      Variable(std::string name) :
        Container(name) {
      }

      virtual std::string GetName() {return name_;}
  };

  class Tensor : public Container {
    protected:
      std::string name_;
    public:
      Tensor(std::string name, std::vector<int>& dim_size) :
        Container(name, dim_size)
      {

      }
  };


  class Statement {
    protected:

    public:

  }; // End of class Statement

  class ForLoop : public Statement {
    protected:
      int loop_base;
      int loop_bound;
      int incremental;

    public:

  }; // End of class ForLoop

}; // End of namespace maestro

#endif
