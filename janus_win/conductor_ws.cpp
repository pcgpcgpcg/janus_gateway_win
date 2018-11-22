#include "conductor_ws.h"

#include <memory>
#include <utility>
#include <vector>

#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "api/test/fakeconstraints.h"
#include "defaults.h"
#include "media/engine/webrtcvideocapturerfactory.h"
#include "modules/audio_device/include/audio_device.h"
#include "modules/audio_processing/include/audio_processing.h"
#include "modules/video_capture/video_capture_factory.h"


// Names used for a IceCandidate JSON object.
const char kCandidateSdpMidName[] = "sdpMid";
const char kCandidateSdpMlineIndexName[] = "sdpMLineIndex";
const char kCandidateSdpName[] = "candidate";

// Names used for a SessionDescription JSON object.
const char kSessionDescriptionTypeName[] = "type";
const char kSessionDescriptionSdpName[] = "sdp";
const char kJanusOptName[] = "janus";

//json handle function
std::string OptString(std::string message, list<string> key);
long long int OptLLInt(std::string message, list<string> key);



ConductorWs::ConductorWs(PeerConnectionWsClient* client, MainWindow* main_wnd)
	: peer_id_(-1), loopback_(false), client_(client), main_wnd_(main_wnd) {
	client_->RegisterObserver(this);
	main_wnd->RegisterObserver(this);
}

ConductorWs::~ConductorWs() {
	for (auto &pc : m_peer_connection_map) {
		RTC_DCHECK(!pc.second);
	}
	//RTC_DCHECK(!peer_connection_);
	//TODO suitable here?
	client_->CloseJanusConn();
}

bool ConductorWs::connection_active(long long int handleId) const {
	return m_peer_connection_map.at(handleId)->peer_connection_ != nullptr;
	//return peer_connection_ != nullptr;
}

bool ConductorWs::connection_active() const {
	return m_peer_connection_map.size() > 0;
	//return peer_connection_ != nullptr;
}

//called by main_wnd receive onClose message
void ConductorWs::Close() {
	for (auto &key : m_peer_connection_map) {
		DeletePeerConnection(key.first);
	}
}
	

bool ConductorWs::InitializePeerConnection(long long int handleId,bool bPublisher) {
	RTC_DCHECK(!peer_connection_factory_);
	if (m_peer_connection_map.find(handleId) != m_peer_connection_map.end()) {
		//existing
		return false;
	}

	peer_connection_factory_ = webrtc::CreatePeerConnectionFactory(
		nullptr /* network_thread */, nullptr /* worker_thread */,
		nullptr /* signaling_thread */, nullptr /* default_adm */,
		webrtc::CreateBuiltinAudioEncoderFactory(),
		webrtc::CreateBuiltinAudioDecoderFactory(),
		webrtc::CreateBuiltinVideoEncoderFactory(),
		webrtc::CreateBuiltinVideoDecoderFactory(), nullptr /* audio_mixer */,
		nullptr /* audio_processing */);

	if (!peer_connection_factory_) {
		main_wnd_->MessageBox("Error", "Failed to initialize PeerConnectionFactory",
			true);
		DeletePeerConnection(handleId);
		return false;
	}

	if (!CreatePeerConnection(handleId,/*dtls=*/true)) {
		main_wnd_->MessageBox("Error", "CreatePeerConnection failed", true);
		DeletePeerConnection(handleId);
	}
	//subscriber no need local tracks(audio and video)
	if (bPublisher) {
		AddTracks(handleId);
	}
	m_peer_connection_map[handleId]->b_publisher_ = true;

	return m_peer_connection_map[handleId]->peer_connection_ != nullptr;
}

