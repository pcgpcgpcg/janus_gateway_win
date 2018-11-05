/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "defaults.h"

#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#endif

#include "rtc_base/arraysize.h"

const char kAudioLabel[] = "audio_label";
const char kVideoLabel[] = "video_label";
const char kStreamId[] = "stream_id";
const uint16_t kDefaultServerPort = 8188;

std::string GetEnvVarOrDefault(const char* env_var_name,
                               const char* default_value) {
  std::string value;
  size_t len;
  const char* env_var = getenv(env_var_name);
  if (env_var)
    value = env_var;

  if (value.empty())
    value = default_value;

  return value;
}

std::string GetPeerConnectionString() {
  return GetEnvVarOrDefault("WEBRTC_CONNECT", "stun:stun.l.google.com:19302");
}

std::string GetDefaultServerName() {
  return GetEnvVarOrDefault("WEBRTC_SERVER", "39.106.100.180");
}

std::string GetPeerName() {
  char computer_name[256];
  std::string ret(GetEnvVarOrDefault("USERNAME", "user"));
  ret += '@';
  if (gethostname(computer_name, arraysize(computer_name)) == 0) {
    ret += computer_name;
  } else {
    ret += "host";
  }
  return ret;
}

//c++ 11 random
std::string RandomString(int len) {
	std::string charSet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
	std::string randomString = "";
	std::random_device rd;
	std::default_random_engine e(rd());
	std::uniform_int_distribution<unsigned> u(0, charSet.length());
	srand((int)time(0));
	for (int i = 0; i < len; i++) {
		double rand_num = rand() / double(RAND_MAX);
		int randomPoz = u(e) % charSet.length();
		//long double randomPoz = std::floor(rand_num*(charSet.length));
		randomString += charSet.substr(randomPoz, 1);
	}
	return randomString;
}

//old c style rand
std::string RandomString2(int len) {
	std::string charSet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
	std::string randomString = "";
	srand((int)time(0));
	for (int i = 0; i < len; i++) {
		double rand_num = rand() / double(RAND_MAX);
		int randomPoz = rand() % charSet.length();
		//long double randomPoz = std::floor(rand_num*(charSet.length));
		randomString += charSet.substr(randomPoz, 1);
	}
	return randomString;
}
