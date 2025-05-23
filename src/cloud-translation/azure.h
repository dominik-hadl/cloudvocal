#pragma once
#include "ITranslator.h"
#include <memory>

class CurlHelper; // Forward declaration

class AzureTranslator : public ITranslator {
public:
	AzureTranslator(
		const std::string &api_key, const std::string &location = "",
		const std::string &endpoint = "https://api.cognitive.microsofttranslator.com");
	~AzureTranslator() override;

	std::string translate(const std::string &text, const std::string &target_lang,
			      const std::string &source_lang = "auto") override;

private:
	std::string parseResponse(const std::string &response_str);

	std::string api_key_;
	std::string location_;
	std::string endpoint_;
	std::unique_ptr<CurlHelper> curl_helper_;
};
