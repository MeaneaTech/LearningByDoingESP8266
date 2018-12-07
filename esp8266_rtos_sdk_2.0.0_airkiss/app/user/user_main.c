/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "esp_common.h"

#include "smartconfig.h"
#include "airkiss.h"
#include "gpio.h"

//ETSTimer
os_timer_t btntimer;


/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;
        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;
        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;
        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

void ICACHE_FLASH_ATTR smartconfig_done(sc_status status, void *pdata) {

	switch (status) {

	//连接未开始，请勿在此阶段开始连接
	case SC_STATUS_WAIT:
		printf("SC_STATUS_WAIT\n");
		break;

	//发现信道
	case SC_STATUS_FIND_CHANNEL:
		printf("SC_STATUS_FIND_CHANNEL\n");
		break;


	//得到wifi名字和密码
	case SC_STATUS_GETTING_SSID_PSWD:
		printf("SC_STATUS_GETTING_SSID_PSWD\n");
		sc_type *type = pdata;
		if (*type == SC_TYPE_ESPTOUCH) {
			printf("SC_TYPE:SC_TYPE_ESPTOUCH\n");
		} else {
			printf("SC_TYPE:SC_TYPE_AIRKISS\n");
		}
		break;

	case SC_STATUS_LINK:
		printf("SC_STATUS_LINK\n");
		struct station_config *sta_conf = pdata;

		wifi_station_set_config(sta_conf);
		wifi_station_disconnect();
		wifi_station_connect();
		break;

	//成功获取到IP，连接路由完成。
	case SC_STATUS_LINK_OVER:
		printf("SC_STATUS_LINK_OVER \n\n");
		if (pdata != NULL) {
			uint8 phone_ip[4] = { 0 };
			memcpy(phone_ip, (uint8*) pdata, 4);
			printf("Phone ip: %d.%d.%d.%d\n", phone_ip[0], phone_ip[1],	phone_ip[2], phone_ip[3]);
		}

		//停止配置
		smartconfig_stop();
		break;
	}

}



void ICACHE_FLASH_ATTR
smartconfig_task(void *pvParameters)
{
	printf("----------Begin to airkiss\n\n\n-----");
	smartconfig_set_type(SC_TYPE_ESPTOUCH_AIRKISS); //SC_TYPE_ESPTOUCH,SC_TYPE_AIRKISS,SC_TYPE_ESPTOUCH_AIRKISS
    smartconfig_start(smartconfig_done);

    vTaskDelete(NULL);
}



void ICACHE_FLASH_ATTR btncheck()
{
	uint8 statu = 0;
	os_timer_disarm(&btntimer);
	if(GPIO_INPUT_GET(GPIO_ID_PIN(0))==0x00)
	{
		os_printf("start net config\r\n");
		while(GPIO_INPUT_GET(GPIO_ID_PIN(0))==0x00);
		statu = 2;
		smartconfig_set_type(SC_TYPE_ESPTOUCH_AIRKISS); //SC_TYPE_ESPTOUCH,SC_TYPE_AIRKISS,SC_TYPE_ESPTOUCH_AIRKISS
		smartconfig_start(smartconfig_done);

	}
	os_timer_arm(&btntimer, 200, 0);
}


/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
	struct station_config s_staconf;

	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U,FUNC_GPIO0);//GPIO0
	GPIO_DIS_OUTPUT(GPIO_ID_PIN(0));
	//GPIO_AS_INPUT(1<<0);

    printf("SDK version:%s\n", system_get_sdk_version());

    /* need to set opmode before you set config */
    wifi_set_opmode(STATION_MODE);

    wifi_station_get_config_default(&s_staconf);

    if (strlen(s_staconf.ssid) != 0)
    {

    	uint8 status = 0;
    	status = wifi_station_get_connect_status();
    	if(status == STATION_GOT_IP)
    	{
    			os_printf("Wifi connect success!\r\n");
    			//wifi_get_ip_info(STATION_IF,&info);
    			//my_server_init(&info.ip,1213);
    	        //return;
    	}
    	else
    	{

    	}

    }
    else
    {
    	xTaskCreate(smartconfig_task, "smartconfig_task", 256, NULL, 2, NULL);
    }

	os_timer_setfn(&btntimer,btncheck,NULL);
	os_timer_arm(&btntimer, 2000, 0);

}

