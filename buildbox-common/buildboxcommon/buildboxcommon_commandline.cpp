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

#include <buildboxcommon_commandline.h>

#include <buildboxcommon_fileutils.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <libgen.h>
#include <sstream>
#include <string.h>

namespace buildboxcommon {

using ArgumentSpec = buildboxcommon::CommandLineTypes::ArgumentSpec;
using TypeInfo = buildboxcommon::CommandLineTypes::TypeInfo;

namespace {

std::string padString(const std::string &str, const int fill)
{
    std::ostringstream oss;
    size_t chunkStart = 0;
    size_t chunkEnd = 0;
    while (true) {
        chunkEnd = str.find('\n', chunkStart);
        if (chunkEnd == std::string::npos) {
            oss << str.substr(chunkStart);
            break;
        }
        ++chunkEnd;
        oss << str.substr(chunkStart, (chunkEnd - chunkStart))
            << std::setw(fill) << ' ';
        chunkStart = chunkEnd;
    }

    return oss.str();
}

bool isRequiredPositional(const ArgumentSpec &spec)
{
    return (spec.isPositional() && spec.isRequired());
}

bool split(const std::string &str, std::string *s1, std::string *s2,
           const char delim = '=')
{
    size_t pos = str.find(delim);
    if (std::string::npos == pos) {
        return false;
    }

    *s1 = str.substr(0, pos);
    *s2 = str.substr(pos + 1);

    return (!s1->empty() && !s2->empty());
}

std::string prefix(const int lineNumber)
{
    std::ostringstream oss;
    oss << "[" << FileUtils::pathBasename(__FILE__) << ":" << lineNumber
        << "]";
    return oss.str();
}

} // namespace

bool CommandLine::existsInSpec(const std::string &argvArg) const
{
    return (std::count_if(d_spec, d_spec + d_specSize,
                          [&argvArg](const ArgumentSpec &spec) {
                              return (spec.d_name == argvArg);
                          }) >= 1);
}

bool CommandLine::findOptionSpecByName(const std::string &argvArg,
                                       const ArgumentSpec **spec)
{
    const ArgumentSpec *result = std::find_if(
        d_spec, d_spec + d_specSize,
        [&argvArg](const ArgumentSpec &s) { return (s.d_name == argvArg); });

    if (result != (d_spec + d_specSize)) {
        *spec = result;
        return true;
    }

    return false;
}

bool CommandLine::findNextPositionalSpec(const ArgumentSpec **spec)
{
    // positional parameters have empty names, so just return them in
    // order
    bool found = false;
    for (; d_idxLastPositionalFound < d_specSize; ++d_idxLastPositionalFound) {
        if (d_spec[d_idxLastPositionalFound].d_name.empty()) {
            *spec = &d_spec[d_idxLastPositionalFound];
            ++d_idxLastPositionalFound;
            found = true;
            break;
        }
    }

    return found;
}

bool CommandLine::buildArgumentValue(const std::string &optionValue,
                                     const ArgumentSpec &spec,
                                     std::ostream &out,
                                     ArgumentValue *argumentValue)
{
    try {
        switch (spec.d_type.type()) {
            case TypeInfo::DT_STRING:
                *argumentValue = optionValue;
                if (spec.d_type.isBindable()) {
                    *(static_cast<std::string *>(spec.d_type.getBindable())) =
                        optionValue;
                }
                break;
            case TypeInfo::DT_INT: {
                const int val = std::stoi(optionValue.c_str());
                *argumentValue = val;
                if (spec.d_type.isBindable()) {
                    *(static_cast<int *>(spec.d_type.getBindable())) = val;
                }
                break;
            }
            case TypeInfo::DT_DOUBLE: {
                const double val = std::stod(optionValue.c_str());
                *argumentValue = val;
                if (spec.d_type.isBindable()) {
                    *(static_cast<double *>(spec.d_type.getBindable())) = val;
                }
                break;
            }
            case TypeInfo::DT_BOOL: {
                std::string tmpVal(optionValue);
                if (tmpVal.empty() &&
                    spec.d_constraint == ArgumentSpec::C_WITHOUT_ARG) {
                    tmpVal = "true";
                }
                const bool val = (tmpVal == "true");
                *argumentValue = val;
                if (spec.d_type.isBindable()) {
                    *(static_cast<bool *>(spec.d_type.getBindable())) = val;
                }
                break;
            }
            case TypeInfo::DT_STRING_ARRAY: {
                auto it = d_parsedArgs.find(spec.d_name);
                if (it == d_parsedArgs.end()) {
                    TypeInfo::VectorOfString vs = {optionValue};
                    *argumentValue = vs;
                }
                else {
#ifdef BUILDBOXCOMMON_COMMANDLINE_USES_CXX17
                    std::get<TypeInfo::VectorOfString>(
                        it->second.d_argumentValue)
                        .emplace_back(optionValue);
#else
                    it->second.d_argumentValue.d_vs.emplace_back(optionValue);
#endif
                }
                if (spec.d_type.isBindable()) {
                    static_cast<TypeInfo::VectorOfString *>(
                        spec.d_type.getBindable())
                        ->emplace_back(optionValue);
                }
                break;
            }
            case TypeInfo::DT_STRING_PAIR_ARRAY: {
                std::string key, value;
                split(optionValue, &key, &value);
                auto it = d_parsedArgs.find(spec.d_name);
                if (it == d_parsedArgs.end()) {
                    TypeInfo::VectorOfPairOfString vps = {{key, value}};
                    *argumentValue = vps;
                }
                else {
#ifdef BUILDBOXCOMMON_COMMANDLINE_USES_CXX17
                    std::get<TypeInfo::VectorOfPairOfString>(
                        it->second.d_argumentValue)
                        .emplace_back(TypeInfo::PairOfString{key, value});
#else
                    it->second.d_argumentValue.d_vps.emplace_back(
                        TypeInfo::PairOfString{key, value});
#endif
                }
                if (spec.d_type.isBindable()) {
                    static_cast<TypeInfo::VectorOfPairOfString *>(
                        spec.d_type.getBindable())
                        ->emplace_back(TypeInfo::PairOfString{key, value});
                }
            } break;
        }
    }
    catch (const std::invalid_argument &e) {
        out << prefix(__LINE__)
            << ": invalid_argument error caught converting argument \""
            << optionValue << "\" to "
            << (spec.d_type.type() == TypeInfo::DT_DOUBLE ? "double" : "int")
            << std::endl;
        return false;
    }
    catch (const std::out_of_range &e) {
        out << prefix(__LINE__)
            << ": out_of_range error caught converting argument \""
            << optionValue << "\" to "
            << (spec.d_type.type() == TypeInfo::DT_DOUBLE ? "double" : "int")
            << std::endl;
        return false;
    }

    return true;
}

bool CommandLine::parseOptions(std::ostream &out)
{
    // iterate over options first, then positionals
    for (; d_argIdx < d_rawArgv.size(); ++d_argIdx) {
        const std::string &currentArg = d_rawArgv[d_argIdx];

        // enforce long-style options for simplicity
        if (currentArg[0] == '-' && currentArg[1] != '-') {
            out << prefix(__LINE__) << ": parse error: "
                << "unrecognized short option format for argument \""
                << currentArg << "\"" << std::endl;
            return false;
        }

        // either this is a positional arg or an unsupported short-form option
        if (currentArg[0] != '-' && currentArg[1] != '-') {
            if (readyForPositionals(d_argIdx, currentArg, out)) {
                break;
            }
            else {
                // encounter some error
                return false;
            }
        }

        // grab an option name and find it's specification
        // support 2 formats: "--foo bar" and  "--foo=bar"
        std::string optionName(currentArg.substr(2));
        std::string optionValue;
        size_t pos = optionName.find('=');
        if (std::string::npos != pos) {
            optionValue = optionName.substr(pos + 1);
            optionName.erase(pos);
        }

        // find the spec now so we can deterministically know if
        // we need to look at the next argv to find the option value
        const ArgumentSpec *spec = nullptr;
        const bool found = findOptionSpecByName(optionName, &spec);
        if (!found) {
            out << prefix(__LINE__) << ": parse error: "
                << "option \"" << optionName
                << "\" not part of argument specification" << std::endl;
            return false;
        }

        // format "--foo bar"
        if (spec->hasArgument() && std::string::npos == pos) {
            // sanity check if we're at the end of the supplied args
            if (d_rawArgv.size() == (d_argIdx + 1)) {
                out << prefix(__LINE__) << ": parse error: "
                    << "option \"" << optionName
                    << "\" is configured to accept an argument but none was "
                       "provided"
                    << std::endl;
                return false;
            }

            ++d_argIdx;
            optionValue = d_rawArgv.at(d_argIdx);

            // confirm optionValue is not erroneously the next option
            if (optionValue[0] == '-' && optionValue[1] == '-') {
                out << prefix(__LINE__) << ": parse error: "
                    << "option \"" << optionName
                    << "\" is configured to accept an argument but none was "
                       "provided"
                    << std::endl;
                return false;
            }
        }

        // populate 'argumentValue' based on it's specified data type
        ArgumentValue argumentValue;
        if (!buildArgumentValue(optionValue, *spec, out, &argumentValue)) {
            return false;
        }

        // add everything to the container
        d_parsedArgs.emplace(CommandLineArgs::value_type(
            optionName, ArgumentMetaData(argumentValue, *spec)));
    }

    return true;
}

bool CommandLine::parsePositionals(std::ostream &out)
{
    // sanity check for missing positionals
    const size_t numRequiredSpecPositionals =
        std::count_if(d_spec, d_spec + d_specSize, isRequiredPositional);
    if (d_rawArgv.size() < (d_argIdx + numRequiredSpecPositionals)) {
        out << prefix(__LINE__) << ": parse error: "
            << "required positional argument(s) missing from command line"
            << std::endl;
        return false;
    }

    for (; d_argIdx < d_rawArgv.size(); ++d_argIdx) {
        // grab an option name and find it's specification
        std::string positional = d_rawArgv.at(d_argIdx);
        const ArgumentSpec *spec = nullptr;
        if (!findNextPositionalSpec(&spec)) {
            out << prefix(__LINE__) << ": parse warning: "
                << "unexpected positional argument \"" << positional
                << "\" found, but not defined in specification" << std::endl;
            continue;
        }

        // add everything to the container
        ArgumentValue argumentValue;
        if (!buildArgumentValue(positional, *spec, out, &argumentValue)) {
            return false;
        }
    }

    return true;
}

bool CommandLine::readyForPositionals(const size_t argIdx,
                                      const std::string &currentArg,
                                      std::ostream &out)
{
    if (existsInSpec(currentArg)) {
        out << prefix(__LINE__) << ": parse error: "
            << "long option format is required for argument \"" << currentArg
            << "\"" << std::endl;
        return false;
    }

    // confirm we have all required options before breaking out of loop
    std::string errorMsg;
    if (!validateRequiredArgs(&errorMsg)) {
        // 2 Scenarios:
        //  a. Misplaced positional: positional argument has been
        //     encountered before all required options have been parsed
        //  b. Missing required option: we've hit a properly positioned
        //     positional but are missing required options

        // if we're not at the end of argv[], peek ahead
        // to see if the next argument is another option
        // ie; --option1=value1 positional --option2=value2
        if (argIdx < (d_rawArgv.size() - 1)) {
            const std::string &nextArg = d_rawArgv.at(argIdx + 1);
            if (nextArg[0] == '-' && nextArg[1] == '-') {
                // Scenario #1
                out << prefix(__LINE__) << ": parse error: "
                    << "positional arguments must come after options"
                    << std::endl;
            }
            else {
                // Scenario #2
                out << prefix(__LINE__) << ": parse error: " << errorMsg
                    << std::endl;
            }
        }
        else {
            // Scenario #2 when we've gone thru all the args
            out << prefix(__LINE__) << ": parse error: " << errorMsg
                << std::endl;
        }
        return false;
    }

    // ready for positionals
    return true;
}

bool CommandLine::parse(int argc, char *argv[], std::ostream &out)
{
    return parse(argc, const_cast<const char **>(argv), out);
}

bool CommandLine::parse(const int argc, const char *argv[], std::ostream &out)
{
    // sanity check
    if (argc == 0 || argv == nullptr) {
        out << "invalid argc/argv parameters" << std::endl;
        return false;
    }

    // copy argv[] into a vector of strings
    std::copy(&argv[0], &argv[argc], std::back_inserter(d_rawArgv));

    // save argv[0] and then erase it
    d_processName = d_rawArgv[0];
    d_rawArgv.erase(d_rawArgv.begin());

    // first parse options, return false on error
    bool result = parseOptions(out);
    if (!result) {
        return false;
    }

    // if run with only --help, no need to go further
    if (d_parsedArgs.size() == 1 && d_parsedArgs.count("help") > 0) {
        return true;
    }

    // validate that we have all required options
    std::string errorMsg;
    if (!validateRequiredArgs(&errorMsg)) {
        out << prefix(__LINE__) << ": " << errorMsg << std::endl;
        return false;
    }

    // parse positionals
    return parsePositionals(out);
}

bool CommandLine::validateRequiredArgs(std::string *out)
{
    // iterate over the entire argument spec and confirm the command line
    // container contains the required options;
    // generate error message describing exactly which required arguments are
    // missing
    size_t numMissing = 0;
    const std::string tmp = prefix(__LINE__);
    const std::string filler(tmp.length() + 2, ' ');
    std::ostringstream oss;
    for (size_t i = 0; i < d_specSize; ++i) {
        const ArgumentSpec *spec = &d_spec[i];

        // do not count positionals
        if (spec->d_name.empty()) {
            continue;
        }

        if (spec->isRequired() && d_parsedArgs.count(spec->d_name) == 0) {
            oss << filler << "\"" << (spec->d_name.empty() ? "" : "--")
                << (spec->d_name.empty() ? spec->d_desc : spec->d_name)
                << "\"\n";
            ++numMissing;
        }
    }

    if (0 == numMissing) {
        return true;
    }

    if (out != nullptr) {
        std::ostringstream error;
        error << d_processName << ": " << numMissing
              << " required argument(s) missing\n"
              << oss.str();
        *out = error.str();
    }

    return false;
}

void CommandLine::usage(std::ostream &out)
{
    // to print out a clean, properly formatted usage(), save the longest
    // argument name to come up with correct padding between optionName and
    // optionDescription
    size_t maxOptionLength = 0;
    for (size_t i = 0; i < d_specSize; ++i) {
        maxOptionLength =
            std::max<size_t>(maxOptionLength, d_spec[i].d_name.length());
    }

    const size_t prefixSize = 3;
    const size_t gapSize = 5;
    const size_t maxPadding = maxOptionLength + gapSize;
    static const std::string prefixFill(prefixSize, ' ');
    out << "Usage: " << d_processName << "\n";
    for (size_t i = 0; i < d_specSize; ++i) {
        const ArgumentSpec *spec = &d_spec[i];
        const int fill = static_cast<int>(
            maxPadding - (spec->d_name.empty() ? spec->d_desc.length()
                                               : spec->d_name.length()));
        const std::string paddedDesc =
            padString(spec->d_desc, (int)(maxPadding + prefixSize + gapSize));
        out << prefixFill << (spec->d_name.empty() ? "  " : "--")
            << (spec->d_name.empty() ? paddedDesc : spec->d_name)
            << std::setw(fill) << " "
            << (spec->d_name.empty() ? "POSITIONAL" : paddedDesc)
            << (spec->isOptional() ? " [optional]" : "") << "\n";
    }

    out << std::endl;
}

#ifdef BUILDBOXCOMMON_COMMANDLINE_USES_CXX17
template <typename T> const T &CommandLine::get(const std::string &name) const
{
    const auto it = d_parsedArgs.find(name);
    if (it == d_parsedArgs.end()) {
        throw std::runtime_error("argument \"" + name + "\" not found");
    }

    if (!std::holds_alternative<T>(it->second.d_argumentValue)) {
        throw std::runtime_error("mismatched types in lookup of arg \"" +
                                 name + "\"");
    }

    return std::get<T>(it->second.d_argumentValue);
}

template const std::string &
CommandLine::get<std::string>(const std::string &name) const;
template const int &CommandLine::get<int>(const std::string &name) const;
template const double &CommandLine::get<double>(const std::string &name) const;
template const bool &CommandLine::get<bool>(const std::string &name) const;
template const TypeInfo::VectorOfString &
CommandLine::get<TypeInfo::VectorOfString>(const std::string &name) const;
template const TypeInfo::VectorOfPairOfString &
CommandLine::get<TypeInfo::VectorOfPairOfString>(
    const std::string &name) const;

#endif

const std::string &CommandLine::getString(const std::string &name) const
{
#ifdef BUILDBOXCOMMON_COMMANDLINE_USES_CXX17
    return this->get<std::string>(name);
#else
    const auto it = d_parsedArgs.find(name);
    if (it == d_parsedArgs.end()) {
        throw std::runtime_error("argument \"" + name + "\" not found");
    }
    return it->second.d_argumentValue.d_str;
#endif
}

int CommandLine::getInt(const std::string &name) const
{
#ifdef BUILDBOXCOMMON_COMMANDLINE_USES_CXX17
    return this->get<int>(name);
#else
    const auto it = d_parsedArgs.find(name);
    if (it == d_parsedArgs.end()) {
        throw std::runtime_error("argument \"" + name + "\" not found");
    }
    return it->second.d_argumentValue.d_int;
#endif
}

double CommandLine::getDouble(const std::string &name) const
{
#ifdef BUILDBOXCOMMON_COMMANDLINE_USES_CXX17
    return this->get<double>(name);
#else
    const auto it = d_parsedArgs.find(name);
    if (it == d_parsedArgs.end()) {
        throw std::runtime_error("argument \"" + name + "\" not found");
    }
    return it->second.d_argumentValue.d_double;
#endif
}

bool CommandLine::getBool(const std::string &name) const
{
#ifdef BUILDBOXCOMMON_COMMANDLINE_USES_CXX17
    return this->get<bool>(name);
#else
    const auto it = d_parsedArgs.find(name);
    if (it == d_parsedArgs.end()) {
        throw std::runtime_error("argument \"" + name + "\" not found");
    }
    return it->second.d_argumentValue.d_bool;
#endif
}

const TypeInfo::VectorOfString &
CommandLine::getVS(const std::string &name) const
{
#ifdef BUILDBOXCOMMON_COMMANDLINE_USES_CXX17
    return this->get<TypeInfo::VectorOfString>(name);
#else
    const auto it = d_parsedArgs.find(name);
    if (it == d_parsedArgs.end()) {
        throw std::runtime_error("argument \"" + name + "\" not found");
    }
    return it->second.d_argumentValue.d_vs;
#endif
}

const TypeInfo::VectorOfPairOfString &
CommandLine::getVPS(const std::string &name) const
{
#ifdef BUILDBOXCOMMON_COMMANDLINE_USES_CXX17
    return this->get<TypeInfo::VectorOfPairOfString>(name);
#else
    const auto it = d_parsedArgs.find(name);
    if (it == d_parsedArgs.end()) {
        throw std::runtime_error("argument \"" + name + "\" not found");
    }
    return it->second.d_argumentValue.d_vps;
#endif
}

#ifndef BUILDBOXCOMMON_COMMANDLINE_USES_CXX17
CommandLine::ArgumentValue &CommandLine::ArgumentValue::
operator=(const std::string &rhs)
{
    this->d_str = rhs;
    return *this;
}

CommandLine::ArgumentValue &CommandLine::ArgumentValue::
operator=(const int rhs)
{
    this->d_int = rhs;
    return *this;
}

CommandLine::ArgumentValue &CommandLine::ArgumentValue::
operator=(const double rhs)
{
    this->d_double = rhs;
    return *this;
}

CommandLine::ArgumentValue &CommandLine::ArgumentValue::
operator=(const bool rhs)
{
    this->d_bool = rhs;
    return *this;
}

CommandLine::ArgumentValue &CommandLine::ArgumentValue::
operator=(const TypeInfo::VectorOfString &rhs)
{
    this->d_vs = rhs;
    return *this;
}

CommandLine::ArgumentValue &CommandLine::ArgumentValue::
operator=(const TypeInfo::VectorOfPairOfString &rhs)
{
    this->d_vps = rhs;
    return *this;
}

#endif

} // namespace buildboxcommon
