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

#ifndef _CEPHMESOS_SCHEDULER_CEPHSCHEDULERAGENT_HPP_
#define _CEPHMESOS_SCHEDULER_CEPHSCHEDULERAGENT_HPP_

#include <mesos/scheduler.hpp>
#include <mesos/resources.hpp>
#include <mesos/type_utils.hpp>
#include <time.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "EventLoop.hpp"
#include "common/Config.hpp"
#include "common/TaskType.hpp"
#include "common/FrameworkMessage.hpp"
#include "common/StringUtil.hpp"
#include "common/NetUtil.hpp"
#include "state/Phase.hpp"
#include "state/StateMachine.hpp"
#include "state/TaskState.hpp"

using boost::lexical_cast;
using ceph::Phase;
using ceph::TaskType;
using std::string;
using std::vector;
using namespace mesos;

template <class T>
class CephSchedulerAgent
{
public:
  CephSchedulerAgent(Config* _config);

  virtual ~CephSchedulerAgent();

  void registered(
      T* driver,
      const FrameworkID& frameworkId,
      const MasterInfo& masterInfo);

  void reregistered(T* driver, const MasterInfo& masterInfo);

  void disconnected(T* driver);

  void resourceOffers(T* driver, const vector<Offer>& offers);

  void offerRescinded(T* driver, const OfferID& offerId);

  void statusUpdate(T* driver, const TaskStatus& status);

  void frameworkMessage(
      T* driver,
      const ExecutorID& executorId,
      const SlaveID& slaveId,
      const string& data);

  void slaveLost(T* driver, const SlaveID& slaveId);

  void executorLost(
      T* driver,
      const ExecutorID& executorId,
      const SlaveID& slaveId,
      int status);

  void error(T* driver, const std::string& message);

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

  void handleWaitingOSDTasks(T* driver);

  bool fetchPendingRESTfulRequest();

  bool hasRole(const Offer& offer, string role);

  void launchNode(
      T* driver,
      const Offer& offer,
      TaskType taskType,
      int token,
      int isInitialMonNode,
      string& taskId,
      string& executorId);

  void tryLaunchDiskTask(
      T* driver,
      const Offer& offer,
      string hostname);

  Config* config;
  StateMachine* stateMachine;
  EventLoop* eventLoop;
};

template <class T>
void CephSchedulerAgent<T>::reregistered(
      T* driver,
      const MasterInfo& masterInfo)
{
}

template <class T>
void CephSchedulerAgent<T>::disconnected(T* driver){}

template <class T>
void CephSchedulerAgent<T>::offerRescinded(
      T* driver,
      const OfferID& offerId)
{
}

template <class T>
void CephSchedulerAgent<T>::slaveLost(
      T* driver,
      const SlaveID& slaveId)
{
}

template <class T>
void CephSchedulerAgent<T>::executorLost(
      T* driver,
      const ExecutorID& executorId,
      const SlaveID& slaveId,
      int status)
{
}

template <class T>
void CephSchedulerAgent<T>::error(
      T* driver,
      const std::string& message)
{
}

template <class T>
CephSchedulerAgent<T>::CephSchedulerAgent(Config* _config)
{
  config = _config;
  stateMachine = new StateMachine(_config);
  eventLoop = new EventLoop();
}


template <class T>
CephSchedulerAgent<T>::~CephSchedulerAgent()
{
  delete stateMachine;
  delete eventLoop;
  stateMachine = NULL;
  eventLoop = NULL;
}

template <class T>
void CephSchedulerAgent<T>::registered(
      T* driver,
      const FrameworkID& frameworkId,
      const MasterInfo& masterInfo)
{
  LOG(INFO) << "Registered! FrameworkID=" << frameworkId.value();
}