bool ConductorWs::CreatePeerConnection(long long int handleId,bool dtls) {
	RTC_DCHECK(peer_connection_factory_);
	if (m_peer_connection_map.find(handleId) != m_peer_connection_map.end()) {
		//existed
		return false;
	}

	webrtc::PeerConnectionInterface::RTCConfiguration config;
	config.tcp_candidate_policy = webrtc::PeerConnectionInterface::TcpCandidatePolicy::kTcpCandidatePolicyDisabled;
	config.bundle_policy = webrtc::PeerConnectionInterface::BundlePolicy::kBundlePolicyMaxBundle;
	config.rtcp_mux_policy = webrtc::PeerConnectionInterface::RtcpMuxPolicy::kRtcpMuxPolicyRequire;
	config.continual_gathering_policy = webrtc::PeerConnectionInterface::ContinualGatheringPolicy::GATHER_CONTINUALLY;
	config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
	config.enable_dtls_srtp = dtls;
	webrtc::PeerConnectionInterface::IceServer server;
	server.uri = GetPeerConnectionString();
	config.servers.push_back(server);

	webrtc::PeerConnectionInterface::BitrateParameters bitrateParam;
	bitrateParam.min_bitrate_bps = absl::optional<int>(128000);
	bitrateParam.current_bitrate_bps = absl::optional<int>(256000);
	bitrateParam.max_bitrate_bps = absl::optional<int>(512000);

	rtc::scoped_refptr<PeerConnection> peer_connection(
		new rtc::RefCountedObject<PeerConnection>());

	peer_connection->peer_connection_= peer_connection_factory_->CreatePeerConnection(
		config, nullptr, nullptr, peer_connection);
	//set max/min bitrate
	peer_connection->peer_connection_->SetBitrate(bitrateParam);
	//add to the map
	peer_connection->RegisterObserver(this);
	peer_connection->SetHandleId(handleId);
	m_peer_connection_map[handleId] = peer_connection;

	return m_peer_connection_map[handleId]->peer_connection_ != nullptr;
}

void ConductorWs::DeletePeerConnection(long long int handleId) {
	if (m_peer_connection_map[handleId]->b_publisher_) {
		main_wnd_->StopLocalRenderer();
	}
	main_wnd_->StopRemoteRenderer();
	m_peer_connection_map[handleId]->peer_connection_ = nullptr;
	//peer_connection_factory_ = nullptr; //TODO should destroy before quit
}

void ConductorWs::EnsureStreamingUI() {
	if (main_wnd_->IsWindow()) {
		if (main_wnd_->current_ui() != MainWindow::STREAMING)
			main_wnd_->SwitchToStreamingUI();
	}
}

//
// peerconnectionCallback implementation.
//

void ConductorWs::PCSendSDP(long long int handleId, std::string sdpType, std::string sdp) {
	SendOffer(handleId, sdpType, sdp);
}

void ConductorWs::PCQueueUIThreadCallback(int msg_id, void* data){
	main_wnd_->QueueUIThreadCallback(msg_id,
		data);
}
void ConductorWs::PCTrickleCandidate(long long int handleId, const webrtc::IceCandidateInterface* candidate) {
	trickleCandidate(handleId, candidate);
}
void ConductorWs::PCTrickleCandidateComplete(long long int handleId) {
	trickleCandidateComplete(handleId);
}
//
// PeerConnectionClientObserver implementation.
//

void ConductorWs::OnSignedIn() {
	RTC_LOG(INFO) << __FUNCTION__;
	main_wnd_->SwitchToPeerList(client_->peers());
}

//TODO transport layer should emit the event while disconnected
void ConductorWs::OnJanusDisconnected() {
	//shift the thread from ws to ui
	main_wnd_->QueueUIThreadCallback(PEER_CONNECTION_CLOSED, NULL);
}
void ConductorWs::OnDisconnected() {
	RTC_LOG(INFO) << __FUNCTION__;

	for (auto &key : m_peer_connection_map) {
		DeletePeerConnection(key.first);
	}

	if (main_wnd_->IsWindow())
		main_wnd_->SwitchToConnectUI();
}

void ConductorWs::OnPeerConnected(int id, const std::string& name) {
	RTC_LOG(INFO) << __FUNCTION__;
	// Refresh the list if we're showing it.
	if (main_wnd_->current_ui() == MainWindow::LIST_PEERS)
		main_wnd_->SwitchToPeerList(client_->peers());
}

void ConductorWs::OnServerConnectionFailure() {
	main_wnd_->MessageBox("Error", ("Failed to connect to " + server_).c_str(),
		true);
}

void ConductorWs::ConnectToPeer(int peer_id) {

}

void ConductorWs::OnMessageSent(int err) {

}

//
// MainWndCallback implementation.
//

void ConductorWs::StartLogin(const std::string& server, int port) {
	if (client_->is_connected())
		return;
	server_ = server;
	auto ws_server = std::string("ws://") + server + std::string(":") + std::to_string(port);

	client_->Connect(ws_server, "1111");
}

