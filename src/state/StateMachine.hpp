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

#ifndef _CEPHMESOS_STATE_STATEMACHINE_HPP_
#define _CEPHMESOS_STATE_STATEMACHINE_HPP_

#include <boost/unordered_map.hpp>
#include <vector>

#include "Phase.hpp"
#include "TaskState.hpp"
#include "common/Config.hpp"
#include "common/TaskType.hpp"

using ceph::Phase;
using ceph::TaskState;
using ceph::TaskType;
using boost::unordered_map;
using std::string;
using std::vector;

class StateMachine
{
public:
  StateMachine();

  StateMachine(Config* _config);

  ~StateMachine();

  void addStagingTask(
      string taskId,
      string executorId,
      TaskType taskType,
      string hostname,
      string slaveId);

  void updateTaskToRunning(string taskId);

  void updateTaskToWaitingOSDID(string taskId);

  void updateTaskToStarting(string taskId);

  void updateTaskToFailed(string taskId);

  void updateCounters(TaskType taskType,Status oldStatus, Status newStatus);

  void printCounters();

  void moveState();

  void decreaseOSDIndex();

  TaskState getInitialMon();

  vector<TaskState> getWaitingOSDTask();

  vector<int>* getPendingOSDID();

  void addPendingOSDID(int osdID);

  Phase getCurrentPhase();

  bool nextMove(TaskType& taskType, int& token, string hostName);

private:
  unordered_map<std::string, ceph::TaskState> taskMap;

  vector<int> pendingOSDID;

  Phase currentPhase;

  int monRunningNum = 0;
  int monStagingNum = 0;
  int monIndex = 0;

  int osdRunningNum = 0;
  int osdStagingNum = 0;
  int osdIndex = 0;

  int radosgwRunningNum = 0;
  int radosgwStagingNum = 0;
  int radosgwIndex = 0;

  Config* config;
};

#endif
