#include "gSensor/gSensor_manage.h"
#include "app_config.h"


#if TCFG_P11GSENSOR_EN

#include "user_p11_cmd.h"

int p11_gsensor_cbuf_read(u8 *data, u16 len)
{
    int ret = 0;
    ret =  master_p11cbuf_read(0, data, len);
    if (ret == 0) {
        printf("%s %d ret =%d,len=%d\n", __FUNCTION__, __LINE__, ret, len);
    }
    return ret;
}


static void sensor_ctl(u8 cmd, void *arg)
{
    char res;
    switch (cmd) {
    case GSENSOR_DISABLE:
        res = 0;//SC7A20_disable();
        user_main_post_to_p11_system(P11_SYS_GSENSOR_STOP, 0, NULL, 1);
        memcpy(arg, &res, 1);
        break;
    case GSENSOR_RESET_INT:
        /* res = SC7A20_Config(); */
        user_main_post_to_p11_system(P11_SYS_GSENSOR_START, 0, NULL, 1);
        res = 0;
        memcpy(arg, &res, 1);
        break;
    case GSENSOR_RESUME_INT:
        break;
    case READ_GSENSOR_DATA:
        printf("%s %d\n", __FUNCTION__, __LINE__);
        break;
    case SEARCH_SENSOR:
        res = 1;
        memcpy(arg, &res, 1);
        break;
    default:

        break;
    }
}


static u8 gsensor_init(void)
{
    printf("p11 gsensor init\n");
    //返回0 成功
    return 0;
}


REGISTER_GRAVITY_SENSOR(gSensor) = {
    .logo = "p11gsensor",
    .gravity_sensor_init  = gsensor_init,
    .gravity_sensor_check = NULL,
    .gravity_sensor_ctl   = sensor_ctl,
};

#endif
