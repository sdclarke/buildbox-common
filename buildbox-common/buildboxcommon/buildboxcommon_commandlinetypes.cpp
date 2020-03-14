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

namespace buildboxcommon {

std::ostream &operator<<(std::ostream &out,
                         const CommandLineTypes::ArgumentSpec &obj)
{
    out << "[\"" << obj.d_name << "\", \"" << obj.d_desc << "\", "
        << obj.d_type << ", " << obj.d_occurrence << ", " << obj.d_constraint
        << "]";
    return out;
}

std::ostream &operator<<(std::ostream &out,
                         const CommandLineTypes::TypeInfo &obj)
{
    out << obj.d_type;
    return out;
}

} // namespace buildboxcommon