template <class T>
void CephSchedulerAgent<T>::statusUpdate(
      T* driver,
      const TaskStatus& status)
{
  LOG(INFO) << "Got status update from " << status.source();
  string taskId = status.task_id().value();
  if (status.state() == TASK_RUNNING) {
    LOG(INFO) << taskId << " is Running!";
    stateMachine->updateTaskToRunning(taskId);
    if (status.has_message()){
      vector<string> tokens = StringUtil::explode(status.message(), '.');
      if ((MessageToScheduler)lexical_cast<int>(tokens[0])
          == MessageToScheduler::CONSUMED_OSD_ID
          ){
        string consumedOSDId = tokens[1];
        LOG(INFO) << "Got message of \"consumed_OSD_ID\": "<<consumedOSDId;

      }
    }
  } else if (status.state() == TASK_STARTING) {
    LOG(INFO) << taskId << " is Waiting OSDID, ready for assign osd id!";
    stateMachine->updateTaskToWaitingOSDID(taskId);
  } else if (status.state() == TASK_FAILED) {
    LOG(INFO) << taskId << " failed";
    stateMachine->updateTaskToFailed(taskId);
    //TODO: if has message , add the OSD ID back to StateMachine
  } else if (status.state() == TASK_FINISHED) {
    //only disk executor will have this finished status
    if (status.has_message()){
      vector<string> tokens = StringUtil::explode(status.message(), '.');
      if ((MessageToScheduler)lexical_cast<int>(tokens[0])
          == MessageToScheduler::DISK_READY
          ){
        string failedDevsStr = tokens[1];
        LOG(INFO) << "Got message of \"DISK_READY\": "<<failedDevsStr;
        vector<string> failedDevs = StringUtil::explode(failedDevsStr, ':');
        string hostname = failedDevs[0];
        vector<string> devs; 
        if ("-" != failedDevs[1]) {
          vector<string> devs = StringUtil::explode(failedDevs[1], ',');
        }
        HostConfig* hostconfig = stateMachine->getConfig(hostname);
        //TODO: get this "4" from yml config
        hostconfig->updateDiskPartition(devs,lexical_cast<int>("4"));
        hostconfig->setDiskPreparationDone();
      }
    }
  }
}

template <class T>
void CephSchedulerAgent<T>::frameworkMessage(
      T* driver,
      const ExecutorID& executorId,
      const SlaveID& slaveId,
      const string& data)
{
  ceph::TaskState initialMon = stateMachine->getInitialMon();
  if (initialMon.executorId == executorId.value()) {
    LOG(INFO) << "Got osd id from inital MON: " << data;
    stateMachine->addPendingOSDID(lexical_cast<int>(data));
  }
}

template <class T>
void CephSchedulerAgent<T>::resourceOffers(
      T* driver,
      const vector<Offer>& offers)
{
  LOG(INFO) << "Received " << offers.size() << " offers! ";
  TaskType taskType;
  int token;
  int isInitialMonNode = 0;
  //handle waiting OSD task, give them osdID to start docker
  handleWaitingOSDTasks(driver);
  Phase currentPhase = stateMachine->getCurrentPhase();
  //try start new node
  foreach (const Offer& offer, offers) {
    //check offer with the correct role
    LOG(INFO) << "Hostname: " << offer.hostname();
    if (!hasRole(offer, config->role)) {
      LOG(INFO) << "Decline this offer. Host " << offer.hostname() << " don't have correct role:"
          << config->role;
      Filters refuse;
      refuse.set_refuse_seconds(86400.0);
      driver->declineOffer(offer.id(),refuse);
      continue;
    }
    //reload or new hostconfig
    stateMachine->addConfig(offer.hostname());
    tryLaunchDiskTask(driver, offer, offer.hostname());
    bool accept = stateMachine->nextMove(taskType,token,offer.hostname());
    if (!accept) {
      LOG(INFO) << "In the "
          << static_cast<int>(currentPhase)
          << " Staging Phase, cannot accept offer from "
          << offer.hostname()
          << " in this phase";
      driver->declineOffer(offer.id());
      continue;
    }
    LOG(INFO) << "Check offer's resources from " <<offer.hostname();
    if (offerNotEnoughResources(offer,taskType)) {
      LOG(INFO) << "Not enough, decline it from " << offer.hostname();
      driver->declineOffer(offer.id());
      continue;
    }
    if (currentPhase == Phase::WAINTING_REQUEST){
      accept = fetchPendingRESTfulRequest();
      if (!accept){
        LOG(INFO) << "No pending OSD RESTful request.";
        driver->declineOffer(offer.id());
        stateMachine->decreaseOSDIndex();
        continue;
      }
    }

    LOG(INFO) << "Accepted offer from" << offer.hostname() << ", launch "
        << static_cast<int>(taskType) <<":" << token << " node";
    if (taskType == TaskType::MON && token == 0) {
        LOG(INFO) << "This is the initial MON";
        isInitialMonNode = 1;
    }
    string taskId;
    string executorId;
    launchNode(
        driver,
        offer,
        taskType,
        token,
        isInitialMonNode,
        taskId,
        executorId);
    stateMachine->addStagingTask(
        taskId,
        executorId,
        taskType,
        offer.hostname(),
        offer.slave_id().value());
    if (!isInitialMonNode && taskType == TaskType::OSD) {
      ceph::TaskState initialMon = stateMachine->getInitialMon();
      const string m = lexical_cast<string>(static_cast<int>(MessageToExecutor::REGISTER_OSD));
      ExecutorID eId;
      eId.set_value(initialMon.executorId);
      SlaveID sId;
      sId.set_value(initialMon.slaveId);
      driver->sendFrameworkMessage(
          eId,
          sId,
          m);
    }//end if

  }//end foreach
}

