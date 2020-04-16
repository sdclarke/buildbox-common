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

// clang-format off
// Description
//   A simple command line argument parsing component. To reduce the
//   complexity, it has the following constraints:
//     1. All options need to be in long-form
//     2. Positional arguments must come after options
//
// Basic Usage:
//   1. Applications using this new component would create a specification in
//      main which defines the command line argument names, their types and
//      whether or not it's optional or required. An example spec could be:
//        ArgumentSpec spec[] = {
//            {"help", "Display usage and exit", TypeInfo(TypeInfo::DT_BOOL)},
//            {"hostname", "Name of host to connect to", TypeInfo(TypeInfo::DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
//            {"request-timeout", "Number of seconds to wait for connection to complete", TypeInfo(TypeInfo::DT_INT), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG}
//        };
//
//   2. Applications would then create the component, passing in the specification to the constructor and invoke the parser.
//      Subsequently the object can be passed into application classes for self-configuration:
//        CommandLine commandLine(spec);
//        const bool success = commandLine.parse(argc, argv);
//        if (!success) {
//            commandLine.usage();
//            return 1;
//        }
//
//        MyComponent component(commandLine);
//        MyOtherComponent myOtherComponent;
//        myOtherComponent.configure(commandLine);
//
// Specification usage
//   1. More complex types are supported by the spec which are required by some applications. For example if a
//      command line argument semantically represents a vector of strings, you can modify your spec as follows:
//        ArgumentSpec spec[] = {
//            {"help", "Display usage and exit", TypeInfo(TypeInfo::DT_BOOL)},
//            {"runner-arg", "Args to pass to the runner", TypeInfo(TypeInfo::DT_STRING_ARRAY), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
//        };
//        CommandLine commandLine(spec);
//        const char argv[] = { "--runner-arg", "arg1", "--runner-arg", "arg2" };
//        const bool success = commandLine.parse(4, argv);
//        if (!success) {
//            commandLine.usage();
//            return 1;
//        }
//        const TypeInfo::VectorOfString &vs = commandLine.getVS("runner-arg");
//
//      In this example, the vector of strings `vs` will contain:
//        vs[0] = "arg1"
//        vs[2] = "arg2"
//
//   2. Another supported usage is for a vector of a pair of strings.
//        ArgumentSpec spec[] = {
//            {"help", "Display usage and exit", TypeInfo(TypeInfo::DT_BOOL)},
//            {"platform", "Set a platform property(repeated):\n--platform KEY=VALUE\n--platform KEY=VALUE", TypeInfo(TypeInfo::DT_STRING_PAIR_ARRAY), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
//        };
//        CommandLine commandLine(spec);
//        const char argv[] = { "--platform", "OSFamily=linux", "--platform", "ISA=x86-64", "--platform",
//                              "chrootRootDigest=8533ec9ba7494cc8295ccd0bfdca08457421a28b4e92c8eb18e7178fb400f5d4/930"};
//        const bool success = commandLine.parse(4, argv);
//        if (!success) {
//            commandLine.usage();
//            return 1;
//        }
//        const TypeInfo::VectorOfPairOfString &vps = commandLine.getVPS("runner-arg");
//
//      In this example, the vector of pair of strings `vps` will contain:
//        vps[0].first -> "OSFamily"
//        vps[0].second -> "linux"
//        vps[1].first -> "ISA"
//        vps[1].second -> "x86-64"
//        vps[2].first -> "chrootRootDigest"
//        vps[2].second -> "8533ec9ba7494cc8295ccd0bfdca08457421a28b4e92c8eb18e7178fb400f5d4/930"
//
// clang-format on

#ifndef INCLUDED_BUILDBOXCOMMON_COMMANDLINE
#define INCLUDED_BUILDBOXCOMMON_COMMANDLINE

#if BUILDBOXCOMMON_CXX_STANDARD == 17
#define BUILDBOXCOMMON_COMMANDLINE_USES_CXX17
#endif

#include <buildboxcommon_commandlinetypes.h>

#include <iostream>
#include <sstream>
#include <map>
#include <string>
#ifdef BUILDBOXCOMMON_COMMANDLINE_USES_CXX17
#include <variant>
#endif
#include <vector>

