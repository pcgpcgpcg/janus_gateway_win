#pragma once

#include <deque>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "api/mediastreaminterface.h"
#include "api/peerconnectioninterface.h"
#include "main_wnd.h"
#include "peer_connection_wsclient.h"
#include "JanusTransaction.h"

#include "defaults.h"

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
	public MainWndCallback {
public:
	enum CallbackID {
		MEDIA_CHANNELS_INITIALIZED = 1,
		PEER_CONNECTION_CLOSED,
		SEND_MESSAGE_TO_PEER,
		NEW_TRACK_ADDED,
		TRACK_REMOVED,
	};

	ConductorWs(PeerConnectionWsClient* client, MainWindow* main_wnd);

	bool connection_active() const;

	void Close() override;

protected:
	~ConductorWs();
	bool InitializePeerConnection();
	bool ReinitializePeerConnectionForLoopback();
	bool CreatePeerConnection(bool dtls);
	void DeletePeerConnection();
	void EnsureStreamingUI();
	void AddTracks();
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

	void OnPeerDisconnected(int id) override;

	void OnMessageFromJanus(int peer_id, const std::string& message) override;

	void OnMessageSent(int err) override;

	void OnServerConnectionFailure() override;

	void OnJanusConnected() override;

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
	// Send a message to the remote peer.
	void SendMessage(const std::string& json_object);

	int peer_id_;
	bool loopback_;
	rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;
	rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
		peer_connection_factory_;
	PeerConnectionWsClient* client_;
	MainWindow* main_wnd_;
	std::deque<std::string*> pending_messages_;
	std::string server_;
	std::map<std::string, std::shared_ptr<JanusTransaction>> m_transactionMap;
	long long int m_SessionId;
	long long int m_HandleId;

	private:
		void CreateSession();
		void CreateHandle();
		void JoinRoom(long long int handleId, long long int feedId);
		void SendOffer(long long int handleId, std::string sdp_type, std::string sdp_desc);
		void trickleCandidate(long long int handleId, const webrtc::IceCandidateInterface* candidate);
		void trickleCandidateComplete(long long int handleId);
		public:
			void* this_ptr;
};


