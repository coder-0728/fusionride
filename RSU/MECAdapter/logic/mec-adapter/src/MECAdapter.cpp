#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <linux/tcp.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <errno.h>
#include <stddef.h>
#include <iomanip>
#include <pthread.h>
#include "rapidjson/document.h"
#include "MECAdapter.h"

#ifdef __cplusplus
extern "C"
{
#endif
#include "libemqtt.h"
#ifdef __cplusplus
}
#endif

using namespace std;

#define MAX_CONNECT_NUM 2
#define RCVBUFSIZE 1024*16

pthread_mutex_t lock;

const char *rsm_filename = "/tmp/mec_to_rsm.socket";
const char *rsi_filename = "/tmp/mec_adapter_clientRSI.socket";

char rsm_msg[RCVBUFSIZE];
char rsi_msg[RCVBUFSIZE];
int rsi_ready = 0;
int rsm_ready = 0;

uint8_t packet_buffer[RCVBUFSIZE];
int keepalive = 10;
mqtt_broker_handle_t broker;

int socket_id;

int Port;
string IP;

// 读配置文件
void Parse(const char *json_str)
{

	rapidjson::Document doc;
	//首先进行解析，没有解析错误才能进行具体字段的解析
	if (!doc.Parse(json_str).HasParseError())
	{
		if (doc.HasMember("Port") && doc["Port"].IsInt())
		{
			Port = doc["Port"].GetInt();
		}
		if (doc.HasMember("IP") && doc["IP"].IsString())
		{
			IP = doc["IP"].GetString();
		}
	}
}

string ReadConf(string &json)
{

	FILE *conf_fd = fopen("../config/mec_adapter_conf.json", "rb");
	if (!conf_fd)
	{
		cout << "open mec_adapter conf file error!" << endl;
		return NULL;
	}

	if (conf_fd)
	{
		char *buf = new char[RCVBUFSIZE];
		int n = fread(buf, 1, RCVBUFSIZE, conf_fd); // 将读取的内容转换为dom元素
		fclose(conf_fd);
		if (n >= 0)
			json.append(buf, 0, n);
		delete[] buf;
		buf = nullptr;
	}
	return json;
}

int MECAdapter::uds_socket_server(const char *filename)
{
	int fd;
	int len;
	struct sockaddr_un un, client_addr;
	len = sizeof(client_addr);
	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0)
	{
		cout << "Request socket failecd!" << endl;
		return -1;
	}
	un.sun_family = AF_UNIX;
	unlink(filename);
	strcpy(un.sun_path, filename);

	if (bind(fd, (struct sockaddr *)&un, sizeof(un)) < 0)
	{
		cout << "bind failed!" << endl;
		return -1;
	}

	if (listen(fd, MAX_CONNECT_NUM) < 0)
	{
		cout << "listen failed!" << endl;
		return -1;
	}
	return fd;
}

void *send_rsm(void *data)
{
	int rsm_fd;
	MECAdapter uds_rsm;
	rsm_fd = uds_rsm.uds_socket_server(rsm_filename);
	int new_rsm_fd = accept(rsm_fd, NULL, NULL);
	if (new_rsm_fd < 0)
	{
		printf("rsm accept failed\n");
	}

	while (1)
	{
		if (rsm_ready == 1)
		{
			pthread_mutex_lock(&lock);
			rsm_ready = 0;
			pthread_mutex_unlock(&lock);
			char rsm_buf[RCVBUFSIZE] = {0};
			char head_buf[2] = {0};		   // 数据长度申请2字节空间
			strcpy(rsm_buf, (char *)data);
			int rsm_len = strlen(rsm_buf); // 获取消息长度
			head_buf[0] = rsm_len / 256;
			head_buf[1] = rsm_len % 256;
			send(new_rsm_fd, head_buf, 2, MSG_NOSIGNAL); // 发送rsm消息长度
			cout << "rsm数据长度:" << rsm_len << endl;
			send(new_rsm_fd, rsm_buf, rsm_len, MSG_NOSIGNAL);
			cout << "rsm消息已发送" << endl;
		}
	}
	// close(rsm_fd);
	// close(new_rsm_fd);
}

