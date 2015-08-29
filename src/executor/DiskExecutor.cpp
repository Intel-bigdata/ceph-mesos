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

#include "DiskExecutor.hpp"

#include <glog/logging.h>
#include <boost/lexical_cast.hpp>

#include "common/FrameworkMessage.hpp"

using boost::lexical_cast;

void DiskExecutor::reregistered(
      ExecutorDriver* driver,
      const SlaveInfo& slaveInfo){}

void DiskExecutor::disconnected(ExecutorDriver* driver){}

void DiskExecutor::killTask(ExecutorDriver* driver, const TaskID& taskId){}

void DiskExecutor::error(ExecutorDriver* driver, const string& message){}

void DiskExecutor::frameworkMessage(ExecutorDriver* driver, const string& data){}

void DiskExecutor::shutdown(ExecutorDriver* driver){}

void DiskExecutor::registered(
      ExecutorDriver* driver,
      const ExecutorInfo& executorInfo,
      const FrameworkInfo& frameworkInfo,
      const SlaveInfo& slaveInfo)
{
  LOG(INFO) << "DiskExecutor registed";
  myHostname = slaveInfo.hostname();
}

void DiskExecutor::launchTask(ExecutorDriver* driver, const TaskInfo& task)
{
  TaskStatus status;
  myTaskId = task.task_id();
  status.mutable_task_id()->MergeFrom(myTaskId);
  //get the disks
  if (!addPendingDisks(task)) {
    status.set_state(TASK_FAILED);                                                                                                 
    driver->sendStatusUpdate(status);
  }
  //partition them
  prepareDisks(); 
  //return success partitions and finish
  string msg = prepareReturnValue();
  status.set_message(msg);
  status.set_state(TASK_FINISHED);
  driver->sendStatusUpdate(status); 
}

string DiskExecutor::runShellCommand(string cmd)
{
  FILE* pipe = popen(cmd.c_str(), "r");
  if (!pipe) return "Run shell command ERROR!";
  char buffer[128];
  std::string result = "";
  while (!feof(pipe)) {
    if (fgets(buffer, 128, pipe) != NULL) {
      result += buffer;
    }
  }
  size_t last = result.find_last_not_of('\n');
  result = result.substr(0,(last+1));
  pclose(pipe);
  return result;
}

void DiskExecutor::prepareDisks()
{
  bool r = true;
  string partition = "";
  string osdlabel = "cephosddata";
  string jnllabel = "cephjnldata";
  for (size_t i = 0; i < pendingOSDDevs.size(); i++) {
    partitionDisk(pendingOSDDevs[i], 1, osdlabel);
    partition = "/dev/" + pendingOSDDevs[i] + "1";
    mkfsDisk(partition, fsType, mkfsFLAGS);
    r = checkSuccess(pendingOSDDevs[i], 1, osdlabel);
    if (!r) {
      failedOSDDevs.push_back(pendingOSDDevs[i]);
    }
  }
  for (size_t i = 0; i < pendingJNLDevs.size(); i++) {
    partitionDisk(pendingJNLDevs[i], jnlPartitionCount, jnllabel);
    for (int j = 1; j < jnlPartitionCount + 1; j++) {
      partition = "/dev/" + pendingJNLDevs[i] + lexical_cast<string>(j);
      mkfsDisk(partition, fsType, mkfsFLAGS);
    }//end for jnlPartitionCount
    r = checkSuccess(pendingJNLDevs[i], jnlPartitionCount, jnllabel);
    if (!r) {
      failedJNLDevs.push_back(pendingJNLDevs[i]);
    }
  }
}

