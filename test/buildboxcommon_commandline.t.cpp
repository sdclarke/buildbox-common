/*
 * Copyright 2020 Bloomberg Finance LP
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <buildboxcommon_commandline.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <sstream>

using namespace buildboxcommon;
using ArgumentSpec = buildboxcommon::CommandLineTypes::ArgumentSpec;
using Type = buildboxcommon::CommandLineTypes::Type;
using TypeInfo = buildboxcommon::CommandLineTypes::TypeInfo;
using DataType = buildboxcommon::CommandLineTypes::DataType;
using CMLDefaultValue = buildboxcommon::CommandLineTypes::DefaultValue;

using namespace testing;

std::string positional1;
int positional2;
double positional3;
std::string botId;
Type::VectorOfString runnerArgs;
Type::VectorOfPairOfString platformProperties;

// clang-format off
ArgumentSpec defaultSpec[] = {
    {"help", "Display usage and exit", TypeInfo(DataType::COMMANDLINE_DT_BOOL)},
    {"instance", "Name of instance", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"cas-remote", "IP/port of remote CAS server", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"bots-remote", "IP/port of remote BOTS server", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"log-level", "Log verbosity level", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG},
    {"request-timeout", "Request timeout", TypeInfo(DataType::COMMANDLINE_DT_INT), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"buildbox-run", "Absolute path to runner exectuable", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"platform", "Set a platform property(repeated):\n--platform KEY=VALUE\n--platform KEY=VALUE", TypeInfo(DataType::COMMANDLINE_DT_STRING_PAIR_ARRAY), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"runner-arg", "Args to pass to the runner", TypeInfo(DataType::COMMANDLINE_DT_STRING_ARRAY), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"metrics-mode", "Metrics Mode: --metrics-mode=MODE - options for MODE are\n"
     "udp://<hostname>:<port>\nfile:///path/to/file\nstderr", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"metrics-publish-interval", "Metrics publishing interval", TypeInfo(DataType::COMMANDLINE_DT_INT), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"config-file", "Absolute path to config file", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"verbose", "Adjust log verbosity", TypeInfo(DataType::COMMANDLINE_DT_BOOL)},
    {"", "BOT Id", TypeInfo(&botId), ArgumentSpec::O_REQUIRED}
};

ArgumentSpec noPositionalsSpec[] = {
    {"help", "Display usage and exit", TypeInfo(DataType::COMMANDLINE_DT_BOOL)},
    {"instance", "Name of instance", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"cas-remote", "IP/port of remote CAS server", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"bots-remote", "IP/port of remote BOTS server", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"log-level", "Log verbosity level", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG},
    {"request-timeout", "Request timeout", TypeInfo(DataType::COMMANDLINE_DT_INT), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"buildbox-run", "Absolute path to runner exectuable", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"platform", "Set a platform property(repeated):\n--platform KEY=VALUE\n--platform KEY=VALUE", TypeInfo(DataType::COMMANDLINE_DT_STRING_PAIR_ARRAY), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"runner-arg", "Args to pass to the runner", TypeInfo(DataType::COMMANDLINE_DT_STRING_ARRAY), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"metrics-mode", "Metrics Mode: --metrics-mode=MODE - options for MODE are\n"
     "udp://<hostname>:<port>\nfile:///path/to/file\nstderr", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"metrics-publish-interval", "Metrics publishing interval", TypeInfo(DataType::COMMANDLINE_DT_INT), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"config-file", "Absolute path to config file", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"verbose", "Adjust log verbosity", TypeInfo(DataType::COMMANDLINE_DT_BOOL)}
};

ArgumentSpec positionalNotRequiredSpec[] = {
    {"help", "Display usage and exit", TypeInfo(DataType::COMMANDLINE_DT_BOOL)},
    {"instance", "Name of instance", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"cas-remote", "IP/port of remote CAS server", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"bots-remote", "IP/port of remote BOTS server", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"log-level", "Log verbosity level", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG},
    {"request-timeout", "Request timeout", TypeInfo(DataType::COMMANDLINE_DT_INT), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"buildbox-run", "Absolute path to runner exectuable", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"platform", "Set a platform property(repeated):\n--platform KEY=VALUE\n--platform KEY=VALUE", TypeInfo(DataType::COMMANDLINE_DT_STRING_PAIR_ARRAY), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"runner-arg", "Args to pass to the runner", TypeInfo(DataType::COMMANDLINE_DT_STRING_ARRAY), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"metrics-mode", "Metrics Mode: --metrics-mode=MODE - options for MODE are\n"
     "udp://<hostname>:<port>\nfile:///path/to/file\nstderr", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"metrics-publish-interval", "Metrics publishing interval", TypeInfo(DataType::COMMANDLINE_DT_INT), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"config-file", "Absolute path to config file", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"verbose", "Adjust log verbosity", TypeInfo(DataType::COMMANDLINE_DT_BOOL)},
    {"", "BOT Id", TypeInfo(&botId), ArgumentSpec::O_OPTIONAL}
};

ArgumentSpec bindSpec[] = {
    {"help", "Display usage and exit", TypeInfo(DataType::COMMANDLINE_DT_BOOL)},
    {"instance", "Name of instance", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"cas-remote", "IP/port of remote CAS server", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"bots-remote", "IP/port of remote BOTS server", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"log-level", "Log verbosity level", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG},
    {"request-timeout", "Request timeout", TypeInfo(DataType::COMMANDLINE_DT_INT), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"buildbox-run", "Absolute path to runner exectuable", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"platform", "Platform properties", TypeInfo(&platformProperties), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"runner-arg", "Args to pass to the runner", TypeInfo(&runnerArgs), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"metrics-mode", "Metrics Mode", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"metrics-publish-interval", "Metrics publishing interval", TypeInfo(DataType::COMMANDLINE_DT_INT), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"config-file", "Absolute path to config file", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"verbose", "Adjust log verbosity", TypeInfo(DataType::COMMANDLINE_DT_BOOL)},
    {"", "BOT Id", TypeInfo(&botId), ArgumentSpec::O_REQUIRED}
};

ArgumentSpec twoPositionalSpec[] = {
    {"help", "Display usage and exit", TypeInfo(DataType::COMMANDLINE_DT_BOOL)},
    {"instance", "Name of instance", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"cas-remote", "IP/port of remote CAS server", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"bots-remote", "IP/port of remote BOTS server", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"log-level", "Log verbosity level", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG},
    {"request-timeout", "Request timeout", TypeInfo(DataType::COMMANDLINE_DT_INT), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"buildbox-run", "Absolute path to runner exectuable", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"platform", "Set a platform property(repeated):\n--platform KEY=VALUE\n--platform KEY=VALUE", TypeInfo(DataType::COMMANDLINE_DT_STRING_PAIR_ARRAY), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"runner-arg", "Args to pass to the runner", TypeInfo(DataType::COMMANDLINE_DT_STRING_ARRAY), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"metrics-mode", "Metrics Mode: --metrics-mode=MODE - options for MODE are\n"
     "udp://<hostname>:<port>\nfile:///path/to/file\nstderr", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"metrics-publish-interval", "Metrics publishing interval", TypeInfo(DataType::COMMANDLINE_DT_INT), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"config-file", "Absolute path to config file", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"verbose", "Adjust log verbosity", TypeInfo(DataType::COMMANDLINE_DT_BOOL)},
    {"", "Positional1", TypeInfo(&positional1), ArgumentSpec::O_REQUIRED},
    {"", "Positional2", TypeInfo(&positional2), ArgumentSpec::O_REQUIRED}
};

ArgumentSpec positionalOnlySpec[] = {
    {"", "Positional 1", TypeInfo(&positional1), ArgumentSpec::O_REQUIRED},
    {"", "Positional 2", TypeInfo(&positional2), ArgumentSpec::O_REQUIRED},
    {"", "Positional 3", TypeInfo(&positional3), ArgumentSpec::O_REQUIRED}
};

ArgumentSpec booleanSpecWithArgs[] = {
    {"use-sockets", "include on CML to enable networked logging", TypeInfo(DataType::COMMANDLINE_DT_BOOL), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"use-file", "Set to 'true' to use file logging", TypeInfo(DataType::COMMANDLINE_DT_BOOL), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"verbose", "Set to 'true' to enable DEBUG level logging", TypeInfo(DataType::COMMANDLINE_DT_BOOL), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG}
};

ArgumentSpec booleanSpecWithoutArgs[] = {
    {"use-sockets", "include on CML to enable networked logging", TypeInfo(DataType::COMMANDLINE_DT_BOOL), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITHOUT_ARG},
    {"use-file", "Set to 'true' to use file logging", TypeInfo(DataType::COMMANDLINE_DT_BOOL), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITHOUT_ARG},
    {"verbose", "Set to 'true' to enable DEBUG level logging", TypeInfo(DataType::COMMANDLINE_DT_BOOL), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITHOUT_ARG}
};

ArgumentSpec booleanSpecWithoutArgsOptional[] = {
    {"use-sockets", "include on CML to enable networked logging", TypeInfo(DataType::COMMANDLINE_DT_BOOL), ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITHOUT_ARG},
    {"use-file", "Set to 'true' to use file logging", TypeInfo(DataType::COMMANDLINE_DT_BOOL), ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITHOUT_ARG},
    {"verbose", "Set to 'true' to enable DEBUG level logging", TypeInfo(DataType::COMMANDLINE_DT_BOOL), ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITHOUT_ARG}
};

ArgumentSpec booleanSpecMixed[] = {
    {"use-sockets", "include on CML to enable networked logging", TypeInfo(DataType::COMMANDLINE_DT_BOOL), ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITHOUT_ARG},
    {"use-file", "Set to 'true' to use file logging", TypeInfo(DataType::COMMANDLINE_DT_BOOL), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"verbose", "Set to 'true' to enable DEBUG level logging", TypeInfo(DataType::COMMANDLINE_DT_BOOL), ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITHOUT_ARG}
};

ArgumentSpec specWithDefaultValues[] = {
    {"help", "Display usage and exit", TypeInfo(DataType::COMMANDLINE_DT_BOOL)},
    {"instance", "Name of instance", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG, CMLDefaultValue("dev")},
    {"cas-remote", "IP/port of remote CAS server", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"bots-remote", "IP/port of remote BOTS server", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"log-level", "Log verbosity level", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG, CMLDefaultValue("debug")},
    {"request-timeout", "Request timeout", TypeInfo(DataType::COMMANDLINE_DT_INT), ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG, CMLDefaultValue(30)},
    {"buildbox-run", "Absolute path to runner exectuable", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"platform", "Set a platform property(repeated):\n--platform KEY=VALUE\n--platform KEY=VALUE", TypeInfo(DataType::COMMANDLINE_DT_STRING_PAIR_ARRAY), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"runner-arg", "Args to pass to the runner", TypeInfo(DataType::COMMANDLINE_DT_STRING_ARRAY), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"metrics-mode", "Metrics Mode: --metrics-mode=MODE - options for MODE are\n"
     "udp://<hostname>:<port>\nfile:///path/to/file\nstderr", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG, CMLDefaultValue("udp://127.0.0.1:8125")},
    {"metrics-publish-interval", "Metrics publishing interval", TypeInfo(DataType::COMMANDLINE_DT_INT), ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG, CMLDefaultValue(10)},
    {"config-file", "Absolute path to config file", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG},
    {"verbose", "Adjust log verbosity", TypeInfo(DataType::COMMANDLINE_DT_BOOL), ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITHOUT_ARG, CMLDefaultValue(false)},
    {"", "BOT Id", TypeInfo(&botId), ArgumentSpec::O_OPTIONAL}
};

// test an option that is set as required and has a default option
ArgumentSpec specWithDefaultValuesFail[] = {
    {"help", "Display usage and exit", TypeInfo(DataType::COMMANDLINE_DT_BOOL)},
    {"instance", "Name of instance", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG, CMLDefaultValue("dev")},
    {"cas-remote", "IP/port of remote CAS server", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"bots-remote", "IP/port of remote BOTS server", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"log-level", "Log verbosity level", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG, CMLDefaultValue("debug")},
    {"request-timeout", "Request timeout", TypeInfo(DataType::COMMANDLINE_DT_INT), ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG, CMLDefaultValue(30)},
    {"buildbox-run", "Absolute path to runner exectuable", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"platform", "Set a platform property(repeated):\n--platform KEY=VALUE\n--platform KEY=VALUE", TypeInfo(DataType::COMMANDLINE_DT_STRING_PAIR_ARRAY), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"runner-arg", "Args to pass to the runner", TypeInfo(DataType::COMMANDLINE_DT_STRING_ARRAY), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"metrics-mode", "Metrics Mode: --metrics-mode=MODE - options for MODE are\n"
     "udp://<hostname>:<port>\nfile:///path/to/file\nstderr", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG, CMLDefaultValue("udp://127.0.0.1:8125")},
    {"metrics-publish-interval", "Metrics publishing interval", TypeInfo(DataType::COMMANDLINE_DT_INT), ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG, CMLDefaultValue(10)},
    {"config-file", "Absolute path to config file", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG},
    {"verbose", "Adjust log verbosity", TypeInfo(DataType::COMMANDLINE_DT_BOOL), ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITHOUT_ARG, CMLDefaultValue(false)},
    {"", "BOT Id", TypeInfo(&botId), ArgumentSpec::O_OPTIONAL}
};

// test type mismatch between option type
ArgumentSpec specWithMisMatchedTypes[] = {
    {"help", "Display usage and exit", TypeInfo(DataType::COMMANDLINE_DT_BOOL)},
    {"instance", "Name of instance", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG, CMLDefaultValue(42)},
    {"cas-remote", "IP/port of remote CAS server", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"bots-remote", "IP/port of remote BOTS server", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"log-level", "Log verbosity level", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG, CMLDefaultValue("debug")},
    {"request-timeout", "Request timeout", TypeInfo(DataType::COMMANDLINE_DT_INT), ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG, CMLDefaultValue(30.0)},
    {"buildbox-run", "Absolute path to runner exectuable", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"platform", "Set a platform property(repeated):\n--platform KEY=VALUE\n--platform KEY=VALUE", TypeInfo(DataType::COMMANDLINE_DT_STRING_PAIR_ARRAY), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"runner-arg", "Args to pass to the runner", TypeInfo(DataType::COMMANDLINE_DT_STRING_ARRAY), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"metrics-mode", "Metrics Mode: --metrics-mode=MODE - options for MODE are\n"
     "udp://<hostname>:<port>\nfile:///path/to/file\nstderr", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG, CMLDefaultValue("udp://127.0.0.1:8125")},
    {"metrics-publish-interval", "Metrics publishing interval", TypeInfo(DataType::COMMANDLINE_DT_INT), ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG, CMLDefaultValue(10)},
    {"config-file", "Absolute path to config file", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG},
    {"verbose", "Adjust log verbosity", TypeInfo(DataType::COMMANDLINE_DT_BOOL), ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITHOUT_ARG, CMLDefaultValue(false)},
    {"", "BOT Id", TypeInfo(&botId), ArgumentSpec::O_OPTIONAL}
};

// test optional args but use the getter API's that provide default values
ArgumentSpec specWithOptionalComplexTypes[] = {
    {"config-file", "Absolute path to config file", TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG},
    {"platform", "Set a platform property(repeated):\n--platform KEY=VALUE\n--platform KEY=VALUE", TypeInfo(DataType::COMMANDLINE_DT_STRING_PAIR_ARRAY), ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG},
    {"runner-arg", "Args to pass to the runner", TypeInfo(DataType::COMMANDLINE_DT_STRING_ARRAY), ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG},
    {"", "BOT Id", TypeInfo(&botId), ArgumentSpec::O_REQUIRED}
};

// format "--option=value"
const char *argvOptionEqualsValue[] = {
    "/some/path/to/some_program.tsk",
    "--instance=dev",
    "--cas-remote=http://127.0.0.1:50011",
    "--bots-remote=http://distributedbuild-bgd-dev-ob.bdns.bloomberg.com:50051",
    "--log-level=debug",
    "--request-timeout=30",
    "--buildbox-run=/opt/bb/bin/buildbox-run-userchroot",
    "--runner-arg=--use-localcas",
    "--runner-arg=--userchroot-bin=/bb/dbldroot/bin/userchroot",
    "--platform",
    "OSFamily=linux",
    "--platform",
    "ISA=x86-64",
    "--platform",
    "chrootRootDigest=8533ec9ba7494cc8295ccd0bfdca08457421a28b4e92c8eb18e7178fb400f5d4/930",
    "--platform",
    "chrootRootDigest=1e7088e7aca9e8713a84122218a89c8908b39b5797d32170f1afa6e474b9ade6/930",
    "--metrics-mode=udp://127.0.0.1:8125",
    "--metrics-publish-interval=10",
    "--config-file=/bb/data/dbldwr-config/buildboxworker.conf",
    "wrldev-ob-623-buildboxworker-20"
};

// format "--option value"
const char *argvOptionSpaceValue[] = {
    "/some/path/to/some_program.tsk",
    "--instance",
    "dev",
    "--cas-remote",
    "http://127.0.0.1:50011",
    "--bots-remote",
    "http://distributedbuild-bgd-dev-ob.bdns.bloomberg.com:50051",
    "--log-level",
    "debug",
    "--request-timeout",
    "30",
    "--buildbox-run",
    "/opt/bb/bin/buildbox-run-userchroot",
    "--runner-arg=--use-localcas",
    "--runner-arg=--userchroot-bin=/bb/dbldroot/bin/userchroot",
    "--platform",
    "OSFamily=linux",
    "--platform",
    "ISA=x86-64",
    "--platform",
    "chrootRootDigest=8533ec9ba7494cc8295ccd0bfdca08457421a28b4e92c8eb18e7178fb400f5d4/930",
    "--platform",
    "chrootRootDigest=1e7088e7aca9e8713a84122218a89c8908b39b5797d32170f1afa6e474b9ade6/930",
    "--metrics-mode",
    "udp://127.0.0.1:8125",
    "--metrics-publish-interval",
    "10",
    "--config-file",
    "/bb/data/dbldwr-config/buildboxworker.conf",
    "wrldev-ob-623-buildboxworker-20"
};

// purposely missing "--instance=dev"
const char *argvMissingRequired[] = {
    "/some/path/to/some_program.tsk",
    "--cas-remote",
    "http://127.0.0.1:50011",
    "--bots-remote",
    "http://distributedbuild-bgd-dev-ob.bdns.bloomberg.com:50051",
    "--log-level",
    "debug",
    "--request-timeout",
    "30",
    "--buildbox-run",
    "/opt/bb/bin/buildbox-run-userchroot",
    "--runner-arg=--use-localcas",
    "--runner-arg=--userchroot-bin=/bb/dbldroot/bin/userchroot",
    "--platform",
    "OSFamily=linux",
    "--platform",
    "ISA=x86-64",
    "--platform",
    "chrootRootDigest=8533ec9ba7494cc8295ccd0bfdca08457421a28b4e92c8eb18e7178fb400f5d4/930",
    "--metrics-mode",
    "udp://127.0.0.1:8125",
    "--metrics-publish-interval",
    "10",
    "--config-file",
    "/bb/data/dbldwr-config/buildboxworker.conf",
    "wrldev-ob-623-buildboxworker-20"
};

// --request-timeout not-a-number
const char *argvNotANumber[] = {
    "/some/path/to/some_program.tsk",
    "--instance",
    "dev",
    "--cas-remote",
    "http://127.0.0.1:50011",
    "--bots-remote",
    "http://distributedbuild-bgd-dev-ob.bdns.bloomberg.com:50051",
    "--log-level",
    "debug",
    "--request-timeout",
    "not-a-number",       // purposely not a number
    "--buildbox-run",
    "/opt/bb/bin/buildbox-run-userchroot",
    "--runner-arg=--use-localcas",
    "--runner-arg=--userchroot-bin=/bb/dbldroot/bin/userchroot",
    "--platform",
    "OSFamily=linux",
    "--platform",
    "ISA=x86-64",
    "--platform",
    "chrootRootDigest=8533ec9ba7494cc8295ccd0bfdca08457421a28b4e92c8eb18e7178fb400f5d4/930",
    "--metrics-mode",
    "udp://127.0.0.1:8125",
    "--metrics-publish-interval",
    "10",
    "--config-file",
    "/bb/data/dbldwr-config/buildboxworker.conf",
    "wrldev-ob-623-buildboxworker-20"
};

// --request-timeout <missing>
const char *argvMissingRequiredValue[] = {
    "/some/path/to/some_program.tsk",
    "--instance",
    "dev",
    "--cas-remote",
    "http://127.0.0.1:50011",
    "--bots-remote",
    "http://distributedbuild-bgd-dev-ob.bdns.bloomberg.com:50051",
    "--log-level",
    "debug",
    "--request-timeout",
    "--buildbox-run",
    "/opt/bb/bin/buildbox-run-userchroot",
    "--runner-arg=--use-localcas",
    "--runner-arg=--userchroot-bin=/bb/dbldroot/bin/userchroot",
    "--platform",
    "OSFamily=linux",
    "--platform",
    "ISA=x86-64",
    "--platform",
    "chrootRootDigest=8533ec9ba7494cc8295ccd0bfdca08457421a28b4e92c8eb18e7178fb400f5d4/930",
    "--metrics-mode",
    "udp://127.0.0.1:8125",
    "--metrics-publish-interval",
    "10",
    "--config-file",
    "/bb/data/dbldwr-config/buildboxworker.conf",
    "wrldev-ob-623-buildboxworker-20"
};

const char *argvMisplacedPositional[] = {
    "/some/path/to/some_program.tsk",
    "--instance",
    "dev",
    "wrldev-ob-623-buildboxworker-20",
    "--cas-remote",
    "http://127.0.0.1:50011",
    "--bots-remote",
    "http://distributedbuild-bgd-dev-ob.bdns.bloomberg.com:50051",
    "--log-level",
    "debug",
    "--request-timeout",
    "not-a-number",
    "--buildbox-run",
    "/opt/bb/bin/buildbox-run-userchroot",
    "--runner-arg=--use-localcas",
    "--runner-arg=--userchroot-bin=/bb/dbldroot/bin/userchroot",
    "--platform",
    "OSFamily=linux",
    "--platform",
    "ISA=x86-64",
    "--platform",
    "chrootRootDigest=8533ec9ba7494cc8295ccd0bfdca08457421a28b4e92c8eb18e7178fb400f5d4/930",
    "--metrics-mode",
    "udp://127.0.0.1:8125",
    "--metrics-publish-interval",
    "10",
    "--config-file",
    "/bb/data/dbldwr-config/buildboxworker.conf"
};

const char *argvMissingPositional1[] = {
    "/some/path/to/some_program.tsk",
    "--instance",
    "dev",
    "--cas-remote",
    "http://127.0.0.1:50011",
    "--bots-remote",
    "http://distributedbuild-bgd-dev-ob.bdns.bloomberg.com:50051",
    "--log-level",
    "debug",
    "--request-timeout",
    "10",
    "--buildbox-run",
    "/opt/bb/bin/buildbox-run-userchroot",
    "--runner-arg=--use-localcas",
    "--runner-arg=--userchroot-bin=/bb/dbldroot/bin/userchroot",
    "--platform",
    "OSFamily=linux",
    "--platform",
    "ISA=x86-64",
    "--platform",
    "chrootRootDigest=8533ec9ba7494cc8295ccd0bfdca08457421a28b4e92c8eb18e7178fb400f5d4/930",
    "--metrics-mode",
    "udp://127.0.0.1:8125",
    "--metrics-publish-interval",
    "10",
    "--config-file",
    "/bb/data/dbldwr-config/buildboxworker.conf"
};

const char *argvMissingPositional2[] = {
    "/some/path/to/some_program.tsk",
    "--instance",
    "dev",
    "--cas-remote",
    "http://127.0.0.1:50011",
    "--bots-remote",
    "http://distributedbuild-bgd-dev-ob.bdns.bloomberg.com:50051",
    "--log-level",
    "debug",
    "--request-timeout",
    "10",
    "--buildbox-run",
    "/opt/bb/bin/buildbox-run-userchroot",
    "--runner-arg=--use-localcas",
    "--runner-arg=--userchroot-bin=/bb/dbldroot/bin/userchroot",
    "--platform",
    "OSFamily=linux",
    "--platform",
    "ISA=x86-64",
    "--platform",
    "chrootRootDigest=8533ec9ba7494cc8295ccd0bfdca08457421a28b4e92c8eb18e7178fb400f5d4/930",
    "--metrics-mode",
    "udp://127.0.0.1:8125",
    "--metrics-publish-interval",
    "10",
    "--config-file",
    "/bb/data/dbldwr-config/buildboxworker.conf",
    "wrldev-ob-623-buildboxworker-20"
};

const char *argvOptionEqualsValueNoPositional[] = {
    "/some/path/to/some_program.tsk",
    "--instance=dev",
    "--cas-remote=http://127.0.0.1:50011",
    "--bots-remote=http://distributedbuild-bgd-dev-ob.bdns.bloomberg.com:50051",
    "--log-level=debug",
    "--request-timeout=30",
    "--buildbox-run=/opt/bb/bin/buildbox-run-userchroot",
    "--runner-arg=--use-localcas",
    "--runner-arg=--userchroot-bin=/bb/dbldroot/bin/userchroot",
    "--platform",
    "OSFamily=linux",
    "--platform",
    "ISA=x86-64",
    "--platform",
    "chrootRootDigest=8533ec9ba7494cc8295ccd0bfdca08457421a28b4e92c8eb18e7178fb400f5d4/930",
    "--platform",
    "chrootRootDigest=1e7088e7aca9e8713a84122218a89c8908b39b5797d32170f1afa6e474b9ade6/930",
    "--metrics-mode=udp://127.0.0.1:8125",
    "--metrics-publish-interval=10",
    "--config-file=/bb/data/dbldwr-config/buildboxworker.conf"
};

const char *argvOptionSpaceValueNoPositional[] = {
    "/some/path/to/some_program.tsk",
    "--instance",
    "dev",
    "--cas-remote",
    "http://127.0.0.1:50011",
    "--bots-remote",
    "http://distributedbuild-bgd-dev-ob.bdns.bloomberg.com:50051",
    "--log-level",
    "debug",
    "--request-timeout",
    "30",
    "--buildbox-run",
    "/opt/bb/bin/buildbox-run-userchroot",
    "--runner-arg=--use-localcas",
    "--runner-arg=--userchroot-bin=/bb/dbldroot/bin/userchroot",
    "--platform",
    "OSFamily=linux",
    "--platform",
    "ISA=x86-64",
    "--platform",
    "chrootRootDigest=8533ec9ba7494cc8295ccd0bfdca08457421a28b4e92c8eb18e7178fb400f5d4/930",
    "--platform",
    "chrootRootDigest=1e7088e7aca9e8713a84122218a89c8908b39b5797d32170f1afa6e474b9ade6/930",
    "--metrics-mode",
    "udp://127.0.0.1:8125",
    "--metrics-publish-interval",
    "10",
    "--config-file",
    "/bb/data/dbldwr-config/buildboxworker.conf"
};

const char *argvHelpOnly[] = {
    "/some/path/to/some_program.tsk",
    "--help"
};

const char *argvPositionalOnly[] = {
    "/some/path/to/some_program.tsk",
    "first-postional-arg",
    "42",
    "42.2"
};

const char *argvBooleanWithArgs[] = {
    "/some/path/to/some_program.tsk",
    "--use-sockets",
    "true",
    "--use-file",
    "false",
    "--verbose",
    "false"
};

const char *argvBooleanWithoutArgs[] = {
    "/some/path/to/some_program.tsk",
    "--use-sockets",
    "--use-file",
    "--verbose"
};

const char *argvBooleanWithoutArgsOptional[] = {
    "/some/path/to/some_program.tsk",
    "--use-sockets",
    "--use-file"
};

const char *argvBooleanMixed[] = {
    "/some/path/to/some_program.tsk",
    "--use-file",
    "true",
    "--verbose"
};

// format "--option=value"
const char *argvTestDefaultValues[] = {
    "/some/path/to/some_program.tsk",
    "--cas-remote=http://127.0.0.1:50011",
    "--bots-remote=http://distributedbuild-bgd-dev-ob.bdns.bloomberg.com:50051",
    "--buildbox-run=/opt/bb/bin/buildbox-run-userchroot",
    "--runner-arg=--use-localcas",
    "--runner-arg=--userchroot-bin=/bb/dbldroot/bin/userchroot",
    "--platform",
    "OSFamily=linux",
    "--platform",
    "ISA=x86-64",
    "--platform",
    "chrootRootDigest=8533ec9ba7494cc8295ccd0bfdca08457421a28b4e92c8eb18e7178fb400f5d4/930",
    "--platform",
    "chrootRootDigest=1e7088e7aca9e8713a84122218a89c8908b39b5797d32170f1afa6e474b9ade6/930",
    "--config-file=/bb/data/dbldwr-config/buildboxworker.conf",
    "wrldev-ob-623-buildboxworker-20"
};

const char *argvDefaultGetters[] = {
    "/some/path/to/some_program.tsk",
    "--config-file=/bb/data/dbldwr-config/buildboxworker.conf",
    "wrldev-ob-623-buildboxworker-20"
};

const std::string expectedUsage(
    "Usage: \n"
    "   --help                         Display usage and exit [optional]\n"
    "   --instance                     Name of instance [required]\n"
    "   --cas-remote                   IP/port of remote CAS server [required]\n"
    "   --bots-remote                  IP/port of remote BOTS server [required]\n"
    "   --log-level                    Log verbosity level [optional]\n"
    "   --request-timeout              Request timeout [required]\n"
    "   --buildbox-run                 Absolute path to runner exectuable [required]\n"
    "   --platform                     Set a platform property(repeated):\n"
    "                                     --platform KEY=VALUE\n"
    "                                     --platform KEY=VALUE [required]\n"
    "   --runner-arg                   Args to pass to the runner [required]\n"
    "   --metrics-mode                 Metrics Mode: --metrics-mode=MODE - options for MODE are\n"
    "                                     udp://<hostname>:<port>\n"
    "                                     file:///path/to/file\n"
    "                                     stderr [required]\n"
    "   --metrics-publish-interval     Metrics publishing interval [required]\n"
    "   --config-file                  Absolute path to config file [required]\n"
    "   --verbose                      Adjust log verbosity [optional]\n"
    "     BOT Id                       POSITIONAL [required]\n\n");

const std::string expectedUsageWithDefaults(
    "Usage: \n"
    "   --help                         Display usage and exit [optional]\n"
    "   --instance                     Name of instance [optional, default = \"dev\"]\n"
    "   --cas-remote                   IP/port of remote CAS server [required]\n"
    "   --bots-remote                  IP/port of remote BOTS server [required]\n"
    "   --log-level                    Log verbosity level [optional, default = \"debug\"]\n"
    "   --request-timeout              Request timeout [optional, default = 30]\n"
    "   --buildbox-run                 Absolute path to runner exectuable [required]\n"
    "   --platform                     Set a platform property(repeated):\n"
    "                                     --platform KEY=VALUE\n"
    "                                     --platform KEY=VALUE [required]\n"
    "   --runner-arg                   Args to pass to the runner [required]\n"
    "   --metrics-mode                 Metrics Mode: --metrics-mode=MODE - options for MODE are\n"
    "                                     udp://<hostname>:<port>\n"
    "                                     file:///path/to/file\n"
    "                                     stderr [optional, default = \"udp://127.0.0.1:8125\"]\n"
    "   --metrics-publish-interval     Metrics publishing interval [optional, default = 10]\n"
    "   --config-file                  Absolute path to config file [optional]\n"
    "   --verbose                      Adjust log verbosity [optional, default = false]\n"
    "     BOT Id                       POSITIONAL [optional]\n\n");
// clang-format on

void initVectorOfSpecs(std::vector<ArgumentSpec> &spec)
{
    spec.emplace_back("help", "Display usage and exit",
                      TypeInfo(DataType::COMMANDLINE_DT_BOOL));
    spec.emplace_back("instance", "Name of instance",
                      TypeInfo(DataType::COMMANDLINE_DT_STRING),
                      ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG);
    spec.emplace_back("concurrent-jobs", "Stop after running this many jobs",
                      TypeInfo(DataType::COMMANDLINE_DT_INT),
                      ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG);
    spec.emplace_back("stop-after", "Stop after running this many jobs",
                      TypeInfo(DataType::COMMANDLINE_DT_INT),
                      ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG);
    spec.emplace_back("cas-remote", "IP/port of remote CAS server",
                      TypeInfo(DataType::COMMANDLINE_DT_STRING),
                      ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG);
    spec.emplace_back("bots-remote", "IP/port of remote BOTS server",
                      TypeInfo(DataType::COMMANDLINE_DT_STRING),
                      ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG);
    spec.emplace_back("request-timeout", "Request timeout",
                      TypeInfo(DataType::COMMANDLINE_DT_INT),
                      ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG);
    spec.emplace_back("buildbox-run", "Absolute path to runner exectuable",
                      TypeInfo(DataType::COMMANDLINE_DT_STRING),
                      ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG);
    spec.emplace_back("runner-arg", "Args to pass to the runner",
                      TypeInfo(DataType::COMMANDLINE_DT_STRING_ARRAY),
                      ArgumentSpec::O_REQUIRED, ArgumentSpec::C_WITH_ARG);
    spec.emplace_back("platform",
                      "Set a platform property(repeated):\n--platform "
                      "KEY=VALUE\n--platform KEY=VALUE",
                      TypeInfo(DataType::COMMANDLINE_DT_STRING_PAIR_ARRAY),
                      ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG);
    spec.emplace_back(
        "metrics-mode",
        "Metrics Mode: --metrics-mode=MODE - options for MODE are\n"
        "udp://<hostname>:<port>\nfile:///path/to/file\nstderr",
        TypeInfo(DataType::COMMANDLINE_DT_STRING), ArgumentSpec::O_OPTIONAL,
        ArgumentSpec::C_WITH_ARG);
    spec.emplace_back("metrics-publish-interval",
                      "Metrics publishing interval",
                      TypeInfo(DataType::COMMANDLINE_DT_INT),
                      ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG);
    spec.emplace_back("log-level", "Log verbosity level",
                      TypeInfo(DataType::COMMANDLINE_DT_STRING),
                      ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG);
    spec.emplace_back("verbose", "Set log level to debug",
                      TypeInfo(DataType::COMMANDLINE_DT_BOOL),
                      ArgumentSpec::O_OPTIONAL);
    spec.emplace_back("log-file", "Log file name",
                      TypeInfo(DataType::COMMANDLINE_DT_STRING),
                      ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG);
    spec.emplace_back("config-file", "Absolute path to config file",
                      TypeInfo(DataType::COMMANDLINE_DT_STRING),
                      ArgumentSpec::O_OPTIONAL, ArgumentSpec::C_WITH_ARG);
    spec.emplace_back("", "BOT Id", TypeInfo(&botId),
                      ArgumentSpec::O_OPTIONAL);
}

void validate(const CommandLine &cml)
{
    // primitive types
    EXPECT_EQ("dev", cml.getString("instance"));
    EXPECT_EQ("http://127.0.0.1:50011", cml.getString("cas-remote"));
    EXPECT_EQ("http://distributedbuild-bgd-dev-ob.bdns.bloomberg.com:50051",
              cml.getString("bots-remote"));
    EXPECT_EQ("debug", cml.getString("log-level"));
    EXPECT_EQ(30, cml.getInt("request-timeout"));
    EXPECT_EQ("/opt/bb/bin/buildbox-run-userchroot",
              cml.getString("buildbox-run"));
    EXPECT_EQ("udp://127.0.0.1:8125", cml.getString("metrics-mode"));
    EXPECT_EQ(10, cml.getInt("metrics-publish-interval"));
    EXPECT_EQ("/bb/data/dbldwr-config/buildboxworker.conf",
              cml.getString("config-file"));

    // more complex types
    const Type::VectorOfString &vs = cml.getVS("runner-arg");
    EXPECT_EQ(2, vs.size());
    EXPECT_THAT(vs,
                ElementsAre("--use-localcas",
                            "--userchroot-bin=/bb/dbldroot/bin/userchroot"));

    const Type::VectorOfPairOfString &vps = cml.getVPS("platform");
    EXPECT_EQ(4, vps.size());
    EXPECT_THAT(vps,
                ElementsAre(Pair("OSFamily", "linux"), Pair("ISA", "x86-64"),
                            Pair("chrootRootDigest",
                                 "8533ec9ba7494cc8295ccd0bfdca08457421a28b"
                                 "4e92c8eb18e7178fb400f5d4/930"),
                            Pair("chrootRootDigest",
                                 "1e7088e7aca9e8713a84122218a89c8908b39b57"
                                 "97d32170f1afa6e474b9ade6/930")));

    EXPECT_STREQ("wrldev-ob-623-buildboxworker-20", botId.c_str());
}

// test "--foo=bar" option formatting
TEST(CommandLineTests, Format1)
{
    const int argc = sizeof(argvOptionEqualsValue) / sizeof(const char *);
    CommandLine commandLine(defaultSpec);
    const bool success = commandLine.parse(argc, argvOptionEqualsValue);
    EXPECT_TRUE(success);
    validate(commandLine);
}

// test "--foo bar" option formatting
TEST(CommandLineTests, Format2)
{
    const int argc = sizeof(argvOptionSpaceValue) / sizeof(const char *);
    CommandLine commandLine(defaultSpec);
    const bool success = commandLine.parse(argc, argvOptionSpaceValue);
    EXPECT_TRUE(success);
    validate(commandLine);
}

TEST(CommandLineTests, EmptyArgs)
{
    const char *argv[] = {"/some/path/to/some_program.tsk"};
    CommandLine commandLine(defaultSpec);
    EXPECT_FALSE(commandLine.parse(1, argv));
}

TEST(CommandLineTests, MissingRequired)
{
    const int argc = sizeof(argvMissingRequired) / sizeof(const char *);
    CommandLine commandLine(defaultSpec);
    std::ostringstream oss;
    const bool success = commandLine.parse(argc, argvMissingRequired, oss);
    EXPECT_FALSE(success);
}

// test "--foo bar" option with binding variables
TEST(CommandLineTests, Binding)
{
    const int argc = sizeof(argvOptionSpaceValue) / sizeof(const char *);
    CommandLine commandLine(bindSpec);
    const bool success = commandLine.parse(argc, argvOptionSpaceValue);
    EXPECT_TRUE(success);
    validate(commandLine);

    EXPECT_EQ(2, runnerArgs.size());
    EXPECT_EQ("--use-localcas", runnerArgs[0]);
    EXPECT_EQ("--userchroot-bin=/bb/dbldroot/bin/userchroot", runnerArgs[1]);

    EXPECT_EQ(4, platformProperties.size());
    EXPECT_EQ("OSFamily", platformProperties[0].first);
    EXPECT_EQ("linux", platformProperties[0].second);

    EXPECT_EQ("ISA", platformProperties[1].first);
    EXPECT_EQ("x86-64", platformProperties[1].second);

    EXPECT_EQ("chrootRootDigest", platformProperties[2].first);
    EXPECT_EQ(
        "8533ec9ba7494cc8295ccd0bfdca08457421a28b4e92c8eb18e7178fb400f5d4/930",
        platformProperties[2].second);

    EXPECT_EQ("chrootRootDigest", platformProperties[3].first);
    EXPECT_EQ(
        "1e7088e7aca9e8713a84122218a89c8908b39b5797d32170f1afa6e474b9ade6/930",
        platformProperties[3].second);
}

TEST(CommandLineTests, TestUsage)
{
    CommandLine commandLine(defaultSpec);
    std::ostringstream oss;
    commandLine.usage(oss);
    EXPECT_EQ(oss.str(), expectedUsage);
}

TEST(CommandLineTests, TestUsageWithDefaults)
{
    CommandLine commandLine(specWithDefaultValues);
    std::ostringstream oss;
    commandLine.usage(oss);
    EXPECT_EQ(oss.str(), expectedUsageWithDefaults);
}

TEST(CommandLineTests, NoSuchOptionException)
{
    const int argc = sizeof(argvOptionEqualsValue) / sizeof(const char *);
    CommandLine commandLine(defaultSpec);
    const bool success = commandLine.parse(argc, argvOptionEqualsValue);
    EXPECT_TRUE(success);
    validate(commandLine);

    EXPECT_THROW(commandLine.getString("nosuchoption"), std::runtime_error);
}

TEST(CommandLineTests, BadStringToIntegerException)
{
    const int argc = sizeof(argvNotANumber) / sizeof(const char *);
    CommandLine commandLine(defaultSpec);
    const bool success = commandLine.parse(argc, argvNotANumber);
    EXPECT_FALSE(success);
}

TEST(CommandLineTests, HelpOnly)
{
    const int argc = sizeof(argvHelpOnly) / sizeof(const char *);
    CommandLine commandLine(defaultSpec);
    EXPECT_TRUE(commandLine.parse(argc, argvHelpOnly));
}

TEST(CommandLineTests, MissingRequiredValue)
{
    const int argc = sizeof(argvMissingRequiredValue) / sizeof(const char *);
    CommandLine commandLine(defaultSpec);
    EXPECT_FALSE(commandLine.parse(argc, argvMissingRequiredValue));
}

TEST(CommandLineTests, MisplacedPositional)
{
    const int argc = sizeof(argvMisplacedPositional) / sizeof(const char *);
    CommandLine commandLine(defaultSpec);
    EXPECT_FALSE(commandLine.parse(argc, argvMisplacedPositional));
}

TEST(CommandLineTests, PositionalOnly)
{
    const int argc = sizeof(argvPositionalOnly) / sizeof(const char *);
    CommandLine commandLine(positionalOnlySpec);
    EXPECT_TRUE(commandLine.parse(argc, argvPositionalOnly));
    EXPECT_EQ(positional1, "first-postional-arg");
    EXPECT_EQ(positional2, 42);
    EXPECT_EQ(positional3, 42.2);
}

TEST(CommandLineTests, MissingPositional1)
{
    const int argc = sizeof(argvMissingPositional1) / sizeof(const char *);
    CommandLine commandLine(defaultSpec);
    EXPECT_FALSE(commandLine.parse(argc, argvMissingPositional1));
}

TEST(CommandLineTests, MissingPositional2)
{
    const int argc = sizeof(argvMissingPositional2) / sizeof(const char *);
    CommandLine commandLine(twoPositionalSpec);
    EXPECT_FALSE(commandLine.parse(argc, argvMissingPositional2));
}

TEST(CommandLineTests, TestBooleanWithArgs)
{
    const int argc = sizeof(argvBooleanWithArgs) / sizeof(const char *);
    CommandLine commandLine(booleanSpecWithArgs);
    EXPECT_TRUE(commandLine.parse(argc, argvBooleanWithArgs));

    EXPECT_TRUE(commandLine.getBool("use-sockets"));
    EXPECT_FALSE(commandLine.getBool("use-file"));
    EXPECT_FALSE(commandLine.getBool("verbose"));
}

TEST(CommandLineTests, TestBooleanWithoutArgs)
{
    const int argc = sizeof(argvBooleanWithoutArgs) / sizeof(const char *);
    CommandLine commandLine(booleanSpecWithoutArgs);
    EXPECT_TRUE(commandLine.parse(argc, argvBooleanWithoutArgs));

    EXPECT_TRUE(commandLine.getBool("use-sockets"));
    EXPECT_TRUE(commandLine.getBool("use-file"));
    EXPECT_TRUE(commandLine.getBool("verbose"));
}

TEST(CommandLineTests, TestBooleanWithoutArgsOptional)
{
    const int argc =
        sizeof(argvBooleanWithoutArgsOptional) / sizeof(const char *);
    CommandLine commandLine(booleanSpecWithoutArgsOptional);
    EXPECT_TRUE(commandLine.parse(argc, argvBooleanWithoutArgsOptional));

    EXPECT_TRUE(commandLine.getBool("use-sockets"));
    EXPECT_TRUE(commandLine.getBool("use-file"));
    EXPECT_FALSE(commandLine.exists("verbose"));
    EXPECT_THROW(commandLine.getBool("verbose"), std::runtime_error);
}

TEST(CommandLineTests, TestBooleanMixed)
{
    const int argc = sizeof(argvBooleanMixed) / sizeof(const char *);
    CommandLine commandLine(booleanSpecMixed);
    EXPECT_TRUE(commandLine.parse(argc, argvBooleanMixed));

    EXPECT_FALSE(commandLine.exists("use-sockets"));
    EXPECT_TRUE(commandLine.getBool("use-file"));
    EXPECT_TRUE(commandLine.getBool("verbose"));
}

TEST(CommandLineTests, TestMissingOptionalPositional)
{
    const int argc =
        sizeof(argvOptionSpaceValueNoPositional) / sizeof(const char *);
    CommandLine commandLine(positionalNotRequiredSpec);
    EXPECT_TRUE(commandLine.parse(argc, argvOptionSpaceValueNoPositional));
}

TEST(CommandLineTests, TestNoPositionals1)
{
    const int argc =
        sizeof(argvOptionEqualsValueNoPositional) / sizeof(const char *);
    CommandLine commandLine(noPositionalsSpec);
    EXPECT_TRUE(commandLine.parse(argc, argvOptionEqualsValueNoPositional));
}

TEST(CommandLineTests, TestNoPositionals2)
{
    const int argc =
        sizeof(argvOptionSpaceValueNoPositional) / sizeof(const char *);
    CommandLine commandLine(noPositionalsSpec);
    EXPECT_TRUE(commandLine.parse(argc, argvOptionSpaceValueNoPositional));
}

TEST(CommandLineTests, GettersWithFallbackValuesPresent)
{
    const ArgumentSpec testSpec[] = {
        {"bool-option", "", TypeInfo(DataType::COMMANDLINE_DT_BOOL)},
        {"int-option", "", TypeInfo(DataType::COMMANDLINE_DT_INT)},
        {"double-option", "", TypeInfo(DataType::COMMANDLINE_DT_DOUBLE)},
        {"string-option", "", TypeInfo(DataType::COMMANDLINE_DT_STRING)},
    };

    const char *argvOptions[] = {
        "--bool-option",
        "--int-option=1024",
        "--double-option=3.14",
        "--string-option=foo",
    };

    const int argc = sizeof(argvOptions) / sizeof(const char *);
    CommandLine commandLine(testSpec);

    const bool success = commandLine.parse(argc, argvOptions);
    ASSERT_TRUE(success);

    EXPECT_EQ(commandLine.getString("string-option", "default-string"), "foo");

    EXPECT_EQ(commandLine.getInt("int-option", 0), 1024);

    EXPECT_EQ(commandLine.getDouble("double-option", 1.11), 3.14);

    EXPECT_EQ(commandLine.getString("string-option", "bar"), "foo");
}

TEST(CommandLineTests, GettersWithFallbackDefaultValues)
{
    CommandLine commandLine(defaultSpec);

    const auto option_name = "option123";
    EXPECT_FALSE(commandLine.exists("option_name"));

    EXPECT_EQ(commandLine.getString(option_name, "foo"), "foo");

    EXPECT_EQ(commandLine.getBool(option_name, true), true);

    EXPECT_EQ(commandLine.getInt(option_name, 1024), 1024);

    EXPECT_EQ(commandLine.getDouble(option_name, 3.14), 3.14);
}

TEST(CommandLineTests, TestVectorOfSpecs)
{
    std::vector<ArgumentSpec> spec;
    initVectorOfSpecs(spec);

    CommandLine commandLine(spec);
    const bool success = commandLine.parse(
        (sizeof(argvOptionEqualsValue) / sizeof(const char *)),
        argvOptionEqualsValue);
    EXPECT_TRUE(success);
    validate(commandLine);
}

TEST(CommandLineTests, TestDefaultValues)
{
    CommandLine commandLine(specWithDefaultValues);
    const bool success = commandLine.parse(
        (sizeof(argvTestDefaultValues) / sizeof(const char *)),
        argvTestDefaultValues);
    EXPECT_TRUE(success);
    validate(commandLine);
}

TEST(CommandLineTests, TestDefaultValuesError)
{
    CommandLine commandLine(specWithDefaultValuesFail);
    const bool success = commandLine.parse(
        (sizeof(argvOptionEqualsValue) / sizeof(const char *)),
        argvOptionEqualsValue);
    EXPECT_FALSE(success);
}

TEST(CommandLineTests, TestWithMisMatchedTypes)
{
    CommandLine commandLine(specWithMisMatchedTypes);
    const bool success = commandLine.parse(
        (sizeof(argvTestDefaultValues) / sizeof(const char *)),
        argvTestDefaultValues);
    EXPECT_FALSE(success);
}

TEST(CommandLineTests, TestDefaultAPIWithComplexTypes)
{
    // runner-arg and is intentionally optional and not provided so we
    // can confirm that the getters return the provided defaults
    CommandLine commandLine(specWithOptionalComplexTypes);
    const bool success =
        commandLine.parse((sizeof(argvDefaultGetters) / sizeof(const char *)),
                          argvDefaultGetters);
    EXPECT_TRUE(success);

    // confirm the library provided default values are indeed empty
    EXPECT_TRUE(commandLine.getVS("runner-arg").empty());
    EXPECT_TRUE(commandLine.getVS("platform").empty());

    // confirm that user provided default values are returned
    Type::VectorOfString vsDefaultValue = {std::string("one"),
                                           std::string("two")};
    Type::VectorOfPairOfString vpsDefaultValue = {
        Type::PairOfString("first-1", "second-1"),
        Type::PairOfString("first-2", "second-2")};

    EXPECT_EQ(vsDefaultValue.size(),
              commandLine.getVS("runner-arg", vsDefaultValue).size());
    EXPECT_THAT(vsDefaultValue, ElementsAre("one", "two"));

    EXPECT_EQ(vpsDefaultValue.size(),
              commandLine.getVPS("platform", vpsDefaultValue).size());
    EXPECT_THAT(vpsDefaultValue, ElementsAre(Pair("first-1", "second-1"),
                                             Pair("first-2", "second-2")));
}
