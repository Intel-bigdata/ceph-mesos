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

#include "StateMachine.hpp"

#include <glog/logging.h>
#include <boost/foreach.hpp>

#include "Status.hpp"

StateMachine::StateMachine()
{
  //TODO : change this to RECONCILING_TASKS after we can put the taskMap to zookeeper
  currentPhase = Phase::BOOTSTRAP_MON;
}

StateMachine::StateMachine(Config* _config) : config(_config)
{
  //TODO : change this to RECONCILING_TASKS after we can put the taskMap to zookeeper
  currentPhase = Phase::BOOTSTRAP_MON;
  defaultHostConfig = HostConfig(_config);
}

StateMachine::~StateMachine(){}

void StateMachine::addStagingTask(
      string taskId,
      string executorId,
      TaskType taskType,
      string hostname,
      string slaveId)
{
  Status currentStatus = Status::STAGING;
  switch (taskType) {
    case TaskType::MON:
      monStagingNum++;
      break;
    case TaskType::OSD:
      osdStagingNum++;
      break;
    case TaskType::RADOSGW:
      radosgwStagingNum++;
      break;
  }
  TaskState taskState(
        taskId,
        executorId,
        taskType,
        hostname,
        slaveId,
        currentStatus);
  taskMap[taskId] = taskState;
}
//TODO: Optimize this. How about this? define a inner clss Counter and
//use a hashmap<TaskType,Counters> to unify the counters update
void StateMachine::updateCounters(
    TaskType taskType,
    Status oldStatus,
    Status newStatus)
{
  if ((oldStatus == Status::STAGING || oldStatus == Status::STARTING)
       && newStatus == Status::RUNNING) {
    switch (taskType) {
      case TaskType::MON:
        monRunningNum++;
        monStagingNum--;
        break;
      case TaskType::OSD:
        osdRunningNum++;
        osdStagingNum--;
        break;
      case TaskType::RADOSGW:
        radosgwRunningNum++;
        radosgwStagingNum--;
      break;
    }
  }

  if (oldStatus == Status::STAGING && newStatus == Status::FAILED) {
    switch (taskType) {
      case TaskType::MON:
        monStagingNum--;
        break;
      case TaskType::OSD:
        osdStagingNum--;
        break;
      case TaskType::RADOSGW:
        radosgwStagingNum--;
      break;
    }
  }
  if (oldStatus == Status::RUNNING && newStatus == Status::FAILED) {
    switch (taskType) {
      case TaskType::MON:
        monRunningNum--;
        break;
      case TaskType::OSD:
        osdRunningNum--;
        break;
      case TaskType::RADOSGW:
        radosgwRunningNum--;
      break;
    }
  }
  printCounters();
}

void StateMachine::printCounters()
{
  LOG(INFO) <<"Current Counters:";
  LOG(INFO) <<"monRunningNum: " <<monRunningNum;
  LOG(INFO) <<"monStagingNum: " <<monStagingNum;
  LOG(INFO) <<"osdRunningNum: " <<osdRunningNum;
  LOG(INFO) <<"osdStagingNum: " <<osdStagingNum;
  LOG(INFO) <<"radosgwRunningNum: " <<radosgwRunningNum;
  LOG(INFO) <<"radosgwStagingNum: " <<radosgwStagingNum;


}

void StateMachine::moveState()
{
  if (monRunningNum < 1) {
    currentPhase = Phase::BOOTSTRAP_MON;
  } else if (osdRunningNum < 3) {
    currentPhase = Phase::BOOTSTRAP_OSD;
  } else if (radosgwRunningNum < 1) {
    currentPhase = Phase::BOOTSTRAP_RADOSGW;
  } else {
    currentPhase = Phase::WAINTING_REQUEST;
  }
  LOG(INFO) << "After moveState: "
      << static_cast<int>(currentPhase);
}

void StateMachine::decreaseOSDIndex()
{
  osdIndex--;
}

//TODO: how about this ? merge the three updateTaskTo<*> functions
void StateMachine::updateTaskToRunning(string taskId)
{
  auto it = taskMap.find(taskId);
  if (it != taskMap.end()) {
    LOG(INFO) << "Update the taskState to RUNNING for " << taskId;
    TaskState& taskState = taskMap[taskId];
    updateCounters(taskState.taskType,
        taskState.currentStatus,
        Status::RUNNING);
    taskState.currentStatus = Status::RUNNING;
    LOG(INFO) << "After update: ";
    LOG(INFO) << taskState.toString();
    moveState();
  } else {
    LOG(ERROR) << "No value found in taskMap with key: " << taskId;
  }

}

void StateMachine::updateTaskToWaitingOSDID(string taskId)
{
  auto it = taskMap.find(taskId);
  if (it != taskMap.end()) {
    LOG(INFO) << "Update the taskState to WAITING_OSDID for " << taskId;
    TaskState& taskState = taskMap[taskId];
    updateCounters(taskState.taskType,
        taskState.currentStatus,
        Status::WAITING_OSDID);
    taskState.currentStatus = Status::WAITING_OSDID;
    LOG(INFO) << "After update: ";
    LOG(INFO) << taskState.toString();
  } else {
    LOG(ERROR) << "No value found in taskMap with key: " << taskId;
  }

}

