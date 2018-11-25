#include "peer_connection.h"
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

class DummySetSessionDescriptionObserver
	: public webrtc::SetSessionDescriptionObserver {
public:
	static DummySetSessionDescriptionObserver* Create() {
		return new rtc::RefCountedObject<DummySetSessionDescriptionObserver>();
	}
	virtual void OnSuccess() { RTC_LOG(INFO) << __FUNCTION__; }
	virtual void OnFailure(webrtc::RTCError error) {
		RTC_LOG(INFO) << __FUNCTION__ << " " << ToString(error.type()) << ": "      
			<< error.message();
	}
};

PeerConnection::PeerConnection()
{
}


PeerConnection::~PeerConnection()
{
}

void PeerConnection::RegisterObserver(PeerConnectionCallback* callback) {
	m_pConductorCallback = callback;
}

void PeerConnection::SetHandleId(long long int handleId) {
	m_HandleId = handleId;
}

long long int PeerConnection::GetHandleId() {
	return m_HandleId;
}

//CreateSessionDescriptionObserver implementation.
void PeerConnection::OnSuccess(webrtc::SessionDescriptionInterface* desc) {
	peer_connection_->SetLocalDescription(
		DummySetSessionDescriptionObserver::Create(), desc);

	std::string sdp;
	desc->ToString(&sdp);
	m_pConductorCallback->PCSendSDP(m_HandleId, webrtc::SdpTypeToString(desc->GetType()), sdp);
}

void PeerConnection::OnFailure(webrtc::RTCError error) {
	RTC_LOG(LERROR) << ToString(error.type()) << ": " << error.message();
}


// PeerConnectionObserver implementation.
//

void PeerConnection::OnAddTrack(
	rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
	const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&
	streams) {
	RTC_LOG(INFO) << __FUNCTION__ << " " << receiver->id();
	m_pConductorCallback->PCQueueUIThreadCallback(NEW_TRACK_ADDED,
		receiver->track().release());
	/*main_wnd_->QueueUIThreadCallback(NEW_TRACK_ADDED,
		receiver->track().release());*/
}

void PeerConnection::OnRemoveTrack(
	rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) {
	RTC_LOG(INFO) << __FUNCTION__ << " " << receiver->id();
	m_pConductorCallback->PCQueueUIThreadCallback(TRACK_REMOVED, receiver->track().release());
	//main_wnd_->QueueUIThreadCallback(TRACK_REMOVED, receiver->track().release());
}

void PeerConnection::OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
	RTC_LOG(INFO) << __FUNCTION__ << " " << candidate->sdp_mline_index();

	if (candidate) {
		m_pConductorCallback->PCTrickleCandidate(m_HandleId, candidate);
		//trickleCandidate(0, candidate);
	}
	else {
		m_pConductorCallback->PCTrickleCandidateComplete(m_HandleId);
		//trickleCandidateComplete(0);
	}
}

//peer connection interface implementation
void PeerConnection::CreateOffer() {
	//TODO RTCOfferAnswerOptions should set as a option
	peer_connection_->CreateOffer(
		this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
}

void PeerConnection::CreateAnswer() {
	//TODO RTCOfferAnswerOptions should set as a option
	peer_connection_->CreateAnswer(
		this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
}


void PeerConnection::SetRemoteDescription(webrtc::SessionDescriptionInterface* session_description) {
	peer_connection_->SetRemoteDescription(
		DummySetSessionDescriptionObserver::Create(),
		session_description);
}

