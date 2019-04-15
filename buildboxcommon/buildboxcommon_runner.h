/*
 * Copyright 2018 Bloomberg Finance LP
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

#ifndef INCLUDED_BUILDBOXCOMMON_RUNNER
#define INCLUDED_BUILDBOXCOMMON_RUNNER

#include <buildboxcommon_client.h>
#include <buildboxcommon_connectionoptions.h>
#include <buildboxcommon_stageddirectory.h>

namespace buildboxcommon {

class Runner {
  public:
    /**
     * Execute the given Command in the given input root and return an
     * ActionResult. Subclasses should override this to implement sandboxing
     * behaviors.
     */
    virtual ActionResult execute(const Command &command,
                                 const Digest &inputRootDigest) = 0;

    /**
     * Subclasses can override this to add support for special arguments.
     * Return true if an argument was handled successfully.
     */
    virtual bool parseArg(const char *arg) { return false; }

    /**
     * Subclasses can override this to print a message after Runner prints
     * its usage message.
     */
    virtual void printSpecialUsage() {}

    int main(int argc, char *argv[]);
    virtual ~Runner(){};

  protected:
    /**
     * Execute the given command (without attempting to sandbox it) and
     * store its stdout, stderr, and exit code in the given ActionResult.
     */
    void executeAndStore(std::vector<std::string> command,
                         ActionResult *result);
    /**
     * Stage the directory with the given digest at an arbitrary path and
     * return a StagedDirectory object representing it.
     */
    std::unique_ptr<StagedDirectory> stage(const Digest &directoryDigest);

    std::shared_ptr<Client> d_casClient =
        std::shared_ptr<Client>(new Client());

    bool d_verbose;

  private:
    /**
     * Attempt to parse all of the given arguments and update this object to
     * reflect them. If an argument is invalid or missing, return false.
     * Otherwise, return true.
     */
    bool parseArguments(int argc, char *argv[]);

    /**
     * If the given string is larger than the maximum size, upload it to CAS,
     * store its digest in the given digest pointer, and clear it.
     */
    void uploadIfNeeded(std::string *str, Digest *digest);

    ConnectionOptions d_casRemote;
    const char *d_inputPath;
    const char *d_outputPath;
};

#define BUILDBOX_RUNNER_MAIN(x)                                               \
    int main(int argc, char *argv[]) { x().main(argc, argv); }

} // namespace buildboxcommon

#endif