void ConductorWs::DisconnectFromServer() {
	
}

std::unique_ptr<cricket::VideoCapturer> ConductorWs::OpenVideoCaptureDevice() {
	std::vector<std::string> device_names;
	{
		std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
			webrtc::VideoCaptureFactory::CreateDeviceInfo());
		if (!info) {
			return nullptr;
		}
		int num_devices = info->NumberOfDevices();
		for (int i = 0; i < num_devices; ++i) {
			const uint32_t kSize = 256;
			char name[kSize] = { 0 };
			char id[kSize] = { 0 };
			if (info->GetDeviceName(i, name, kSize, id, kSize) != -1) {
				device_names.push_back(name);
			}
		}
	}

	cricket::WebRtcVideoDeviceCapturerFactory factory;
	std::unique_ptr<cricket::VideoCapturer> capturer;
	for (const auto& name : device_names) {
		capturer = factory.Create(cricket::Device(name, 0));
		if (capturer) {
			break;
		}
	}
	return capturer;
}

void ConductorWs::AddTracks(long long int handleId) {
	if (!m_peer_connection_map[handleId]->peer_connection_->GetSenders().empty()) {
		return;  // Already added tracks.
	}

	rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track(
		peer_connection_factory_->CreateAudioTrack(
			kAudioLabel, peer_connection_factory_->CreateAudioSource(
				cricket::AudioOptions())));
	auto result_or_error = m_peer_connection_map[handleId]->peer_connection_->AddTrack(audio_track, { kStreamId });
	if (!result_or_error.ok()) {
		RTC_LOG(LS_ERROR) << "Failed to add audio track to PeerConnection: "
			<< result_or_error.error().message();
	}

	std::unique_ptr<cricket::VideoCapturer> video_device =
		OpenVideoCaptureDevice();
	if (video_device) {
		webrtc::FakeConstraints constraints;
		std::list<std::string> keyList = { webrtc::MediaConstraintsInterface::kMinWidth, webrtc::MediaConstraintsInterface::kMaxWidth,
			webrtc::MediaConstraintsInterface::kMinHeight, webrtc::MediaConstraintsInterface::kMaxHeight,
			webrtc::MediaConstraintsInterface::kMinFrameRate, webrtc::MediaConstraintsInterface::kMaxFrameRate,
			webrtc::MediaConstraintsInterface::kMinAspectRatio, webrtc::MediaConstraintsInterface::kMaxAspectRatio };

		//set media constraints
		std::map<std::string, std::string> opts;
		opts[webrtc::MediaConstraintsInterface::kMaxFrameRate] = 18;
		opts[webrtc::MediaConstraintsInterface::kMaxWidth] = 1280;
		opts[webrtc::MediaConstraintsInterface::kMaxHeight] = 720;

		for (auto key : keyList) {
			if (opts.find(key) != opts.end()) {
				constraints.AddMandatory(key, opts.at(key));
			}
		}
		rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track_(
			peer_connection_factory_->CreateVideoTrack(
				kVideoLabel, peer_connection_factory_->CreateVideoSource(
					std::move(video_device), nullptr)));
		main_wnd_->StartLocalRenderer(video_track_);

		result_or_error = m_peer_connection_map[handleId]->peer_connection_->AddTrack(video_track_, { kStreamId });
		if (!result_or_error.ok()) {
			RTC_LOG(LS_ERROR) << "Failed to add video track to PeerConnection: "
				<< result_or_error.error().message();
		}
	}
	else {
		RTC_LOG(LS_ERROR) << "OpenVideoCaptureDevice failed";
	}

	main_wnd_->SwitchToStreamingUI();
}

void ConductorWs::DisconnectFromCurrentPeer() {
	RTC_LOG(INFO) << __FUNCTION__;
	/*if (peer_connection_.get()) {
		DeletePeerConnection();
	}*/
	for (auto &key : m_peer_connection_map) {
		DeletePeerConnection(key.first);
	}

	if (main_wnd_->IsWindow())
		main_wnd_->SwitchToConnectUI();
}

