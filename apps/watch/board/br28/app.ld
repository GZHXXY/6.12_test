#include "app_config.h"


SECTIONS
{
    .text : ALIGN(4)
    {
        gsensor_dev_begin = .;
        KEEP(*(.gsensor_dev))
        gsensor_dev_end = .;

        . = ALIGN(4);
        hrsensor_dev_begin = .;
        KEEP(*(.hrsensor_dev))
        hrsensor_dev_end = .;


        . = ALIGN(4);
        fm_dev_begin = .;
        KEEP(*(.fm_dev))
        fm_dev_end = .;

        fm_emitter_dev_begin = .;
        KEEP(*(.fm_emitter_dev))
        fm_emitter_dev_end = .;

        storage_device_begin = .;
        KEEP(*(.storage_device))
        storage_device_end = .;

        p11_event_handler_begin = .;
        KEEP(*(.p11_irq_handler))
        p11_event_handler_end = .;

        _watch_syscfg_begin = .;
        PROVIDE(watch_syscfg_begin = .);
        KEEP(*(.watch_syscfg))
        _watch_syscfg_end = .;
        PROVIDE(watch_syscfg_end = .);

		. = ALIGN(32);
    }

	.data ALIGN(32):
	{
	} > ram0

	.bss ALIGN(32):
    {
    } > ram0

	.data_code ALIGN(32):
	{
	} > ram0

}
