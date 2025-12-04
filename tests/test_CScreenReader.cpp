#include <gio/gio.h>
#include <glib.h>
#include <chrono>

#include "gtest/gtest.h"

#include "CTTSClientMock.h"
#include "CAtspiRegistryServiceMock.h"
#include "CAtspiAccessibleObjectMock.h"

#include "CScreenReader.h"

namespace CScreenReaderTests
{

using ::testing::_;
using ::testing::AtLeast;
using ::testing::AnyNumber;
using ::testing::Return;
using ::testing::InSequence;

class CScreenReaderInitializationTest : public ::testing::Test
{
protected:
    CScreenReaderInitializationTest() = default;
    virtual ~CScreenReaderInitializationTest() = default;

    virtual void SetUp()
    {
        m_screen_reader = new CScreenReader(&m_registry_service, &m_tts_client);

        ASSERT_NE(nullptr, m_screen_reader);
    }

    virtual void TearDown()
    {
        delete m_screen_reader;
    }

protected:
    CScreenReader *m_screen_reader = {nullptr};
    CTTSClientMock m_tts_client;
    CAtspiRegistryServiceMock m_registry_service;
};

TEST_F(CScreenReaderInitializationTest, init_uninit_with_tts_enabled)
{
    // We test the initialize and uninitialize calls in the same test, because in order to test
    // the uninitialize sequence, we'd' need to perform a proper initialize sequence first,
    // which would mean repeating same code in 2 different (but related) tests.
    // To have a cleaner separation, we verify midway that the initialize sequence succeeded

    {
        InSequence seq;

        EXPECT_CALL(m_tts_client, initialize())
            .Times(1)
            .WillOnce(Return(true));

        EXPECT_CALL(m_tts_client, registerListener(_))
            .Times(1);
    }

    EXPECT_CALL(m_tts_client, isEnabled())
        .Times(AnyNumber())
        .WillRepeatedly(Return(true));


    EXPECT_CALL(m_registry_service, start())
        .Times(1);

    EXPECT_CALL(m_registry_service, registerEventListener("Document:LoadComplete",_))
        .Times(1);
    EXPECT_CALL(m_registry_service, registerEventListener("Object:StateChanged",_))
        .Times(1);

    m_screen_reader->initialize();


    ASSERT_TRUE(::testing::Mock::VerifyAndClearExpectations(&m_tts_client)) << "'initialize' call FAILED";
    ASSERT_TRUE(::testing::Mock::VerifyAndClearExpectations(&m_registry_service)) << "'initialize' call FAILED";


    EXPECT_CALL(m_tts_client, isEnabled())
        .Times(AnyNumber())
        .WillRepeatedly(Return(true));

    {
        InSequence seq;
        EXPECT_CALL(m_tts_client, unregisterListener())
            .Times(1);

        EXPECT_CALL(m_tts_client, uninitialize())
            .Times(1);
    }

    EXPECT_CALL(m_registry_service, unregisterEventListener("Document:LoadComplete"))
        .Times(1);
    EXPECT_CALL(m_registry_service, unregisterEventListener("Object:StateChanged"))
        .Times(1);    

    EXPECT_CALL(m_registry_service, stop())
        .Times(1);

    m_screen_reader->uninitialize();
}

TEST_F(CScreenReaderInitializationTest, init_uninit_with_tts_disabled)
{
    // We test the initialize and uninitialize calls in the same test, because in order to test
    // the uninitialize sequence, we'd' need to perform a proper initialize sequence first,
    // which would mean repeating same code in 2 different (but related) tests.
    // To have a cleaner separation, we verify midway that the initialize sequence succeeded

    {
        InSequence seq;

        EXPECT_CALL(m_tts_client, initialize())
            .Times(1)
            .WillOnce(Return(true));

        EXPECT_CALL(m_tts_client, registerListener(_))
            .Times(1);
    }

    EXPECT_CALL(m_tts_client, isEnabled())
        .Times(AnyNumber())
        .WillRepeatedly(Return(false));


    EXPECT_CALL(m_registry_service, start())
        .Times(1);

    m_screen_reader->initialize();


    ASSERT_TRUE(::testing::Mock::VerifyAndClearExpectations(&m_tts_client)) << "'initialize' call FAILED";
    ASSERT_TRUE(::testing::Mock::VerifyAndClearExpectations(&m_registry_service)) << "'initialize' call FAILED";


    EXPECT_CALL(m_tts_client, isEnabled())
        .Times(AnyNumber())
        .WillRepeatedly(Return(false));

    EXPECT_CALL(m_registry_service, stop())
        .Times(1);

    {
        InSequence seq;

        EXPECT_CALL(m_tts_client, unregisterListener())
            .Times(1);

        EXPECT_CALL(m_tts_client, uninitialize())
            .Times(1);
    }

    m_screen_reader->uninitialize();
}

typedef struct
{
    std::string name;
    std::string description;
    std::string role_name;
    uint32_t role;
    std::vector<uint32_t> states;
    std::string cell_description;
    std::string object_path;
} TTestParamAccessible;

typedef struct
{
    std::string event;
    std::string detail;
    uint32_t data1;
    uint32_t data2;
    TTestParamAccessible accessible;
    std::string expected_speech;
} TTestParam;

class CScreenReaderBaseTest : public ::testing::TestWithParam<TTestParam> 
{
protected:
    CScreenReaderBaseTest() = default;
    virtual ~CScreenReaderBaseTest() = default;