void ConductorWs::UIThreadCallback(int msg_id, void* data) {
	switch (msg_id) {
	case PEER_CONNECTION_CLOSED:
		RTC_LOG(INFO) << "PEER_CONNECTION_CLOSED";
		for (auto &key : m_peer_connection_map) {
			DeletePeerConnection(key.first);
		}
		
		if (main_wnd_->IsWindow()) {
				main_wnd_->SwitchToConnectUI();
		}
		else {
			DisconnectFromServer();
		}
		break;

	case NEW_TRACK_ADDED: {
		auto* track = reinterpret_cast<webrtc::MediaStreamTrackInterface*>(data);
		if (track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
			auto* video_track = static_cast<webrtc::VideoTrackInterface*>(track);
			main_wnd_->StartRemoteRenderer(video_track);
		}
		track->Release();
		break;
	}

	case TRACK_REMOVED: {
		// Remote peer stopped sending a track.
		auto* track = reinterpret_cast<webrtc::MediaStreamTrackInterface*>(data);
		track->Release();
		break;
	}

	case CREATE_OFFER: {
		long long int handleId = (long long int)(data);
		if (InitializePeerConnection(handleId,true)) {
			m_peer_connection_map[handleId]->CreateOffer();
		}
		else {
			main_wnd_->MessageBox("Error", "Failed to initialize PeerConnection", true);
		}
		break;
	}

	case SET_REMOTE_SDP: {
		REMOTE_SDP_INFO* pInfo = (REMOTE_SDP_INFO *)(data);
		long long int handleId = pInfo->handleId;
		std::string jsep_str = pInfo->jsep_str;
		m_peer_connection_map[handleId]->CreateSessionSDP(jsep_str);
		std::unique_ptr<webrtc::SessionDescriptionInterface> session_description =
			webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer, jsep_str);
		//auto* session_description = reinterpret_cast<webrtc::SessionDescriptionInterface*>(data);
		//auto* session_description = reinterpret_cast<webrtc::SessionDescriptionInterface*>(pInfo->pInterface);	
		m_peer_connection_map[handleId]->SetRemoteDescription(session_description.release());
		//delete pInfo;
		//TODO fixme suitable here?
		SendBitrateConstraint(handleId);
		break;
	}

	default:
		RTC_NOTREACHED();
		break;
	}
}

/*----------------------------------------------------------------*/
/*-----------------janus protocol implementation------------------*/
void ConductorWs::OnJanusConnected() {
	CreateSession();
}

void ConductorWs::OnSendKeepAliveToJanus() {
	KeepAlive();
}

void ConductorWs::KeepAlive() {
	if (m_SessionId > 0) {
		std::string transactionID = RandomString(12);
		Json::StyledWriter writer;
		Json::Value jmessage;

		jmessage["janus"] = "keepalive";
		jmessage["session_id"] = m_SessionId;
		jmessage["transaction"] = transactionID;
		client_->SendToJanus(writer.write(jmessage));
	}
}

void ConductorWs::CreateSession() {

	int rev_tid1 = GetCurrentThreadId();
	std::string transactionID=RandomString(12);
	std::shared_ptr<JanusTransaction> jt(new JanusTransaction());
	jt->transactionId = transactionID;

	//TODO Is it possible for lamda expression here?
	jt->Success = [=](std::string message) mutable {		
		list<string> sessionList = {"data","id" };
		m_SessionId = OptLLInt(message, sessionList);
		//lauch the timer for keep alive breakheart
		//Then Create the handle
		CreateHandle("janus.plugin.echotest",0,"pcg");
	};

	jt->Error = [=](std::string code, std::string reason) {
		RTC_LOG(INFO) << "Ooops: " << code << " " << reason;
	};

	m_transactionMap[transactionID]=jt;

	Json::StyledWriter writer;
	Json::Value jmessage;
	jmessage["janus"] = "create";
	jmessage["transaction"] = transactionID;
	client_->SendToJanus(writer.write(jmessage));
}