template <class T>
string CephSchedulerAgent<T>::getFileServerIP()
{
  return NetUtil::getLocalIP();
}

// token is used to distinguish same executorName with same taskType
// should be maintained by StateMachine
// TODO: add resources limit as a param
template <class T>
string CephSchedulerAgent<T>::createExecutor(
      string executorName,
      TaskType taskType,
      int token,
      int isInitialMonNode,
      ExecutorInfo& executor)
{
  string name = executorName + "." +
      lexical_cast<string>(static_cast<int>(taskType)) + "." +
      lexical_cast<string>(token);
  time_t timestamp;
  time(&timestamp);
  string id = name + "." + lexical_cast<string>(timestamp);
  string binary_uri = "http://"+ getFileServerIP() + ":" +
      lexical_cast<string>(config->fileport) +
      "/" + executorName;
  executor.mutable_executor_id()->set_value(id);
  CommandInfo::URI* uri = executor.mutable_command()->add_uris();
  uri->set_value(binary_uri);
  uri->set_executable(true);
  //give "wfn.sh" to executor for waiting container NIC ready
  string wfn_uri = "http://"+ getFileServerIP() + ":" +
      lexical_cast<string>(config->fileport) +
      "/wfn.sh";
  uri = executor.mutable_command()->add_uris();
  uri->set_value(wfn_uri);
  uri->set_executable(true);
  //config files:
  //ceph.client.admin.keyring
  //ceph.conf
  //ceph.mon.keyring
  //monmap
  if (!isInitialMonNode) {
    ceph::TaskState initialMon = stateMachine->getInitialMon();
    string prefix = "http://" + initialMon.hostName + ":7777/";
    string configUriClientKeyring = prefix + "ceph.client.admin.keyring";
    string configUriCephConf = prefix + "ceph.conf";
    string configUriMonKeyring = prefix + "ceph.mon.keyring";
    string configMonMap = prefix + "monmap";
    uri = executor.mutable_command()->add_uris();
    uri->set_value(configUriClientKeyring);
    uri = executor.mutable_command()->add_uris();
    uri->set_value(configUriCephConf);
    uri = executor.mutable_command()->add_uris();
    uri->set_value(configUriMonKeyring);
    uri = executor.mutable_command()->add_uris();
    uri->set_value(configMonMap);
  }
  executor.mutable_command()->set_value(
      "./" + executorName);

  executor.set_name(name);
  return id;
}

template <class T>
void CephSchedulerAgent<T>::addHostConfig(TaskInfo &taskinfo, TaskType taskType, string hostname)
{
  HostConfig* hostconfig = stateMachine->getConfig(hostname);
  Labels labels;
  //mgmt NIC name
  Label* one_label = labels.add_labels();
  one_label->set_key(CustomData::mgmtdevKey);
  one_label->set_value(hostconfig->getMgmtDev());
  //data NIC name
  one_label = labels.add_labels();
  one_label->set_key(CustomData::datadevKey);
  one_label->set_value(hostconfig->getDataDev());
  if (TaskType::OSD == taskType) {
    // osd disk
    one_label = labels.add_labels();
    string osddisk = hostconfig->popOSDPartition();
    one_label->set_key(CustomData::osddevsKey);
    one_label->set_value(osddisk);
    // journal disk
    one_label = labels.add_labels();
    string journaldisk = hostconfig->popJournalPartition();
    one_label->set_key(CustomData::jnldevsKey);
    one_label->set_value(journaldisk);
  }
  taskinfo.mutable_labels()->MergeFrom(labels);
}

