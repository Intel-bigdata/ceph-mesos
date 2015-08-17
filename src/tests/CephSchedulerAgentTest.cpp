#include "scheduler/CephScheduler.hpp"
#include "scheduler/CephSchedulerAgent.hpp"
#include "MockMesosSchedulerDriver.hpp"
#include "common/Config.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace mesos;
using std::cout;
using ::testing::Return;
using ::testing::_;

class SchedulerAgentTest : public testing::Test
{
  public:
    virtual void SetUp()
    {
      google::InitGoogleLogging("");
      vector<string> v;
      Config _config = {
        "ceph",//id
        "*",//role
        "cmsnmh:2181/mesos",//mesos master
        "",//zookeeper
        8889,//rest server port
        8888,//file server port
        "./",//file server root dir
        "ens2f0",//mgmt dev name
        "",//data dev name
        v,//osd devs
        v//journal devs
      };
      Config* config = new Config(_config);
      agent = new CephSchedulerAgent<MockMesosSchedulerDriver>(config);
    }

    Offer createOffer(string hostname, string role, double cpu, double mem)
    {
      Offer offer; 
      offer.set_hostname(hostname);
      Resource* res = offer.add_resources();
      //cpus
      res->set_name("cpus");
      res->set_type(Value::SCALAR);
      res->set_role(role);
      Value::Scalar* scalar = res->mutable_scalar();
      scalar->set_value(cpu);
      //mem
      res = offer.add_resources();
      res->set_name("mem");
      res->set_type(Value::SCALAR);
      res->set_role(role);
      scalar = res->mutable_scalar();
      scalar->set_value(mem); 
      return offer;
    }

    Config* config;
    MockMesosSchedulerDriver driver;
    CephSchedulerAgent<MockMesosSchedulerDriver>* agent;
};

TEST_F(SchedulerAgentTest, decline_offer_from_wrong_role) {
  EXPECT_CALL(driver, declineOffer(_,_)).Times(1);
  Offer one = createOffer("cmsnmh","ceph",8,4096);
  vector<Offer> v;
  v.push_back(one);
  agent->resourceOffers(&driver,v);
}
