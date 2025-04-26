#pragma once

#include <opendaq/data_descriptor_builder.h>
#include <opendaq/dimension_builder.h>
#include <opendaq/dimension_rule_factory.h>
#include <coretypes/number.h>
#include <coretypes/number_ptr.h>
#include <coretypes/number_impl.h>
#include <opendaq/sample_type.h>
#include <coretypes/stringobject_impl.h> // for String
#include <coretypes/string_ptr.h> // for String
#include <coretypes/stringobject_factory.h> // for String

#include <coretypes/integer_factory.h> // for createInt
#include <coretypes/float_factory.h>   // for createFloat 

#include <coretypes/simple_type_factory.h>
#include <coretypes/errors.h>
#include <coretypes/type.h>
#include <coretypes/type_ptr.h>
#include <coretypes/type_manager.h>
#include <coretypes/type_manager_ptr.h>
#include "coretypes/common.h"
#include "websocket_streaming/websocket_streaming.h"

#include <coretypes/simple_type_factory.h>  
#include <coretypes/simple_type.h>

#pragma pack(push, 1)
struct CANData
{
    uint32_t arbId;
    uint8_t length;
    uint8_t data[64];
};
#pragma pack(pop)

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

inline daq::INumber* createNumber(double value)
{
    daq::IFloat* floatObj = nullptr;
    if (daq::createFloat(&floatObj, value) != OPENDAQ_SUCCESS)
        return nullptr;

    daq::INumber* number = nullptr;
    if (floatObj->queryInterface(daq::INumber::Id, reinterpret_cast<void**>(&number)) != OPENDAQ_SUCCESS)
        return nullptr;

    return number;
}

inline daq::DataDescriptorPtr getCANDataStructDescriptor()
{
    using namespace daq;

    auto arbIdBuilder = DataDescriptorBuilder();
    arbIdBuilder.setName("ArbId")
                .setSampleType(SampleType::UInt32);
    auto arbIdDescriptor = arbIdBuilder.build();

    auto lengthBuilder = DataDescriptorBuilder();
    lengthBuilder.setName("Length")
                 .setSampleType(SampleType::UInt8);
    auto lengthDescriptor = lengthBuilder.build();

    IDimensionBuilder* rawDimBuilder = nullptr;
    daq::createDimensionBuilder(&rawDimBuilder);

    rawDimBuilder->setName(String("Dimension").detach());

    // Create INumber* needed for the dimension rule
    INumber* start = websocket_streaming::createNumber(0);    // 0 é o valor inicial
    INumber* increment = websocket_streaming::createNumber(1); // 1 é o incremento

    // Criar a regra de dimensão
    IDimensionRule* rule = nullptr;
    daq::createLinearDimensionRule(&rule, start, increment, 64);

    // Agora define a rule no builder
    rawDimBuilder->setRule(rule);

    IDimension* dimension = nullptr;
    rawDimBuilder->build(&dimension);

    auto dataBuilder = DataDescriptorBuilder();
    dataBuilder.setName("Data")
               .setSampleType(SampleType::UInt8)
               .setDimensions(List<IDimension>(dimension));
    auto dataDescriptor = dataBuilder.build();

    auto structBuilder = DataDescriptorBuilder();
    structBuilder.setSampleType(SampleType::Struct)
                 .setStructFields(List<IDataDescriptor>(arbIdDescriptor, lengthDescriptor, dataDescriptor))
                 .setName("Value");

    return structBuilder.build();
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

// Template specialization for SynchronousSignal
#include <nlohmann/json.hpp>

namespace daq::streaming_protocol
{
// for SynchronousSignal<CANData>
template<>
inline SampleType SynchronousSignal<CANData>::getSampleType() const
{
    //return SampleType::Struct;
    //SAMPLETYPE_STRUCT
    return SampleType::SAMPLETYPE_STRUCT;
}

template<>
inline nlohmann::json SynchronousSignal<CANData>::getMemberInformation() const
{
    return {
        {"arbId", "uint32"},
        {"length", "uint8"},
        {"data", "uint8[64]"}
    };
}

// for ConstantSignal<CANData>
template<>
inline SampleType ConstantSignal<CANData>::getSampleType() const
{
    return SampleType::SAMPLETYPE_STRUCT;
}

template<>
inline nlohmann::json ConstantSignal<CANData>::getMemberInformation() const
{
    return {
        {"arbId", "uint32"},
        {"length", "uint8"},
        {"data", "uint8[64]"}
    };
}

} // namespace daq::streaming_protocol