#include "asm/includes.h"
#include "generic/gpio.h"
#include "asm/rdec.h"

#define RDEC_DEBUG_ENABLE
#ifdef RDEC_DEBUG_ENABLE
#define rdec_debug(fmt, ...) 	printf("[RDEC] "fmt, ##__VA_ARGS__)
#else
#define rdec_debug(...)
#endif

#define rdec_error(fmt, ...) 	printf("[RDEC] ERR: "fmt, ##__VA_ARGS__)


struct rdec {
    u8 init;
    const struct rdec_platform_data *user_data;
};

static struct rdec _rdec = {0};

#define __this 			(&_rdec)

typedef struct {
    u32 con;
    int dat;
    int smp;
} RDEC_REG;

#define     JL_RDEC_SEL         0
#define     RDEC_IRQ_MODE       0


RDEC_REG *rdec_get_reg(u8 index)
{
    RDEC_REG *reg = NULL;

    switch (index) {
    case 0:
        reg = (RDEC_REG *)JL_RDEC0;
        break;
    case 1:
        reg = (RDEC_REG *)JL_RDEC1;
        break;
    case 2:
        reg = (RDEC_REG *)JL_RDEC2;
        break;
    }

    return reg;
}

static void __rdec_port_init(u8 port)
{
    gpio_set_pull_down(port, 0);
    gpio_set_pull_up(port, 1);
    gpio_set_die(port, 1);
    gpio_set_direction(port, 1);
}

static void rdec_port_init(const struct rdec_device *rdec)
{
    const u32 d0_table[] = {PFI_RDEC0_DAT0, PFI_RDEC1_DAT0, PFI_RDEC2_DAT0};
    const u32 d1_table[] = {PFI_RDEC0_DAT1, PFI_RDEC1_DAT1, PFI_RDEC2_DAT1};
    u32 index = rdec->index;

    __rdec_port_init(rdec->sin_port0);
    __rdec_port_init(rdec->sin_port1);

    printf("rdec->sin_port0 = %d", rdec->sin_port0);
    printf("rdec->sin_port1 = %d", rdec->sin_port1);

    gpio_set_fun_input_port(rdec->sin_port0, d0_table[index], LOW_POWER_FREE);
    gpio_set_fun_input_port(rdec->sin_port1, d1_table[index], LOW_POWER_FREE);
}

static s8 rdec_isr(u32 index)
{
    s8 rdat = 0;
    s8 rdat1 = 0;
    static int z = 0;
    RDEC_REG *reg = NULL;

    reg = rdec_get_reg(index);
    if (reg->con & BIT(7)) {
        rdat = reg->dat;
        reg->con |= BIT(6); //clr pending, DAT reg will reset as well
        rdat1 = reg->dat;
        //TODO: notify rdec event
        z += rdat;
        printf("RDEC%d isr, dat: %d, %d %d", index, rdat, rdat1, z);

    }
    return rdat;
}
___interrupt
static void rdec0_isr()
{
    rdec_isr(0);
}

___interrupt
static void rdec1_isr()
{
    rdec_isr(1);
}

___interrupt
static void rdec2_isr()
{
    rdec_isr(2);
}

static void log_rdec_info(u32 index)
{
    RDEC_REG *reg = NULL;
    reg = rdec_get_reg(index);
    rdec_debug("RDEC CON = 0x%x", reg->con);
    printf("FI_RDEC3_DA0 = 0x%x", JL_IMAP->FI_RDEC0_DAT0);
    printf("FI_RDEC3_DA1 = 0x%x", JL_IMAP->FI_RDEC0_DAT1);

    printf("PB_DIR = 0x%x", JL_PORTB->DIR);
    printf("PB_OUT = 0x%x", JL_PORTB->OUT);
    printf("PB_PU = 0x%x",  JL_PORTB->PU);
    printf("PB_PD = 0x%x",  JL_PORTB->PD);
}



//for test
int rdec_init(const struct rdec_platform_data *user_data)
{
    u8 i;
    RDEC_REG *reg = NULL;
    const struct rdec_device *rdec;
    rdec_debug("rdec init...");

    __this->init = 0;

    if (user_data == NULL) {
        return -1;
    }

    for (i = 0; i < user_data->num; i++) {
        //rdec = &(user_data->rdec[i]);
        u32 index = user_data->rdec[i].index;
        reg = rdec_get_reg(index);

        rdec_port_init(&(user_data->rdec[i]));
        //module init
        reg->con = 0;
        reg->con |= (0xe << 2); //2^14, fpga lsb = 24MHz, 2^14 / 24us = 682us = 0.68ms
        reg->con &= ~BIT(1); //pol = 0, io should pull up
        //reg->con |= BIT(1); //pol = 1, io should pull down
        reg->con |= BIT(6); //clear pending
        reg->con |= BIT(8); //mouse mode
        reg->smp = 0;
        /* reg->con |= BIT(9); // */
        reg->con |= BIT(0); //RDECx EN

#if RDEC_IRQ_MODE
        const u32 irq_table[] = {IRQ_RDEC0_IDX, IRQ_RDEC1_IDX, IRQ_RDEC2_IDX};
        const void (*irq_fun[3])(void) = {rdec0_isr, rdec1_isr, rdec2_isr};
        request_irq(irq_table[index], 1, irq_fun[index], 0);
#endif
        log_rdec_info(index);
    }

    __this->init = 1;
    __this->user_data = user_data;


    return 0;
}


const struct rdec_device rdec_device_list[] = {
    {
        .index = JL_RDEC_SEL,
        .sin_port0 = IO_PORTB_06,
        .sin_port1 = IO_PORTB_07,
    },
};

// *INDENT-OFF*
RDEC_PLATFORM_DATA_BEGIN(rdec_data)
.enable = 1,
.num = ARRAY_SIZE(rdec_device_list),
.rdec = rdec_device_list,
RDEC_PLATFORM_DATA_END()
#include "timer.h"

static void rdec_poll(void *p)
{
    for (int i = 0; i < 3; i++) {
        rdec_isr(i);
    }
}

void rdec_test(void)
{
    rdec_init(&rdec_data);

#if !RDEC_IRQ_MODE
    sys_timer_add(NULL, rdec_poll, 100);
#endif
}
