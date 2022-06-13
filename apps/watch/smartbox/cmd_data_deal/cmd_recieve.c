#include "smartbox/config.h"
#include "smartbox/smartbox.h"
#include "smartbox/function.h"
#include "smartbox/feature.h"
#include "smartbox/switch_device.h"
#include "file_transfer.h"
#include "file_delete.h"
#include "file_env_prepare.h"
#include "file_trans_back.h"
#include "dev_format.h"
#include "smartbox/event.h"
#include "btstack/avctp_user.h"
#include "app_action.h"
#include "smartbox_adv_bluetooth.h"
#include "smartbox_extra_flash_cmd.h"
#include "smartbox_update.h"
#include "fs.h"
#include "file_bluk_trans_prepare.h"
#include "sensors_data_opt.h"
#include "file_simple_transfer.h"
#include "custom_cfg.h"
#include "JL_rcsp_api.h"

////<<<<<<<<APP 下发命令响应处理
#if (SMART_BOX_EN)

#define RES_MD5_FILE	SDFILE_RES_ROOT_PATH"md5.bin"

#define ASSET_CMD_DATA_LEN(len, limit) 	\
	do{	\
		if(len >= limit){	\
		}else{				\
			return ;   \
		}\
	}while(0);


static void get_target_feature(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    struct smartbox *smart = (struct smartbox *)priv;
    if (smart == NULL) {
        return ;
    }

    smart->A_platform = data[4];

    u32 mask = READ_BIG_U32(data);
    u32 wlen = 0;
    u8 *resp = zalloc(TARGET_FEATURE_RESP_BUF_SIZE);
    ASSERT(resp, "func = %s, line = %d, no ram!!\n", __FUNCTION__, __LINE__);
    wlen = target_feature_parse_packet(priv, resp, TARGET_FEATURE_RESP_BUF_SIZE, mask);
    JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, resp, wlen);
    free(resp);
}

static void switch_device(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    struct smartbox *smart = (struct smartbox *)priv;
    if (smart == NULL) {
        return ;
    }
    smart->trans_chl = data[0];//指spp还是ble
    printf("smart->trans_chl:%x\n", smart->trans_chl);
    JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, NULL, 0);
    smartbox_switch_device(data);
}

static void get_sys_info(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    ASSET_CMD_DATA_LEN(len, 1);
    u8 function = data[0];
    u8 *resp = zalloc(TARGET_FEATURE_RESP_BUF_SIZE);
    resp[0] = function;
    u32 rlen = smartbox_function_get(priv, function, data + 1, len - 1, resp + 1, TARGET_FEATURE_RESP_BUF_SIZE - 1);
    if (rlen == 0) {
        printf("get_sys_info LTV NULL !!!!\n");
    }
    JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, resp, (u16)rlen + 1);
    free(resp);
}

static void set_sys_info(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    ASSET_CMD_DATA_LEN(len, 1);

    u8 function = data[0];
    bool ret = smartbox_function_set(priv, function, data + 1, len - 1);
    if (ret == true) {
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, NULL, 0);
    } else {
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_FAIL, OpCode_SN, NULL, 0);
    }
}

static void disconnect_edr(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    struct smartbox *smart = (struct smartbox *)priv;
    if (smart == NULL) {
        return ;
    }
    printf("notify disconnect edr\n");
    u8 op = 0;
    if (len) {
        op = data[0];
    }

    switch (op) {
    case 0 :
        smartbox_msg_post(USER_MSG_SMARTBOX_DISCONNECT_EDR, 1, (int)priv);
        break;
    case 1:
        smartbox_msg_post(USER_MSG_SMARTBOX_CONNECT_RESET, 1, (int)priv);
#if (0 == BT_CONNECTION_VERIFY)
        JL_rcsp_auth_reset();
#endif
        break;
    }
    JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, NULL, 0);
}

static void file_bs_start(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    /* printf("JL_OPCODE_FILE_BROWSE_REQUEST_START\n"); */
    /* put_buf(data, len); */
#if RCSP_FILE_OPT
    if (smartbox_browser_busy() == false) {
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_FAIL, OpCode_SN, NULL, 0);
    } else {
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, NULL, 0);
        smartbox_browser_start(data, len);
    }
