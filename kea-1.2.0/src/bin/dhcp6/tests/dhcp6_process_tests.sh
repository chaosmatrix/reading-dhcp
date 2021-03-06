# Copyright (C) 2014-2017 Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Path to the temporary configuration file.
CFG_FILE=/home/wlodek/dev/releases/120_3/src/bin/dhcp6/tests/test_config.json
# Path to the Kea log file.
LOG_FILE=/home/wlodek/dev/releases/120_3/src/bin/dhcp6/tests/test.log
# Path to the Kea lease file.
LEASE_FILE=/home/wlodek/dev/releases/120_3/src/bin/dhcp6/tests/test_leases.csv
# Expected version
EXPECTED_VERSION="1.2.0"
# Kea configuration to be stored in the configuration file.
CONFIG="{
    \"Dhcp6\":
    {   \"interfaces-config\": {
          \"interfaces\": [ ]
        },
        \"server-id\": {
          \"type\": \"LLT\",
          \"persist\": false
        },
        \"preferred-lifetime\": 3000,
        \"valid-lifetime\": 4000,
        \"renew-timer\": 1000,
        \"rebind-timer\": 2000,
        \"lease-database\":
        {
            \"type\": \"memfile\",
            \"name\": \"$LEASE_FILE\",
            \"persist\": false,
            \"lfc-interval\": 0
        },
        \"subnet6\": [
        {
            \"subnet\": \"2001:db8:1::/64\",
            \"pools\": [ { \"pool\": \"2001:db8:1::10-2001:db8:1::100\" } ]
        } ],
        \"dhcp-ddns\": {
            \"enable-updates\": true,
            \"qualifying-suffix\": \"\"
        }
    },

    \"Logging\":
    {
        \"loggers\": [
        {
            \"name\": \"kea-dhcp6\",
            \"output_options\": [
                {
                    \"output\": \"$LOG_FILE\"
                }
            ],
            \"severity\": \"INFO\"
        }
        ]
    }
}"
# Invalid configuration (syntax error) to check that Kea can check syntax.
# This config has following errors:
# - it should be interfaces-config/interfaces, not interfaces
# - it should be subnet6/pools, no subnet6/pool
CONFIG_BAD_SYNTAX="{
    \"Dhcp6\":
    {
        \"interfaces\": [ ],
        \"preferred-lifetime\": 3000,
        \"valid-lifetime\": 4000,
        \"renew-timer\": 1000,
        \"rebind-timer\": 2000,
        \"lease-database\":
        {
            \"type\": \"memfile\",
            \"persist\": false
        },
        \"subnet6\": [
        {
            \"subnet\": \"2001:db8:1::/64\",
            \"pool\": [ { \"pool\": \"2001:db8:1::10-2001:db8:1::100\" } ]
        } ]
    },

    \"Logging\":
    {
        \"loggers\": [
        {
            \"name\": \"kea-dhcp6\",
            \"output_options\": [
                {
                    \"output\": \"$LOG_FILE\"
                }
            ],
            \"severity\": \"INFO\"
        }
        ]
    }
}"
# Invalid configuration (negative preferred-lifetime) to check that Kea
# gracefully handles reconfiguration errors.
CONFIG_INVALID="{
    \"Dhcp6\":
    {
        \"interfaces-config\": {
          \"interfaces\": [ ]
        },
        \"preferred-lifetime\": -3,
        \"valid-lifetime\": 4000,
        \"renew-timer\": 1000,
        \"rebind-timer\": 2000,
        \"lease-database\":
        {
            \"type\": \"memfile\",
            \"persist\": false
        },
        \"subnet6\": [
        {
            \"subnet\": \"2001:db8:1::/64\",
            \"pool\": [ { \"pool\": \"2001:db8:1::10-2001:db8:1::100\" } ]
        } ]
    },

    \"Logging\":
    {
        \"loggers\": [
        {
            \"name\": \"kea-dhcp6\",
            \"output_options\": [
                {
                    \"output\": \"$LOG_FILE\"
                }
            ],
            \"severity\": \"INFO\"
        }
        ]
    }
}"