//publisher send attach
void ConductorWs::CreateHandle(std::string pluginName, long long int feedId, std::string display) {
	std::string transactionID = RandomString(12);
	std::shared_ptr<JanusTransaction> jt(new JanusTransaction());
	jt->transactionId = transactionID;
	jt->Success = [=](std::string message) {
		list<string> handleList = { "data","id" };
		long long int handle_id = OptLLInt(message, handleList);
		//add handle to the map
		std::shared_ptr<JanusHandle> jh(new JanusHandle());
		jh->handleId = handle_id;
		jh->display = display;
		jh->feedId = feedId;
		m_handleMap[handle_id] = jh;
		JoinRoom(pluginName, handle_id, feedId);//TODO feedid means nothing in echotest,else?
	};

	jt->Event = [=](std::string message) {

	};

	jt->Error = [=](std::string, std::string) {
		RTC_LOG(INFO) << "CreateHandle error:";
	};

	m_transactionMap[transactionID] = jt;

	Json::StyledWriter writer;
	Json::Value jmessage;
	jmessage["janus"] = "attach";
	jmessage["plugin"] = pluginName;
	jmessage["transaction"] = transactionID;
	jmessage["session_id"] = m_SessionId;
	client_->SendToJanus(writer.write(jmessage));
}


void ConductorWs::JoinRoom(std::string pluginName,long long int handleId,long long int feedId) {
	//rtcEvents.onPublisherJoined(handle.handleId);
	std::string transactionID = RandomString(12);
	std::shared_ptr<JanusTransaction> jt(new JanusTransaction());
	jt->transactionId = transactionID;

	jt->Event = [=](std::string message) {
		//get sender
		list<string> senderList = { "sender" };
		long long int sender = OptLLInt(message, senderList);
		//get room
		list<string> resultList = { "plugindata","data","result" };
		list<string> roomList = { "plugindata","data","videoroom" };
		
		//echotest return result=ok
		std::string result = OptString(message, resultList);
		if (result == "ok") {
			RTC_LOG(WARNING) << "echotest negotiation ok! ";
		}
		
		std::string videoroom= OptString(message, roomList);
		//joined the room as a publisher
		if (videoroom == "joined") {
			main_wnd_->QueueUIThreadCallback(CREATE_OFFER, (void*)(&handleId));
		}
		//joined the room as a subscriber
		if (videoroom == "attached") {
			//parse the remote sdp
			//events.onRemoteJsep(handleId, jsep);
		}
	};

	m_transactionMap[transactionID] = jt;

	Json::StyledWriter writer;
	Json::Value jmessage;
	Json::Value jbody;
	if (pluginName == "janus.plugin.videoroom") {
		jbody["request"] = "join";
		jbody["room"] = "1234";//FIXME should be variable
		if (feedId == 0) {
			jbody["ptype"] = "publisher";
			jbody["display"] = "pcg";//FIXME should be variable
		}
		else {
			jbody["ptype"] = "subscriber";
			jbody["feed"] = feedId;
			jbody["private_id"] = 0;//FIXME should be variable
		}
		jmessage["body"] = jbody;
		jmessage["janus"] = "message";
		jmessage["transaction"] = transactionID;
		jmessage["session_id"] = m_SessionId;
		jmessage["handle_id"] = handleId;
	}
	else if (pluginName == "janus.plugin.audiobridge") {

	}
	else if (pluginName == "janus.plugin.echotest") {
		jbody["audio"] = true;
		jbody["video"] = true;
		jmessage["body"] = jbody;
		jmessage["janus"] = "message";
		jmessage["transaction"] = transactionID;
		jmessage["session_id"] = m_SessionId;
		jmessage["handle_id"] = handleId;
		client_->SendToJanus(writer.write(jmessage));
		//shift the process to UI thread to createOffer
		main_wnd_->QueueUIThreadCallback(CREATE_OFFER, (void*)(handleId));
	}
	
	
}

