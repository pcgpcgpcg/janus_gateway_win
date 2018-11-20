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

class PeerConnection:public webrtc::PeerConnectionObserver,
	public webrtc::CreateSessionDescriptionObserver
{
public:
	PeerConnection();
	~PeerConnection();
protected:
	// PeerConnectionObserver implementation.
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
	// CreateSessionDescriptionObserver implementation.
	void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;
	void OnFailure(webrtc::RTCError error) override;
protected:
	rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;
};