# This config has bad pool values. The pool it out of scope for the subnet
# it is defined in. Syntactically the config is correct, though.
CONFIG_BAD_VALUES="{
    \"Dhcp6\":
    {   \"interfaces-config\": {
          \"interfaces\": [ ]
        },
        \"server-id\": {
          \"type\": \"LLT\",
          \"persist\": false
        },
        \"preferred-lifetime\": 3000,
        \"valid-lifetime\": 4000,
        \"renew-timer\": 1000,
        \"rebind-timer\": 2000,
        \"lease-database\":
        {
            \"type\": \"memfile\",
            \"name\": \"$LEASE_FILE\",
            \"persist\": false,
            \"lfc-interval\": 0
        },
        \"subnet6\": [
        {
            \"subnet\": \"2001:db8::/64\",
            \"pools\": [ { \"pool\": \"3000::-3000::ffff\" } ]
        } ],
        \"dhcp-ddns\": {
            \"enable-updates\": true,
            \"qualifying-suffix\": \"\"
        }
    }
}"


# Set the location of the executable.
bin="kea-dhcp6"
bin_path=/home/wlodek/dev/releases/120_3/src/bin/dhcp6

# Import common test library.
. /home/wlodek/dev/releases/120_3/src/lib/testutils/dhcp_test_lib.sh

# This test verifies that syntax checking works properly. This function
# requires 3 parameters:
# testname
# config - string with a content of the config (will be written to a file)
# exp_code - expected exit code returned by kea (0 - success, 1 - failure)
syntax_check_test() {
    local TESTNAME="${1}"
    local CONFIG="${2}"
    local EXP_CODE="${3}"

    # Log the start of the test and print test name.
    test_start $TESTNAME
    # Remove dangling Kea instances and remove log files.
    cleanup
    # Create correct configuration file.
    create_config "${CONFIG}"
    # Check it
    printf "Running command %s.\n" "\"${bin_path}/${bin} -t ${CFG_FILE}\""
    ${bin_path}/${bin} -t ${CFG_FILE}
    exit_code=$?
    if [ ${exit_code} -ne $EXP_CODE ]; then
        printf "ERROR: expected exit code $EXP_CODE, got ${exit_code}\n"
        clean_exit 1
    fi

    test_finish 0
}

# This test verifies that DHCPv6 can be reconfigured with a SIGHUP signal.
dynamic_reconfiguration_test() {
    # Log the start of the test and print test name.
    test_start "dhcpv6_srv.dynamic_reconfiguration"
    # Remove dangling Kea instances and remove log files.
    cleanup
    # Create new configuration file.
    create_config "${CONFIG}"
    # Instruct Kea to log to the specific file.
    set_logger
    # Start Kea.
    start_kea ${bin_path}/${bin}
    # Wait up to 20s for Kea to start.
    wait_for_kea 20
    if [ ${_WAIT_FOR_KEA} -eq 0 ]; then
        printf "ERROR: timeout waiting for Kea to start.\n"
        clean_exit 1
    fi

    # Check if it is still running. It could have terminated (e.g. as a result
    # of configuration failure).
    get_pid ${bin}
    if [ ${_GET_PIDS_NUM} -ne 1 ]; then
        printf "ERROR: expected one Kea process to be started. Found %d processes\
 started.\n" ${_GET_PIDS_NUM}
        clean_exit 1
    fi

    # Check in the log file, how many times server has been configured. It should
    # be just once on startup.
    get_reconfigs
    if [ ${_GET_RECONFIGS} -ne 1 ]; then
        printf "ERROR: server hasn't been configured.\n"
        clean_exit 1
    else
        printf "Server successfully configured.\n"
    fi

    # Now use invalid configuration.
    create_config "${CONFIG_INVALID}"

    # Try to reconfigure by sending SIGHUP
    send_signal 1 ${bin}

    # The configuration should fail and the error message should be there.
    wait_for_message 10 "DHCP6_CONFIG_LOAD_FAIL" 1

    # After receiving SIGHUP the server should try to reconfigure itself.
    # The configuration provided is invalid so it should result in
    # reconfiguration failure but the server should still be running.
    get_reconfigs
    if [ ${_GET_RECONFIGS} -ne 1 ]; then
        printf "ERROR: server has been reconfigured despite bogus configuration.\n"
        clean_exit 1
    elif [ ${_GET_RECONFIG_ERRORS} -ne 1 ]; then
        printf "ERROR: server did not report reconfiguration error despite attempt\
 to configure it with invalid configuration.\n"
        clean_exit 1
    fi

    # Make sure the server is still operational.
    get_pid ${bin}
    if [ ${_GET_PIDS_NUM} -ne 1 ]; then
        printf "ERROR: Kea process was killed when attempting reconfiguration.\n"
        clean_exit 1
    fi

    # Restore the good configuration.
    create_config "${CONFIG}"

    # Reconfigure the server with SIGHUP.
    send_signal 1 ${bin}

    # There should be two occurrences of the DHCP6_CONFIG_COMPLETE messages.
    # Wait for it up to 10s.
    wait_for_message 10 "DHCP6_CONFIG_COMPLETE" 2

    # After receiving SIGHUP the server should get reconfigured and the
    # reconfiguration should be noted in the log file. We should now
    # have two configurations logged in the log file.
    if [ ${_WAIT_FOR_MESSAGE} -eq 0 ]; then
        printf "ERROR: server hasn't been reconfigured.\n"
        clean_exit 1
    else
        printf "Server successfully reconfigured.\n"
    fi

    # Make sure the server is still operational.
    get_pid ${bin}
    if [ ${_GET_PIDS_NUM} -ne 1 ]; then
        printf "ERROR: Kea process was killed when attempting reconfiguration.\n"
        clean_exit 1
    fi

    # When the server receives a signal the call to select() function is
    # interrupted. This should not be logged as an error.
    get_log_messages "DHCP6_PACKET_RECEIVE_FAIL"
    assert_eq 0 ${_GET_LOG_MESSAGES} \
        "Expected get_log_messages DHCP6_PACKET_RECEIVE_FAIL return %d, \
returned %d."

    # All ok. Shut down Kea and exit.
    test_finish 0
}

