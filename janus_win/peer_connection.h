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
#include "rtc_base/refcount.h"

enum CallbackID {
	MEDIA_CHANNELS_INITIALIZED = 1,
	PEER_CONNECTION_CLOSED,
	SEND_MESSAGE_TO_PEER,
	NEW_TRACK_ADDED,
	TRACK_REMOVED,
	CREATE_OFFER,//added by pcg
	SET_REMOTE_SDP//added by pcg
};

class PeerConnectionCallback {
public:
	virtual void PCSendSDP(long long int handleId,std::string sdpType,std::string sdp) = 0;
	virtual void PCQueueUIThreadCallback(int msg_id, void* data) = 0;
	virtual void PCTrickleCandidate(long long int handleId, const webrtc::IceCandidateInterface* candidate) = 0;
	virtual void PCTrickleCandidateComplete(long long int handleId) = 0;

protected:
	virtual ~PeerConnectionCallback() {}
};

class PeerConnection:public webrtc::PeerConnectionObserver,
	public webrtc::CreateSessionDescriptionObserver
{
public:
	PeerConnection();
	~PeerConnection();
	void RegisterObserver(PeerConnectionCallback* callback);
	void SetHandleId(long long int handleId);
	long long int GetHandleId();
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
public:
	rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;
	bool b_publisher_=false;//pub or sub
private:
	PeerConnectionCallback *m_pConductorCallback=NULL;
	long long int m_HandleId=0;//coresponding to the janus handleId
};

