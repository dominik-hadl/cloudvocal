# CloudVocal - Professional Cloud AI Transcription & Translation Plugin

<div align="center">

[![GitHub](https://img.shields.io/github/license/locaal-ai/cloudvocal)](https://github.com/locaal-ai/cloudvocal/blob/main/LICENSE)
[![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/locaal-ai/cloudvocal/push.yaml)](https://github.com/locaal-ai/cloudvocal/actions/workflows/push.yaml)
[![Total downloads](https://img.shields.io/github/downloads/locaal-ai/cloudvocal/total)](https://github.com/locaal-ai/cloudvocal/releases)
[![GitHub release (latest by date)](https://img.shields.io/github/v/release/locaal-ai/cloudvocal)](https://github.com/locaal-ai/cloudvocal/releases)
[![GitHub stars](https://badgen.net/github/stars/locaal-ai/cloudvocal)](https://github.com/locaal-ai/cloudvocal/stargazers/)
[![Discord](https://img.shields.io/discord/1200229425141252116)](https://discord.gg/KbjGU2vvUz)
<br/>
Download:</br>
<a href="https://github.com/locaal-ai/cloudvocal/releases/latest/download/cloudvocal-0.0.1-windows-x64-Installer.exe"><img src="https://img.shields.io/badge/Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white" /></a>
<a href="https://github.com/locaal-ai/cloudvocal/releases/latest/download/cloudvocal-0.0.1-macos-universal.pkg"><img src="https://img.shields.io/badge/mac-000000?style=for-the-badge" /></a>
<a href="https://github.com/locaal-ai/cloudvocal/releases/latest/download/cloudvocal-0.0.1-x86_64-linux-gnu.deb"><img src="https://img.shields.io/badge/Linux-FCC624?style=for-the-badge&logo=linux&logoColor=black"/></a>
</div>

## Introduction

CloudVocal brings professional-grade cloud transcription and translation to your OBS streams and recordings. Powered by industry-leading cloud providers, it delivers exceptional accuracy and real-time performance for your live streaming needs. ✅ Professional-grade accuracy, ✅ support for 100+ languages, ✅ enterprise-level reliability, and ✅ blazing-fast performance!

CloudVocal integrates seamlessly with leading cloud providers to deliver enterprise-grade speech recognition and translation services. Simply configure your API credentials and start streaming with professional-quality captions and translations.

## Features

Current Features:
- Professional-grade transcription with industry-leading accuracy
- Providers: [Google Cloud](https://cloud.google.com/speech-to-text/docs/), [Naver Clova](https://developers.naver.com/docs/clova/api/), [Rev AI](https://www.rev.ai/), [Deepgram](https://developers.deepgram.com/docs/introduction), [AWS Transcribe](https://docs.aws.amazon.com/transcribe/latest/APIReference/Welcome.html) (upcoming)
- Real-time translation using enterprise cloud translation services
- Translation providers: [Google Cloud](https://cloud.google.com/translate/docs/reference/rest/), [Naver Papago](https://developers.naver.com/docs/papago/), [DeepL](https://www.deepl.com/en/products/api), [AWS Translate](https://aws.amazon.com/translate/), [Anthropic Claude](https://www.anthropic.com/api), [OpenAI](https://openai.com/api/)
- Streaming-optimized performance with minimal latency
- Caption output in multiple formats (.txt, .srt)
- Sync'ed captions with OBS recording timestamps
- Direct streaming to platforms (YouTube, Twitch) with embedded captions
- Partial transcriptions for a streaming-captions experience

Roadmap:
- Additional cloud providers and services (e.g. Microsoft Azure)
- Custom vocabulary and pronunciation support
- Professional terminology handling for specific industries
- Advanced text filtering and customization options
- Speaker diarization for multi-speaker environments
- Advanced profanity filtering options
- Custom translation glossaries
- Additional subtitle format support
- Enhanced analytics and caption quality metrics

## Usage

Tutorial videos and screenshots - coming soon!

## Download and Installation

Check out the [latest releases](https://github.com/locaal-ai/cloudvocal/releases) for downloads and install instructions.

### Configuration

1. Download and install the appropriate version for your operating system
1. Add CloudVocal as a filter to your audio source
1. Configure your cloud provider credentials in the plugin settings
1. Select your desired transcription and translation options
1. Select an output text source for the captions and translations, send the captions to the stream or a file

## Building

The plugin can be built on Windows, macOS, and Linux platforms. The build process is straightforward as all processing happens in the cloud.

Both Mac OSX and Linux rely on Conan for dependencies. Install Conan, e.g. `pip install conan`, and install the dependencies:
```sh
$ conan profile detect --force
$ conan install . --update --output-folder=./build_conan --build=missing -g CMakeDeps
```

### Mac OSX

Build the plugin:

```sh
$ ./.github/scripts/build-macos --config Release
```

You may want to change to `RelWithDebInfo` for a debug build.

If you're developing the plugin, I find this command to be useful for direct deploymet into OBS after building:

```sh
$ ./.github/scripts/build-macos --skip-deps && cp -R release/RelWithDebInfo/*.plugin ~/Library/Application\ Support/obs-studio/plugins/
```

### Linux

Build the plugin:
```sh
$ ./.github/scripts/build-linux
```

### Windows

Windows also needs Conan for OpenSSL. Run `conan` to get the dependency (make sure to run `conan` on the `conanfile_win.txt`):
```powershell
> pip install conan
> conan profile detect --force
> conan install .\conanfile_win.txt --output-folder=./build_conan --build=missing -g CMakeDeps
```

Build the plugin:

```powershell
> .\.github\scripts\Build-Windows.ps1 -Configuration Release
```

If you're developing the plugin, I find this command to be useful for direct deploymet into OBS after building:

```powershell
> pwsh -ExecutionPolicy Bypass -File .\.github\scripts\Build-Windows.ps1 -Configuration RelWithDebInfo -SkipDeps && Copy-Item -Force -Recurse .\release\RelWithDebInfo\* "C:\Program Files\obs-studio\"
```

## Contributing

We welcome contributions from the community!
Please fork the repository and submit a pull request with your changes. We will review and merge your changes as soon as possible.

## License

This project is licensed under the GPLv2 License - see the [LICENSE](LICENSE) file for details.
