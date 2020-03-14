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

#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace buildboxcommon {

struct CommandLineTypes {

    struct TypeInfo {
        enum DataType {
            DT_STRING,
            DT_INT,
            DT_DOUBLE,
            DT_BOOL,
            DT_STRING_ARRAY,
            DT_STRING_PAIR_ARRAY
        };

        typedef std::vector<std::string> VectorOfString;
        typedef std::pair<std::string, std::string> PairOfString;
        typedef std::vector<PairOfString> VectorOfPairOfString;

        TypeInfo(const DataType type) : d_type(type), d_variable(nullptr) {}
        TypeInfo(std::string *var) : d_type(DT_STRING), d_variable(var) {}
        TypeInfo(int *var) : d_type(DT_INT), d_variable(var) {}
        TypeInfo(double *var) : d_type(DT_DOUBLE), d_variable(var) {}
        TypeInfo(bool *var) : d_type(DT_BOOL), d_variable(var) {}
        TypeInfo(VectorOfString *var)
            : d_type(DT_STRING_ARRAY), d_variable(var)
        {
        }
        TypeInfo(VectorOfPairOfString *var)
            : d_type(DT_STRING_PAIR_ARRAY), d_variable(var)
        {
        }

        DataType type() const { return d_type; }
        bool isBindable() const { return (d_variable != nullptr); }
        void *getBindable() const { return d_variable; }

        DataType d_type;
        void *d_variable;
    };

    struct ArgumentSpec {
        enum Occurrence { O_OPTIONAL, O_REQUIRED };
        enum Constraint { C_WITH_ARG, C_WITHOUT_ARG };

        ArgumentSpec(const std::string &name, const std::string &desc,
                     const TypeInfo type,
                     const Occurrence occurrence = O_OPTIONAL,
                     const Constraint constraint = C_WITHOUT_ARG)
            : d_name(name), d_desc(desc), d_type(type),
              d_occurrence(occurrence), d_constraint(constraint)
        {
        }

        ArgumentSpec &operator=(const ArgumentSpec &rhs) = default;

        bool isOptional() const { return d_occurrence == O_OPTIONAL; }
        bool isRequired() const { return !isOptional(); }
        bool hasArgument() const { return d_constraint == C_WITH_ARG; }
        bool isPositional() const { return d_name.empty(); }
        TypeInfo::DataType type() const { return d_type.type(); }
        std::string d_name;
        std::string d_desc;
        TypeInfo d_type;
        Occurrence d_occurrence;
        Constraint d_constraint;
    };
};

std::ostream &operator<<(std::ostream &out,
                         const CommandLineTypes::ArgumentSpec &obj);
std::ostream &operator<<(std::ostream &out,
                         const CommandLineTypes::TypeInfo &obj);

} // namespace buildboxcommon

#endif
