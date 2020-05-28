// Copyright 2020 Bloomberg Finance L.P
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <buildboxcommon_commandlinetypes.h>

#if BUILDBOXCOMMON_CXX_STANDARD == 17
#define BUILDBOXCOMMON_COMMANDLINE_USES_CXX17
#endif

namespace buildboxcommon {

using DefaultValue = CommandLineTypes::DefaultValue;

#ifdef BUILDBOXCOMMON_COMMANDLINE_USES_CXX17
template <typename T> T DefaultValue::value() const
{
    if (!d_value.has_value()) {
        throw std::runtime_error("default value was not set");
    }
    return std::any_cast<T>(d_value.value());
}

template std::string DefaultValue::value<std::string>() const;
template int DefaultValue::value<int>() const;
template double DefaultValue::value<double>() const;
template bool DefaultValue::value<bool>() const;

#endif

const std::string DefaultValue::getString() const
{
#ifdef BUILDBOXCOMMON_COMMANDLINE_USES_CXX17
    return this->value<std::string>();
#else
    return this->d_str;
#endif
}

int DefaultValue::getInt() const
{
#ifdef BUILDBOXCOMMON_COMMANDLINE_USES_CXX17
    return this->value<int>();
#else
    return this->d_int;
#endif
}

double DefaultValue::getDouble() const
{
#ifdef BUILDBOXCOMMON_COMMANDLINE_USES_CXX17
    return this->value<double>();
#else
    return this->d_double;
#endif
}

bool DefaultValue::getBool() const
{
#ifdef BUILDBOXCOMMON_COMMANDLINE_USES_CXX17
    return this->value<bool>();
#else
    return this->d_bool;
#endif
}

#ifndef BUILDBOXCOMMON_COMMANDLINE_USES_CXX17
CommandLineTypes::ArgumentValue &CommandLineTypes::ArgumentValue::
operator=(const std::string &rhs)
{
    this->d_str = rhs;
    return *this;
}

CommandLineTypes::ArgumentValue &CommandLineTypes::ArgumentValue::
operator=(const int rhs)
{
    this->d_int = rhs;
    return *this;
}

CommandLineTypes::ArgumentValue &CommandLineTypes::ArgumentValue::
operator=(const double rhs)
{
    this->d_double = rhs;
    return *this;
}

CommandLineTypes::ArgumentValue &CommandLineTypes::ArgumentValue::
operator=(const bool rhs)
{
    this->d_bool = rhs;
    return *this;
}

CommandLineTypes::ArgumentValue &CommandLineTypes::ArgumentValue::
operator=(const Type::VectorOfString &rhs)
{
    this->d_vs = rhs;
    return *this;
}

CommandLineTypes::ArgumentValue &CommandLineTypes::ArgumentValue::
operator=(const Type::VectorOfPairOfString &rhs)
{
    this->d_vps = rhs;
    return *this;
}

#endif

std::ostream &operator<<(std::ostream &out,
                         const CommandLineTypes::ArgumentSpec &obj)
{
    out << "[\"" << obj.d_name << "\", \"" << obj.d_desc << "\", "
        << obj.d_typeInfo << ", " << obj.d_occurrence << ", "
        << obj.d_constraint << ", ";
    obj.d_defaultValue.print(out, obj.dataType());

    return out;
}

void CommandLineTypes::DefaultValue::print(std::ostream &out,
                                           const DataType dataType) const
{
    out << "[";
    switch (dataType) {
        case CommandLineTypes::DataType::COMMANDLINE_DT_STRING:
            out << "\"" << getString() << "\"";
            break;
        case CommandLineTypes::DataType::COMMANDLINE_DT_INT:
            out << getInt();
            break;
        case CommandLineTypes::DataType::COMMANDLINE_DT_DOUBLE:
            out << std::fixed << getDouble();
            break;
        case CommandLineTypes::DataType::COMMANDLINE_DT_BOOL: {
            out << std::boolalpha << getBool();
            break;
        }
        default:
            break;
    }

    out << "]";
}

std::ostream &operator<<(std::ostream &out,
                         const CommandLineTypes::TypeInfo &obj)
{
    out << obj.d_dataType;
    return out;
}

} // namespace buildboxcommon
