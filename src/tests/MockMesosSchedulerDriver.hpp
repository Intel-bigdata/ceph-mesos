#ifndef _CEPHMESOS_TESTS_MOCKSCHEDULERDRIVER_HPP_
#define _CEPHMESOS_TESTS_MOCKSCHEDULERDRIVER_HPP

#include "gmock/gmock.h"

using namespace mesos;
using std::vector;

class MockMesosSchedulerDriver
{
  public:
    MOCK_METHOD1(declineOffer, void(const OfferID& offerId));
    MOCK_METHOD2(declineOffer, void(const OfferID& offerId,
        const Filters& filters));
    MOCK_METHOD2(launchTasks, void(const OfferID& offerId,
        const vector<TaskInfo> tasks));
    MOCK_METHOD0(abort, void());
    MOCK_METHOD1(stop, void(bool failover));
    MOCK_METHOD0(stop, void());
    MOCK_METHOD3(sendFrameworkMessage, void(
        const ExecutorID &executorId,
        const SlaveID &slaveId,
        const std::string &data));
    
};

#endif
