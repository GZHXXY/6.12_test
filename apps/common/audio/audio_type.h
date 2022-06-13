
#ifndef _AUDIO_TYPE_H_
#define _AUDIO_TYPE_H_


/*
 * audio输出方式的种类
 */
#define AUDIO_WAY_TYPE_DAC        	(1UL << (0))
#define AUDIO_WAY_TYPE_IIS        	(1UL << (1))
#define AUDIO_WAY_TYPE_FM         	(1UL << (2))
#define AUDIO_WAY_TYPE_HDMI       	(1UL << (3))
#define AUDIO_WAY_TYPE_SPDIF      	(1UL << (4))
#define AUDIO_WAY_TYPE_BT      	 	(1UL << (5))
#define AUDIO_WAY_TYPE_DONGLE		(1UL << (6))

#define AUDIO_WAY_TYPE_MAX			7
#define AUDIO_WAY_TYPE_ALL			0x00ffffff


#endif /*_AUDIO_TYPE_H_*/

