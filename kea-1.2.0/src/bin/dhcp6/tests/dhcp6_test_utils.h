// Copyright (C) 2013-2017 Internet Systems Consortium, Inc. ("ISC")
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

/// @file   dhcp6_test_utils.h
///
/// @brief  This file contains utility classes used for DHCPv6 server testing

#ifndef DHCP6_TEST_UTILS_H
#define DHCP6_TEST_UTILS_H

#include <gtest/gtest.h>

#include <dhcp/pkt6.h>
#include <dhcp/option6_ia.h>
#include <dhcp/option6_iaaddr.h>
#include <dhcp/option6_iaprefix.h>
#include <dhcp/option6_status_code.h>
#include <dhcp/option_int_array.h>
#include <dhcp/option_custom.h>
#include <dhcp/option.h>
#include <dhcp/iface_mgr.h>
#include <dhcpsrv/cfgmgr.h>
#include <dhcpsrv/lease_mgr.h>
#include <dhcpsrv/lease_mgr_factory.h>
#include <dhcp6/dhcp6_srv.h>
#include <dhcp6/parser_context.h>
#include <hooks/hooks_manager.h>

#include <list>

namespace isc {
namespace dhcp {
namespace test {

/// @brief Generic wrapper to provide strongly typed values.
///
/// In many cases, the test fixture class methods require providing many
/// parameters, of which some are optional. Some of the parameters may also
/// be implicitly converted to other types. Non-careful test implementer
/// may often "shift by one" or swap two values on the arguments list, which
/// will be accepted by the compiler but will result in troubles running the
/// function. Sometimes it takes non trivial amount of debugging to find out
/// why the particular function fails until we find that the arguments were
/// swapped or shifted. In addition, the use of classes wrapping simple types
/// results in better readability of the test code.
///
/// @tparam ValueType Type of the wrapped value.
template<typename ValueType>
struct SpecializedTypeWrapper {

    /// @brief Constructor
    ///
    /// @param value Wrapped value
    explicit SpecializedTypeWrapper(const ValueType& value)
        : value_(value) { }

    /// @brief Operator returning a wrapped value.
    operator ValueType () const {
        return (value_);
    }

    /// @brief Wrapped value.
    ValueType value_;
};


/// @brief Class representing strongly typed IAID.
struct IAID : public SpecializedTypeWrapper<uint32_t> {
    /// @brief Constructor
    ///
    /// @param iaid IAID.
    explicit IAID(const uint32_t iaid)
        : SpecializedTypeWrapper<uint32_t>(iaid) { }
};

/// @brief Class representing strongly typed value for strict IAID checks.
///
/// Strict IAID checks are used to verify that  the particular address has been
/// assign to a specific IA. In many cases we don't check that because it may
/// not be possible to predict to which IA the specific lease will be assigned.
struct StrictIAIDChecking : public SpecializedTypeWrapper<bool> {
    /// @brief Constructor.
    ///
    /// @param strict_check Boolean value indicating if strict checking should
    /// be performed.
    explicit StrictIAIDChecking(const bool strict_check)
        : SpecializedTypeWrapper<bool>(strict_check) { }

    /// @brief Convenience function returning an object indicating that strict
    /// checks should be performed.
    static const StrictIAIDChecking YES() {
        static StrictIAIDChecking strict_check(true);
        return (strict_check);
    }

    /// @brief Convenience function returning an object indicating that strict
    /// checks should not be performed.
    static StrictIAIDChecking NO() {
        static StrictIAIDChecking strict_check(false);
        return (strict_check);
    }
};


/// @brief Base class for DHCPv6 server testing.
///
/// Currently it configures the test data path directory in
/// the @c CfgMgr. When the object is destroyed, the original
/// path is reverted.
class BaseServerTest : public ::testing::Test {
public:

    /// @brief Location of a test DUID file
    static const char* DUID_FILE;

