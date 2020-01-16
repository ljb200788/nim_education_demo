#pragma once
#include "base/memory/singleton.h"

namespace nim_conf {

const std::string kAppKey			= "appkey";
const std::string kWebBoardAddress	= "app_webboard_address";
const std::string kServerAddress	= "app_server_address";
const std::string kChatRoomAddress	= "app_chatroom_address";
const std::string kChatRoomCreate	= "app_chatroom_create_address";

const std::string kRTSRecord		= "rts_record";
const std::string kAudioRecord		= "audio_record";
const std::string kVideoRecord		= "video_record";
const std::string kVideoBitrate		= "video_bitrate";
const std::string kVideoWidth		= "video_width";
const std::string kVideoHeight		= "video_height";
const std::string kVideoQuality		= "video_quality";
const std::string kRTMPRecord		= "rtmp_record";
const std::string kSingleRecord		= "single_record";
const std::string kSplitMode		= "split_mode";

class ConfigManager
{
public:
	SINGLETON_DEFINE(ConfigManager);

	ConfigManager();
	~ConfigManager();

	bool LoadConfig(const std::string& config_file);
	std::string GetValue(const std::string& key);
	void SetDefaultValue(const std::string& key, const std::string& value);

	void SetConfigFile(const std::string& config_file);
	std::string GetConfigFile() { return config_file_; }

private:
	Json::Value config_values_ = Json::nullValue;
	std::string config_file_;
	std::map<std::string, std::string> default_values_;
};

}
