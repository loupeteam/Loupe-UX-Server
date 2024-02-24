// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2020 - 2022 Beckhoff Automation GmbH & Co. KG
 */

#pragma once

#include <ads/AdsDevice.h>

//Implement virtual function for the RPC
struct AdsRpcBase {
    AdsRpcBase(const AdsDevice& route, const std::string& symbolName)
        : m_Route(route),
        m_IndexGroup(ADSIGRP_SYM_VALBYHND),
        m_Handle(route.GetHandle(symbolName))
    {}    
protected:
    const AdsDevice& m_Route;
    const uint32_t m_IndexGroup;
    const AdsHandle m_Handle;
};

template<typename T_return, typename T_input, typename T_output>
struct AdsRpc : AdsRpcBase {
    //implement base class constructor
    AdsRpc(const AdsDevice& route, const std::string& symbolName) : AdsRpcBase(route, symbolName){}

    T_return Invoke(T_input* input_data, T_output* output_data) const
    {
        uint32_t bytesRead = 0;

        struct {
            T_return method_result;
            T_output method_output;
        } out_data; 

        auto error = m_Route.ReadWriteReqEx2(m_IndexGroup,
                                        *m_Handle,
                                        sizeof(out_data),
                                        &out_data,
                                        sizeof(T_input),
                                        input_data,
                                        &bytesRead);

        if (error || (sizeof(out_data) != bytesRead)) {
            throw AdsException(error);
        }

        memcpy(output_data, &out_data.method_output, sizeof(T_output));
        return out_data.method_result;
    }

};

template<typename T_return, typename T_input>
struct AdsRpcInputReturn : AdsRpcBase {
    //implement base class constructor
    AdsRpcInputReturn(const AdsDevice& route, const std::string& symbolName) : AdsRpcBase(route, symbolName){}

    T_return Invoke(T_input* input_data) const
    {
        uint32_t bytesRead = 0;

        struct {
            T_return method_result;
        } out_data; 

        auto error = m_Route.ReadWriteReqEx2(m_IndexGroup,
                                        *m_Handle,
                                        sizeof(out_data),
                                        &out_data,
                                        sizeof(T_input),
                                        input_data,
                                        &bytesRead);

        if (error || (sizeof(out_data) != bytesRead)) {
            throw AdsException(error);
        }
        return out_data.method_result;
    }

};

template<typename T_input, typename T_output>
struct AdsRpcInputOutput : AdsRpcBase {
    //implement base class constructor
    AdsRpcInputOutput(const AdsDevice& route, const std::string& symbolName) : AdsRpcBase(route, symbolName){}

    void Invoke(T_input* input_data, T_output* output_data) const
    {
        uint32_t bytesRead = 0;

        struct {
            T_output method_output;
        } out_data; 

        auto error = m_Route.ReadWriteReqEx2(m_IndexGroup,
                                        *m_Handle,
                                        sizeof(out_data),
                                        &out_data,
                                        sizeof(T_input),
                                        input_data,
                                        &bytesRead);

        if (error || (sizeof(out_data) != bytesRead)) {
            throw AdsException(error);
        }

        memcpy(output_data, &out_data.method_output, sizeof(T_output));
    }

};

template<typename T_return, typename T_output>
struct AdsRpcReturnOutput : AdsRpcBase {
    //implement base class constructor
    AdsRpcReturnOutput(const AdsDevice& route, const std::string& symbolName) : AdsRpcBase(route, symbolName){}

    T_return Invoke(T_output* output_data) const
    {
        uint32_t bytesRead = 0;

        struct {
            T_return method_result;
            T_output method_output;
        } out_data; 

        auto error = m_Route.ReadWriteReqEx2(m_IndexGroup,
                                        *m_Handle,
                                        sizeof(out_data),
                                        &out_data,
                                        0,
                                        0,
                                        &bytesRead);

        if (error || (sizeof(out_data) != bytesRead)) {
            throw AdsException(error);
        }

        memcpy(output_data, &out_data.method_output, sizeof(T_output));
        return out_data.method_result;
    }

};

template<typename T_return>
struct AdsRpcReturn : AdsRpcBase {
    //implement base class constructor
    AdsRpcReturn(const AdsDevice& route, const std::string& symbolName) : AdsRpcBase(route, symbolName){}

    T_return Invoke() const
    {
        uint32_t bytesRead = 0;

        struct {
            T_return method_result;
        } out_data; 

        auto error = m_Route.ReadWriteReqEx2(m_IndexGroup,
                                        *m_Handle,
                                        sizeof(out_data),
                                        &out_data,
                                        0,
                                        0,
                                        &bytesRead);

        if (error || (sizeof(out_data) != bytesRead)) {
            throw AdsException(error);
        }
        return out_data.method_result;
    }

};

template<typename T_input>
struct AdsRpcInput : AdsRpcBase {
    //implement base class constructor
    AdsRpcInput(const AdsDevice& route, const std::string& symbolName) : AdsRpcBase(route, symbolName){}

    void Invoke(T_input* input_data) const
    {
        uint32_t bytesRead = 0;

        auto error = m_Route.ReadWriteReqEx2(m_IndexGroup,
                                        *m_Handle,
                                        0,
                                        0,
                                        sizeof(T_input),
                                        input_data,
                                        &bytesRead);

        if (error) {
            throw AdsException(error);
        }
    }

};

template<typename T_output>
struct AdsRpcOutput : AdsRpcBase {
    //implement base class constructor
    AdsRpcOutput(const AdsDevice& route, const std::string& symbolName) : AdsRpcBase(route, symbolName){}

    void Invoke(T_output* output_data) const
    {
        uint32_t bytesRead = 0;

        struct {
            T_output method_output;
        } out_data; 

        auto error = m_Route.ReadWriteReqEx2(m_IndexGroup,
                                        *m_Handle,
                                        sizeof(out_data),
                                        &out_data,
                                        0,
                                        0,
                                        &bytesRead);

        if (error || (sizeof(out_data) != bytesRead)) {
            throw AdsException(error);
        }

        memcpy(output_data, &out_data.method_output, sizeof(T_output));
    }

};

struct AdsRpcVoid : AdsRpcBase {
    //implement base class constructor
    AdsRpcVoid(const AdsDevice& route, const std::string& symbolName) : AdsRpcBase(route, symbolName){}

    void Invoke() const
    {
        uint32_t bytesRead = 0;

        auto error = m_Route.ReadWriteReqEx2(m_IndexGroup,
                                        *m_Handle,
                                        0,
                                        0,
                                        0,
                                        0,
                                        &bytesRead);

        if (error) {
            throw AdsException(error);
        }
   }

};