#endif
}

static void open_bt_scan(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    u8 result = 0;
    bool ret = smartbox_msg_post(USER_MSG_SMARTBOX_BT_SCAN_OPEN, 1, (int)priv);
    if (ret == true) {
        result = 1;
    }
    JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, &result, 1);
}

static void stop_bt_scan(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    u8 result = 0;
    bool ret = smartbox_msg_post(USER_MSG_SMARTBOX_BT_SCAN_STOP, 1, (int)priv);
    if (ret == true) {
        result = 1;
    }
    JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, &result, 1);
}

static void connect_bt_spec_addr(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    u8 result = 0;
    struct smartbox *smart = (struct smartbox *)priv;
    if (smart == NULL) {
        return ;
    }
    if (len != 6) {
        printf("connect_bt_spec_addr data len err = %d\n", len);
    }
    memcpy(smart->emitter_con_addr, data, 6);
    bool ret = smartbox_msg_post(USER_MSG_SMARTBOX_BT_CONNECT_SPEC_ADDR, 1, (int)priv);
    if (ret == true) {
        result = 1;
    }
    JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, &result, 1);
}

static void function_cmd_handle(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    ASSET_CMD_DATA_LEN(len, 1);

    u8 function = data[0];
    bool ret = smartbox_function_cmd_set(priv, function, data + 1, len - 1);
    if (ret == true) {
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, NULL, 0);
    } else {
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_FAIL, OpCode_SN, NULL, 0);
    }
}

static void find_device_handle(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
#if RCSP_ADV_FIND_DEVICE_ENABLE
    struct smartbox *smart = (struct smartbox *)priv;
    if (smart == NULL) {
        return ;
    }
    if (!smart->find_dev_en) {
        return;
    }
    if (BT_CALL_HANGUP != get_call_status()) {
        return ;
    }
    u8 type = data[0];
    u8 opt = data[1];
    if (opt) {
        // 播放铃声
        u16 timeout = READ_BIG_U16(data + 2);
        extern void find_device_timeout_handle(u32 sec);
        find_device_timeout_handle(timeout);
    } else {
        // 关闭铃声
        extern void smartbox_stop_find_device(void *priv);
        smartbox_stop_find_device(NULL);
        extern void smartbox_find_phone_reset(void);
        smartbox_find_phone_reset();
        extern u8 ui_return_page_pop(u8 dir);
        ui_return_page_pop(1);
    }
    JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, NULL, 0);
#endif
}

static void get_md5_handle(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    struct smartbox *smart = (struct smartbox *)priv;
    if (smart == NULL) {
        return ;
    }
    u8 md5[32] = {0};
    FILE *fp = fopen(RES_MD5_FILE, "r");
    if (!fp) {
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, NULL, 0);
        return;
    }
    u32 r_len = fread(fp, (void *)md5, 32);
    if (r_len != 32) {
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, NULL, 0);
        return;
    }
    /* printf("get [md5] succ:"); */
    /* put_buf(md5, 32); */
    JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, md5, 32);
    fclose(fp);
}

static void get_low_latency_param(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    struct smartbox *smart = (struct smartbox *)priv;
    if (smart == NULL) {
        return ;
    }
    u8 low_latency_param[6] = {0};
    JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, low_latency_param, 6);
}

