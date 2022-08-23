#include <iostream>
#include <libemqtt.h>


#ifndef _MECADAPTER_H_
#define _MECADAPTER_H_

using namespace std;

class MECAdapter
{
public:
	
	int uds_socket_server(const char *filename);
};


#endif