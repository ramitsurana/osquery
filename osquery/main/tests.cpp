/*
 *  Copyright (c) 2014-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Shlwapi.h>
#include <Windows.h>
#endif

#include <memory>

#include <gtest/gtest.h>

#include <osquery/logger.h>

#include "osquery/core/process.h"
#include "osquery/core/test_util.h"
#include "osquery/core/testing.h"

namespace osquery {

/// This is exposed for process_tests.cpp. Without exporting this variable, we
/// would need to use more complicated measures to determine the current
/// executing file's path.
std::string kProcessTestExecPath;

/// This is the expected module name of the launcher process.
const char *kOsqueryTestModuleName = "osquery_tests.exe";

/// These are the expected arguments for our test worker process.
const char *kExpectedWorkerArgs[] = {"worker-test", "--socket",
                                     "fake-socket", nullptr};
const size_t kExpectedWorkerArgsCount =
    (sizeof(osquery::kExpectedWorkerArgs) / sizeof(char *)) - 1;

/// These are the expected arguments for our test extensions process.
const char *kExpectedExtensionArgs[] = {
    "osquery extension: extension-test", "--socket",  "socket-name",
    "--timeout",                         "100",       "--interval",
    "5",                                 "--verbose", nullptr};
const size_t kExpectedExtensionArgsCount =
    (sizeof(osquery::kExpectedExtensionArgs) / sizeof(char *)) - 1;

static bool compareArguments(char *result[],
                             unsigned int result_nelms,
                             const char *expected[],
                             unsigned int expected_nelms) {
  if (result_nelms != expected_nelms) {
    return false;
  }

  for (size_t i = 0; i < expected_nelms; i++) {
    if (strlen(result[i]) != strlen(expected[i])) {
      return false;
    }

    if (strncmp(result[i], expected[i], strlen(expected[i])) != 0) {
      return false;
    }
  }

  return true;
}
}

int workerMain(int argc, char *argv[]) {
  if (!osquery::compareArguments(argv,
                                 argc,
                                 osquery::kExpectedWorkerArgs,
                                 osquery::kExpectedWorkerArgsCount)) {
    return ERROR_COMPARE_ARGUMENT;
  }

  auto process = osquery::PlatformProcess::getLauncherProcess();
  if (process.get() == nullptr) {
    return ERROR_LAUNCHER_PROCESS;
  }

#ifdef WIN32
  CHAR buffer[1024] = {0};
  DWORD size = 1024;
  if (!QueryFullProcessImageNameA(process->nativeHandle(), 0, buffer, &size)) {
    return ERROR_QUERY_PROCESS_IMAGE;
  }
  PathStripPathA(buffer);

  if (strlen(buffer) != strlen(osquery::kOsqueryTestModuleName)) {
    return ERROR_IMAGE_NAME_LENGTH;
  }

  if (strncmp(buffer, osquery::kOsqueryTestModuleName, strlen(buffer)) != 0) {
    return ERROR_LAUNCHER_MISMATCH;
  }
#else
  if (process->nativeHandle() != getppid()) {
    return ERROR_LAUNCHER_MISMATCH;
  }
#endif
  return WORKER_SUCCESS_CODE;
}

int extensionMain(int argc, char *argv[]) {
  if (!osquery::compareArguments(argv,
                                 argc,
                                 osquery::kExpectedExtensionArgs,
                                 osquery::kExpectedExtensionArgsCount)) {
    return ERROR_COMPARE_ARGUMENT;
  }
  return EXTENSION_SUCCESS_CODE;
}

int main(int argc, char *argv[]) {
  if (auto val = osquery::getEnvVar("OSQUERY_WORKER")) {
    return workerMain(argc, argv);
  } else if ((val = osquery::getEnvVar("OSQUERY_EXTENSION"))) {
    return extensionMain(argc, argv);
  }
  osquery::kProcessTestExecPath = argv[0];

  osquery::initTesting();
  testing::InitGoogleTest(&argc, argv);
  // Optionally enable Goggle Logging
  // google::InitGoogleLogging(argv[0]);
  auto result = RUN_ALL_TESTS();

  osquery::shutdownTesting();
  return result;
}