    /// @brief Constructor.
    BaseServerTest();

    /// @brief Destructor.
    virtual ~BaseServerTest();

private:

    /// @brief Holds the original data directory.
    std::string original_datadir_;

};

/// @brief "naked" Dhcpv6Srv class that exposes internal members
class NakedDhcpv6Srv: public isc::dhcp::Dhcpv6Srv {
public:
    NakedDhcpv6Srv(uint16_t port) : isc::dhcp::Dhcpv6Srv(port) {
        // Open the "memfile" database for leases
        std::string memfile = "type=memfile universe=6 persist=false";
        isc::dhcp::LeaseMgrFactory::create(memfile);
    }

    /// @brief fakes packet reception
    /// @param timeout ignored
    ///
    /// The method receives all packets queued in receive
    /// queue, one after another. Once the queue is empty,
    /// it initiates the shutdown procedure.
    ///
    /// See fake_received_ field for description
    virtual isc::dhcp::Pkt6Ptr receivePacket(int /*timeout*/) {

        // If there is anything prepared as fake incoming
        // traffic, use it
        if (!fake_received_.empty()) {
            isc::dhcp::Pkt6Ptr pkt = fake_received_.front();
            fake_received_.pop_front();
            return (pkt);
        }

        // If not, just trigger shutdown and
        // return immediately
        shutdown();
        return (isc::dhcp::Pkt6Ptr());
    }

    /// @brief fake packet sending
    ///
    /// Pretend to send a packet, but instead just store
    /// it in fake_send_ list where test can later inspect
    /// server's response.
    virtual void sendPacket(const isc::dhcp::Pkt6Ptr& pkt) {
        fake_sent_.push_back(pkt);
    }

    /// @brief adds a packet to fake receive queue
    ///
    /// See fake_received_ field for description
    void fakeReceive(const isc::dhcp::Pkt6Ptr& pkt) {
        fake_received_.push_back(pkt);
    }

    virtual ~NakedDhcpv6Srv() {
        // Close the lease database
        isc::dhcp::LeaseMgrFactory::destroy();
    }

    using Dhcpv6Srv::processSolicit;
    using Dhcpv6Srv::processRequest;
    using Dhcpv6Srv::processRenew;
    using Dhcpv6Srv::processRebind;
    using Dhcpv6Srv::processConfirm;
    using Dhcpv6Srv::processRelease;
    using Dhcpv6Srv::processDecline;
    using Dhcpv6Srv::processInfRequest;
    using Dhcpv6Srv::processClientFqdn;
    using Dhcpv6Srv::createNameChangeRequests;
    using Dhcpv6Srv::selectSubnet;
    using Dhcpv6Srv::testServerID;
    using Dhcpv6Srv::testUnicast;
    using Dhcpv6Srv::sanityCheck;
    using Dhcpv6Srv::classifyPacket;
    using Dhcpv6Srv::shutdown_;
    using Dhcpv6Srv::name_change_reqs_;
    using Dhcpv6Srv::VENDOR_CLASS_PREFIX;
    using Dhcpv6Srv::initContext;

    /// @brief packets we pretend to receive
    ///
    /// Instead of setting up sockets on interfaces that change between
    /// OSes, it is much easier to fake packet reception. This is a list
    /// of packets that we pretend to have received. You can schedule
    /// new packets to be received using fakeReceive() and
    /// NakedDhcpv6Srv::receivePacket() methods.
    std::list<isc::dhcp::Pkt6Ptr> fake_received_;

    std::list<isc::dhcp::Pkt6Ptr> fake_sent_;
};

/// @brief Test fixture for any tests requiring blank/empty configuration
///        serves as base class for additional tests
class NakedDhcpv6SrvTest : public BaseServerTest {
public:

    /// @brief Constructor
    NakedDhcpv6SrvTest();