    virtual void SetUp()
    {
        m_screen_reader = new CScreenReader(&m_registry_service, &m_tts_client);

        ASSERT_NE(nullptr, m_screen_reader);

        initialize();
    }

    virtual void TearDown()
    {
        uninitialize();

        delete m_screen_reader;
    }

    virtual void initialize() = 0;
    virtual void uninitialize() = 0;

    void setExpectedCallCount(int count)
    {
        m_pending_calls = count;
    }

    void notifySingleCall()
    {
        std::lock_guard<std::mutex> lock(m_pending_calls_mutex);
        m_pending_calls--;        
        m_pending_calls_condition.notify_one();
    }

    bool waitAllCalls(uint timeout_ms)
    {
        std::unique_lock<std::mutex> lock(m_pending_calls_mutex);
        return m_pending_calls_condition.wait_for(lock, std::chrono::milliseconds(timeout_ms), [&]{ return m_pending_calls == 0; });
    }

protected:
    CScreenReader *m_screen_reader = { nullptr };
    CTTSClientMock m_tts_client;
    CAtspiRegistryServiceMock m_registry_service;
    ITTSClient::StateChangeListener m_tts_listener;
    bool m_tts_enabled = { false };
    std::mutex m_pending_calls_mutex;
    std::condition_variable m_pending_calls_condition;
    int m_pending_calls;
};

class CScreenReaderRegistryServiceListenersTest : public CScreenReaderBaseTest 
{
protected:
    CScreenReaderRegistryServiceListenersTest()
    {        
        m_tts_enabled = false;
    }
    
    virtual ~CScreenReaderRegistryServiceListenersTest()
    {        
    }

    virtual void initialize()
    {
        EXPECT_CALL(m_tts_client, initialize())
            .Times(1)
            .WillOnce(Return(true));

        EXPECT_CALL(m_tts_client, registerListener(_))
            .Times(1)
            .WillOnce(::testing::SaveArg<0>(&m_tts_listener));

        EXPECT_CALL(m_tts_client, isEnabled())
            .Times(AnyNumber())
            .WillRepeatedly([&]() { return m_tts_enabled; });


        EXPECT_CALL(m_registry_service, start())
            .Times(1);

        m_screen_reader->initialize();

        ASSERT_TRUE(::testing::Mock::VerifyAndClearExpectations(&m_tts_client)) << "'initialize' call FAILED. Verify initialization related tests first.";
        ASSERT_TRUE(::testing::Mock::VerifyAndClearExpectations(&m_registry_service)) << "'initialize' call FAILED. Verify initialization related tests first.";
    }