void ConductorWs::SendOffer(long long int handleId, std::string sdp_type,std::string sdp_desc) {
	std::string transactionID = RandomString(12);
	std::shared_ptr<JanusTransaction> jt(new JanusTransaction());
	jt->transactionId = transactionID;

	jt->Event = [=](std::string message) {
		list<string> resultList = { "jsep","sdp" };
		std::string jsep_str=OptString(message, resultList);
		REMOTE_SDP_INFO* pInfo = new REMOTE_SDP_INFO;
		pInfo->handleId = handleId;
		pInfo->jsep_str = jsep_str;
		std::unique_ptr<webrtc::SessionDescriptionInterface> session_description =
			webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer, jsep_str);
		main_wnd_->QueueUIThreadCallback(SET_REMOTE_SDP, pInfo);
	
		/*REMOTE_SDP_INFO *rsInfo=new REMOTE_SDP_INFO;
		rsInfo->handleId = handleId;
		rsInfo->pInterface = session_description.get();
		main_wnd_->QueueUIThreadCallback(SET_REMOTE_SDP, (void*)(rsInfo));*/
	};

	m_transactionMap[transactionID] = jt;

	Json::StyledWriter writer;
	Json::Value jmessage;
	Json::Value jbody;
	Json::Value jjsep;

	jbody["request"] = "configure";
	jbody["audio"] = true;
	jbody["video"] = true;

	jjsep["type"] = sdp_type;
	jjsep["sdp"] = sdp_desc;

	jmessage["body"] = jbody;
	jmessage["jsep"] = jjsep;
	jmessage["janus"] = "message";
	jmessage["transaction"] = transactionID;
	jmessage["session_id"] = m_SessionId;
	jmessage["handle_id"] = handleId;
	//beacause the thread is on UI,so shift thread to ws thread
	client_->SendToJanusAsync(writer.write(jmessage));
}

void ConductorWs::trickleCandidate(long long int handleId, const webrtc::IceCandidateInterface* candidate) {
	std::string transactionID = RandomString(12);
	Json::StyledWriter writer;
	Json::Value jmessage;
	Json::Value jcandidate;

	std::string sdp;
	if (!candidate->ToString(&sdp)) {
		RTC_LOG(LS_ERROR) << "Failed to serialize candidate";
		return;
	}

	jcandidate["sdpMid"]= candidate->sdp_mid();
	jcandidate["sdpMLineIndex"] = candidate->sdp_mline_index();
	jcandidate["candidate"] = sdp;

	jmessage["janus"] = "trickle";
	jmessage["candidate"] = jcandidate;	
	jmessage["transaction"] = transactionID;
	jmessage["session_id"] = m_SessionId;
	jmessage["handle_id"] = handleId;
	client_->SendToJanusAsync(writer.write(jmessage));
}

void ConductorWs::trickleCandidateComplete(long long int handleId) {
	std::string transactionID = RandomString(12);
	Json::StyledWriter writer;
	Json::Value jmessage;
	Json::Value jcandidate;

	jcandidate["completed"] = true;

	jmessage["janus"] = "trickle";
	jmessage["candidate"] = jcandidate;
	jmessage["transaction"] = transactionID;
	jmessage["session_id"] = m_SessionId;
	jmessage["handle_id"] = handleId;
	client_->SendToJanusAsync(writer.write(jmessage));
}

void ConductorWs::SendBitrateConstraint(long long int handleId) {
	std::string transactionID = RandomString(12);
	std::shared_ptr<JanusTransaction> jt(new JanusTransaction());
	jt->transactionId = transactionID;

	jt->Event = [=](std::string message) {
		//parse json
		Json::Reader reader;
		Json::Value jmessage;
		if (!reader.parse(message, jmessage)) {
			RTC_LOG(WARNING) << "Received unknown message. " << message;
			return;
		}
		std::string janus_str;
		std::string json_object;
		Json::Value janus_value;
		Json::Value janus_value_sub1;
		std::string jsep_str;
		//see if has remote jsep
		if (rtc::GetValueFromJsonObject(jmessage, "plugindata",
			&janus_value)) {
			if (rtc::GetValueFromJsonObject(janus_value, "data",
				&janus_value_sub1)) {
				if (rtc::GetStringFromJsonObject(janus_value_sub1, "result",
					&jsep_str)) {
					if (jsep_str != "ok") {
						//没有设置成功
					}
				}
			}
			
		}
	};
	m_transactionMap[transactionID] = jt;

	Json::StyledWriter writer;
	Json::Value jmessage;
	Json::Value jbody;

	jbody["bitrate"] = 128000;

	jmessage["janus"] = "message";
	jmessage["body"] = jbody;
	jmessage["transaction"] = transactionID;
	jmessage["session_id"] = m_SessionId;
	jmessage["handle_id"] = handleId;
	client_->SendToJanusAsync(writer.write(jmessage));
}



