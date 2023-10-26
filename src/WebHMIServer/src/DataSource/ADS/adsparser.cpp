#include "adsparser.h"


dataType_member_base * dataType_member_base::parserForType( std::string name, std::string type, unsigned long iOffs, unsigned long size ){
      if(type._Starts_with("STRING")){
        return new dataType_member_string(name, type, iOffs, size);
      }
      else if(type._Starts_with("WSTRING")){
        return new dataType_member_wstring(name, type, iOffs, size);
      }
      else if(type == "BOOL" || type == "BIT" || type == "BIT8"){
        return new dataType_member_bool(name, type, iOffs, size);
      }
      else if(type == "BYTE" || type == "USINT" || type == "BITARR8" || type == "UINT8"){
        return new dataType_member_base_typed<uint8_t>{name, type, iOffs, size};
      }
      else if(type == "SINT" || type == "INT8" ){
        return new dataType_member_base_typed<int8_t>{name, type, iOffs, size};
      }
      else if(type == "UINT" || type == "WORD" || type == "BITARR16" || type == "UINT16"){
        return new dataType_member_base_typed<uint16_t>{name, type, iOffs, size};
      }
      else if(type == "INT" || type == "INT16"){
        return new dataType_member_base_typed<int16_t>{name, type, iOffs, size};
      }
      else if(type == "ENUM" ){
        return new dataType_member_enum{name, type, iOffs, size};
      }
      else if(type == "DINT" || type == "INT32"){
        return new dataType_member_base_typed<int32_t>{name, type, iOffs, size};
      }
      else if(type == "UDINT" || type == "DWORD" || type == "TIME" || type == "TIME_OF_DAY" || type == "TOD" || type == "BITARR32" || type == "UINT32"){
        return new dataType_member_base_typed<uint32_t>{name, type, iOffs, size};
      }
      else if(type == "DATE_AND_TIME" || type == "DT" || type == "DATE" ){
        return new dataType_member_base_typed<uint32_t>{name, type, iOffs, size};
      }
      else if(type == "REAL" || type == "FLOAT"){
        return new dataType_member_base_typed<float>{name, type, iOffs, size};
      }
      else if(type == "DOUBLE" || type == "LREAL" ){
        return new dataType_member_base_typed<double>{name, type, iOffs, size};
      }
      else if(type == "LWORD" || type == "ULINT" || type == "LTIME" || type == "UINT64" ){
        return new dataType_member_base_typed<uint64_t>{name, type, iOffs, size};
      }
      else if(type == "LINT" || type == "INT64" ){
        return new dataType_member_base_typed<int64_t>{name, type, iOffs, size};
      }
      else if(type._Starts_with("POINTER") ){
        return new dataType_member_pointer{name, type, iOffs, size};
      }
      else{
        return new datatype_member(name, type, iOffs, size);
      }
      
}