# This test verifies that DHCPv6 server is shut down gracefully when it
# receives a SIGINT or SIGTERM signal.
shutdown_test() {
    test_name=${1}  # Test name
    signum=${2}      # Signal number

    # Log the start of the test and print test name.
    test_start ${test_name}
    # Remove dangling Kea instances and remove log files.
    cleanup
    # Create new configuration file.
    create_config "${CONFIG}"
    # Instruct Kea to log to the specific file.
    set_logger
    # Start Kea.
    start_kea ${bin_path}/${bin}
    # Wait up to 20s for Kea to start.
    wait_for_kea 20
    if [ ${_WAIT_FOR_KEA} -eq 0 ]; then
        printf "ERROR: timeout waiting for Kea to start.\n"
        clean_exit 1
    fi

    # Check if it is still running. It could have terminated (e.g. as a result
    # of configuration failure).
    get_pid ${bin}
    if [ ${_GET_PIDS_NUM} -ne 1 ]; then
        printf "ERROR: expected one Kea process to be started. Found %d processes\
 started.\n" ${_GET_PIDS_NUM}
        clean_exit 1
    fi

    # Check in the log file, how many times server has been configured. It should
    # be just once on startup.
    get_reconfigs
    if [ ${_GET_RECONFIGS} -ne 1 ]; then
        printf "ERROR: server hasn't been configured.\n"
        clean_exit 1
    else
        printf "Server successfully configured.\n"
    fi

    # Send signal to Kea (SIGTERM, SIGINT etc.)
    send_signal ${signum} ${bin}

    # Wait up to 10s for the server's graceful shutdown. The graceful shut down
    # should be recorded in the log file with the appropriate message.
    wait_for_message 10 "DHCP6_SHUTDOWN" 1
    if [ ${_WAIT_FOR_MESSAGE} -eq 0 ]; then
        printf "ERROR: Server did not record shutdown in the log.\n"
        clean_exit 1
    fi

    # Make sure the server is down.
    wait_for_server_down 5 ${bin}
    assert_eq 1 ${_WAIT_FOR_SERVER_DOWN} \
        "Expected wait_for_server_down return %d, returned %d"

    # When the server receives a signal the call to select() function is
    # interrupted. This should not be logged as an error.
    get_log_messages "DHCP6_PACKET_RECEIVE_FAIL"
    assert_eq 0 ${_GET_LOG_MESSAGES} \
        "Expected get_log_messages DHCP6_PACKET_RECEIVE_FAIL return %d, \
returned %d."

    test_finish 0
}

