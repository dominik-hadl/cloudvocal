#pragma once

#include <atomic>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/ssl.hpp>
#include <semaphore>
#include "cloud-providers/cloud-provider.h"

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

class SpeechmaticsProvider : public CloudProvider {
public:
	SpeechmaticsProvider(TranscriptionCallback callback, cloudvocal_data *gf_);
	bool init() override;

protected:
	void sendAudioBufferToTranscription(const std::deque<float> &audio_buffer) override;
	void readResultsFromTranscription() override;
	void shutdown() override;

private:
	net::io_context ioc;
	ssl::context ssl_ctx;
	tcp::resolver resolver;
	websocket::stream<beast::ssl_stream<tcp::socket>> ws;
	std::atomic<int> chunks_sent = 0;
	std::atomic<bool> session_started = false;
};