    // Generate IA_NA or IA_PD option with specified parameters
    boost::shared_ptr<isc::dhcp::Option6IA> generateIA
        (uint16_t type, uint32_t iaid, uint32_t t1, uint32_t t2);

    /// @brief generates interface-id option, based on text
    ///
    /// @param iface_id textual representation of the interface-id content
    ///
    /// @return pointer to the option object
    isc::dhcp::OptionPtr generateInterfaceId(const std::string& iface_id) {
        isc::dhcp::OptionBuffer tmp(iface_id.begin(), iface_id.end());
        return (isc::dhcp::OptionPtr
                (new isc::dhcp::Option(isc::dhcp::Option::V6,
                                       D6O_INTERFACE_ID, tmp)));
    }

    // Generate client-id option
    isc::dhcp::OptionPtr generateClientId(size_t duid_size = 32) {

        isc::dhcp::OptionBuffer clnt_duid(duid_size);
        for (size_t i = 0; i < duid_size; i++) {
            clnt_duid[i] = 100 + i;
        }

        duid_ = isc::dhcp::DuidPtr(new isc::dhcp::DUID(clnt_duid));

        return (isc::dhcp::OptionPtr
                (new isc::dhcp::Option(isc::dhcp::Option::V6, D6O_CLIENTID,
                                       clnt_duid.begin(),
                                       clnt_duid.begin() + duid_size)));
    }

    // Checks if server response (ADVERTISE or REPLY) includes proper
    // server-id.
    void checkServerId(const isc::dhcp::Pkt6Ptr& rsp,
                       const isc::dhcp::OptionPtr& expected_srvid)
    {
        // check that server included its server-id
        isc::dhcp::OptionPtr tmp = rsp->getOption(D6O_SERVERID);
        EXPECT_EQ(tmp->getType(), expected_srvid->getType() );
        ASSERT_EQ(tmp->len(), expected_srvid->len() );
        EXPECT_TRUE(tmp->getData() == expected_srvid->getData());
    }

    // Checks if server response (ADVERTISE or REPLY) includes proper
    // client-id.
    void checkClientId(const isc::dhcp::Pkt6Ptr& rsp,
                       const isc::dhcp::OptionPtr& expected_clientid)
    {
        // check that server included our own client-id
        isc::dhcp::OptionPtr tmp = rsp->getOption(D6O_CLIENTID);
        ASSERT_TRUE(tmp);
        EXPECT_EQ(expected_clientid->getType(), tmp->getType());
        ASSERT_EQ(expected_clientid->len(), tmp->len());

        // check that returned client-id is valid
        EXPECT_TRUE(expected_clientid->getData() == tmp->getData());
    }

    // Checks if server response is a NAK
    void checkNakResponse(const isc::dhcp::Pkt6Ptr& rsp,
                          uint8_t expected_message_type,
                          uint32_t expected_transid,
                          uint16_t expected_status_code,
                          uint32_t expected_t1, uint32_t expected_t2)
    {
        // Check if we get response at all
        checkResponse(rsp, expected_message_type, expected_transid);

        // Check that IA_NA was returned
        isc::dhcp::OptionPtr option_ia_na = rsp->getOption(D6O_IA_NA);
        ASSERT_TRUE(option_ia_na);

        // check that the status is no address available
        boost::shared_ptr<isc::dhcp::Option6IA> ia =
            boost::dynamic_pointer_cast<isc::dhcp::Option6IA>(option_ia_na);
        ASSERT_TRUE(ia);

        checkIA_NAStatusCode(ia, expected_status_code, expected_t1,
                             expected_t2);
    }

