#pragma once

#include <deque>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <list>

#include "api/mediastreaminterface.h"
#include "api/video/video_frame.h"
#include "api/video/i420_buffer.h"
#include "third_party/libyuv/include/libyuv/convert_argb.h"
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
	SET_REMOTE_ANSWER,//added by pcg
	SET_REMOTE_OFFER
};

struct NEW_TRACK {
	long long int handleId;
	webrtc::MediaStreamTrackInterface* pInterface;
};

// A little helper class to make sure we always to proper locking and
// unlocking when working with VideoRenderer buffers.
template <typename T>
class AutoLock {
public:
	explicit AutoLock(T* obj) : obj_(obj) { obj_->Lock(); }
	~AutoLock() { obj_->Unlock(); }

protected:
	T * obj_;
};

class VideoRenderer : public rtc::VideoSinkInterface<webrtc::VideoFrame> {
public:
	VideoRenderer(HWND wnd,
		int width,
		int height,
		webrtc::VideoTrackInterface* track_to_render);
	virtual ~VideoRenderer();

	void Lock() { ::EnterCriticalSection(&buffer_lock_); }

	void Unlock() { ::LeaveCriticalSection(&buffer_lock_); }

	// VideoSinkInterface implementation
	void OnFrame(const webrtc::VideoFrame& frame) override;

	const BITMAPINFO& bmi() const { return bmi_; }
	const uint8_t* image() const { return image_.get(); }

protected:
	void SetSize(int width, int height);

	enum {
		SET_SIZE,
		RENDER_FRAME,
	};

	HWND wnd_;
	BITMAPINFO bmi_;
	std::unique_ptr<uint8_t[]> image_;
	CRITICAL_SECTION buffer_lock_;
	rtc::scoped_refptr<webrtc::VideoTrackInterface> rendered_track_;
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
	void CreateOffer();
	void CreateAnswer();
	void SetRemoteDescription(webrtc::SessionDescriptionInterface* session_description);
	void StartRenderer(HWND wnd,webrtc::VideoTrackInterface* remote_video);
	void StopRenderer();
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
	std::unique_ptr<VideoRenderer> renderer_;//b_publisher decide local_render or remote_render
private:
	PeerConnectionCallback *m_pConductorCallback=NULL;
	long long int m_HandleId=0;//coresponding to the janus handleId	
};