    virtual void uninitialize()
    {
        EXPECT_CALL(m_registry_service, stop())
            .Times(1);

        EXPECT_CALL(m_tts_client, unregisterListener())
            .Times(1);

        EXPECT_CALL(m_tts_client, uninitialize())
            .Times(1);

        m_screen_reader->uninitialize();        

        ASSERT_TRUE(::testing::Mock::VerifyAndClearExpectations(&m_tts_client)) << "'uninitialize' call FAILED. Verify initialization related tests first.";
        ASSERT_TRUE(::testing::Mock::VerifyAndClearExpectations(&m_registry_service)) << "'uninitialize' call FAILED. Verify initialization related tests first.";
    }
};

TEST_F(CScreenReaderRegistryServiceListenersTest, tts_state_change)
{
    EXPECT_CALL(m_registry_service, registerEventListener("Document:LoadComplete",_))
        .Times(1)
        .WillOnce([&]{notifySingleCall();});
    EXPECT_CALL(m_registry_service, registerEventListener("Object:StateChanged",_))
        .Times(1)
        .WillOnce([&]{notifySingleCall();});

    setExpectedCallCount(2);

    m_tts_enabled = true;
    m_tts_listener(m_tts_enabled);

    ASSERT_TRUE(waitAllCalls(50));


    ASSERT_TRUE(::testing::Mock::VerifyAndClearExpectations(&m_tts_client)) << "TTS enable sequence FAILED";
    ASSERT_TRUE(::testing::Mock::VerifyAndClearExpectations(&m_registry_service)) << "TTS enable sequence FAILED";


    EXPECT_CALL(m_registry_service, unregisterEventListener("Document:LoadComplete"))
        .Times(1)
        .WillOnce([&]{notifySingleCall();});
    EXPECT_CALL(m_registry_service, unregisterEventListener("Object:StateChanged"))
        .Times(1)
        .WillOnce([&]{notifySingleCall();});

    setExpectedCallCount(2);

    m_tts_enabled = false;
    m_tts_listener(m_tts_enabled);

    ASSERT_TRUE(waitAllCalls(50));

    ASSERT_TRUE(::testing::Mock::VerifyAndClearExpectations(&m_tts_client)) << "TTS disable sequence FAILED";
    ASSERT_TRUE(::testing::Mock::VerifyAndClearExpectations(&m_registry_service)) << "TTS disable sequence FAILED";
}

class CScreenReaderRegistryServiceEventsTest : public CScreenReaderBaseTest 
{
protected:
    CScreenReaderRegistryServiceEventsTest()
    {        
        m_tts_enabled = true;
    }

    virtual ~CScreenReaderRegistryServiceEventsTest()
    {        
    }

    virtual void initialize()
    {
        EXPECT_CALL(m_tts_client, initialize())
            .Times(1)
            .WillOnce(Return(true));

        EXPECT_CALL(m_tts_client, registerListener(_))
            .Times(1)
            .WillOnce(::testing::SaveArg<0>(&m_tts_listener));

        EXPECT_CALL(m_tts_client, isEnabled())
            .Times(AnyNumber())
            .WillRepeatedly([&]() { return m_tts_enabled; });

        EXPECT_CALL(m_registry_service, registerEventListener("Document:LoadComplete",_))
            .Times(1)
            .WillOnce(::testing::SaveArg<1>(&m_load_complete_listener));

        EXPECT_CALL(m_registry_service, registerEventListener("Object:StateChanged",_))
            .Times(1)
            .WillOnce(::testing::SaveArg<1>(&m_state_changed_listener));

        EXPECT_CALL(m_registry_service, start())
            .Times(1);

        m_screen_reader->initialize();

        ASSERT_TRUE(::testing::Mock::VerifyAndClearExpectations(&m_tts_client)) << "'initialize' call FAILED. Verify initialization related tests first.";
        ASSERT_TRUE(::testing::Mock::VerifyAndClearExpectations(&m_registry_service)) << "'initialize' call FAILED. Verify initialization related tests first.";
    }

