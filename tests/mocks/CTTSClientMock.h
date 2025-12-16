#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "ITTSClient.h"

class CTTSClientMock : public ITTSClient
{
public:
    MOCK_METHOD(void, speak,              (const std::string &text),      (override));
    MOCK_METHOD(bool, initialize,         (),                             (override));
    MOCK_METHOD(void, uninitialize,       (),                             (override));
    MOCK_METHOD(void, registerListener,   (StateChangeListener listener), (override));
    MOCK_METHOD(void, unregisterListener, (),                             (override));
    MOCK_METHOD(bool, isEnabled,          (),                             (override));
};