namespace buildboxcommon {

class CommandLine {
  public:
#ifdef BUILDBOXCOMMON_COMMANDLINE_USES_CXX17
    typedef std::variant<std::string, int, double, bool,
                         CommandLineTypes::TypeInfo::VectorOfString,
                         CommandLineTypes::TypeInfo::VectorOfPairOfString>
        ArgumentValue;
#else
    struct ArgumentValue {
        ArgumentValue() : d_int(0), d_double(0.0) {}

        ArgumentValue &operator=(const std::string &rhs);
        ArgumentValue &operator=(const int rhs);
        ArgumentValue &operator=(const double rhs);
        ArgumentValue &operator=(const bool rhs);
        ArgumentValue &
        operator=(const CommandLineTypes::TypeInfo::VectorOfString &rhs);
        ArgumentValue &
        operator=(const CommandLineTypes::TypeInfo::VectorOfPairOfString &rhs);

        std::string d_str;
        int d_int;
        double d_double;
        bool d_bool;
        CommandLineTypes::TypeInfo::VectorOfString d_vs;
        CommandLineTypes::TypeInfo::VectorOfPairOfString d_vps;
    };
#endif

    template <int LENGTH>
    CommandLine(const CommandLineTypes::ArgumentSpec (&optionSpec)[LENGTH]);

    void usage(std::ostream &out = std::cerr);

    bool parse(int argc, char *argv[], std::ostream &out = std::cerr);
    bool parse(const int argc, const char *argv[],
               std::ostream &out = std::cerr);

    const std::string &getString(const std::string &name) const;
    int getInt(const std::string &name) const;
    double getDouble(const std::string &name) const;
    bool getBool(const std::string &name) const;
    const CommandLineTypes::TypeInfo::VectorOfString &
    getVS(const std::string &name) const;
    const CommandLineTypes::TypeInfo::VectorOfPairOfString &
    getVPS(const std::string &name) const;

    bool exists(const std::string &name) const
    {
        return (d_parsedArgs.find(name) != d_parsedArgs.end());
    }

#ifdef BUILDBOXCOMMON_COMMANDLINE_USES_CXX17
    template <typename T> const T &get(const std::string &name) const;
#endif

    const std::string &processName() const { return d_processName; }

  private:
    struct ArgumentMetaData {
        ArgumentMetaData(const ArgumentValue &value,
                         const CommandLineTypes::ArgumentSpec &spec)
            : d_argumentValue(value), d_spec(spec)
        {
        }

        ArgumentValue d_argumentValue;
        CommandLineTypes::ArgumentSpec d_spec;
    };

    typedef std::map<std::string, ArgumentMetaData> CommandLineArgs;

    const CommandLineTypes::ArgumentSpec *const d_spec;
    const size_t d_specSize;
    CommandLineArgs d_parsedArgs;
    std::string d_processName;
    size_t d_argIdx;
    size_t d_idxLastPositionalFound;
    std::vector<std::string> d_rawArgv;

    bool parseOptions(std::ostream &out);
    bool parsePositionals(std::ostream &out);
    bool readyForPositionals(const size_t argIdx,
                             const std::string &currentArg, std::ostream &out);
    bool validateRequiredArgs(std::string *out = nullptr);
    bool findOptionSpecByName(const std::string &name,
                              const CommandLineTypes::ArgumentSpec **spec);
    bool findNextPositionalSpec(const CommandLineTypes::ArgumentSpec **spec);
    bool existsInSpec(const std::string &name) const;

    bool buildArgumentValue(const std::string &optionValue,
                            const CommandLineTypes::ArgumentSpec &spec,
                            std::ostream &out, ArgumentValue *argumentValue);
};

template <int LENGTH>
CommandLine::CommandLine(
    const CommandLineTypes::ArgumentSpec (&optionSpec)[LENGTH])
    : d_spec(optionSpec), d_specSize(LENGTH), d_argIdx(0),
      d_idxLastPositionalFound(0)
{
}

} // namespace buildboxcommon

#endif
