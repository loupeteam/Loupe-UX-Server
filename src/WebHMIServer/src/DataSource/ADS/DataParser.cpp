#include "DataParser.h"

bool dataType_member_base::encode(unsigned long type, BYTE* buffer, std::string& value, unsigned long size)
{
    switch (type) {
    case 0x2:     //INT
        *reinterpret_cast<int16_t*>(buffer) = std::stoi(value);
        break;

    case 0x3:     //DINT
        *reinterpret_cast<int32_t*>(buffer) = std::stol(value);
        break;

    case 0x4:     //REAL
        *reinterpret_cast<float*>(buffer) = std::stof(value);
        break;

    case 0x5:     //LREAL
        *reinterpret_cast<double*>(buffer) = std::stod(value);
        break;

    case 0x10:     //INT8
        *reinterpret_cast<int8_t*>(buffer) = (int8_t)std::stoi(value);
        break;

    case 0x11:     // UINT8
        *reinterpret_cast<uint8_t*>(buffer) = (uint8_t)std::stoi(value);
        break;            

    case 0x12:     // WORD, UINT
        *reinterpret_cast<uint16_t*>(buffer) = std::stoi(value);
        break;

    case 0x13:     // DWORD, UDINT
        *reinterpret_cast<uint32_t*>(buffer) = std::stoul(value);
        break;

    case 0x14:     // LINT
        *reinterpret_cast<int64_t*>(buffer) = std::stoll(value);
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

    case 0x1F:     // WSTRING
        if (size > 0) {
            // Convert from a wstring to a string.
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            std::wstring wvalue = converter.from_bytes(value);
            memcpy(buffer, wvalue.c_str(), size);
            return true;
        }
        break;

    case 0x21:     // BOOL
        //Handle all the ways json can represent a boolean
        if ((value == "true") || (value == "True") || (value == "TRUE") || (value == "1")) {
            *reinterpret_cast<bool*>(buffer) = 0x01;
        } else {
            *reinterpret_cast<bool*>(buffer) = 0x00;
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

    case 0x10:     // INT8
        variable = *reinterpret_cast<int8_t*>(buffer);
        break;

    case 0x11:     // UINT8
        variable = *reinterpret_cast<uint8_t*>(buffer);
        break;

    case 0x21:     // BOOL
        variable = *reinterpret_cast<bool*>(buffer);
        break;

    case 0x12:     // WORD, UINT
        variable = *reinterpret_cast<uint16_t*>(buffer);
        break;

    case 0x13:     // DWORD, UDINT
        variable = *reinterpret_cast<uint32_t*>(buffer);
        break;

    case 0x14:     // LINT
        variable = *reinterpret_cast<int64_t*>(buffer);
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

    case 0x1F:     // WSTRING
        if (size > 0) {
            // Convert from a wstring to a string.
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            variable = converter.to_bytes((wchar_t *)buffer);
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
        } else if (size == 0) {
            variable.empty_object();
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