void *send_rsi(void *data)
{
	int rsi_fd;
	MECAdapter uds_rsi;
	rsi_fd = uds_rsi.uds_socket_server(rsi_filename); // 建立rsi的uds连接
	int new_rsi_fd = accept(rsi_fd, NULL, NULL);
	if (new_rsi_fd < 0)
	{
		cout << "rsi accept failed!" << endl;
	}
	while (1)
	{
		if (rsi_ready == 1)
		{
			pthread_mutex_lock(&lock);
			rsi_ready = 0;
			pthread_mutex_unlock(&lock);
			char rsi_buf[RCVBUFSIZE] = {0};
			// char head_buf[4] = {0}; // 数据长度申请4字节空间
			strcpy(rsi_buf, (char *)data);
			int rsi_len = strlen(rsi_buf); // 获取消息长度
			// sprintf(head_buf, "%d",rsi_len);

			// send(new_rsi_fd, head_buf, 4, MSG_NOSIGNAL); // 发送rsi消息长度
			// cout << "rsi数据长度:" << rsi_len << endl;
			send(new_rsi_fd, rsi_buf, rsi_len, MSG_NOSIGNAL); // 开始发送rsi消息内容
			cout << "rsi消息已发送" << endl;
		}
	}
	// close(rsm_fd);
	// close(new_rsm_fd);
}

int send_packet(void *socket_info, const void *buf, unsigned int count)
{
	int fd = *((int *)socket_info);
	return send(fd, buf, count, 0);
}