//because janus self act as an end,so always define peer_id=0
void ConductorWs::OnMessageFromJanus(int peer_id, const std::string& message) {
	RTC_DCHECK(!message.empty());
	RTC_LOG(INFO) << "got msg: " << message;
	//TODO make sure in right state
	//parse json
	Json::Reader reader;
	Json::Value jmessage;
	if (!reader.parse(message, jmessage)) {
		RTC_LOG(WARNING) << "Received unknown message. " << message;
		return;
	}
	std::string janus_str;
	std::string json_object;

	rtc::GetStringFromJsonObject(jmessage, "janus",
		&janus_str);
	if (!janus_str.empty()) {
		if (janus_str == "ack") {
			// Just an ack, we can probably ignore
			RTC_LOG(INFO) << "Got an ack on session. ";
		}
		else if (janus_str == "success") {
			rtc::GetStringFromJsonObject(jmessage, "transaction",
				&janus_str);
			std::shared_ptr<JanusTransaction> jt = m_transactionMap.at(janus_str);
			//call signal
			if (jt) {
				jt->Success(message);//handle_id not ready yet
			}
			m_transactionMap.erase(janus_str);
		}
		else if (janus_str == "trickle") {
			RTC_LOG(INFO) << "Got a trickle candidate from Janus. ";
		}
		else if (janus_str == "webrtcup") {
			RTC_LOG(INFO) << "The PeerConnection with the gateway is up!";
		}
		else if (janus_str == "hangup") {
			RTC_LOG(INFO) << "A plugin asked the core to hangup a PeerConnection on one of our handles! ";
		}
		else if (janus_str == "detached") {
			RTC_LOG(INFO) << "A plugin asked the core to detach one of our handles! ";
		}
		else if (janus_str == "media") {
			RTC_LOG(INFO) << "Media started/stopped flowing. ";
		}
		else if (janus_str == "slowlink") {
			RTC_LOG(INFO) << "Got a slowlink event! ";
		}
		else if (janus_str == "error") {
			RTC_LOG(INFO) << "Got an error. ";
			// Oops, something wrong happened
			/*rtc::GetStringFromJsonObject(jmessage, "transaction",
			&janus_str);
			std::shared_ptr<JanusTransaction> jt = m_transactionMap.at(janus_str);
			//call signal
			if (jt) {
			jt->Error("123","456");//TODO need to parse the error code and desc
			}
			m_transactionMap.erase(janus_str);*/
		}
		else {

			if (janus_str == "event") {
				RTC_LOG(INFO) << "Got a plugin event! ";

				bool bSuccess = rtc::GetStringFromJsonObject(jmessage, "transaction",
					&janus_str);
				if (bSuccess) {
					std::shared_ptr<JanusTransaction> jt = m_transactionMap.at(janus_str);
					if (jt) {
						jt->Event(message);
					}
				}
			}
		}
	}
}

//json handle functions

//jmessage:the json to be parse
//keylist: the recursive key from parent to sub
std::string OptString(std::string message, list<string> keyList) {
	//parse json
	Json::Reader reader;
	Json::Value jmessage;
	if (!reader.parse(message, jmessage)) {
		RTC_LOG(WARNING) << "Received unknown message. " << message;
		return std::string("");//FIXME should return by another param with type enum
	}
	Json::Value jvalue=jmessage;
	Json::Value jvalue2;
	for (auto key: keyList) {
		if (rtc::GetValueFromJsonObject(jvalue, key,
			&jvalue2)) {
			jvalue = jvalue2;
		}
		else {
			return std::string("");
		}
	}
	std::string tmp_str = rtc::JsonValueToString(jvalue);
	return tmp_str;
}
//jmessage:the json to be parse
//keylist: the recursive key from parent to sub
long long int OptLLInt(std::string message, list<string> keyList) {
	//parse json
	Json::Reader reader;
	Json::Value jmessage;
	if (!reader.parse(message, jmessage)) {
		RTC_LOG(WARNING) << "Received unknown message. " << message;
		return 0;//FIXME should return by another param with type enum
	}
	Json::Value jvalue = jmessage;
	Json::Value jvalue2;
	for (auto key : keyList) {
		if (rtc::GetValueFromJsonObject(jvalue, key,
			&jvalue2)) {
			jvalue = jvalue2;
		}
		else {
			return 0;
		}
	}
	std::string tmp_str = rtc::JsonValueToString(jvalue);
	return std::stoll(tmp_str);
}


//we have arrived at OnLocalStream and OnRemoteSteam
//thread problem should fix when debug