# This test verifies that DHCPv6 can be configured to run lease file cleanup
# periodially.
lfc_timer_test() {
    # Log the start of the test and print test name.
    test_start "dhcpv6_srv.lfc_timer_test"
    # Remove dangling Kea instances and remove log files.
    cleanup
    # Create a configuration with the LFC enabled, by replacing the section
    # with the lfc-interval and persist parameters.
    LFC_CONFIG=$(printf "${CONFIG}" | sed -e 's/\"lfc-interval\": 0/\"lfc-interval\": 3/g' \
                        | sed -e 's/\"persist\": false,/\"persist\": true,/g')
    # Create new configuration file.
    create_config "${LFC_CONFIG}"
    # Instruct Kea to log to the specific file.
    set_logger
    # Start Kea.
    start_kea ${bin_path}/${bin}
    # Wait up to 20s for Kea to start.
    wait_for_kea 20
    if [ ${_WAIT_FOR_KEA} -eq 0 ]; then
        printf "ERROR: timeout waiting for Kea to start.\n"
        clean_exit 1
    fi

    # Check if it is still running. It could have terminated (e.g. as a result
    # of configuration failure).
    get_pid ${bin}
    if [ ${_GET_PIDS_NUM} -ne 1 ]; then
        printf "ERROR: expected one Kea process to be started. Found %d processes\
 started.\n" ${_GET_PIDS_NUM}
        clean_exit 1
    fi

    # Check if Kea emits the log message indicating that LFC is started.
    wait_for_message 10 "DHCPSRV_MEMFILE_LFC_EXECUTE" 1
    if [ ${_WAIT_FOR_MESSAGE} -eq 0 ]; then
        printf "ERROR: Server did not execute LFC.\n"
        clean_exit 1
    fi

    # Give it a short time to run.
    sleep 1

    # Modify the interval.
    LFC_CONFIG=$(printf "${LFC_CONFIG}" | sed -e 's/\"lfc-interval\": 3/\"lfc-interval\": 4/g')
    # Create new configuration file.
    create_config "${LFC_CONFIG}"

    # Reconfigure the server with SIGHUP.
    send_signal 1 ${bin}

    # There should be two occurrences of the DHCP4_CONFIG_COMPLETE messages.
    # Wait for it up to 10s.
    wait_for_message 10 "DHCP6_CONFIG_COMPLETE" 2

    # After receiving SIGHUP the server should get reconfigured and the
    # reconfiguration should be noted in the log file. We should now
    # have two configurations logged in the log file.
    if [ ${_WAIT_FOR_MESSAGE} -eq 0 ]; then
        printf "ERROR: server hasn't been reconfigured.\n"
        clean_exit 1
    else
        printf "Server successfully reconfigured.\n"
    fi

    # Make sure the server is still operational.
    get_pid ${bin}
    if [ ${_GET_PIDS_NUM} -ne 1 ]; then
        printf "ERROR: Kea process was killed when attempting reconfiguration.\n"
        clean_exit 1
    fi

    # Wait for the LFC to run the second time.
    wait_for_message 10 "DHCPSRV_MEMFILE_LFC_EXECUTE" 2
    if [ ${_WAIT_FOR_MESSAGE} -eq 0 ]; then
        printf "ERROR: Server did not execute LFC.\n"
        clean_exit 1
    fi

    # Send signal to Kea SIGTERM
    send_signal 15 ${bin}

    # Wait up to 10s for the server's graceful shutdown. The graceful shut down
    # should be recorded in the log file with the appropriate message.
    wait_for_message 10 "DHCP6_SHUTDOWN" 1
    if [ ${_WAIT_FOR_MESSAGE} -eq 0 ]; then
        printf "ERROR: Server did not record shutdown in the log.\n"
        clean_exit 1
    fi

    # Make sure the server is down.
    wait_for_server_down 5 ${bin}
    assert_eq 1 ${_WAIT_FOR_SERVER_DOWN} \
        "Expected wait_for_server_down return %d, returned %d"

    # All ok. Shut down Kea and exit.
    test_finish 0
}

server_pid_file_test "${CONFIG}" DHCP6_ALREADY_RUNNING
dynamic_reconfiguration_test
shutdown_test "dhcpv6.sigterm_test" 15
shutdown_test "dhcpv6.sigint_test" 2
version_test "dhcpv6.version"
logger_vars_test "dhcpv6.variables"
lfc_timer_test
syntax_check_test "dhcpv6.syntax_check_success" "${CONFIG}" 0
syntax_check_test "dhcpv6.syntax_check_bad_syntax" "${CONFIG_BAD_SYNTAX}" 1
syntax_check_test "dhcpv6.syntax_check_bad_values" "${CONFIG_BAD_VALUES}" 1