    /// @brief Checks that the server inserted expected status code in IA_NA
    ///
    /// Check that the server used status code as expected, i.e. that it has
    /// no addresses (if status code is non-zero) and that expected status
    /// code really appears there. In some limited cases (reply to RELEASE)
    /// it may be used to verify positive case, where IA_NA response is
    /// expected to not include address.
    ///
    /// Status code indicates type of error encountered (in theory it can also
    /// indicate success, but servers typically don't send success status
    /// as this is the default result and it saves bandwidth)
    /// @param ia IA_NA container to be checked
    /// @param expected_status_code expected value in status-code option
    /// @param expected_t1 expected T1 in IA_NA option
    /// @param expected_t2 expected T2 in IA_NA option
    /// @param check_addr whether to check for address with 0 lifetimes
    void checkIA_NAStatusCode
        (const boost::shared_ptr<isc::dhcp::Option6IA>& ia,
         uint16_t expected_status_code, uint32_t expected_t1,
         uint32_t expected_t2, bool check_addr = true);

    void checkMsgStatusCode(const isc::dhcp::Pkt6Ptr& msg,
                            uint16_t expected_status)
    {
        isc::dhcp::Option6StatusCodePtr status =
            boost::dynamic_pointer_cast<isc::dhcp::Option6StatusCode>
                (msg->getOption(D6O_STATUS_CODE));

        // It is ok to not include status success as this is the default
        // behavior
        if (expected_status == STATUS_Success && !status) {
            return;
        }

        EXPECT_TRUE(status);
        if (status) {
            // We don't have dedicated class for status code, so let's
            // just interpret first 2 bytes as status. Remainder of the
            // status code option content is just a text explanation
            // what went wrong.
            EXPECT_EQ(static_cast<uint16_t>(expected_status),
                      status->getStatusCode());
        }
    }

    // Basic checks for generated response (message type and transaction-id).
    void checkResponse(const isc::dhcp::Pkt6Ptr& rsp,
                       uint8_t expected_message_type,
                       uint32_t expected_transid) {
        ASSERT_TRUE(rsp);
        EXPECT_EQ(expected_message_type, rsp->getType());
        EXPECT_EQ(expected_transid, rsp->getTransid());
    }

    virtual ~NakedDhcpv6SrvTest();

    // A DUID used in most tests (typically as client-id)
    isc::dhcp::DuidPtr duid_;

    int rcode_;
    isc::data::ConstElementPtr comment_;

    // Name of a valid network interface
    std::string valid_iface_;
};

// We need to pass one reference to the Dhcp6Client, which is defined in
// dhcp6_client.h. That header includes this file. To avoid circular
// dependencies, we use forward declaration here.
class Dhcp6Client;

// Provides support for tests against a preconfigured subnet6
// extends upon NakedDhcp6SrvTest
class Dhcpv6SrvTest : public NakedDhcpv6SrvTest {
public:
    /// @brief Specifies expected outcome
    enum ExpectedResult {
        SHOULD_PASS, // pass = accept decline, move lease to declined state.
        SHOULD_FAIL  // fail = reject the decline
    };

    /// @brief Specifies what address should the client include in its Decline
    enum AddressInclusion {
        VALID_ADDR, // Client will include its own, valid address
        BOGUS_ADDR, // Client will include an address it doesn't own
        NO_ADDR,    // Client will send empty IA_NA (without address)
        NO_IA       // Client will not send IA_NA at all
    };

    /// @brief Constructor that initializes a simple default configuration
    ///
    /// Sets up a single subnet6 with one pool for addresses and second
    /// pool for prefixes.
    Dhcpv6SrvTest();

    /// @brief destructor
    ///
    /// Removes existing configuration.
    ~Dhcpv6SrvTest() {
        isc::dhcp::CfgMgr::instance().clear();
    };

    /// @brief Runs DHCPv6 configuration from the JSON string.
    ///
    /// @param config String holding server configuration in JSON format.
    void configure(const std::string& config);

    /// @brief Configure the DHCPv6 server using the JSON string.
    ///
    /// @param config String holding server configuration in JSON format.
    /// @param srv Server to be configured.
    void configure(const std::string& config, NakedDhcpv6Srv& srv);

