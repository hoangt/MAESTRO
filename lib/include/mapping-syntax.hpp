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

#ifndef MAESTRO_MAPPING_SYNTAX_HPP_
#define MAESTRO_MAPPING_SYNTAX_HPP_

#include <string>
#include <iostream>

#include<boost/format.hpp>

#include "program-syntax.hpp"

namespace maestro {

  enum class PragmaClass {
    MAP,
	  TEMPORAL_MAP,
	  SPATIAL_MAP,
	  UNROLL,
	  MERGE,
	  TILE,
	  INVALID
  };

  const std::string tkn_temporal_map = "Temporal_Map";
  const std::string tkn_spatial_map = "Spatial_Map";
  const std::string tkn_unroll = "unroll";
  const std::string tkn_merge = "merge";
  const std::string tkn_tile = "Cluster";
  const std::string tkn_delimiters = " ,->()";
  const std::string default_loop_var = "zz";

  class Pragma {
    protected:
      std::string target_loop_variable_name_;
      PragmaClass pragma_class_;
    public:
      Pragma() :
        pragma_class_(PragmaClass::INVALID),
        target_loop_variable_name_("")
      {
      }

      Pragma(PragmaClass cls) :
        pragma_class_(cls),
        target_loop_variable_name_("")
      {
      }

      Pragma(PragmaClass cls, std::string var_nm) :
        pragma_class_(cls),
        target_loop_variable_name_(var_nm)
      {
      }

      std::string GetVarName() {
        return target_loop_variable_name_;
      }

      virtual ~Pragma() {}

      virtual std::string ToString() {
        return "Invalid pragma";
      }

      virtual PragmaClass GetClass() {
        return PragmaClass::INVALID;
      }

      virtual int GetSize() {
          return 1;
      }

      virtual int GetOffset(){
          return 1;
      }
  }; // End of class Pragma

  class Map : public Pragma {
    protected:
      int map_size_;
      int offset_;
    public:
      Map(int mapSz, int ofs, std::string var_nm) :
        Pragma(PragmaClass::MAP, var_nm),
        map_size_(mapSz),
        offset_(ofs)
      {
      }
      virtual ~Map() {}

      virtual std::string ToString() {
        return "Invalid pragma";
      }

      virtual PragmaClass GetClass() {
        return PragmaClass::INVALID;
      }

      virtual int GetSize() {
        return map_size_;
      }
      virtual int GetOffset() {
        return offset_;
      }
  }; // End of class Map

  class TemporalMap : public Map {
    public:
      TemporalMap(std::string var_nm, int mapSz, int ofs) :
        Map(mapSz, ofs, var_nm)
      {
      }

      virtual ~TemporalMap() {}

      virtual std::string ToString() {
        std::string ret = boost::str(boost::format("Temporal Map(%s), map_size: %d, offset: %d")
                                        % target_loop_variable_name_
                                        % map_size_
                                        % offset_ );
        return ret;
      }

      virtual PragmaClass GetClass() {
        return PragmaClass::TEMPORAL_MAP;
      }

      virtual int GetSize() {
        return map_size_;
      }
      virtual int GetOffset() {
        return offset_;
      }
  }; // End of class TemporalMap

  class SpatialMap : public Map {
    protected:
      int num_spatial_components_;
    public:
      SpatialMap(std::string var_nm, int mapSz, int ofs) :
        Map(mapSz, ofs, var_nm),
        num_spatial_components_(1)
      {
      }

      SpatialMap(std::string var_nm, int mapSz, int ofs, int num_components) :
        Map(mapSz, ofs, var_nm),
        num_spatial_components_(num_components)
      {
      }

      void SetNumSpatialComponents(int num_comp) {
        num_spatial_components_ = num_comp;
      }

      virtual ~SpatialMap() {}

      virtual std::string ToString() {
        std::string ret = boost::str(boost::format("Spatial Map(%s), map_size: %d, offset: %d, spatial_components: %d")
                                        % target_loop_variable_name_
                                        % map_size_
                                        % offset_
                                        % num_spatial_components_);
        return ret;
      }

      virtual PragmaClass GetClass() {
        return PragmaClass::SPATIAL_MAP;
      }

      virtual int GetSize() {
        return map_size_;
      }
      virtual int GetOffset() {
        return offset_;
      }
  }; // End of class SpatialMap

  class Unroll : public Pragma {
    protected:

    public:
      Unroll(std::string var_nm) :
        Pragma(PragmaClass::UNROLL, var_nm)
      {
      }

      virtual std::string ToString() {
        std::string ret = boost::str(boost::format("Unroll(%s)")
                                        % target_loop_variable_name_);
        return ret;
      }

      virtual PragmaClass GetClass() {
        return PragmaClass::UNROLL;
      }


  }; // End of class Unroll

  class Merge : public Pragma {
    protected:

    public:
      Merge(std::string var_nm) :
        Pragma(PragmaClass::MERGE, var_nm)
      {
      }

      virtual std::string ToString() {
        std::string ret = boost::str(boost::format("Merge(%s)")
                                        % target_loop_variable_name_);
        return ret;
      }

      virtual PragmaClass GetClass() {
        return PragmaClass::MERGE;
      }

  }; // End of class Merge

  class Tile : public Pragma {
    protected:
      int tile_size_;

    public:
      Tile(std::string var_nm, int tile_sz) :
        Pragma(PragmaClass::TILE, var_nm),
        tile_size_(tile_sz)
      {
      }

      virtual std::string ToString() {
        std::string ret = boost::str(boost::format("Cluster(%s), cluster_size: %d")
                                        % target_loop_variable_name_
                                        % tile_size_);
        return ret;
      }

      virtual PragmaClass GetClass() {
        return PragmaClass::TILE;
      }

      virtual int GetSize() {
          return tile_size_;
      }

  }; // End of class Tile

}; // End of namespace maestro


#endif