template <class T>
string CephSchedulerAgent<T>::createTaskName(TaskType taskType, int token)
{
  string prefix;
  switch (taskType) {
    case TaskType::MON:
      prefix = "mon";
      break;
    case TaskType::OSD:
      prefix = "osd";
      break;
    case TaskType::RADOSGW:
      prefix = "radosgw";
      break;
  }
  return prefix + lexical_cast<string>(token);
}
//assign one OSD ID to one OSD task
//TODO: we can give one OSD tasks multiple OSDID to supprot
//runnig multiple OSD in one container. But if so, current task id and name
//osd<id>.<hostname>.<slaveid> maybe confusing. Need to change too
template <class T>
void CephSchedulerAgent<T>::handleWaitingOSDTasks(T* driver)
{
  vector<int>* OSDIDs = stateMachine->getPendingOSDID();
  vector<ceph::TaskState> OSDTasks = stateMachine->getWaitingOSDTask();
  ExecutorID eId;
  SlaveID sId;
  string m;
  ceph::TaskState taskState;
  for(size_t i = 0; i < OSDTasks.size(); i++) {
    taskState = OSDTasks[i];
    if (!OSDIDs->empty()){
      int Id = OSDIDs->back();
      //OSD executor will send status with message containing OSD
      OSDIDs->pop_back();
      m = lexical_cast<string>(static_cast<int>(MessageToExecutor::LAUNCH_OSD)) +
          "." + lexical_cast<string>(Id);
      eId.set_value(taskState.executorId);
      sId.set_value(taskState.slaveId);
      driver->sendFrameworkMessage(
          eId,
          sId,
          m);
      LOG(INFO) << "Assign: OSD ID " << Id << " to this task: ";
      LOG(INFO) << taskState.toString();
      //if set the task to Starting, scheduler will know 
      //it has assigned OSDID to it
      stateMachine->updateTaskToStarting(taskState.taskId);
    } else {
      LOG(INFO) << "No OSD ID available!";
      break;
    }
  }//end foreach

}

//currently only support OSD REST request
template <class T>
bool CephSchedulerAgent<T>::fetchPendingRESTfulRequest()
{
  //fetch a request gets from REST server
  LOG(INFO) << "FetchPending requests..";
  vector<int>* OSDRequests = eventLoop->getPendingOSD();
  if (OSDRequests->empty()){
    return false;
  } else {
    LOG(INFO) << "Total received " << OSDRequests->size() << " requests!";
    int OSDInstanceCount = (*OSDRequests)[0];
    OSDInstanceCount--;
    if (0 == OSDInstanceCount){
      LOG(INFO) << "InstanceCount is 0, remove this request";
      OSDRequests->erase(OSDRequests->begin());
    } else {
      (*OSDRequests)[0] = OSDInstanceCount;
    }
    LOG(INFO) << "Consumed 1 OSD instance of first request,"
        << "remaining " << OSDInstanceCount << " instance(s)";
    return true;
  }
}

template <class T>
bool CephSchedulerAgent<T>::hasRole(const Offer& offer, string role)
{
  Resources res = offer.resources();
  foreach (Resource resource, res) {
    if (role == resource.role()) {
      return true;
    }
  }
  return false;
}

template <class T>
void CephSchedulerAgent<T>::launchNode(
      T* driver,
      const Offer& offer,
      TaskType taskType,
      int token,
      int isInitialMonNode,
      string& taskId,
      string& executorId)
{

  vector<TaskInfo> tasks;

  ExecutorInfo executor;
  executorId = createExecutor(
      "ceph-mesos-executor",
      taskType,
      token,
      isInitialMonNode,
      executor);

  TaskInfo task;
  //set host info to TaskInfo
  addHostConfig(task, taskType, offer.hostname());
  task.set_name(createTaskName(taskType,token));
  //set this to indicate executor whether to download shared config
  task.set_data(
      lexical_cast<string>(isInitialMonNode) +
      "." +
      lexical_cast<string>(static_cast<int>(taskType)));
  taskId = task.name() + "."
      + offer.hostname() + "."
      + offer.slave_id().value();
  task.mutable_task_id()->set_value(taskId);
  task.mutable_slave_id()->MergeFrom(offer.slave_id());

  task.mutable_executor()->MergeFrom(executor);

  Resource* resource;
  resource = task.add_resources();
  resource->set_name("cpus");
  resource->set_type(Value::SCALAR);
  resource->set_role(config->role);
  //TODO: use resource limit in config, like "mesos.ceph.mon.cpus"
  resource->mutable_scalar()->set_value(1);

  resource = task.add_resources();
  resource->set_name("mem");
  resource->set_type(Value::SCALAR);
  resource->set_role(config->role);
  //TODO: use resource limit in config, like "mesos.ceph.mon.mem"
  resource->mutable_scalar()->set_value(1024);

  tasks.push_back(task);

  driver->launchTasks(offer.id(), tasks);
}