    /// @brief Checks that server response (ADVERTISE or REPLY) contains proper
    ///        IA_NA option
    ///
    /// @param rsp server's response
    /// @param expected_iaid expected IAID value
    /// @param expected_t1 expected T1 value
    /// @param expected_t2 expected T2 value
    /// @return IAADDR option for easy chaining with checkIAAddr method
    boost::shared_ptr<isc::dhcp::Option6IAAddr>
        checkIA_NA(const isc::dhcp::Pkt6Ptr& rsp, uint32_t expected_iaid,
                   uint32_t expected_t1, uint32_t expected_t2);

    /// @brief Checks that server response (ADVERTISE or REPLY) contains proper
    ///        IA_PD option
    ///
    /// @param rsp server's response
    /// @param expected_iaid expected IAID value
    /// @param expected_t1 expected T1 value
    /// @param expected_t2 expected T2 value
    /// @return IAPREFIX option for easy chaining with checkIAAddr method
    boost::shared_ptr<isc::dhcp::Option6IAPrefix>
    checkIA_PD(const isc::dhcp::Pkt6Ptr& rsp, uint32_t expected_iaid,
               uint32_t expected_t1, uint32_t expected_t2);

    // Check that generated IAADDR option contains expected address
    // and lifetime values match the configured subnet
    void checkIAAddr(const boost::shared_ptr<isc::dhcp::Option6IAAddr>& addr,
                     const isc::asiolink::IOAddress& expected_addr,
                     isc::dhcp::Lease::Type type) {

        // Check that the assigned address is indeed from the configured pool.
        // Note that when comparing addresses, we compare the textual
        // representation. IOAddress does not support being streamed to
        // an ostream, which means it can't be used in EXPECT_EQ.
        EXPECT_TRUE(subnet_->inPool(type, addr->getAddress()));
        EXPECT_EQ(expected_addr.toText(), addr->getAddress().toText());
        EXPECT_EQ(subnet_->getPreferred(), addr->getPreferred());
        EXPECT_EQ(subnet_->getValid(), addr->getValid());
    }

    // Checks if the lease sent to client is present in the database
    // and is valid when checked against the configured subnet
    isc::dhcp::Lease6Ptr checkLease
        (const isc::dhcp::DuidPtr& duid, const isc::dhcp::OptionPtr& ia_na,
         boost::shared_ptr<isc::dhcp::Option6IAAddr> addr);

    /// @brief Check if the specified lease is present in the data base.
    ///
    /// @param lease Lease to be searched in the database.
    /// @return Pointer to the lease in the database.
    isc::dhcp::Lease6Ptr checkLease(const isc::dhcp::Lease6& lease);

    /// @brief Verifies received IAPrefix option
    ///
    /// Verifies if the received IAPrefix option matches the lease in the
    /// database.
    ///
    /// @param duid client's DUID
    /// @param ia_pd IA_PD option that contains the IAPRefix option
    /// @param prefix pointer to the IAPREFIX option
    /// @return corresponding IPv6 lease (if found)
    isc::dhcp::Lease6Ptr checkPdLease
      (const isc::dhcp::DuidPtr& duid, const isc::dhcp::OptionPtr& ia_pd,
       boost::shared_ptr<isc::dhcp::Option6IAPrefix> prefix);

    /// @brief Creates a message with specified IA
    ///
    /// A utility function that creates a message of the specified type with
    /// a specified container (IA_NA or IA_PD) and an address or prefix
    /// inside it.
    ///
    /// @param message_type type of the message (e.g. DHCPV6_SOLICIT)
    /// @param lease_type type of a lease (TYPE_NA or TYPE_PD)
    /// @param addr address or prefix to use in IADDRESS or IAPREFIX options
    /// @param prefix_len length of the prefix (used for prefixes only)
    /// @param iaid IA identifier (used in IA_XX option)
    /// @return created message
    isc::dhcp::Pkt6Ptr
    createMessage(uint8_t message_type, isc::dhcp::Lease::Type lease_type,
                  const isc::asiolink::IOAddress& addr,
                  const uint8_t prefix_len, const uint32_t iaid);

