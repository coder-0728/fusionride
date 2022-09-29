#include <iostream>
#include <Heartbeat.h>
#include <rapidjson/document.h>

using namespace std;

// 读配置文件
void Heartbeat::Parse(const char *json_str)
{
	Heartbeat *cloud_hb = NULL;
	cloud_hb = Heartbeat::GetInstance();
	rapidjson::Document doc;
	//首先进行解析，没有解析错误才能进行具体字段的解析
	if (!doc.Parse(json_str).HasParseError())
	{
		if (doc.HasMember("Port") && doc["Port"].IsInt())
		{
			cloud_hb->Port = doc["Port"].GetInt();
		}
		if (doc.HasMember("IP") && doc["IP"].IsString())
		{
			cloud_hb->IP = doc["IP"].GetString();
		}
		if (doc.HasMember("topic") && doc["topic"].IsString())
		{
			cloud_hb->topic = doc["topic"].GetString();
		}
		if (doc.HasMember("dId") && doc["dId"].IsString())
		{
			cloud_hb->dId = doc["dId"].GetString();
		}
	}
}

string Heartbeat::ReadConf(string &json)
{

	FILE *conf_fd = fopen("../config/heartbeat_cloud_conf.json", "rb");
	if (!conf_fd)
	{
		cout << "open heartbeat conf file error!" << endl;
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