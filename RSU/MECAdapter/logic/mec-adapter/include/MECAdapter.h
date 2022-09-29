#include <iostream>
#include <libemqtt.h>


#ifndef _MECADAPTER_H_
#define _MECADAPTER_H_

using namespace std;

class MECAdapter
{
public:
    MECAdapter();
    ~MECAdapter();
private:
	static MECAdapter *mec_ad;

public:
	static MECAdapter *GetInstance();
	static void DestoryInstance();
	int uds_socket_server(const char *filename);
	void Parse(const char *json_str);
    string ReadConf(string &json);

	int init_socket(mqtt_broker_handle_t *broker, const char *hostname, short port, int keepalive);
	int close_socket(mqtt_broker_handle_t *broker);
	int read_packet(int timeout);

	int socket_id;
	int Port;
	string IP;
};

#endif