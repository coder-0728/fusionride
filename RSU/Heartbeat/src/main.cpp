#include <Heartbeat.h>

int main()
{
    Heartbeat *cloud_hb = NULL;
	cloud_hb = Heartbeat::GetInstance();
    cloud_hb->ReadConf(cloud_hb->json_conf);
    cloud_hb->Parse(cloud_hb->json_conf.c_str());
    cloud_hb->init_mqtt_t();
    cloud_hb->send_heart();
    return 0;
}