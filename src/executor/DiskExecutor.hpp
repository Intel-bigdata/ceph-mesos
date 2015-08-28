/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _CEPHMESOS_EXECUTOR_DISKEXECUTOR_HPP_
#define _CEPHMESOS_EXECUTOR_DISKEXECUTOR_HPP_

#include <mesos/executor.hpp>
#include <vector>

using std::string;
using std::vector;
using namespace mesos;

class DiskExecutor : public Executor
{
public:
  DiskExecutor(){}

  ~DiskExecutor(){}

  void registered(
      ExecutorDriver* driver,
      const ExecutorInfo& executorInfo,
      const FrameworkInfo& frameworkInfo,
      const SlaveInfo& slaveInfo);

  void reregistered(ExecutorDriver* driver, const SlaveInfo& slaveInfo);

  void disconnected(ExecutorDriver* driver);

  void launchTask(ExecutorDriver* driver, const TaskInfo& task);

  void killTask(ExecutorDriver* driver, const TaskID& taskId);

  void frameworkMessage(ExecutorDriver* driver, const string& data);

  void shutdown(ExecutorDriver* driver);

  void error(ExecutorDriver* driver, const string& message);

private:
  string runShellCommand(string cmd);

  void prepareDisks();

  void partitionDisk(string diskname, int partitionCount, string partitionLabel);

  void mkfsDisk(string diskname, string type, string flags);

  bool checkSuccess(string diskname, int partitionCount, string partitionLabel);

  string prepareReturnValue();

  bool addPendingDisks(TaskInfo task);

  TaskID myTaskId;

  string myHostname;

  vector<string> pendingOSDDevs;

  vector<string> pendingJNLDevs;

  vector<string> failedOSDDevs;

  vector<string> failedJNLDevs;

  int jnlPartitionCount = 4;

  string fsType = "xfs";

  string mkfsFLAGS = " -f -i size=2048 -n size=64k";
};

#endif
