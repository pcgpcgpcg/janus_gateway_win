#pragma once

#include <deque>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <list>

#include "api/mediastreaminterface.h"
#include "api/peerconnectioninterface.h"
#include "main_wnd.h"
#include "peer_connection_wsclient.h"
#include "JanusTransaction.h"
#include "JanusHandle.h"

#include "defaults.h"

#include "rtc_base/checks.h"
#include "rtc_base/json.h"
#include "rtc_base/logging.h"

#include "peer_connection.h"

using namespace std;

namespace webrtc {
	class VideoCaptureModule;
}  // namespace webrtc

namespace cricket {
	class VideoRenderer;
}  // namespace cricket

class ConductorWs : public sigslot::has_slots<>,
	public webrtc::PeerConnectionObserver,
	public webrtc::CreateSessionDescriptionObserver,
	public PeerConnectionWsClientObserver,
	public MainWndCallback,
    public PeerConnectionCallback {
public:

	ConductorWs(PeerConnectionWsClient* client, MainWindow* main_wnd);

	bool connection_active(long long int handleId) const;

	void Close() override;

protected:
	~ConductorWs();
	bool InitializePeerConnection(long long int handleId, bool bPublisher);
	bool CreatePeerConnection(long long int handleId,bool dtls);
	void DeletePeerConnection(long long int handleId);
	void EnsureStreamingUI();
	void AddTracks(long long int handleId);
	std::unique_ptr<cricket::VideoCapturer> OpenVideoCaptureDevice();

	//
	// PeerConnectionObserver implementation.
	//

	void OnSignalingChange(
		webrtc::PeerConnectionInterface::SignalingState new_state) override {};
	void OnAddTrack(
		rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
		const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&
		streams) override;
	void OnRemoveTrack(
		rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;
	void OnDataChannel(
		rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override {}
	void OnRenegotiationNeeded() override {}
	void OnIceConnectionChange(
		webrtc::PeerConnectionInterface::IceConnectionState new_state) override {};
	void OnIceGatheringChange(
		webrtc::PeerConnectionInterface::IceGatheringState new_state) override {};
	void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
	void OnIceConnectionReceivingChange(bool receiving) override {}

	//
	// PeerConnectionClientObserver implementation.
	//

	void OnSignedIn() override;

	void OnDisconnected() override;

	void OnPeerConnected(int id, const std::string& name) override;

	void OnMessageFromJanus(int peer_id, const std::string& message) override;

	void OnMessageSent(int err) override;

	void OnServerConnectionFailure() override;

	void OnJanusConnected() override;

	void OnJanusDisconnected() override;

	void OnSendKeepAliveToJanus() override;

	//
	// MainWndCallback implementation.
	//

	void StartLogin(const std::string& server, int port) override;

	void DisconnectFromServer() override;

	void ConnectToPeer(int peer_id) override;

	void DisconnectFromCurrentPeer() override;

	void UIThreadCallback(int msg_id, void* data) override;

	// CreateSessionDescriptionObserver implementation.
	void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;
	void OnFailure(webrtc::RTCError error) override;

protected:
	int peer_id_;
	bool loopback_;
	//rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;
	std::map<long long int, rtc::scoped_refptr<webrtc::PeerConnectionInterface>> m_peer_connection_map;
	rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
		peer_connection_factory_;
	PeerConnectionWsClient* client_;
	MainWindow* main_wnd_;
	std::deque<std::string*> pending_messages_;
	std::string server_;
	std::map<std::string, std::shared_ptr<JanusTransaction>> m_transactionMap;
	std::map<long long int, std::shared_ptr<JanusHandle>> m_handleMap;
	long long int m_SessionId=0LL;
	long long int m_HandleId=0LL;

	private:
		void KeepAlive();
		void CreateSession();
		void CreateHandle();
		void CreateHandle(std::string pluginName, long long int feedId, std::string display);
		void JoinRoom(std::string pluginName, long long int handleId, long long int feedId);
		void SendOffer(long long int handleId, std::string sdp_type, std::string sdp_desc);
		void trickleCandidate(long long int handleId, const webrtc::IceCandidateInterface* candidate);
		void trickleCandidateComplete(long long int handleId);
		void SendBitrateConstraint();
		public:
			void* this_ptr;
};


