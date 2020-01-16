#include "stdafx.h"
#include "config_manager.h"

#include <fstream>

namespace nim_conf {

ConfigManager::ConfigManager()
{
}


ConfigManager::~ConfigManager()
{
}

bool ConfigManager::LoadConfig(const std::string& config_file)
{
	bool result = false;

	if (config_file.empty())
	{
		return result;
	}

	config_file_ = config_file;

	// 读入配置
	Json::Reader reader;
	std::fstream fs(config_file_, ios::in);
	if (fs.is_open())
	{
		fs.seekg(0, ios_base::end);
		int length = fs.tellg();
		fs.seekg(0, ios_base::beg);
		std::unique_ptr<char[]> buffer(new char[length]());
		fs.read(buffer.get(), length);

		std::cout << buffer.get() << std::endl;

		if (reader.parse(buffer.get(), config_values_))
		{
			result = true;
		}

		fs.close();
	}

	return result;
}

std::string ConfigManager::GetValue(const std::string& key)
{
	if (!config_values_.isObject())
	{
		LoadConfig(config_file_);
	}

	if (config_values_.isMember(key))
		return config_values_[key].asString();
	
	if (default_values_.find(key) != default_values_.end())
		return default_values_[key];

	return "";
}

void ConfigManager::SetDefaultValue(const std::string& key, const std::string& value)
{
	default_values_[key] = value;
}

void ConfigManager::SetConfigFile(const std::string& config_file)
{
	config_file_ = config_file;
	LoadConfig(config_file_);
}

}
