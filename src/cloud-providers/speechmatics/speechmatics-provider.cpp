#include "speechmatics-provider.h"
#include <cstdint>
#include <nlohmann/json.hpp>
#include <boost/algorithm/string.hpp>

#include "language-codes/language-codes.h"

using json = nlohmann::json;

namespace http = beast::http;

SpeechmaticsProvider::SpeechmaticsProvider(TranscriptionCallback callback, cloudvocal_data *gf_)
	: CloudProvider(callback, gf_),
	  ioc(),
	  ssl_ctx(ssl::context::tlsv12_client),
	  resolver(ioc),
	  ws(ioc, ssl_ctx)
{
	needs_results_thread = true; // We need a separate thread for reading results
}

bool SpeechmaticsProvider::init()
{
	const char *SPEECHMATICS_HOST_NAME = "eu2.rt.speechmatics.com";

	try {
		// Setup SSL context
		ssl_ctx.set_verify_mode(ssl::verify_peer);
		ssl_ctx.set_default_verify_paths();

		// Resolve the Speechmatics endpoint
		auto const results = resolver.resolve(SPEECHMATICS_HOST_NAME, "443");

		// Connect to Speechmatics
		net::connect(get_lowest_layer(ws), results);

		// Set SNI hostname (required for TLS)
		if (!SSL_set_tlsext_host_name(ws.next_layer().native_handle(),
					      SPEECHMATICS_HOST_NAME)) {
			throw beast::system_error(
				beast::error_code(static_cast<int>(::ERR_get_error()),
						  net::error::get_ssl_category()),
				"Failed to set SNI hostname");
		}

		// Perform SSL handshake
		ws.next_layer().handshake(ssl::stream_base::client);

		ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));

		ws.set_option(
			websocket::stream_base::decorator([this](websocket::request_type &req) {
				req.set(http::field::authorization,
					"Bearer " + std::string(gf->cloud_provider_api_key));
				req.set("sm-sdk", "cpp-obs");
			}));

		std::string query =
			std::string("/v2/") + language_codes_from_underscore[gf->language];
		// Perform WebSocket handshake
		beast::error_code ec;
		ws.handshake(SPEECHMATICS_HOST_NAME, query, ec);
		if (ec) {
			obs_log(LOG_ERROR, "WebSocket handshake failed: %s", ec.message().c_str());
			return false;
		}

		std::string partial_bool = gf->partial_transcription ? "true" : "false";
		std::string start_msg = R"({"message":"StartRecognition", "audio_format": {
			"type": "raw",
			"sample_rate": 16000,
			"encoding": "pcm_s16le"
		}, "transcription_config": {
			"max_delay": 1.0,
			"language": ")" +
					language_codes_from_underscore[gf->language] + R"(",
			"operating_point": "enhanced",
			"enable_partials": )" +
					partial_bool + R"(
		}})";
		ws.text(true);
		ws.write(net::buffer(start_msg), ec);
		if (ec) {
			obs_log(LOG_ERROR, "WebSocket write failed: %s", ec.message().c_str());
			return false;
		}

		obs_log(LOG_INFO, "Connected to Speechmatics WebSocket successfully");
		return true;
	} catch (std::exception const &e) {
		obs_log(LOG_ERROR, "Error initializing Speechmatics connection: %s", e.what());
		return false;
	}
}

