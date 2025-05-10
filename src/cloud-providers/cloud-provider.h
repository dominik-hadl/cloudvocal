#pragma once

#include <functional>
#include <thread>
#include <atomic>
#include <vector>
#include <string>

#include "cloudvocal-processing.h"
#include "cloudvocal-data.h"
#include "plugin-support.h"

class CloudProvider {
public:
	using TranscriptionCallback = std::function<void(const DetectionResultWithText &)>;

	CloudProvider(TranscriptionCallback callback, cloudvocal_data *gf_)
		: transcription_callback(callback),
		  running(false),
		  gf(gf_),
		  stop_requested(false),
		  needs_results_thread(false)
	{
	}

	virtual ~CloudProvider() { stop(); }

	virtual bool init() = 0;

	void start()
	{
		stop_requested = false;
		transcription_thread = std::thread(&CloudProvider::processAudio, this);
		if (needs_results_thread) {
			results_thread = std::thread(&CloudProvider::processResults, this);
		}
	}

	void stop()
	{
		obs_log(gf->log_level, "Stopping cloud provider");
		stop_requested = true;
		gf->input_buffers_cv.notify_all();
		if (transcription_thread.joinable()) {
			obs_log(gf->log_level, "Joining transcription thread...");
			transcription_thread.join();
		}
		if (results_thread.joinable()) {
			obs_log(gf->log_level, "Joining results thread...");
			results_thread.join();
		}
		running = false;
	}

	bool isRunning() const { return running; }

protected:
	virtual void sendAudioBufferToTranscription(const std::deque<float> &audio_buffer) = 0;
	virtual void readResultsFromTranscription() = 0;
	virtual void shutdown() = 0;

	void processAudio()
	{
		// Initialize the cloud provider
		if (!init()) {
			obs_log(LOG_ERROR, "Failed to initialize cloud provider");
			running = false;
			return;
		}

		running = true;

		uint64_t start_timestamp_offset_ns = 0;
		uint64_t end_timestamp_offset_ns = 0;

		while (running && !stop_requested) {
			get_data_from_buf_and_resample(gf, start_timestamp_offset_ns,
						       end_timestamp_offset_ns);

			if (gf->resampled_buffer.empty()) {
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				continue;
			}

			sendAudioBufferToTranscription(gf->resampled_buffer);

			gf->resampled_buffer.clear();

			// sleep until the next audio packet is ready
			// wait for notificaiton from the audio buffer condition variable
			std::unique_lock<std::mutex> lock(gf->input_buffers_mutex);
			gf->input_buffers_cv.wait(lock, [this] {
				return !(gf->input_buffers[0]).empty() || !running ||
				       stop_requested;
			});
		}

		// Shutdown the cloud provider
		shutdown();

		obs_log(gf->log_level, "Cloud provider audio thread stopped");
		this->running = false;
	}

	void processResults()
	{
		while (!stop_requested) {
			if (!running) {
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				continue;
			}
			readResultsFromTranscription();
		}

		obs_log(gf->log_level, "Cloud provider results thread stopped");
	}

	cloudvocal_data *gf;
	std::atomic<bool> running;
	std::atomic<bool> stop_requested;
	TranscriptionCallback transcription_callback;
	bool needs_results_thread;

private:
	std::thread transcription_thread;
	std::thread results_thread;
};

std::shared_ptr<CloudProvider> createCloudProvider(const std::string &providerType,
						   CloudProvider::TranscriptionCallback callback,
						   cloudvocal_data *gf);

void restart_cloud_provider(cloudvocal_data *gf);
