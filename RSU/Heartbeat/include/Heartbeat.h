#ifndef _HEARTBEAT_H_
#define _HEARTBEAT_H_
#include <stdio.h>
#include <string>
#include "libemqtt.h"
#define RCVBUFSIZE 100000

using namespace std;

class Heartbeat
{
private:
	static Heartbeat *cloud_hb;
public:
    Heartbeat();
    ~Heartbeat();

    int socket_id;
    int keepalive = 10;
    uint8_t packet_buffer[RCVBUFSIZE];
    string json_conf;
    int Port;
    string IP;
    string dId;
    string topic;
    mqtt_broker_handle_t broker;
    static Heartbeat *GetInstance();
	static void DestoryInstance();
    void Parse(const char *json_str);
    string ReadConf(string &json);
    // int send_packet(void* socket_info, const void* buf, unsigned int count);
    int init_socket(mqtt_broker_handle_t* broker, const char* hostname, short port);
    int close_socket(mqtt_broker_handle_t* broker);
    int read_packet(int timeout);
    string heartbeat_handle(string json_data);
    int init_mqtt_t();
    int send_heart();

};

#endif