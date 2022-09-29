#include <iostream>
#include <unistd.h>
#include <sys/timeb.h>
#include <signal.h>
#include "Heartbeat.h"
#include "libemqtt.h"
#include <linux/tcp.h>
#include <arpa/inet.h>
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

using namespace std;


Heartbeat *Heartbeat::cloud_hb = NULL;

Heartbeat::Heartbeat(){

};
Heartbeat::~Heartbeat(){

};

//单例
Heartbeat *Heartbeat::GetInstance()
{
	if (cloud_hb == NULL)
	{
		cloud_hb = new Heartbeat();
	}

	return cloud_hb;
}

void Heartbeat::DestoryInstance()
{
	if (cloud_hb != NULL)
	{
		delete cloud_hb;
		cloud_hb = NULL;
	}
}


int send_packet(void* socket_info, const void* buf, unsigned int count)
{
	int fd = *((int*)socket_info);
	return send(fd, buf, count, 0);
}

int Heartbeat::init_socket(mqtt_broker_handle_t* broker, const char* hostname, short port)
{
    Heartbeat *cloud_hb = NULL;
	cloud_hb = Heartbeat::GetInstance();
	int flag = 1;
	int keepalive = 3; // Seconds

	// Create the socket
	if((cloud_hb->socket_id = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		return -1;

	// Disable Nagle Algorithm
	if (setsockopt(cloud_hb->socket_id, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag)) < 0)
		return -2;

	struct sockaddr_in socket_address;
	// Create the stuff we need to connect
	socket_address.sin_family = AF_INET;
	socket_address.sin_port = htons(port);
	socket_address.sin_addr.s_addr = inet_addr(hostname);

	// Connect the socket
	if((connect(cloud_hb->socket_id, (struct sockaddr*)&socket_address, sizeof(socket_address))) < 0)
		return -1;

	// MQTT stuffs
	mqtt_set_alive(broker, keepalive);
	broker->socket_info = (void*)&cloud_hb->socket_id;
	broker->send = send_packet;

	return 0;
}

int Heartbeat::close_socket(mqtt_broker_handle_t* broker)
{
	int fd = *((int*)broker->socket_info);
	return close(fd);
}

int Heartbeat::read_packet(int timeout)
{
    Heartbeat *cloud_hb = NULL;
	cloud_hb = Heartbeat::GetInstance();
	if(timeout > 0)
	{
		fd_set readfds;
		struct timeval tmv;

		// Initialize the file descriptor set
		FD_ZERO (&readfds);
		FD_SET (cloud_hb->socket_id, &readfds);

		// Initialize the timeout data structure
		tmv.tv_sec = timeout;
		tmv.tv_usec = 0;

		// select returns 0 if timeout, 1 if input available, -1 if error
		if(select(1, &readfds, NULL, NULL, &tmv))
			return -2;
	}

	int total_bytes = 0, bytes_rcvd, packet_length;
	memset(cloud_hb->packet_buffer, 0, sizeof(cloud_hb->packet_buffer));

	while(total_bytes < 2) // Reading fixed header
	{
		if((bytes_rcvd = recv(cloud_hb->socket_id, (cloud_hb->packet_buffer+total_bytes), RCVBUFSIZE, 0)) <= 0)
			return -1;
		total_bytes += bytes_rcvd; // Keep tally of total bytes
	}

	packet_length = packet_buffer[1] + 2; // Remaining length + fixed header length

	while(total_bytes < packet_length) // Reading the packet
	{
		if((bytes_rcvd = recv(cloud_hb->socket_id, (packet_buffer+total_bytes), RCVBUFSIZE, 0)) <= 0)
			return -1;
		total_bytes += bytes_rcvd; // Keep tally of total bytes
	}

	return packet_length;
}

int Heartbeat::init_mqtt_t()
{
    Heartbeat *cloud_hb = NULL;
	cloud_hb = Heartbeat::GetInstance();
	int packet_length;
	uint16_t msg_id, msg_id_rcv;

	mqtt_init(&broker, "avengalvon");
	int mqtt_ret = cloud_hb->init_socket(&cloud_hb->broker, IP.c_str(), Port);
	while (1)
	{
		if (mqtt_ret < 0)
		{
			cout << "connect MQTT-Server failed, try again……" << endl;
			mqtt_ret = cloud_hb->init_socket(&cloud_hb->broker, IP.c_str(), Port);
			sleep(20);
		}
		else
		{
			cout << "connect success,wait message" << endl;
			break;
		}

		// cout << "connect MQTT-Server failed" << endl;
		// return -1;
	}
	mqtt_connect(&cloud_hb->broker);
	// <<<<< CONNACK
	packet_length = read_packet(1);
	if(packet_length < 0)
	{
		fprintf(stderr, "Error(%d) on read packet!\n", packet_length);
		return -1;
	}

	if(MQTTParseMessageType(cloud_hb->packet_buffer) != MQTT_MSG_CONNACK)
	{
		fprintf(stderr, "CONNACK expected!\n");
		return -2;
	}

	if(cloud_hb->packet_buffer[3] != 0x00)
	{
		fprintf(stderr, "CONNACK failed!\n");
		return -2;
	}

    signal(SIGPIPE, SIG_IGN);

	return 0;
}

string Heartbeat::heartbeat_handle(string json_data)
{
    Heartbeat *cloud_hb = NULL;
	cloud_hb = Heartbeat::GetInstance();
    timeb t;
    ftime(&t);
    long timestamp_ms = t.time * 1000 + t.millitm;
    rapidjson::StringBuffer strBuf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(strBuf);
    writer.StartObject();
    writer.Key("dId");
    writer.String(cloud_hb->dId.c_str());
    writer.Key("msgFrame");
    writer.String("009");
    writer.Key("protocolVersion");
    writer.String("V2.0");
    writer.Key("dEsn");
    writer.String("2930993833");
    writer.Key("timestamp");
    writer.Int64(timestamp_ms);
    writer.Key("faultCodes");
    writer.StartArray();
    writer.StartObject();
    writer.Key("faultCode");
    writer.String("00000");
    writer.Key("faultDesc");
    writer.String("正常");
    writer.EndObject();
    writer.EndArray();
    writer.Key("seqNum");
    writer.String("3456778");
    writer.Key("ack");
    writer.Bool("false");
    writer.EndObject();

    json_data = strBuf.GetString();
    return json_data;
}

int Heartbeat::send_heart()
{
    Heartbeat *cloud_hb = NULL;
	cloud_hb = Heartbeat::GetInstance();
    while(1)
    {
        string send_data = cloud_hb->heartbeat_handle(cloud_hb->json_conf);
        cout << send_data << endl;
        cout << topic << endl;
        mqtt_publish(&cloud_hb->broker, cloud_hb->topic.c_str(), send_data.c_str(), 0);
        sleep(10);
    }
    return 0;
}