    virtual void uninitialize()
    {
        EXPECT_CALL(m_registry_service, stop())
            .Times(1);

        EXPECT_CALL(m_tts_client, unregisterListener())
            .Times(1);

        EXPECT_CALL(m_tts_client, uninitialize())
            .Times(1);

        EXPECT_CALL(m_registry_service, unregisterEventListener("Document:LoadComplete"))
            .Times(1);

        EXPECT_CALL(m_registry_service, unregisterEventListener("Object:StateChanged"))
            .Times(1);

        m_screen_reader->uninitialize();        

        ASSERT_TRUE(::testing::Mock::VerifyAndClearExpectations(&m_tts_client)) << "'uninitialize' call FAILED. Verify initialization related tests first.";
        ASSERT_TRUE(::testing::Mock::VerifyAndClearExpectations(&m_registry_service)) << "'uninitialize' call FAILED. Verify initialization related tests first.";
    }

protected:
    IAtspiRegistryService::EventListener m_load_complete_listener;
    IAtspiRegistryService::EventListener m_state_changed_listener;
    CAtspiAccessibleObjectMock m_accessible_object;
};

TEST_P(CScreenReaderRegistryServiceEventsTest, send_events)
{
    auto & param = GetParam();
    bool load_complete_event = (param.event == "LoadComplete");

    EXPECT_CALL(m_accessible_object, name())
        .Times(param.expected_speech.empty() ? AnyNumber() : AtLeast(1))
        .WillRepeatedly(Return(param.accessible.name));

    EXPECT_CALL(m_accessible_object, description())
        .Times(AnyNumber())
        .WillRepeatedly(Return(param.accessible.description));

    EXPECT_CALL(m_accessible_object, roleName())
        .Times(AnyNumber())
        .WillRepeatedly(Return(param.accessible.role_name));

    EXPECT_CALL(m_accessible_object, role())
        .Times(AnyNumber())
        .WillRepeatedly(Return(param.accessible.role));

    EXPECT_CALL(m_accessible_object, states())
        .Times(AnyNumber())
        .WillRepeatedly(Return(param.accessible.states));

    EXPECT_CALL(m_accessible_object, cellDescription())
        .Times(AnyNumber())
        .WillRepeatedly(Return(param.accessible.cell_description));

    EXPECT_CALL(m_accessible_object, objectPath())
        .Times(AnyNumber())
        .WillRepeatedly(Return(param.accessible.object_path));

    if (!param.expected_speech.empty())
    {
        EXPECT_CALL(m_tts_client, speak(param.expected_speech))
            .Times(1)
            .WillOnce([&]{notifySingleCall();});
        
        setExpectedCallCount(1);
    }
    else
    {
        EXPECT_CALL(m_tts_client, speak(_))
            .Times(0);
        
        // Force timeout as no better way currently to verify speak is not called in case
        // implementation is async
        setExpectedCallCount(-1);
    }

    if (load_complete_event)
    {
        m_load_complete_listener(param.event, param.detail, param.data1, param.data2, m_accessible_object);
    }
    else
    {
        m_state_changed_listener(param.event, param.detail, param.data1, param.data2, m_accessible_object);
    }

    if (!param.expected_speech.empty())
    {
        ASSERT_TRUE(waitAllCalls(50));
    }
    else
    {
        ASSERT_FALSE(waitAllCalls(50));
    }
}

INSTANTIATE_TEST_SUITE_P(
    send_events,
    CScreenReaderRegistryServiceEventsTest,
    ::testing::Values(
        
        // Speech expecting tests
        
        TTestParam{
            "LoadComplete",
            "",
            0,
            0,
            {"name", "description", "", IAtspiAccessibleObject::ATSPI_ROLE_DOCUMENT_WEB, {}, "", ""}, 
            "name is loaded"
        },
        TTestParam{
            "StateChanged",
            "focused",
            1,
            0,
            {"name", "description", "button", IAtspiAccessibleObject::ATSPI_ROLE_PUSH_BUTTON, {}, "", ""}, 
            "name button. description"
        },
        TTestParam{
            "StateChanged",
            "focused",
            1,
            0,
            {"name", "description", "push button", IAtspiAccessibleObject::ATSPI_ROLE_PUSH_BUTTON, {}, "", ""}, 
            "name button. description"
        },
        TTestParam{
            "StateChanged",
            "focused",
            1,
            0,
            {"name", "description", "check", IAtspiAccessibleObject::ATSPI_ROLE_CHECK_BOX, { IAtspiAccessibleObject::ATSPI_STATE_CHECKED }, "", ""},
            "name check box is checked. description"
        },
        TTestParam{
            "StateChanged",
            "focused",
            1,
            0,
            {"name", "description", "check box", IAtspiAccessibleObject::ATSPI_ROLE_CHECK_BOX, { IAtspiAccessibleObject::ATSPI_STATE_CHECKED }, "", ""},
            "name check box is checked. description"
        },
        TTestParam{
            "StateChanged",
            "focused",
            1,
            0,
            {"name", "description", "check", IAtspiAccessibleObject::ATSPI_ROLE_CHECK_BOX, {}, "", ""},
            "name check box is unchecked. description"
        },
        TTestParam{
            "StateChanged",
            "focused",
            1,
            0,
            {"name", "description", "check box", IAtspiAccessibleObject::ATSPI_ROLE_CHECK_BOX, {}, "", ""},
            "name check box is unchecked. description"
        },
        TTestParam{
            "StateChanged",
            "focused",
            1,
            0,
            {"name", "description", "table", IAtspiAccessibleObject::ATSPI_ROLE_TABLE, {}, "cell", ""},
            "cell name. description"
        },
        TTestParam{
            "StateChanged",
            "checked",
            1,
            0,
            {"name", "description", "check", IAtspiAccessibleObject::ATSPI_ROLE_CHECK_BOX, {}, "", ""},
            "name check box is checked"
        },
        TTestParam{
            "StateChanged",
            "checked",
            1,
            0,
            {"name", "description", "check box", IAtspiAccessibleObject::ATSPI_ROLE_CHECK_BOX, {}, "", ""},
            "name check box is checked"
        },
        TTestParam{
            "StateChanged",
            "checked",
            0,
            0,
            {"name", "description", "check", IAtspiAccessibleObject::ATSPI_ROLE_CHECK_BOX, {}, "", ""},
            "name check box is unchecked"
        },
        TTestParam{
            "StateChanged",
            "checked",
            0,
            0,
            {"name", "description", "check box", IAtspiAccessibleObject::ATSPI_ROLE_CHECK_BOX, {}, "", ""},
            "name check box is unchecked"
        },
        
        // No speech expected tests

        TTestParam{
            "LoadComplete",
            "",
            0,
            0,
            {"name", "description", "", IAtspiAccessibleObject::ATSPI_ROLE_DOCUMENT_FRAME, {}, "", ""}, 
            ""
        },
        TTestParam{
            "StateChanged",
            "focused",
            0,
            0,
            {"name", "description", "button", IAtspiAccessibleObject::ATSPI_ROLE_PUSH_BUTTON, {}, "", ""}, 
            ""
        },
        TTestParam{
            "StateChanged",
            "focused",
            0,
            0,
            {"name", "description", "push button", IAtspiAccessibleObject::ATSPI_ROLE_PUSH_BUTTON, {}, "", ""}, 
            ""
        },
        TTestParam{
            "StateChanged",
            "focused",
            0,
            0,
            {"name", "description", "check", IAtspiAccessibleObject::ATSPI_ROLE_CHECK_BOX, { IAtspiAccessibleObject::ATSPI_STATE_CHECKED }, "", ""},
            ""
        },
        TTestParam{
            "StateChanged",
            "focused",
            0,
            0,
            {"name", "description", "check box", IAtspiAccessibleObject::ATSPI_ROLE_CHECK_BOX, { IAtspiAccessibleObject::ATSPI_STATE_CHECKED }, "", ""},
            ""
        },
        TTestParam{
            "StateChanged",
            "focused",
            0,
            0,
            {"name", "description", "check", IAtspiAccessibleObject::ATSPI_ROLE_CHECK_BOX, {}, "", ""},
            ""
        },
        TTestParam{
            "StateChanged",
            "focused",
            0,
            0,
            {"name", "description", "check box", IAtspiAccessibleObject::ATSPI_ROLE_CHECK_BOX, {}, "", ""},
            ""
        },
        TTestParam{
            "StateChanged",
            "focused",
            0,
            0,
            {"name", "description", "table", IAtspiAccessibleObject::ATSPI_ROLE_TABLE, {}, "cell", ""},
            ""
        }
    )
);

} // namespace CScreenReaderTests