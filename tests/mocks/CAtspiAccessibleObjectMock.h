#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "IAtspiAccessibleObject.h"

class CAtspiAccessibleObjectMock : public IAtspiAccessibleObject
{
public:
    MOCK_METHOD(std::string,           name,            (), (override));
    MOCK_METHOD(std::string,           description,     (), (override));
    MOCK_METHOD(std::string,           roleName,        (), (override));
    MOCK_METHOD(uint32_t,              role,            (), (override));
    MOCK_METHOD(std::vector<uint32_t>, states,          (), (override));
    MOCK_METHOD(std::string,           cellDescription, (), (override));
    MOCK_METHOD(std::string,           objectPath,      (), (override));
};