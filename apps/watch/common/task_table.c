#include "system/includes.h"
#include "app_config.h"


/*任务列表 */
const struct task_info task_info_table[] = {
    {"app_core",            1,      0,  1024,  1024  },
    {"sys_event",           6,      0,  256,   0    },
    {"btctrler",            4,      0,  512,   512  },
    {"btencry",             1,      0,  512,   128  },
    {"tws",                 5,      0,  512,   128  },
    {"btstack",             3,      0,  768,   512  },

    {"spo2read",			1,	    0,  256	, 	0},
    {"heartrate",			1,		0,  256,	0},
    {"gsensor",				1,	    0,  768,     0  },

#if (TCFG_USER_TWS_ENABLE && TCFG_REVERB_ENABLE)
    {"audio_dec",           3,      1,  768 + 128,   128  },
#else
    {"audio_dec",           3,      1,  768 + 32,   128  },
#endif
    {"dev_mg",           	3,      0,  512,   512  },
    {"audio_enc",           3,      0,  512,   128  },
    {"usb_msd",           	1,      0,  512,   128  },
    {"aec",					2,	    0,  768,   128	},
    {"aec_dbg",				3,	    0,  128,   128	},
    {"update",				1,	    0,  512,   0    },
    {"dw_update",		 	2,	    0,  256,   128  },
#ifdef USER_UART_UPDATE_ENABLE
    {"uart_update",	        1,	    0,  256,   128	},
#endif
    {"systimer",		    6,	    0,  128,   0    },
    {"usb_audio",           5,      0,  256,   256  },
    {"plnk_dbg",            5,      0,  256,   256  },
    {"adc_linein",          2,      0,  768,   128  },
    {"enc_write",           1,      0,  768,   0 	},
    /* {"src_write",           1,      0,  768,   256 	}, */
    {"fm_task",             3,      0,  512,   128  },
#if (RCSP_BTMATE_EN || RCSP_ADV_EN)
    {"rcsp_task",			4,	    0,  768,   128	},
#endif
#if TCFG_SPI_LCD_ENABLE
#if TCFG_LUA_ENABLE
    {"ui",           	    2,      1,  1024 * 2,   1024  },
#else
    {"ui",           	    2,      1,  1024,   1024  },
#endif
#else
    {"ui",                  3,      1,  384,   128  },
#endif

#if(TCFG_CHARGE_BOX_ENABLE)
    {"chgbox_n",            6,      0,  512,   128  },
#endif
#if (SMART_BOX_EN)
    {"smartbox",            4,      0,  512,   128  },
#if (TCFG_DEV_MANAGER_ENABLE)
    {"ftran_back",		    1,	    0,  512,	  0  },
#endif
#if (JL_SMART_BOX_SENSORS_DATA_OPT)
    {"w_nfc_back",		    1,	    0,  512,	  0  },
#endif
#endif//SMART_BOX_EN
#if RCSP_FILE_OPT
    {"file_bs",				1,	    0,  768,	  0  },
#endif
    {"data_export",				1,	    0,  768,	  0  },
    {"mic_stream",          5,      0,  768,   128  },
    {0, 0},
};