template <class T>
void CephSchedulerAgent<T>::tryLaunchDiskTask(
    T* driver,
    const Offer& offer,
    string hostname)
{
  HostConfig* hostconfig = stateMachine->getConfig(hostname);
  if (hostconfig->isPreparingDisk()) {
    LOG(INFO) << "Disk executor already running on " << hostname;
    return;
  }
  if (!hostconfig->havePendingOSDDevs() &&
      !hostconfig->havePendingJNLDevs()) {
    LOG(INFO) << "No pending raw disks";
    return;
  }
  //launch disk executor to partition the osd and journal disk
  vector<TaskInfo> tasks;
  TaskInfo task;
  if (offerNotEnoughResources(offer, TaskType::OSD)) {
    LOG(INFO) << "No enough resource to launch disk executor";
    return;
  }
  ExecutorInfo executor;
  //executor id
  string binary = "ceph-mesos-disk-executor";
  time_t timestamp;
  time(&timestamp);
  string id = binary + "." + lexical_cast<string>(timestamp);
  executor.mutable_executor_id()->set_value(id);
  //binary uri
  CommandInfo::URI* uri = executor.mutable_command()->add_uris();
  string binary_uri = "http://"+ getFileServerIP() + ":" +
    lexical_cast<string>(config->fileport) +
    "/" + binary;
  uri->set_value(binary_uri);
  uri->set_executable(true);
  //command of run disk executor
  executor.mutable_command()->set_value( 
      "./" + binary);
  executor.set_name(id);

  task.set_name(id);
  string taskId = task.name() + "."
      + offer.hostname() + "."
      + offer.slave_id().value();
  task.mutable_task_id()->set_value(taskId);
  task.mutable_slave_id()->MergeFrom(offer.slave_id());

  task.mutable_executor()->MergeFrom(executor);

  //lables indicate the devs to parition
  Labels labels;
  //osd devs
  vector<string>* pendingOSDDevs = hostconfig->getPendingOSDDevs();
  for (size_t i = 0; i < pendingOSDDevs->size(); i++) {
    Label* one_label = labels.add_labels();
    one_label->set_key(CustomData::osddevsKey);
    one_label->set_value((*pendingOSDDevs)[i]);
  }
  //jnl devs
  vector<string>* pendingJNLDevs = hostconfig->getPendingJNLDevs();
  for (size_t i = 0; i < pendingJNLDevs->size(); i++) {
    Label* one_label = labels.add_labels();
    one_label->set_key(CustomData::jnldevsKey);
    one_label->set_value((*pendingJNLDevs)[i]);
  }
  // jnl partition count
  //TODO: get this jnl partition count from yml file
  Label* one_label = labels.add_labels();
  one_label->set_key(CustomData::jnlPartitionCountKey);
  one_label->set_value("4");
  task.mutable_labels()->MergeFrom(labels);
  //reousrces
  Resource* resource;
  resource = task.add_resources();
  resource->set_name("cpus");
  resource->set_type(Value::SCALAR);
  resource->set_role(config->role);
  resource->mutable_scalar()->set_value(1);
  resource = task.add_resources();
  resource->set_name("mem");
  resource->set_type(Value::SCALAR);
  resource->set_role(config->role);
  resource->mutable_scalar()->set_value(256);

  tasks.push_back(task);

  driver->launchTasks(offer.id(), tasks);
  LOG(INFO) << "launch Disk Executor on " <<offer.hostname();
  //set state
  hostconfig->setDiskPreparationOnGoing();
}

template <class T>
bool CephSchedulerAgent<T>::offerNotEnoughResources(const Offer& offer, TaskType taskType)
{
  //TODO: check cpus and mems based on taskType
  int cpus = 1;
  int mems = 1024;
  static const Resources neededResources = Resources::parse(
      "cpus:" + stringify(cpus) +
      ";mem:" + stringify(mems),config->role).get();
  Resources res = offer.resources();
  if (res.flatten(config->role).contains(neededResources)) {
    return false;
  } else {
    return true;
  }
}

#endif
