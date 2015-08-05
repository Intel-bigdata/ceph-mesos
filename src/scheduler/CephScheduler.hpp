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

#ifndef _CEPHMESOS_SCHEDULER_CEPHSCHEDULER_HPP_
#define _CEPHMESOS_SCHEDULER_CEPHSCHEDULER_HPP_

#include <mesos/scheduler.hpp>
#include <mesos/resources.hpp>
#include <mesos/type_utils.hpp>

#include "EventLoop.hpp"
#include "common/Config.hpp"
#include "common/TaskType.hpp"
#include "common/FrameworkMessage.hpp"
#include "state/StateMachine.hpp"

using ceph::TaskType;
using std::string;
using std::vector;
using namespace mesos;

class CephScheduler : public Scheduler
{
public:
  CephScheduler(Config* _config);

  virtual ~CephScheduler();

  void registered(
      SchedulerDriver* driver,
      const FrameworkID& frameworkId,
      const MasterInfo& masterInfo);

  void reregistered(SchedulerDriver* driver, const MasterInfo& masterInfo);

  void disconnected(SchedulerDriver* driver);

  void resourceOffers(SchedulerDriver* driver, const vector<Offer>& offers);

  void offerRescinded(SchedulerDriver* driver, const OfferID& offerId);

  void statusUpdate(SchedulerDriver* driver, const TaskStatus& status);

  void frameworkMessage(
      SchedulerDriver* driver,
      const ExecutorID& executorId,
      const SlaveID& slaveId,
      const string& data);

  void slaveLost(SchedulerDriver* driver, const SlaveID& slaveId);

  void executorLost(
      SchedulerDriver* driver,
      const ExecutorID& executorId,
      const SlaveID& slaveId,
      int status);

  void error(SchedulerDriver* driver, const std::string& message);

private:
  bool offerNotEnoughResources(const Offer& offer, TaskType taskType);

  string getFileServerIP();

  string createExecutor(
      string executorName,
      TaskType taskType,
      int token,
      int isInitialMonNode,
      ExecutorInfo& executor);

  void addHostConfig(TaskInfo &taskinfo, TaskType taskType, string hostname);

  string createTaskName(TaskType taskType, int token);

  void handleWaitingOSDTasks(SchedulerDriver* driver);

  bool fetchPendingRESTfulRequest();

  void launchNode(
      SchedulerDriver* driver,
      const Offer& offer,
      TaskType taskType,
      int token,
      int isInitialMonNode,
      string& taskId,
      string& executorId);

  Config* config;
  StateMachine* stateMachine;
  EventLoop* eventLoop;
};

int framework(Config* config);

#endif
