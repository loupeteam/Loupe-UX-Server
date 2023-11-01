#include "DataParser.h"

dataType_member_base* dataType_member_base::parserForType(std::string   name,
                                                          std::string   type,
                                                          unsigned long iOffs,
                                                          unsigned long size)
{
    if (type._Starts_with("STRING")) {
        return new dataType_member_string(name, type, iOffs, size);
    } else if (type._Starts_with("WSTRING")) {
        return new dataType_member_wstring(name, type, iOffs, size);
    } else if ((type == "BOOL") || (type == "BIT") || (type == "BIT8")) {
        return new dataType_member_bool(name, type, iOffs, size);
    } else if ((type == "BYTE") || (type == "USINT") || (type == "BITARR8") || (type == "UINT8")) {
        return new dataType_member_base_typed<uint8_t>{name, type, iOffs, size};
    } else if ((type == "SINT") || (type == "INT8")) {
        return new dataType_member_base_typed<int8_t>{name, type, iOffs, size};
    } else if ((type == "UINT") || (type == "WORD") || (type == "BITARR16") || (type == "UINT16")) {
        return new dataType_member_base_typed<uint16_t>{name, type, iOffs, size};
    } else if ((type == "INT") || (type == "INT16")) {
        return new dataType_member_base_typed<int16_t>{name, type, iOffs, size};
    } else if (type == "ENUM") {
        return new dataType_member_enum{name, type, iOffs, size};
    } else if ((type == "DINT") || (type == "INT32")) {
        return new dataType_member_base_typed<int32_t>{name, type, iOffs, size};
    } else if ((type == "UDINT") || (type == "DWORD") || (type == "TIME") || (type == "TIME_OF_DAY") ||
               (type == "TOD") || (type == "BITARR32") || (type == "UINT32"))
    {
        return new dataType_member_base_typed<uint32_t>{name, type, iOffs, size};
    } else if ((type == "DATE_AND_TIME") || (type == "DT") || (type == "DATE")) {
        return new dataType_member_base_typed<uint32_t>{name, type, iOffs, size};
    } else if ((type == "REAL") || (type == "FLOAT")) {
        return new dataType_member_base_typed<float>{name, type, iOffs, size};
    } else if ((type == "DOUBLE") || (type == "LREAL")) {
        return new dataType_member_base_typed<double>{name, type, iOffs, size};
    } else if ((type == "LWORD") || (type == "ULINT") || (type == "LTIME") || (type == "UINT64")) {
        return new dataType_member_base_typed<uint64_t>{name, type, iOffs, size};
    } else if ((type == "LINT") || (type == "INT64")) {
        return new dataType_member_base_typed<int64_t>{name, type, iOffs, size};
    } else if (type._Starts_with("POINTER")) {
        return new dataType_member_pointer{name, type, iOffs, size};
    } else {
        return new datatype_member(name, type, iOffs, size);
    }
}

bool dataType_member_base::encode(unsigned long type, BYTE* buffer, std::string& value, unsigned long size)
{
    switch (type) {
    case 0x2:     //INT
        *reinterpret_cast<int16_t*>(buffer) = std::stoi(value);
        break;

    case 0x3:     //DINT
        *reinterpret_cast<int32_t*>(buffer) = std::stoi(value);
        break;

    case 0x4:     //REAL
        *reinterpret_cast<float*>(buffer) = std::stof(value);
        break;

    case 0x5:     //LREAL
        *reinterpret_cast<double*>(buffer) = std::stod(value);
        break;

    case 0x11:     // BYTE
    case 0x21:     // BOOL
        //Handle all the ways json can represent a boolean
        if ((value == "true") || (value == "True") || (value == "TRUE") || (value == "1")) {
            *reinterpret_cast<bool*>(buffer) = 0x01;
        } else {
            *reinterpret_cast<bool*>(buffer) = 0x00;
        }
        break;

    case 0x12:     // WORD, UINT
        *reinterpret_cast<uint16_t*>(buffer) = std::stoi(value);
        break;

    case 0x13:     // DWORD, UDINT
        *reinterpret_cast<uint32_t*>(buffer) = std::stoul(value);
        break;

    case 0x15:     // LWORD, ULINT
        *reinterpret_cast<uint64_t*>(buffer) = std::stoull(value);
        break;

    case 0x1E:     // STRING
        if (size > 0) {
            //TODO: Use safer string copy
            strncpy((char*)buffer, value.c_str(), size);
            return true;
        }
        break;

    case 0x41:     // Structure
        //If we are here, we are parsing a structure with no members
        //Based on the size, we need to determine the type
        if (size == 1) {
            *reinterpret_cast<uint8_t*>(buffer) = std::stoi(value);
        } else if (size == 2) {
            *reinterpret_cast<uint16_t*>(buffer) = std::stoi(value);
        } else if (size == 4) {
            *reinterpret_cast<uint32_t*>(buffer) = std::stoul(value);
        } else if (size == 8) {
            *reinterpret_cast<uint64_t*>(buffer) = std::stoull(value);
        } else {
            return false;
        }
        break;

    default:
        return false;
    }

    return true;
}
bool dataType_member_base::parse(unsigned long type, crow::json::wvalue& variable, void* buffer, unsigned long size)
{
    switch (type) {
    case 0x2:     //INT
        variable = *reinterpret_cast<int16_t*>(buffer);
        break;

    case 0x3:     //DINT
        variable = *reinterpret_cast<int32_t*>(buffer);
        break;

    case 0x4:     //REAL
        variable = *reinterpret_cast<float*>(buffer);
        break;

    case 0x5:     //LREAL
        variable = *reinterpret_cast<double*>(buffer);
        break;

    case 0x11:     // BYTE
    case 0x21:     // BOOL
        variable = *reinterpret_cast<bool*>(buffer);
        break;

    case 0x12:     // WORD, UINT
        variable = *reinterpret_cast<uint16_t*>(buffer);
        break;

    case 0x13:     // DWORD, UDINT
        variable = *reinterpret_cast<uint32_t*>(buffer);
        break;

    case 0x15:     // LWORD, ULINT
        variable = *reinterpret_cast<uint64_t*>(buffer);
        break;

    case 0x1E:     // STRING
        if (size > 0) {
            variable = std::string((char*)buffer);
            return true;
        }
        break;

    case 0x41:     // Structure
        //If we are here, we are parsing a structure with no members
        //Based on the size, we need to determine the type
        if (size == 1) {
            variable = *reinterpret_cast<uint8_t*>(buffer);
        } else if (size == 2) {
            variable = *reinterpret_cast<uint16_t*>(buffer);
        } else if (size == 4) {
            variable = *reinterpret_cast<uint32_t*>(buffer);
        } else if (size == 8) {
            variable = *reinterpret_cast<uint64_t*>(buffer);
        } else {
            return false;
        }
        break;

    default:
        return false;
        break;
    }

    return true;
}