static void smartbox_device_parm_extra(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    u8 op = data[0];
    switch (op) {
    case 0x0:
        file_transfer_download_parm_extra(OpCode_SN, data, len);
        break;;
    case 0x1:
        break;;
    case 0x2:
        break;;
    default:
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_PARAM_ERR, OpCode_SN, 0, 0);
        break;
    }
}
void cmd_recieve(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    switch (OpCode) {
    case JL_OPCODE_GET_TARGET_FEATURE:
        get_target_feature(priv, OpCode, OpCode_SN, data, len);
        break;
    case JL_OPCODE_DISCONNECT_EDR:
        disconnect_edr(priv, OpCode, OpCode_SN, data, len);
        break;
    case JL_OPCODE_SWITCH_DEVICE:
        switch_device(priv, OpCode, OpCode_SN, data, len);
        break;
    case JL_OPCODE_SYS_INFO_GET:
        get_sys_info(priv, OpCode, OpCode_SN, data, len);
        break;
    case JL_OPCODE_SYS_INFO_SET:
        set_sys_info(priv, OpCode, OpCode_SN, data, len);
        break;
    case JL_OPCODE_FILE_BROWSE_REQUEST_START:
        file_bs_start(priv, OpCode, OpCode_SN, data, len);
        break;
    case JL_OPCODE_SYS_OPEN_BT_SCAN:
        open_bt_scan(priv, OpCode, OpCode_SN, data, len);
        break;
    case JL_OPCODE_SYS_STOP_BT_SCAN:
        stop_bt_scan(priv, OpCode, OpCode_SN, data, len);
        break;
    case JL_OPCODE_SYS_BT_CONNECT_SPEC:
        connect_bt_spec_addr(priv, OpCode, OpCode_SN, data, len);
        break;
    case JL_OPCODE_FUNCTION_CMD:
        function_cmd_handle(priv, OpCode, OpCode_SN, data, len);
        break;
    case JL_OPCODE_SYS_FIND_DEVICE:
        find_device_handle(priv, OpCode, OpCode_SN, data, len);
        break;
    case JL_OPCODE_GET_MD5:
        get_md5_handle(priv, OpCode, OpCode_SN, data, len);
        break;
    case JL_OPCODE_LOW_LATENCY_PARAM:
        get_low_latency_param(priv, OpCode, OpCode_SN, data, len);
        break;
    case JL_OPCODE_CUSTOMER_USER:
        smartbox_user_cmd_recieve(priv, OpCode, OpCode_SN, data, len);
        break;
    case JL_OPCODE_FILE_TRANSFER_START:
        file_transfer_download_start(priv, OpCode_SN, data, len);
        break;
    case JL_OPCODE_FILE_TRANSFER_CANCEL:
        file_transfer_download_passive_cancel(OpCode_SN, data, len);
        break;
    case JL_OPCODE_FILE_DELETE:
        file_delete_start(OpCode_SN, data, len);
        break;
    case JL_OPCODE_ACTION_PREPARE:
        app_smartbox_task_prepare(1, data[0], OpCode_SN);
        break;
    case JL_OPCODE_DEVICE_FORMAT:
        dev_format_start(OpCode_SN, data, len);
        break;
    case JL_OPCODE_DEVICE_PARM_EXTRA:
        smartbox_device_parm_extra(priv, OpCode, OpCode_SN, data, len);
        break;
    case JL_OPCODE_ONE_FILE_DELETE:
        file_delete_one_file(OpCode_SN, data, len);
        break;
    case JL_OPCODE_ONE_FILE_TRANS_BACK:
        file_trans_back_opt(priv, OpCode_SN, data, len);
        break;
    case JL_OPCODE_FILE_BLUK_TRANSFER:
        file_bluk_trans_prepare(priv, OpCode_SN, data, len);
        break;
    case JL_OPCODE_SIMPLE_FILE_TRANS:
        file_simple_transfer_for_small_file(priv, OpCode_SN, data, len);
        break;
    default:
        if (0 == JL_smartbox_sensors_data_opt(priv, OpCode, OpCode_SN, data, len)) {
            break;
        }
#if JL_SMART_BOX_EXTRA_FLASH_OPT
        if (0 == JL_smartbox_extra_flash_cmd_resp(priv, OpCode, OpCode_SN, data, len)) {
            break;
        }
#endif
#if RCSP_SMARTBOX_ADV_EN
        if (0 == JL_smartbox_adv_cmd_resp(priv, OpCode, OpCode_SN, data, len)) {
            break;
        }
#endif
#if RCSP_UPDATE_EN
        if (0 == JL_rcsp_update_cmd_resp(priv, OpCode, OpCode_SN, data, len)) {
            break;
        }
#endif
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_UNKOWN_CMD, OpCode_SN, 0, 0);
        break;
    }
}

#endif//SMART_BOX_EN