void SpeechmaticsProvider::sendAudioBufferToTranscription(const std::deque<float> &audio_buffer)
{
	if (audio_buffer.empty())
		return;

	int total_waited = 0;
	while (!session_started && total_waited < 5000) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		total_waited += 10;
	}
	if (!session_started) {
		obs_log(LOG_ERROR, "Speechmatics session not started. Cannot send audio buffer.");
		return;
	}

	const int AUDIO_CHUNK_SIZE = 1024;

	try {
		// Convert float audio to int16_t (linear16 format)
		std::vector<int16_t> pcm_data;
		pcm_data.reserve(audio_buffer.size());

		static std::vector<float> buffered_samples;

		// Add current audio samples to buffer
		buffered_samples.insert(buffered_samples.end(), audio_buffer.begin(),
					audio_buffer.end());

		// Only process if we have at least 512 samples
		if (buffered_samples.size() >= AUDIO_CHUNK_SIZE) {
			// Process complete chunks of 512 samples
			size_t complete_chunks = buffered_samples.size() / AUDIO_CHUNK_SIZE;
			size_t samples_to_process = complete_chunks * AUDIO_CHUNK_SIZE;

			pcm_data.reserve(samples_to_process);

			// Convert only the samples we're processing now
			for (size_t i = 0; i < samples_to_process; i++) {
				float clamped =
					std::max(-1.0f, std::min(1.0f, buffered_samples[i]));
				pcm_data.push_back(static_cast<int16_t>(clamped * 32767.0f));
			}

			// Remove processed samples from buffer
			buffered_samples.erase(buffered_samples.begin(),
					       buffered_samples.begin() + samples_to_process);
		} else {
			// Not enough samples yet, skip processing
			return;
		}

		for (float sample : audio_buffer) {
			// Clamp and convert to int16
			float clamped = std::max(-1.0f, std::min(1.0f, sample));
			pcm_data.push_back(static_cast<int16_t>(clamped * 32767.0f));
		}

		// Send binary message
		ws.binary(true);
		ws.write(net::buffer(pcm_data.data(), pcm_data.size() * sizeof(int16_t)));
		chunks_sent++;
	} catch (beast::system_error const &se) {
		// This indicates the connection was closed
		if (se.code() != websocket::error::closed) {
			obs_log(LOG_ERROR, "Error sending audio to Speechmatics: %s",
				se.code().message().c_str());
		} else if (se.code() == beast::websocket::error::closed) {
			obs_log(LOG_ERROR,
				"Error sending audio to Speechmatics. WebSocket connection closed");
		}
		running = false;
	} catch (std::exception const &e) {
		obs_log(LOG_ERROR, "Error sending audio to Speechmatics: %s", e.what());
		running = false;
	}
}

