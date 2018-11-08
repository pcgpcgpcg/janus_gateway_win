#include "peer_connection_wsclient.h"
#include "defaults.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/nethelpers.h"
#include "rtc_base/stringutils.h"

#include "rtc_base/json.h"

#ifdef WIN32
#include "rtc_base/win32socketserver.h"
#endif

using rtc::sprintfn;

namespace {

	// This is our magical hangup signal.
	const char kByeMessage[] = "BYE";
	// Delay between server connection retries, in milliseconds
	const int kReconnectDelay = 2000;

	rtc::AsyncSocket* CreateClientSocket(int family) {
#ifdef WIN32
		rtc::Win32Socket* sock = new rtc::Win32Socket();
		sock->CreateT(family, SOCK_STREAM);
		return sock;
#elif defined(WEBRTC_POSIX)
		rtc::Thread* thread = rtc::Thread::Current();
		RTC_DCHECK(thread != NULL);
		return thread->socketserver()->CreateAsyncSocket(family, SOCK_STREAM);
#else
#error Platform not supported.
#endif
	}

}  // namespace

PeerConnectionWsClient::PeerConnectionWsClient()
	: callback_(NULL), resolver_(NULL), state_(NOT_CONNECTED), my_id_(-1) {
	//m_ws = NULL;
}

PeerConnectionWsClient::~PeerConnectionWsClient() {}


int PeerConnectionWsClient::id() const {
	return my_id_;
}

bool PeerConnectionWsClient::is_connected() const {
	return my_id_ != -1;
}

const Peers& PeerConnectionWsClient::peers() const {
	return peers_;
}

void PeerConnectionWsClient::RegisterObserver(
	PeerConnectionWsClientObserver* callback) {
	RTC_DCHECK(!callback_);
	callback_ = callback;
}


//uWs init and connnect
void PeerConnectionWsClient::Connect(const std::string& server,
	const std::string& client_id) {
	int rev_tid1 = GetCurrentThreadId();
	m_async = new uS::Async(m_hub.getLoop());
	m_async->setData((void*)this);
	//make sure this lambda run in ws thread
	m_async->start([](uS::Async *a) {
		int rev_tid3 = GetCurrentThreadId();
		PeerConnectionWsClient* pws = (PeerConnectionWsClient*)a->getData();
		pws->m_ws->send(pws->m_msg_to_send.c_str(), uWS::TEXT);
	});
	//create websocket thread
	std::thread t([this,server]() {
		this->m_hub.onError([](void *user) {
			int error_code = (long)user;
			PeerConnectionWsClient* pws = (PeerConnectionWsClient*)user;
			if (pws->state_ != NOT_CONNECTED) {
				RTC_LOG(WARNING)
					<< "The client must not be connected before you can call Connect()";
				pws->callback_->OnServerConnectionFailure();
			}
		});
		//connection handler
		this->m_hub.onConnection([](uWS::WebSocket<uWS::CLIENT> *ws, uWS::HttpRequest req) {
			//get user data
			int rev_tid2 = GetCurrentThreadId();
			PeerConnectionWsClient* pws = (PeerConnectionWsClient*)(ws->getUserData());
			pws->m_ws = ws;
			if (pws->state_ == NOT_CONNECTED) {
				RTC_LOG(WARNING) << "Client established a remote connection over non-SSL";
				pws->state_ = CONNECTED;
				pws->callback_->OnJanusConnected();
			}
		});

		this->m_hub.onDisconnection([](uWS::WebSocket<uWS::CLIENT> *ws, int code, char *message, size_t length) {
			RTC_LOG(WARNING) << "Client got disconnected";
			std::cout << "Client got disconnected with data: " << ws->getUserData() << ", code: " << code << ", message: <" << std::string(message, length) << ">" << std::endl;
		});

		this->m_hub.onMessage([](uWS::WebSocket<uWS::CLIENT> *ws, char *message, size_t length, uWS::OpCode opCode) {
			PeerConnectionWsClient* pws = (PeerConnectionWsClient*)(ws->getUserData());
			pws->handleMessages(message, length);
		});

		std::map<std::string, std::string> protocol_map;
		protocol_map.insert(std::pair<std::string, std::string>(std::string("Sec-WebSocket-Protocol"), std::string("janus-protocol")));
		this->m_hub.connect(server, (void*)this, protocol_map);
		this->m_hub.run();

	});
	ws_thread = std::move(t);
}

void PeerConnectionWsClient::SendToJanusAsync(const std::string& message) {
	if (state_ != CONNECTED)
		return;
	if (m_ws) {
		m_msg_to_send = message;
		m_async->send();
	}
}

void PeerConnectionWsClient::SendToJanus(const std::string& message) {
	if (state_ != CONNECTED)
		return;
	if (m_ws) {
		RTC_LOG(INFO) << "send wsmsg:" << message;
		m_ws->send(message.c_str(), uWS::TEXT);
	}
}




void PeerConnectionWsClient::handleMessages(char* message, size_t length) {
	//解析这个message
	callback_->OnMessageFromJanus(0, std::string(message));
}

void PeerConnectionWsClient::OnMessage(rtc::Message* msg) {
	// ignore msg; there is currently only one supported message ("retry")
	//这里需要实现断线重新连接
	//DoConnect();
}
