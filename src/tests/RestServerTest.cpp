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

#include "gmock/gmock.h"
#include "httpserver/RestServer.hpp"
#include <zmq.hpp>

#define private public
#define protected public

using namespace std;
using namespace testing;

zmq::context_t context(1);

class AnRestServer : public Test {
public:
	RestServer server;
};

TEST_F(AnRestServer, jsonCheckTest_right_mon) {
	const char* json = "{\"instances\":1,\"profile\":\"mon\"}";
	string out = server.jsonCheck(json);
	ASSERT_THAT(out, Eq("[success]"));
}

TEST_F(AnRestServer, jsonCheckTest_right_osd) {
	const char* json = "{\"instances\":1,\"profile\":\"osd\"}";
	string out = server.jsonCheck(json);
	ASSERT_THAT(out, Eq("[success]"));
}

TEST_F(AnRestServer, jsonCheckTest_instances_error) {
	const char* json = "{\"error\":1,\"profile\":\"osd\"}";
	string out = server.jsonCheck(json);
	ASSERT_THAT(out, Eq("[error]:instances"));
}

TEST_F(AnRestServer, jsonCheckTest_instances_num_error) {
	const char* json = "{\"instances\":2222,\"profile\":\"osd\"}";
	string out = server.jsonCheck(json);
	ASSERT_THAT(out, Eq("[error]:instances"));
}

TEST_F(AnRestServer, jsonCheckTest_profile_error) {
	const char* json = "{\"instances\":1,\"error\":\"osd\"}";
	string out = server.jsonCheck(json);
	ASSERT_THAT(out, Eq("[error]:profile"));
}

TEST_F(AnRestServer, jsonCheckTest_profile_type_error) {
	const char* json = "{\"instances\":1,\"profile\":\"error\"}";
	string out = server.jsonCheck(json);
	ASSERT_THAT(out, Eq("[error]:profile"));
}