int init_socket(mqtt_broker_handle_t *broker, const char *hostname, short port, int keepalive)
{
	int flag = 1;

	// Create the socket
	if ((socket_id = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		return -1;

	// Disable Nagle Algorithm
	if (setsockopt(socket_id, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag)) < 0)
		return -2;

	struct sockaddr_in socket_address;
	// Create the stuff we need to connect
	socket_address.sin_family = AF_INET;
	socket_address.sin_port = htons(port);
	socket_address.sin_addr.s_addr = inet_addr(hostname);

	// Connect the socket
	if ((connect(socket_id, (struct sockaddr *)&socket_address, sizeof(socket_address))) < 0)
		return -1;

	// MQTT stuffs
	mqtt_set_alive(broker, keepalive);
	broker->socket_info = (void *)&socket_id;
	broker->send = send_packet;

	return 0;
}

int close_socket(mqtt_broker_handle_t *broker)
{
	int fd = *((int *)broker->socket_info);
	return close(fd);
}

int read_packet(int timeout)
{
	if (timeout > 0)
	{
		fd_set readfds;
		struct timeval tmv;

		// Initialize the file descriptor set
		FD_ZERO(&readfds);
		FD_SET(socket_id, &readfds);

		// Initialize the timeout data structure
		tmv.tv_sec = timeout;
		tmv.tv_usec = 0;

		// select returns 0 if timeout, 1 if input available, -1 if error
		if (select(1, &readfds, NULL, NULL, &tmv))
			return -2;
	}

	int total_bytes = 0, bytes_rcvd, packet_length;
	memset(packet_buffer, 0, sizeof(packet_buffer));

	if ((bytes_rcvd = recv(socket_id, (packet_buffer + total_bytes), RCVBUFSIZE, 0)) <= 0)
	{
		return -1;
	}

	total_bytes += bytes_rcvd; // Keep tally of total bytes
	if (total_bytes < 2)
		return -1;

	// now we have the full fixed header in packet_buffer
	// parse it for remaining length and number of bytes
	uint16_t rem_len = mqtt_parse_rem_len(packet_buffer);
	uint8_t rem_len_bytes = mqtt_num_rem_len_bytes(packet_buffer);

	// packet_length = packet_buffer[1] + 2; // Remaining length + fixed header length
	//  total packet length = remaining length + byte 1 of fixed header + remaning length part of fixed header
	packet_length = rem_len + rem_len_bytes + 1;

	while (total_bytes < packet_length) // Reading the packet
	{
		if ((bytes_rcvd = recv(socket_id, (packet_buffer + total_bytes), RCVBUFSIZE, 0)) <= 0)
			return -1;
		total_bytes += bytes_rcvd; // Keep tally of total bytes
	}

	return packet_length;
}

void alive(int sig)
{
	cout << "Timeout! Sending ping..." << endl;
	mqtt_ping(&broker);

	alarm(keepalive);
}

// void sigPipeHandler(int sig)
// {
// 	cout << "receive signal SIGPIPE" << endl;
// 	//mqtt_ping(&broker);
// 	//alarm(keepalive);
// }

void term(int sig)
{
	cout << "Goodbye!" << endl;
	// DISCONNECT
	mqtt_disconnect(&broker);
	close_socket(&broker);

	exit(0);
}

int main()
{
	uint8_t topic[255], msg[RCVBUFSIZE];
	int packet_length;
	uint16_t msg_id, msg_id_rcv;

	string conf_json;
	ReadConf(conf_json);
	Parse(conf_json.c_str());

	mqtt_init(&broker, "client-id");
	int mqtt_ret = init_socket(&broker, IP.c_str(), Port, keepalive);
	while (1)
	{
		if (mqtt_ret < 0)
		{
			cout << "connect MQTT-Server failed, try again……" << endl;
			mqtt_ret = init_socket(&broker, IP.c_str(), Port, keepalive);
			sleep(5);
		}
		else
		{
			cout << "connect success,wait message" << endl;
			break;
		}

		// cout << "connect MQTT-Server failed" << endl;
		// return -1;
	}

	mqtt_connect(&broker);

	packet_length = read_packet(1);
	if (packet_length < 0)
	{
		cout << "Error(%d) on read packet!" << packet_length << endl;
		return -1;
	}

	if (MQTTParseMessageType(packet_buffer) != MQTT_MSG_CONNACK)
	{
		cout << "CONNACK expected!" << endl;
		return -2;
	}

	if (packet_buffer[3] != 0x00)
	{
		cout << "CONNACK failed!" << endl;
		return -2;
	}

	// Signals after connect MQTT
	signal(SIGALRM, alive);
	alarm(keepalive);
	signal(SIGINT, term);
	// signal(SIGPIPE, sigPipeHandler);

	// >>>>> SUBSCRIBE
	mqtt_subscribe(&broker, "rsu/#", &msg_id);
	// <<<<< SUBACK
	packet_length = read_packet(1);
	if (packet_length < 0)
	{
		cout << "Error(%d) on read packet!" << packet_length << endl;
		return -1;
	}

	if (MQTTParseMessageType(packet_buffer) != MQTT_MSG_SUBACK)
	{
		cout << "SUBACK expected!" << endl;
		return -2;
	}

	msg_id_rcv = mqtt_parse_msg_id(packet_buffer);
	if (msg_id != msg_id_rcv)
	{
		cout << msg_id << "message id was expected"
			 << "but" << msg_id_rcv << "message id was found!" << endl;
		return -3;
	}

	// 建立uds连接
	pthread_t rsm;
	pthread_t rsi;
	int count = 0;

	pthread_create(&rsm, NULL, send_rsm, (void *)rsm_msg);
	pthread_detach(rsm);
	pthread_create(&rsi, NULL, send_rsi, (void *)rsi_msg);
	pthread_detach(rsi);

	while (1)
	{
		packet_length = read_packet(0);
		if (packet_length == -1)
		{
			cout << "Error(" << packet_length << ") on read packet!" << endl;
			return -1;
		}
		else if (packet_length > 0)
		{
			cout << "Packet Header: 0x" << hex << packet_length << "..." << endl;
			if (MQTTParseMessageType(packet_buffer) == MQTT_MSG_PUBLISH)
			{
				uint16_t len;
				len = mqtt_parse_pub_topic(packet_buffer, topic);
				topic[len] = '\0'; // for printf
				len = mqtt_parse_publish_msg(packet_buffer, msg);
				msg[len] = '\0'; // for printf
				count++;
				cout << "第" << dec << count << "条消息" << endl;

				if (!strcmp((char *)topic, "rsu/rsi"))
				{
					strcpy(rsi_msg, (char *)msg);
					pthread_mutex_lock(&lock);
					rsi_ready = 1;
					pthread_mutex_unlock(&lock);
					cout << topic << "  " << rsi_msg << endl;
				}
				else if (!strcmp((char *)topic, "rsu/rsm"))
				{
					strcpy(rsm_msg, (char *)msg);
					pthread_mutex_lock(&lock);
					rsm_ready = 1;
					pthread_mutex_unlock(&lock);
					cout << topic << "  " << rsm_msg << endl;
				}
			}
		}
	}

	return 0;
}