void DiskExecutor::partitionDisk(string diskname, int partitionCount, string partitionLabel)
{
  string diskpath = "/dev/" + diskname;
  string cmd = "dd if=/dev/zero of=" + diskpath +
      " bs=4M count=1 oflag=direct 2>/dev/null;echo $?";
  LOG(INFO) << "dd..( "<<cmd<<" )";
  runShellCommand(cmd);
  LOG(INFO)<<"dd done.";
  cmd = "parted " + diskpath + "  mklabel gpt &>/dev/null 2>/dev/null";
  LOG(INFO) << "parted mklabel..( "<<cmd<<" )";
  runShellCommand(cmd);
  LOG(INFO)<<"parted mklabel done.";
  //support multiple partitions
  int start = 1;
  int end = 100/partitionCount;
  for (int i = 0; i < partitionCount; i++) {
    string suffix = "";
    if (0 != i) {
      suffix = "%";
    }
    cmd = "parted "+diskpath+" mkpart " + partitionLabel + " " +
        lexical_cast<string>(start) + suffix + " " + lexical_cast<string>(end) +
        "%" + " &>/dev/null 2>/dev/null";
    start = end;
    end = start + 100/partitionCount;
    LOG(INFO) << "parted mkpart..( "<<cmd<<" )";
    runShellCommand(cmd);
  }
}

void DiskExecutor::mkfsDisk(string diskname, string type, string flags)
{
  string cmd = "mkfs." + type + flags + " " + diskname;
  LOG(INFO) << "mkfs..( "<<cmd<<" )";
  runShellCommand(cmd);
}

bool DiskExecutor::checkSuccess(string diskname, int partitionCount, string partitionLabel)
{
  LOG(INFO) << "Check parition and fs type count..";
  //check actual partition count to see if success
  string cmd = "parted /dev/" + diskname + " p |grep " +
      partitionLabel + "|wc -l";
  int pCount = lexical_cast<int>(runShellCommand(cmd));
  //check actual filesystem count to see if success
  cmd = "parted /dev/" + diskname + " p |grep " +
      fsType + "|wc -l";
  int fsTypeCount = lexical_cast<int>(runShellCommand(cmd));
  if (partitionCount == pCount && partitionCount == fsTypeCount) {
    LOG(INFO) << "Success";
    return true;
  }

  LOG(INFO) << "Failed!";
  return false;
}

//TODO: use json string here
// return failed disks here
// data format for scheduler to parse:
// MessageToScheduler::DISK_READY.hostname:disk1,disk2
string DiskExecutor::prepareReturnValue()
{
  string msg = lexical_cast<string>(static_cast<int>(MessageToScheduler::DISK_READY));
  string tmp = "";
  for (size_t i = 0; i < failedOSDDevs.size(); i++) {
    tmp = tmp + failedOSDDevs[i] + ",";
  }
  for (size_t i = 0; i< failedJNLDevs.size(); i++) {
    if (i == (failedJNLDevs.size() - 1) ){
      tmp = tmp + failedJNLDevs[i] + ",";
    } else {
      tmp = tmp + failedJNLDevs[i];
    }
  }
  msg = msg + "." + myHostname + ":" + (""==tmp?"-":tmp);
  return msg;
}

bool DiskExecutor::addPendingDisks(TaskInfo task)
{
  if (task.has_labels()) {
    Labels labels = task.labels();
    int count = labels.labels_size();
    LOG(INFO) << "labels count: " <<count;
    if (0 == count) {
      LOG(ERROR) << "Zero host disk data given";
      return false;
    }
    for (int i = 0; i < count; i++) {
      Label* one = labels.mutable_labels(i);
      if (0 == strcmp(CustomData::osddevsKey,one->key().c_str())) {
        LOG(INFO) << "got osd dev: " <<one->value();
        pendingOSDDevs.push_back(one->value());
      }
      if (0 == strcmp(CustomData::jnldevsKey,one->key().c_str())) {
        LOG(INFO) << "got jnl dev: " <<one->value();
        pendingJNLDevs.push_back(one->value());
      }
      if (0 == strcmp(CustomData::jnlPartitionCountKey,one->key().c_str())) {
        LOG(INFO) << "got jnl partition count: " <<one->value();
        jnlPartitionCount = lexical_cast<int>(one->value());
      }
    }
    return true;
  } else {
    LOG(ERROR)<<"No labels found in taskinfo!";
    return false;
  }
}

int main(int argc, const char* argv[])
{
  DiskExecutor executor;
  MesosExecutorDriver driver(&executor);
  return driver.run() == DRIVER_STOPPED ? 0 : 1;
}