void StateMachine::updateTaskToStarting(string taskId)
{
  auto it = taskMap.find(taskId);
  if (it != taskMap.end()) {
    LOG(INFO) << "Update the taskState to STARTING for " << taskId;
    TaskState& taskState = taskMap[taskId];
    updateCounters(taskState.taskType,
        taskState.currentStatus,
        Status::STARTING);
    taskState.currentStatus = Status::STARTING;
    LOG(INFO) << "After update: ";
    LOG(INFO) << taskState.toString();
  } else {
    LOG(ERROR) << "No value found in taskMap with key: " << taskId;
  }
}

void StateMachine::updateTaskToFailed(string taskId)
{
  auto it = taskMap.find(taskId);
  if (it != taskMap.end()) {
    LOG(INFO) << "Update the taskState to Failed for " << taskId;
    TaskState& taskState = taskMap[taskId];
    updateCounters(taskState.taskType,
        taskState.currentStatus,
        Status::FAILED);
    taskState.currentStatus = Status::FAILED;
    LOG(INFO) << "After update: ";
    LOG(INFO) << taskState.toString();
    moveState();
  } else {
    LOG(ERROR) << "No value found in taskMap with key: " << taskId;
  }

}

bool StateMachine::nextMove(TaskType& taskType, int& token, string hostName)
{
  Phase currentPhase = getCurrentPhase();
  HostConfig* hostconfig = getConfig(hostName);
  //TODO: 
  //put the check here will reduce efficiency since a host can launch Disk task and 
  //all tasks expecpt OSD task in one launchTask(). But if we put this check only in OSD phase,
  //then we need to add the disk TaskInfo together with other tasks in one
  //vector<TaskInfo> to use the same offer. Or Mesos will report error that
  //this offer is invalid since other task have used it.
  //will implement this later.
  if (hostconfig->isPreparingDisk()) {
    LOG(INFO) << hostName << " is preparing disks, need to decline this offer";
    return false;
  }
  switch (currentPhase) {
    case Phase::RECONCILING_TASKS:
    case Phase::BOOTSTRAP_MON:
      taskType = TaskType::MON;
      if (monStagingNum < 1 && (monStagingNum + monRunningNum) < 1) {
        token = monIndex++;
        return true;
      } else {
        return false;
      }
      break;
    case Phase::BOOTSTRAP_OSD:
      taskType = TaskType::OSD;
      if (osdStagingNum < 3 && (osdStagingNum + osdRunningNum) < 3) {
        token = osdIndex++;
        return true;
      } else {
        return false;
      }
      break;
    case Phase::BOOTSTRAP_RADOSGW:
      taskType = TaskType::RADOSGW;
      if (radosgwStagingNum < 1) {
        token = radosgwIndex++;
        return true;
      } else {
        return false;
      }
      break;
    case Phase::WAINTING_REQUEST:
      taskType = TaskType::OSD;
      token = osdIndex++;
      return true;
      break;
  }
  return false;
}

void StateMachine::addConfig(string hostname)
{
  auto it = hostConfigMap.find(hostname);
  if (it != hostConfigMap.end()) {
    HostConfig hc = hostConfigMap[hostname]; 
    hc.reload();
  } else {
    HostConfig hc(hostname);
    LOG(INFO)<<"add this host config:";
    LOG(INFO)<<hc.toString();
    hostConfigMap[hostname] = hc;
  }
}

HostConfig* StateMachine::getConfig(string hostname)
{
  auto it = hostConfigMap.find(hostname);
  if (it != hostConfigMap.end()) {
    HostConfig* hc = &hostConfigMap[hostname];
    return hc; 
  } else {
    return &defaultHostConfig;
  }
}

TaskState StateMachine::getInitialMon()
{
  TaskState taskState;
  string prefix = "mon0";
  //TODO: use BOOST_FOREACH to optimize this
  for ( auto it = taskMap.begin(); it != taskMap.end(); it++) {
    taskState = static_cast<TaskState>(it->second);
    string taskId = taskState.taskId;
    if ( taskId.substr(0, prefix.size()) == prefix) {
      return taskState;
    }
  }
  return taskState;
}

vector<TaskState> StateMachine::getWaitingOSDTask()
{
  vector<TaskState> vc;
  TaskState taskState;
  for ( auto it = taskMap.begin(); it != taskMap.end(); it++) {
    taskState = static_cast<TaskState>(it->second);
    if (taskState.taskType == TaskType::OSD
        && taskState.currentStatus == Status::WAITING_OSDID){
      vc.push_back(taskState);
    }
  }
  return vc;
}

void StateMachine::addPendingOSDID(int osdID)
{
  pendingOSDID.push_back(osdID);
}

vector<int>* StateMachine::getPendingOSDID()
{
  return &pendingOSDID;
}

Phase StateMachine::getCurrentPhase()
{
  return currentPhase;
}