    /// @brief Creates instance of IA option holding single address or prefix.
    ///
    /// Utility function that creates an IA option instance with a single
    /// IPv6 address or prefix. This function is internally called by the
    /// @c createMessage function. It may be also used to add additional
    /// IA options to the message generated by @c createMessage (which adds
    /// a single IA option by itself.).
    ///
    /// @param lease_type type of the lease (TYPE_NA or TYPE_PD).
    /// @param addr address or prefix to use in IADDRESS or IAPREFIX options.
    /// @param prefix_len length of the prefix (used for PD, ignored for NA).
    /// @param iaid IA identifier.
    ///
    /// @return Created instance of the IA option.
    isc::dhcp::Option6IAPtr
    createIA(isc::dhcp::Lease::Type lease_type,
             const isc::asiolink::IOAddress& addr,
             const uint8_t prefix_len, const uint32_t iaid);

    /// @brief Compare options
    ///
    /// This method compares whether options content is identical. It writes
    /// both options to a buffer and then compares the buffers. Comparing
    /// two different instances of an option that has identical content
    /// will return true.
    ///
    /// It is safe to pass NULL pointers. Two NULL pointers are equal.
    /// NULL pointer and non-NULL pointers are obviously non-equal.
    ///
    /// @param option1 pointer to the first option
    /// @param option2
    /// @return true, if content is identical
    bool compareOptions(const isc::dhcp::OptionPtr& option1,
                        const isc::dhcp::OptionPtr& option2);

    /// @brief Tests if the acquired lease is or is not declined.
    ///
    /// @param client Dhcp6Client instance
    /// @param duid1 DUID used during lease acquisition
    /// @param iaid1 IAID used during lease acquisition
    /// @param duid2 DUID used during Decline exchange
    /// @param iaid2 IAID used during Decline exchange
    /// @param addr_type specify what sort of address the client should
    ///        include (its own, a bogus one or no address at all)
    /// @param expected_result SHOULD_PASS if the lease is expected to
    /// be successfully declined, or SHOULD_FAIL if the lease is expected
    /// to not be declined.
    void acquireAndDecline(Dhcp6Client& client,
                           const std::string& duid1,
                           const uint32_t iaid1,
                           const std::string& duid2,
                           const uint32_t iaid2,
                           AddressInclusion addr_type,
                           ExpectedResult expected_result);

    /// @brief Performs basic (positive) RENEW test
    ///
    /// See renewBasic and pdRenewBasic tests for detailed explanation.
    /// In essence the test attempts to perform a successful RENEW scenario.
    ///
    /// This method does not throw, but uses gtest macros to signify failures.
    ///
    /// @param type type (TYPE_NA or TYPE_PD)
    /// @param existing_addr address to be preinserted into the database
    /// @param renew_addr address being sent in RENEW
    /// @param prefix_len length of the prefix (128 for addresses)
    /// @param insert_before_renew should the lease be inserted into the database
    ///        before we try renewal?
    void
    testRenewBasic(isc::dhcp::Lease::Type type,
                   const std::string& existing_addr,
                   const std::string& renew_addr, const uint8_t prefix_len,
                   bool insert_before_renew = true);

    /// @brief Checks if RENEW with invalid IAID is processed correctly.
    ///
    /// @param type lease type (currently only IA_NA is supported)
    /// @param addr address to be renewed
    void
    testRenewWrongIAID(isc::dhcp::Lease::Type type,
                       const asiolink::IOAddress& addr);

    /// @brief Checks if client A can renew address used by client B
    ///
    /// @param type lease type (currently only IA_NA is supported)
    /// @param addr address to be renewed
    void
    testRenewSomeoneElsesLease(isc::dhcp::Lease::Type type,
                               const asiolink::IOAddress& addr);

