#pragma once
//make uWebSockets as signal channel
#include <map>
#include <memory>
#include <string>
#include <iostream>
#include <cmath>

#include "rtc_base/nethelpers.h"
#include "rtc_base/physicalsocketserver.h"
#include "rtc_base/signalthread.h"
#include "rtc_base/third_party/sigslot/sigslot.h"

#include "uWs.h"

typedef std::map<int, std::string> Peers;

struct PeerConnectionWsClientObserver {
	virtual void OnSignedIn() = 0;  // Called when we're logged on.
	virtual void OnDisconnected() = 0;
	virtual void OnPeerConnected(int id, const std::string& name) = 0;
	virtual void OnPeerDisconnected(int peer_id) = 0;
	virtual void OnMessageFromJanus(int peer_id, const std::string& message) = 0;
	virtual void OnMessageSent(int err) = 0;
	virtual void OnServerConnectionFailure() = 0;
	virtual void OnJanusConnected() = 0;

protected:
	virtual ~PeerConnectionWsClientObserver() {}
};

class PeerConnectionWsClient : public sigslot::has_slots<>,
	public rtc::MessageHandler {
public:
	enum State {
		NOT_CONNECTED,
		RESOLVING,
		SIGNING_IN,
		CONNECTED,
		SIGNING_OUT_WAITING,
		SIGNING_OUT,
	};

	PeerConnectionWsClient();
	~PeerConnectionWsClient();

	int id() const;
	bool is_connected() const;
	const Peers& peers() const;

	void RegisterObserver(PeerConnectionWsClientObserver* callback);

	void handleMessages(char* message, size_t length);

	void Connect(const std::string& server,
		const std::string& client_id);

	// implements the MessageHandler interface
	void OnMessage(rtc::Message* msg);

public:
	PeerConnectionWsClientObserver* callback_;
	rtc::SocketAddress server_address_;
	rtc::AsyncResolver* resolver_;
	std::unique_ptr<rtc::AsyncSocket> control_socket_;
	std::unique_ptr<rtc::AsyncSocket> hanging_get_;
	std::string onconnect_data_;
	std::string control_data_;
	std::string notification_data_;
	std::string client_name_;
	Peers peers_;
	std::thread ws_thread;
	uWS::Hub m_hub;
	uWS::WebSocket<uWS::CLIENT> *m_ws = nullptr;
	uS::Async *m_async;
	std::string m_msg_to_send;
public:
	State state_;
	int my_id_;
public:
	void SendToJanus(const std::string& message);
	void SendToJanusAsync(const std::string& message);
};


