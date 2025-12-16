#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "IAtspiRegistryService.h"

class CAtspiRegistryServiceMock : public IAtspiRegistryService
{
public:
    MOCK_METHOD(void,        start,                   (),                                                 (override));
    MOCK_METHOD(void,        stop,                    (),                                                 (override));
    MOCK_METHOD(std::string, address,                 (),                                                 (override));
    MOCK_METHOD(void,        registerEventListener,   (const std::string &event, EventListener listener), (override));
    MOCK_METHOD(void,        unregisterEventListener, (const std::string &event),                         (override));
};