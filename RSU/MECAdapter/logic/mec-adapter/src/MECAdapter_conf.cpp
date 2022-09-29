#include <iostream>
#include <MECAdapter.h>
#include <rapidjson/document.h>

using namespace std;

// 读配置文件
void MECAdapter::Parse(const char *json_str)
{
	MECAdapter *mec_ad = NULL;
	mec_ad = MECAdapter::GetInstance();
	rapidjson::Document doc;
	//首先进行解析，没有解析错误才能进行具体字段的解析
	if (!doc.Parse(json_str).HasParseError())
	{
		if (doc.HasMember("Port") && doc["Port"].IsInt())
		{
			mec_ad->Port = doc["Port"].GetInt();
		}
		if (doc.HasMember("IP") && doc["IP"].IsString())
		{
			mec_ad->IP = doc["IP"].GetString();
		}
	}
}

string MECAdapter::ReadConf(string &json)
{

	FILE *conf_fd = fopen("../config/mec_adapter_conf.json", "rb");
	if (!conf_fd)
	{
		cout << "open mec_adapter conf file error!" << endl;
		return NULL;
	}

	if (conf_fd)
	{
		char *buf = new char[1024 * 16];
		int n = fread(buf, 1, 1024 * 16, conf_fd); // 将读取的内容转换为dom元素
		fclose(conf_fd);
		if (n >= 0)
			json.append(buf, 0, n);
		delete[] buf;
		buf = nullptr;
	}
	return json;
}