void SpeechmaticsProvider::readResultsFromTranscription()
{
	try {
		beast::flat_buffer buffer;
		ws.text(true);
		ws.read(buffer);

		std::string msg = beast::buffers_to_string(buffer.data());
		json result = json::parse(msg);

		if (result["message"] == "RecognitionStarted") {
			obs_log(LOG_INFO,
				"Speechmatics connection established successfully, session ID: %s",
				result["id"].get<std::string>().c_str());
			session_started = true;
		} else if ((result["message"] == "AddTranscript" ||
			    result["message"] == "AddPartialTranscript")) {
			DetectionResultWithText detection_result;

			// Static buffer to accumulate transcripts
			static std::vector<std::string> transcript_buffer;
			static int64_t last_flush_timestamp_ms = 1;
			static int64_t buffer_start_time_ms = std::numeric_limits<uint64_t>::max();
			static const int64_t FLUSH_TIMEOUT_MS = 3000;
			static const int MAX_CHARACTERS = 40;

			int64_t current_time_ms =
				result["metadata"]["end_time"].get<float>() * 1000.0f;
			bool is_final = (result["message"] == "AddTranscript");

			if (result["metadata"].is_null()) {
				obs_log(LOG_ERROR, "Speechmatics result metadata is null");
				return;
			}

			// For final transcripts, add to buffer
			if (is_final) {
				std::string transcript =
					result["metadata"]["transcript"].get<std::string>();
				boost::trim(transcript);

				buffer_start_time_ms = std::min(
					buffer_start_time_ms,
					static_cast<int64_t>(
						result["metadata"]["start_time"].get<float>() *
						1000.0f));

				transcript_buffer.push_back(transcript);

				auto buffer_n_characters = std::accumulate(
					transcript_buffer.begin(), transcript_buffer.end(), 0,
					[](size_t sum, const std::string &str) {
						return sum + str.length();
					});
				obs_log(LOG_DEBUG,
					"Speechmatics buffer size: %zu, ms since last flush: %d",
					buffer_n_characters,
					current_time_ms - last_flush_timestamp_ms);
				if (buffer_n_characters >= MAX_CHARACTERS ||
				    (last_flush_timestamp_ms > 0 &&
				     current_time_ms - last_flush_timestamp_ms >
					     FLUSH_TIMEOUT_MS)) {

					// Join all buffered transcripts
					std::string joined_text;
					for (size_t i = 0; i < transcript_buffer.size(); ++i) {
						const auto &segment = transcript_buffer[i];
						if (i > 0 && !segment.empty() &&
						    (segment[0] != '.' && segment[0] != ',' &&
						     segment[0] != '?' && segment[0] != '!' &&
						     segment[0] != ';' && segment[0] != ':')) {
							joined_text += ' ';
						}
						joined_text += segment;
					}

					boost::trim(joined_text);
					// Trim leading punctuation characters
					while (!joined_text.empty() &&
					       (joined_text[0] == '.' || joined_text[0] == ',' ||
						joined_text[0] == '?' || joined_text[0] == '!' ||
						joined_text[0] == ';' || joined_text[0] == ':')) {
						joined_text.erase(0, 1);
					}
					detection_result.text = joined_text;
					last_flush_timestamp_ms = current_time_ms;
					detection_result.start_timestamp_ms = buffer_start_time_ms;
					detection_result.end_timestamp_ms =
						result["metadata"]["end_time"].get<float>() *
						1000.0f;

					buffer_start_time_ms = std::numeric_limits<int64_t>::max();
					transcript_buffer.clear();
				} else {
					return;
				}
			} else {
				// Fill the	 detection result structure
				auto rawTranscript =
					result["metadata"]["transcript"].get<std::string>();
				boost::trim(rawTranscript);
				detection_result.text = rawTranscript;
				detection_result.start_timestamp_ms =
					result["metadata"]["start_time"].get<float>() * 1000.0f;
				detection_result.end_timestamp_ms =
					result["metadata"]["end_time"].get<float>() * 1000.0f;
			}

			detection_result.result = is_final ? DETECTION_RESULT_SPEECH
							   : DETECTION_RESULT_PARTIAL;

			detection_result.language = language_codes_from_underscore[gf->language];

			transcription_callback(detection_result);
		} else if (result["message"] == "EndOfTranscript") {
			obs_log(LOG_INFO, "Speechmatics recognition completed successfully");
		} else if (result["message"] == "Warning") {
			obs_log(LOG_WARNING, "Speechmatics warning %s, reason: %s",
				result["type"].get<std::string>().c_str(),
				result["reason"].get<std::string>().c_str());
		} else if (result["message"] == "Error") {
			obs_log(LOG_ERROR, "Speechmatics error %s, reason: %s",
				result["type"].get<std::string>().c_str(),
				result["reason"].get<std::string>().c_str());
		} else if (result["message"] == "Info") {
			obs_log(LOG_INFO, "Speechmatics info %s, reason: %s",
				result["type"].get<std::string>().c_str(),
				result["reason"].get<std::string>().c_str());
		} else {
			obs_log(LOG_DEBUG, "Unknown message type: %s",
				result["message"].get<std::string>().c_str());
		}
	} catch (beast::system_error const &se) {
		// This indicates the connection was closed
		if (se.code() != websocket::error::closed) {
			obs_log(LOG_ERROR, "Error reading from Speechmatics (code %d): %s",
				se.code().value(), // Use value() to get the error code
				se.code().message().c_str());
			// Attempt to reinitialize the connection
			init();
		} else if (se.code() == beast::websocket::error::closed) {
			obs_log(LOG_ERROR,
				"Error reading from Speechmatics. WebSocket connection closed");
			running = false;
		}
	} catch (std::exception const &e) {
		obs_log(LOG_ERROR, "Error reading from Speechmatics: %s", e.what());
	}
}

void SpeechmaticsProvider::shutdown()
{
	if (!session_started) {
		obs_log(LOG_ERROR, "Speechmatics session not started. Cannot shutdown.");
		return;
	}
	if (!running) {
		obs_log(LOG_ERROR, "Speechmatics session not running. Cannot shutdown.");
		return;
	}
	try {
		// Send close message
		ws.text(true);
		ws.write(net::buffer(std::string(R"({"message":"EndOfStream", "last_seq_no": )") +
				     std::to_string(chunks_sent) + "}"));

		// Close WebSocket connection
		ws.close(websocket::close_code::normal);

		obs_log(LOG_INFO, "Speechmatics connection closed successfully");
	} catch (std::exception const &e) {
		obs_log(LOG_ERROR, "Error during Speechmatics shutdown: %s", e.what());
	}
}
