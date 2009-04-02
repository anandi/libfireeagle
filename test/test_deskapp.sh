#!/bin/bash

function abort_test() {
    echo $1
    exit 1
}

function usage() {
    echo "Environment variables to set:"
    echo "FE_TEST_APP_TOKEN : Set the path to the application token file"
    echo "FE_TEST_GENERAL_TOKEN : Set the path to the application general token file"
    echo "FE_TEST_CONFIG : Set the path to the config file"
    echo ""
    echo "Give phase1 or phase2 as command argument"
}

function abort_test_with_usage() {
    usage
    abort_test "$*"
}

function test_file() {
    [ -f $1 ] || abort_test "$1 is not a proper file"
}

function check_app_token_file() {
    [ -z $FE_TEST_APP_TOKEN ] && abort_test_with_usage "The app token file FE_TEST_APP_TOKEN is not defined in the environment"
    echo "Using application token from file $FE_TEST_APP_TOKEN"
    test_file $FE_TEST_APP_TOKEN
}

function check_general_token_file() {
    # In case of non-availability of general token, do not abort tests.
    [ -z $FE_TEST_GENERAL_TOKEN ] && abort_test_with_usage "The general token file FE_TEST_GENERAL_TOKEN is not defined in the environment"
    echo "Using general token from file $FE_TEST_GENERAL_TOKEN"
    test_file $FE_TEST_GENERAL_TOKEN
}

function config_has_general_token() {
    [ -z $FE_TEST_CONFIG ] && abort_test_with_usage "The config file FE_TEST_CONFIG is not defined in the environment"
    test_file $FE_TEST_CONFIG
    line=`grep "^general_token_data" $FE_TEST_CONFIG | cut -d ':' -f 2`
    -z $line && abort_test "The general token is not defined in the config file $FE_TEST_CONFIG"
}

FE_TEST_ACCESS_TOKEN=./access_token
FE_TEST_REQUEST_TOKEN=./request_token

function run_test() {
    local test_name=$1; shift
    echo ""
    echo "Test: $test_name"
    echo "Command: ./deskapp $*"
    echo "Result:"
    ./deskapp $*
    local result=$?
    if [ $result != "0" ]; then
        echo "Exit status: $?"
        abort_test "Test failed!!"
    fi
    echo "End test"
    echo ""
}

function phase1_tests() {
        run_test "Getting a request token" $* --out-token $FE_TEST_REQUEST_TOKEN --get_request_tok
        run_test "Making a authorize URL" $* --token-file $FE_TEST_REQUEST_TOKEN --get_authorize_url
}

function phase1() {
    [ -f $FE_TEST_REQUEST_TOKEN ] && abort_test "Please remove $FE_TEST_REQUEST_TOKEN to execute tests"
    if [ -z $FE_TEST_CONFIG ]; then
        # We do not have a config file.
        check_app_token_file
        phase1_tests --app-token-file $FE_TEST_APP_TOKEN $*
    else
        phase1_tests --fe_config $FE_TEST_CONFIG $*
    fi
    echo "Access the generated URL to allow the access token to be retrieved in phase 2 of the tests"
}

function phase2_tests() {
    run_test "Getting an access token" $* --token-file $FE_TEST_REQUEST_TOKEN --out-token $FE_TEST_ACCESS_TOKEN --get_access_token
    rm $FE_TEST_REQUEST_TOKEN
}

function phase2() {
    [ -f $FE_TEST_ACCESS_TOKEN ] && abort_test "Please remove $FE_TEST_ACCESS_TOKEN to execute tests"
    [ -f $FE_TEST_REQUEST_TOKEN ] || abort_test "The request token file $FE_TEST_REQUEST_TOKEN does not exist. Please run phase 1 tests first"
    if [ -z $FE_TEST_CONFIG ]; then
        # We do not have a config file.
        check_app_token_file
        phase2_tests --app-token-file $FE_TEST_APP_TOKEN $*
    else
        phase2_tests --fe_config $FE_TEST_CONFIG $*
    fi
}

function basic_apis() {
    run_test "Looking up a known location" $* --lookup q="\"$location\""
    run_test "Setting a new location" $* --update q="\"$location\""
    sleep 5 #Give some time to the update.
    run_test "Looking up current location" $* --get_location
}

function phase3_tests() {
    location="Sydney,Australia"
    basic_apis $* --token-file $FE_TEST_ACCESS_TOKEN
    location="Vancouver,Canada"
    basic_apis $* --token-file $FE_TEST_ACCESS_TOKEN
}

function phase3() {
    [ -f $FE_TEST_ACCESS_TOKEN ] || abort_test "$FE_TEST_ACCESS_TOKEN not present"
    if [ -z $FE_TEST_CONFIG ]; then
        # We do not have a config file.
        check_app_token_file
        phase3_tests --app-token-file $FE_TEST_APP_TOKEN $*
    else
        phase3_tests --fe_config $FE_TEST_CONFIG $*
    fi
}

function phase4_tests() {
    run_test "Checking recent API" $* --recent
    run_test "Checking within API" $* --within
}

function phase4() {
    [ -f $FE_TEST_ACCESS_TOKEN ] || abort_test "$FE_TEST_ACCESS_TOKEN not present"
    if [ -z $FE_TEST_CONFIG ]; then
        # We do not have a config file.
        check_app_token_file
        check_general_token_file
        local general_token=$FE_APP_GENERAL_TOKEN
        phase4_tests --app-token-file $FE_TEST_APP_TOKEN --general-token-file $general_token $*
    else
        config_has_general_token
        phase4_tests --fe_config $FE_TEST_CONFIG $*
    fi
}

choice=$1; shift
case $choice in
  phase1)
    phase1 $*
    ;;
  phase2)
    phase2 $*
    ;;
  phase3)
    phase3 $*
    ;;
  phase4)
    phase4 $*
    ;;
  *)
    usage
    ;;
esac

