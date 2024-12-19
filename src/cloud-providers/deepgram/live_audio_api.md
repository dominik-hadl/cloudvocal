Transcribe - Live audio
Use Deepgram's speech-to-text API to transcribe live-streaming audio.

Deepgram provides its customers with real-time, streaming transcription via its streaming endpoints. These endpoints are high-performance, full-duplex services running over the WebSocket protocol.

To learn more about working with real-time streaming data and results, see Get Started with Streaming Audio.

Endpoint
Production WebSocket server for Deepgram's real-time transcription with streaming audio. TLS encryption will protect your connection and data. We support a minimum of TLS 1.2.

Detail	Description
Path	wss://api.deepgram.com/v1/listen
Accepts
Type	Description
Raw Audio File Data	Unprocessed or uncompressed binary audio data (such as PCM)
Messages	JSON formatted operations.
Headers
Header	Value	Description
Sec-WebSocket-Protocol	token, <DEEPGRAM_API_KEY>	Used to establish a WebSocket connection with a specific protocol, include your Deepgram API key for authentication.
Body Params
Parameter	Type	Description
callback	string	Callback URL to provide if you would like your submitted audio to be processed asynchronously. Learn More.
callback_method	string	Enable a callback method. Use put or post. Learn More.
channels	int32	Number of independent audio channels contained in submitted streaming audio. Only read when a value is provided for encoding. Learn More.
dictation	boolean	Dictation automatically formats spoken commands for punctuation into their respective punctuation marks. Learn More.
diarize	boolean	Indicates whether to recognize speaker changes. When set to true, each word in the transcript will be assigned a speaker number starting at 0. Learn More.
diarize_version	string	Indicates the version of the diarization feature to use. Only available when the diarization feature is enabled. Learn More.
encoding	string	Expected encoding of the submitted streaming audio. If this parameter is set, sample_rate must also be specified. Learn More.
endpointing	boolean	Indicates how long Deepgram will wait to detect whether a speaker has finished speaking or pauses for a significant period of time. When set to true, the streaming endpoint immediately finalizes the transcription for the processed time range and returns the transcript with a speech_final parameter set to true. Learn More.
extra	string	Add any extra key-value pairs to the query string to customize the response. Learn More.
filler_words	boolean	Indicates whether to include filler words like "uh" and "um" in transcript output. When set to true, these words will be included. Learn More.
interim_results	boolean	Specifies whether the streaming endpoint should provide ongoing transcription updates as more audio is received. When set to true, the endpoint sends continuous updates, meaning transcription results may evolve over time. Learn More.
keywords	string	Unique proper nouns or specialized terms you want the model to include in its predictions, which aren't part of the model's default vocabulary. Learn More.
language	string	The BCP-47 language tag that hints at the primary spoken language. Learn More.
model	string	The AI model used to process submitted audio. Learn More.
multichannel	boolean	Indicates whether to transcribe each audio channel independently. Learn More.
numerals	boolean	Indicates whether to convert numbers from written format (e.g., one) to numerical format (e.g., 1). Learn More.
profanity_filter	boolean	Indicates whether to remove profanity from the transcript. Learn More.
punctuate	boolean	Indicates whether to add punctuation and capitalization to the transcript Learn More.
redact	string	Indicates whether to redact sensitive information, replacing redacted content with asterisks *. Learn More.
replace	string	Terms or phrases to search for in the submitted audio and replace. Learn More.
sample_rate	int32	Sample rate of submitted streaming audio. Required (and only read) when a value is provided for encoding. Learn More.
search	string	Terms or phrases to search for in the submitted audio. Learn More.
smart_format	boolean	Indicates whether to apply formatting to transcript output. When set to true, additional formatting will be applied to transcripts to improve readability. Learn More.
tag	string	Set a tag to associate with the request. Learn More.
utterance_end_ms	string	Indicates how long Deepgram will wait to send a {"type": "UtteranceEnd"} message after a word has been transcribed. Learn More.
vad_events	boolean	Indicates that speech has started. {"type": "SpeechStarted"} You'll begin receiving messages upon speech starting. Learn More.
version	string	Version of the model to use. Learn More.
Sending Audio Data
All audio data is transmitted to the streaming endpoint as binary WebSocket messages, with payloads containing the raw audio data. The full-duplex protocol allows for real-time streaming, enabling you to receive transcription responses simultaneously as you upload data. For optimal performance, each streaming buffer should represent between 20 and 250 milliseconds of audio.

Messages
Keep Alive
Optional

Periodically send KeepAlive messages while streaming can ensure uninterrupted communication and minimizing costs. Learn More .

JSON

{
  "type": "KeepAlive"
}
Finalize
Optional

Finalize message can be used to handle specific scenarios where you need to force the server to process all unprocessed audio data and immediately return the final results. Learn More.

JSON

{
  "type": "Finalize"
}
Close Stream
Optional

The CloseStream message can be sent to the Deepgram server, instructing it close the connection. Learn More.

JSON

{
  "type": "CloseStream"
}
Responses
Refer to API Errors for more information.

Status	Description
200	Audio submitted for transcription.
400	Bad Request.
401	Invalid Authorization.
402	Payment Required, insufficient credits
403	Insufficient permissions.
503	Internal server error if the server is temporarily unable to serve requests.
Response Schema
JSON

{
  "metadata": {
    "transaction_key": "string",
    "request_id": "uuid",
    "sha256": "string",
    "created": "string",
    "duration": 0,
    "channels": 0,
    "models": [
      "string"
    ],
  },
  "type": "Results",
  "channel_index": [
    0,
    0
  ],
  "duration": 0.0,
  "start": 0.0,
  "is_final": boolean,
  "speech_final": boolean,
  "channel": {
    "alternatives": [
      {
        "transcript": "string",
        "confidence": 0,
        "words": [
          {
            "word": "string",
            "start": 0,
            "end": 0,
            "confidence": 0
          }
        ]
      }
    ],
    "search": [
      {
        "query": "string",
        "hits": [
          {
            "confidence": 0,
            "start": 0,
            "end": 0,
            "snippet": "string"
          }
        ]
      }
    ]
  }
}
Errors & Warnings
If Deepgram encounters an error during real-time streaming, we will return a WebSocket Close frame. The body of the Close frame will indicate the reason for closing using one of the specification’s pre-defined status codes followed by a UTF-8-encoded payload that represents the reason for the error.

Current codes and payloads in use include:

Code	Payload	Description
1000	N/A	Normal Closure
1008	DATA-0000	The payload cannot be decoded as audio. Either the encoding is incorrectly specified, the payload is not audio data, or the audio is in a format unsupported by Deepgram.
1011	NET-0000	The service has not transmitted a Text frame to the client within the timeout window. This may indicate an issue internally in Deepgram's systems or could be due to Deepgram not receiving enough audio data to transcribe a frame.
1011	NET-0001	The service has not received a Binary or Text frame from the client within the timeout window. This may indicate an internal issue in Deepgram's systems, the client's systems, or the network connecting them.
