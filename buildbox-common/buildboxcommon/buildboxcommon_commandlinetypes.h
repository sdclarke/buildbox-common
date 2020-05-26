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

#ifndef INCLUDED_BUILDBOXCOMMON_COMMANDLINETYPES
#define INCLUDED_BUILDBOXCOMMON_COMMANDLINETYPES

#include <exception>
#include <map>
#include <sstream>
#include <string>
#if BUILDBOXCOMMON_CXX_STANDARD == 17
#include <any>
#include <optional>
#include <variant>
#endif
#include <vector>

namespace buildboxcommon {

struct CommandLineTypes {

    enum DataType {
        DT_STRING,
        DT_INT,
        DT_DOUBLE,
        DT_BOOL,
        DT_STRING_ARRAY,
        DT_STRING_PAIR_ARRAY,

        DT_UNKNOWN
    };

    struct Type {
        typedef std::vector<std::string> VectorOfString;
        typedef std::pair<std::string, std::string> PairOfString;
        typedef std::vector<PairOfString> VectorOfPairOfString;
    };

    struct DefaultValue {
#if BUILDBOXCOMMON_CXX_STANDARD == 17
        DefaultValue() : d_value(std::nullopt) {}

        DefaultValue(const char *defaultValue)
            : d_value(std::string(defaultValue))
        {
        }
        DefaultValue(const std::string &defaultValue) : d_value(defaultValue)
        {
        }
        DefaultValue(const int defaultValue) : d_value(defaultValue) {}
        DefaultValue(const double defaultValue) : d_value(defaultValue) {}
        DefaultValue(const bool defaultValue) : d_value(defaultValue) {}

        std::optional<std::any> d_value;
#else
        DefaultValue()
            : d_dataType(DT_UNKNOWN), d_int(0), d_double(0.0), d_bool(false)
        {
        }
        DefaultValue(const char *defaultValue)
            : d_dataType(DT_STRING), d_str(defaultValue)
        {
        }
        DefaultValue(const std::string &defaultValue)
            : d_dataType(DT_STRING), d_str(defaultValue)
        {
        }
        DefaultValue(const int defaultValue)
            : d_dataType(DT_INT), d_int(defaultValue)
        {
        }
        DefaultValue(const double defaultValue)
            : d_dataType(DT_DOUBLE), d_double(defaultValue)
        {
        }
        DefaultValue(const bool defaultValue)
            : d_dataType(DT_BOOL), d_bool(defaultValue)
        {
        }

        DataType d_dataType;
        std::string d_str;
        int d_int;
        double d_double;
        bool d_bool;
#endif

        DataType dataType() const { return d_dataType; }

        void print(std::ostream &out, const DataType dataType) const;

        bool hasValue() const
        {
#if BUILDBOXCOMMON_CXX_STANDARD == 17
            return d_value.has_value();
#else
            return (d_dataType != DT_UNKNOWN);
#endif
        }

#if BUILDBOXCOMMON_CXX_STANDARD == 17
        template <typename T> T value() const;
#endif
        const std::string getString() const;
        int getInt() const;
        double getDouble() const;
        bool getBool() const;
    };

    struct TypeInfo {
        TypeInfo(const DataType dataType)
            : d_dataType(dataType), d_variable(nullptr)
        {
        }
        TypeInfo(std::string *var) : d_dataType(DT_STRING), d_variable(var) {}
        TypeInfo(int *var) : d_dataType(DT_INT), d_variable(var) {}
        TypeInfo(double *var) : d_dataType(DT_DOUBLE), d_variable(var) {}
        TypeInfo(bool *var) : d_dataType(DT_BOOL), d_variable(var) {}
        TypeInfo(Type::VectorOfString *var)
            : d_dataType(DT_STRING_ARRAY), d_variable(var)
        {
        }
        TypeInfo(Type::VectorOfPairOfString *var)
            : d_dataType(DT_STRING_PAIR_ARRAY), d_variable(var)
        {
        }

        DataType dataType() const { return d_dataType; }
        bool isBindable() const { return (d_variable != nullptr); }
        void *getBindable() const { return d_variable; }

        DataType d_dataType;
        void *d_variable;
    };

    struct ArgumentSpec {
        enum Occurrence { O_OPTIONAL, O_REQUIRED };
        enum Constraint { C_WITH_ARG, C_WITHOUT_ARG };

        ArgumentSpec(const std::string &name, const std::string &desc,
                     const TypeInfo typeInfo,
                     const Occurrence occurrence = O_OPTIONAL,
                     const Constraint constraint = C_WITHOUT_ARG,
                     const DefaultValue value = DefaultValue())
            : d_name(name), d_desc(desc), d_typeInfo(typeInfo),
              d_occurrence(occurrence), d_constraint(constraint),
              d_defaultValue(value)
        {
        }

        ArgumentSpec &operator=(const ArgumentSpec &rhs) = default;

        bool isOptional() const { return d_occurrence == O_OPTIONAL; }
        bool isRequired() const { return !isOptional(); }
        bool hasArgument() const { return d_constraint == C_WITH_ARG; }
        bool isPositional() const { return d_name.empty(); }
        DataType dataType() const { return d_typeInfo.dataType(); }
        bool hasDefaultValue() const { return d_defaultValue.hasValue(); }
        const DefaultValue &defaultValue() const { return d_defaultValue; }

        std::string d_name;
        std::string d_desc;
        TypeInfo d_typeInfo;
        Occurrence d_occurrence;
        Constraint d_constraint;
        DefaultValue d_defaultValue;
    };

#if BUILDBOXCOMMON_CXX_STANDARD == 17
    typedef std::variant<std::string, int, double, bool,
                         CommandLineTypes::Type::VectorOfString,
                         CommandLineTypes::Type::VectorOfPairOfString>
        ArgumentValue;
#else
    struct ArgumentValue {
        ArgumentValue() : d_int(0), d_double(0.0) {}

        ArgumentValue &operator=(const std::string &rhs);
        ArgumentValue &operator=(const int rhs);
        ArgumentValue &operator=(const double rhs);
        ArgumentValue &operator=(const bool rhs);
        ArgumentValue &
        operator=(const CommandLineTypes::Type::VectorOfString &rhs);
        ArgumentValue &
        operator=(const CommandLineTypes::Type::VectorOfPairOfString &rhs);

        std::string d_str;
        int d_int;
        double d_double;
        bool d_bool;
        CommandLineTypes::Type::VectorOfString d_vs;
        CommandLineTypes::Type::VectorOfPairOfString d_vps;
    };
#endif
};

std::ostream &operator<<(std::ostream &out,
                         const CommandLineTypes::TypeInfo &obj);
std::ostream &operator<<(std::ostream &out,
                         const CommandLineTypes::ArgumentSpec &obj);

} // namespace buildboxcommon

#endif
