#include "cloud-provider.h"
#include "cloudvocal-callbacks.h"
#include "clova/clova-provider.h"
#include "google/google-provider.h"
#include "aws/aws_provider.h"
#include "revai/revai-provider.h"
#include "deepgram/deepgram-provider.h"
#include "speechmatics/speechmatics-provider.h"

std::shared_ptr<CloudProvider> createCloudProvider(const std::string &providerType,
						   CloudProvider::TranscriptionCallback callback,
						   cloudvocal_data *gf)
{
	if (providerType == "clova") {
		return std::make_shared<ClovaProvider>(callback, gf);
	} else if (providerType == "google") {
		return std::make_unique<GoogleProvider>(callback, gf);
	} else if (providerType == "aws") {
		return std::make_unique<AWSProvider>(callback, gf);
	} else if (providerType == "revai") {
		return std::make_unique<RevAIProvider>(callback, gf);
	} else if (providerType == "deepgram") {
		return std::make_unique<DeepgramProvider>(callback, gf);
	} else if (providerType == "speechmatics") {
		return std::make_unique<SpeechmaticsProvider>(callback, gf);
	}

	return nullptr; // Return nullptr if no matching provider is found
}

void restart_cloud_provider(cloudvocal_data *gf)
{
	// stop the current cloud provider
	if (gf->cloud_provider != nullptr) {
		gf->cloud_provider->stop();
		gf->cloud_provider = nullptr;
	}
	gf->cloud_provider = createCloudProvider(
		gf->cloud_provider_selection,
		[gf](const DetectionResultWithText &result) {
			// callback
			set_text_callback(gf, result);
		},
		gf);
	if (gf->cloud_provider == nullptr) {
		obs_log(LOG_ERROR, "Failed to create cloud provider '%s'",
			gf->cloud_provider_selection.c_str());
		gf->active = false;
		return;
	}
	gf->cloud_provider->start();
}