    /// @brief Performs basic (positive) RELEASE test
    ///
    /// See releaseBasic and pdReleaseBasic tests for detailed explanation.
    /// In essence the test attempts to perform a successful RELEASE scenario.
    ///
    /// This method does not throw, but uses gtest macros to signify failures.
    ///
    /// @param type type (TYPE_NA or TYPE_PD)
    /// @param existing address to be preinserted into the database
    /// @param release_addr address being sent in RELEASE
    void
    testReleaseBasic(isc::dhcp::Lease::Type type,
                     const isc::asiolink::IOAddress& existing,
                     const isc::asiolink::IOAddress& release_addr);

    /// @brief Performs negative RELEASE test
    ///
    /// See releaseReject and pdReleaseReject tests for detailed
    /// explanation.  In essence the test attempts to perform couple
    /// failed RELEASE scenarios.
    ///
    /// This method does not throw, but uses gtest macros to signify failures.
    ///
    /// @param type type (TYPE_NA or TYPE_PD)
    /// @param addr address being sent in RELEASE
    void
    testReleaseReject(isc::dhcp::Lease::Type type,
                      const isc::asiolink::IOAddress& addr);

    /// @brief simulates reception of a packet of specified type and checks statistic
    ///
    /// @param pkt_type reception of a packet of this type will be simulated
    /// @param stat_name this statistic is expected to be set to 1
    void testReceiveStats(uint8_t pkt_type, const std::string& stat_name);


    /// A subnet used in most tests
    isc::dhcp::Subnet6Ptr subnet_;

    /// A normal, non-temporary pool used in most tests
    isc::dhcp::Pool6Ptr pool_;

    /// A prefix pool used in most tests
    isc::dhcp::Pool6Ptr pd_pool_;

    /// @brief Server object under test.
    NakedDhcpv6Srv srv_;
};

/// @brief Runs parser in JSON mode, useful for parser testing
///
/// @param in string to be parsed
/// @return ElementPtr structure representing parsed JSON
inline isc::data::ElementPtr
parseJSON(const std::string& in)
{
    isc::dhcp::Parser6Context ctx;
    return (ctx.parseString(in, isc::dhcp::Parser6Context::PARSER_JSON));
}

/// @brief Runs parser in Dhcp6 mode
///
/// This is a simplified Dhcp6 mode, so no outer { } and "Dhcp6" is
/// needed. This format is used by most of the tests.
///
/// @param in string to be parsed
/// @param verbose display the exception message when it fails
/// @return ElementPtr structure representing parsed JSON
inline isc::data::ElementPtr
parseDHCP6(const std::string& in, bool verbose = false)
{
    try {
        isc::dhcp::Parser6Context ctx;
        return (ctx.parseString(in, isc::dhcp::Parser6Context::SUBPARSER_DHCP6));
    }
    catch (const std::exception& ex) {
        if (verbose) {
            std::cout << "EXCEPTION: " << ex.what() << std::endl;
        }
        throw;
    }
}

/// @brief Runs parser in option definition mode
///
/// This function parses specified text as JSON that defines option definitions.
///
/// @param in string to be parsed
/// @param verbose display the exception message when it fails
/// @return ElementPtr structure representing parsed JSON
inline isc::data::ElementPtr
parseOPTION_DEF(const std::string& in, bool verbose = false)
{
    try {
        isc::dhcp::Parser6Context ctx;
        return (ctx.parseString(in, isc::dhcp::Parser6Context::PARSER_OPTION_DEF));
    }
    catch (const std::exception& ex) {
        if (verbose) {
            std::cout << "EXCEPTION: " << ex.what() << std::endl;
        }
        throw;
    }
}

}; // end of isc::dhcp::test namespace
}; // end of isc::dhcp namespace
}; // end of isc namespace

#endif // DHCP6_TEST_UTILS_H
