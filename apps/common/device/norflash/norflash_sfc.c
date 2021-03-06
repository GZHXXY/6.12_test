#include "includes.h"
#include "app_config.h"
#include "norflash_sfc.h"

#if (defined(TCFG_NORFLASH_SFC_DEV_ENABLE) && TCFG_NORFLASH_SFC_DEV_ENABLE)

#undef LOG_TAG_CONST
#define LOG_TAG     "[FLASH_SFC]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#include "debug.h"


#define MAX_NORFLASH_PART_NUM       4

struct norflash_partition {
    const char *name;
    u32 start_addr;
    u32 size;
    struct device device;
};

static struct norflash_partition nor_part[MAX_NORFLASH_PART_NUM];

struct norflash_info {
    u32 flash_id;
    u32 flash_capacity;
    int spi_num;
    int spi_err;
    u8 spi_cs_io;
    u8 spi_r_width;
    u8 part_num;
    u8 open_cnt;
    struct norflash_partition *const part_list;
    OS_MUTEX mutex;
    u32 max_end_addr;
};

static struct norflash_info _norflash = {
    .spi_num = (int) - 1,
    .part_list = nor_part,
};

int fat_sfc_norflash_read(u32 addr, u8 *buf, u32 len, u8 cache);
static int _norflash_eraser(u8 eraser, u32 addr);
static void _norflash_cache_sync_timer(void *priv);
static int _norflash_write_pages(u32 addr, u8 *buf, u32 len);

static struct norflash_partition *norflash_find_part(const char *name)
{
    struct norflash_partition *part = NULL;
    u32 idx;
    for (idx = 0; idx < MAX_NORFLASH_PART_NUM; idx++) {
        part = &_norflash.part_list[idx];
        if (part->name == NULL) {
            continue;
        }
        if (!strcmp(part->name, name)) {
            return part;
        }
    }
    return NULL;
}

static struct norflash_partition *norflash_new_part(const char *name, u32 addr, u32 size)
{
    struct norflash_partition *part;
    u32 idx;
    for (idx = 0; idx < MAX_NORFLASH_PART_NUM; idx++) {
        part = &_norflash.part_list[idx];
        if (part->name == NULL) {
            break;
        }
    }
    if (part->name != NULL) {
        log_error("create norflash part fail\n");
        return NULL;
    }
    memset(part, 0, sizeof(*part));
    part->name = name;
    part->start_addr = addr;
    part->size = size;
    if (part->start_addr + part->size > _norflash.max_end_addr) {
        _norflash.max_end_addr = part->start_addr + part->size;
    }
    _norflash.part_num++;
    return part;
}

static void norflash_delete_part(const char *name)
{
    struct norflash_partition *part;
    u32 idx;
    for (idx = 0; idx < MAX_NORFLASH_PART_NUM; idx++) {
        part = &_norflash.part_list[idx];
        if (part->name == NULL) {
            continue;
        }
        if (!strcmp(part->name, name)) {
            part->name = NULL;
            _norflash.part_num--;
        }
    }
}

static int norflash_verify_part(struct norflash_partition *p)
{
    struct norflash_partition *part = NULL;
    u32 idx;
    for (idx = 0; idx < MAX_NORFLASH_PART_NUM; idx++) {
        part = &_norflash.part_list[idx];
        if (part->name == NULL) {
            continue;
        }
        if ((p->start_addr >= part->start_addr) && (p->start_addr < part->start_addr + part->size)) {
            if (strcmp(p->name, part->name) != 0) {
                return -1;
            }
        }
    }
    return 0;
}



#define FLASH_CACHE_ENABLE  		0

#if FLASH_CACHE_ENABLE
static u32 flash_cache_addr;
static u8 *flash_cache_buf = NULL; //??????4K???????????????flash?????????????????????
static u8 flash_cache_is_dirty;
static u16 flash_cache_timer;

#define FLASH_CACHE_SYNC_T_INTERVAL     60

static int _check_0xff(u8 *buf, u32 len)
{
    for (u32 i = 0; i < len; i ++) {
        if ((*(buf + i)) != 0xff) {
            return 1;
        }
    }
    return 0;
}
#endif


static u32 _pow(u32 num, int n)
{
    u32 powint = 1;
    int i;
    for (i = 1; i <= n; i++) {
        powint *= num;
    }
    return powint;
}

static u32 _norflash_read_id()
{
    return sfc_spi_read_id();
}

SEC(.volatile_ram_code)
static u32 sfc1_flash_ext_init()
{
    u32 flash_id = 0;
    //TODO:add ex flash status register
    sfc1_spi_cs(0);
    sfc1_spi_write_byte(0x9f);
    for (int i = 0; i < 3; i++) {
        flash_id |= sfc1_spi_read_byte();
        flash_id <<= 8;
    }
    sfc1_spi_cs(1);
    flash_id >>= 8;
    return flash_id;
}
static int _norflash_init(const char *name, struct norflash_sfc_dev_platform_data *pdata)
{
    log_info("norflash_sfc_init >>>>");

    struct norflash_partition *part;
    if (_norflash.spi_num == (int) - 1) {
        _norflash.spi_num = 1;//
        _norflash.flash_id = 0;
        _norflash.flash_capacity = 0;
        os_mutex_create(&_norflash.mutex);
        _norflash.max_end_addr = 0;
        _norflash.part_num = 0;

        sfc_spi_init((struct sfc_spi_platform_data *)pdata->sfc_spi_pdata);
        sfc1_flash_ext_init();
        printf("ext_flash id %x", sfc1_flash_ext_init());
    }

    part = norflash_find_part(name);

    if (!part) {
        part = norflash_new_part(name, pdata->start_addr, pdata->size);
        ASSERT(part, "not enough norflash partition memory in array\n");
        ASSERT(norflash_verify_part(part) == 0, "norflash partition %s overlaps\n", name);
        log_info("norflash new partition %s\n", part->name);
    } else {
        ASSERT(0, "norflash partition name already exists\n");
    }
    return 0;
}

static void clock_critical_enter()
{

}
static void clock_critical_exit()
{
    /* if (!(_norflash.flash_id == 0 || _norflash.flash_id == 0xffff)) { */
    /* spi_set_baud(_norflash.spi_num, spi_get_baud(_norflash.spi_num)); */
    /* } */
}
CLOCK_CRITICAL_HANDLE_REG(spi_norflash, clock_critical_enter, clock_critical_exit);

static int _norflash_open(void *arg)
{
    int reg = 0;
    os_mutex_pend(&_norflash.mutex, 0);
    log_info("norflash open\n");
    if (!_norflash.open_cnt) {
        _norflash.flash_id = _norflash_read_id();

        log_info("norflash_read_id: 0x%x\n", _norflash.flash_id);
        if ((_norflash.flash_id == 0) || (_norflash.flash_id == 0xffffff)) {
            log_error("read norflash id error !\n");
            reg = -ENODEV;
            goto __exit;
        }
        _norflash.flash_capacity = 64 * _pow(2, (_norflash.flash_id & 0xff) - 0x10) * 1024;
        log_info("norflash_capacity: 0x%x\n", _norflash.flash_capacity);

#if FLASH_CACHE_ENABLE
        flash_cache_buf = (u8 *)malloc(4096);
        ASSERT(flash_cache_buf, "flash_cache_buf is not ok\n");
        flash_cache_addr = 4096;//??????????????????4096??????
        fat_sfc_norflash_read(0, flash_cache_buf, 4096, 1);
        flash_cache_addr = 0;
#endif
        log_info("norflash open success !\n");
    }
    if (_norflash.flash_id == 0 || _norflash.flash_id == 0xffffff)  {
        log_error("re-open norflash id error !\n");
        reg = -EFAULT;
        goto __exit;
    }
    ASSERT(_norflash.max_end_addr <= _norflash.flash_capacity, "max partition end address is greater than flash capacity\n");
    _norflash.open_cnt++;

__exit:
    os_mutex_post(&_norflash.mutex);
    return reg;
}

static int _norflash_close(void)
{
    os_mutex_pend(&_norflash.mutex, 0);
    log_info("norflash close\n");
    if (_norflash.open_cnt) {
        _norflash.open_cnt--;
    }
    if (!_norflash.open_cnt) {
#if FLASH_CACHE_ENABLE
        if (flash_cache_is_dirty) {
            flash_cache_is_dirty = 0;
            _norflash_eraser(IOCTL_ERASE_SECTOR, flash_cache_addr);
            _norflash_write_pages(flash_cache_addr, flash_cache_buf, 4096);
        }
        free(flash_cache_buf);
        flash_cache_buf = NULL;
#endif
        log_info("norflash close done\n");
    }
    os_mutex_post(&_norflash.mutex);
    return 0;
}

int fat_sfc_norflash_read(u32 addr, u8 *buf, u32 len, u8 cache)
{
    int ret = 0;
    os_mutex_pend(&_norflash.mutex, 0);
    ret = sfc_spi_read(addr, buf, len);
    os_mutex_post(&_norflash.mutex);
    if (ret != len) {
        ret = -1;
    } else {
        ret = 0;
    }

    return ret;
}

static int _norflash_write_pages(u32 addr, u8 *buf, u32 len)
{
    /* y_printf("flash write addr = %d, num = %d\n", addr, len); */
    int ret = 0;
    ret = sfc_spi_write_pages(addr, buf, len);
    if (ret != len) {
        ret = -1;
    } else {
        ret = 0;
    }

    return ret;
}

#if FLASH_CACHE_ENABLE
static void _norflash_cache_sync_timer(void *priv)
{
    int reg = 0;
    os_mutex_pend(&_norflash.mutex, 0);
    if (flash_cache_is_dirty) {
        flash_cache_is_dirty = 0;
        reg = _norflash_eraser(IOCTL_ERASE_SECTOR, flash_cache_addr);
        if (reg) {
            goto __exit;
        }
        reg = _norflash_write_pages(flash_cache_addr, flash_cache_buf, 4096);
    }
    if (flash_cache_timer) {
        sys_timeout_del(flash_cache_timer);
        flash_cache_timer = 0;
    }
__exit:
    os_mutex_post(&_norflash.mutex);
}
#endif

static int _norflash_write(u32 addr, void *buf, u32 len, u8 cache)
{
    int reg = 0;
    os_mutex_pend(&_norflash.mutex, 0);

    u8 *w_buf = (u8 *)buf;
    u32 w_len = len;

    /* y_printf("flash write addr = %d, num = %d\n", addr, len); */
#if FLASH_CACHE_ENABLE
    if (!cache) {
        reg = _norflash_write_pages(addr, w_buf, w_len);
        goto __exit;
    }
    u32 align_addr = addr / 4096 * 4096;
    u32 align_len = 4096 - (addr - align_addr);
    align_len = w_len > align_len ? align_len : w_len;
    if (align_addr != flash_cache_addr) {
        if (flash_cache_is_dirty) {
            flash_cache_is_dirty = 0;
            reg = _norflash_eraser(IOCTL_ERASE_SECTOR, flash_cache_addr);
            if (reg) {
                line_inf;
                goto __exit;
            }
            reg = _norflash_write_pages(flash_cache_addr, flash_cache_buf, 4096);
            if (reg) {
                line_inf;
                goto __exit;
            }
        }
        fat_sfc_norflash_read(align_addr, flash_cache_buf, 4096, 0);
        flash_cache_addr = align_addr;
    }
    memcpy(flash_cache_buf + (addr - align_addr), w_buf, align_len);
    if ((addr + align_len) % 4096) {
        flash_cache_is_dirty = 1;
        if (flash_cache_timer) {
            sys_timer_re_run(flash_cache_timer);
        } else {
            flash_cache_timer = sys_timeout_add(0, _norflash_cache_sync_timer, FLASH_CACHE_SYNC_T_INTERVAL);
        }
    } else {
        flash_cache_is_dirty = 0;
        reg = _norflash_eraser(IOCTL_ERASE_SECTOR, align_addr);
        if (reg) {
            line_inf;
            goto __exit;
        }
        reg = _norflash_write_pages(align_addr, flash_cache_buf, 4096);
        if (reg) {
            line_inf;
            goto __exit;
        }
    }
    addr += align_len;
    w_buf += align_len;
    w_len -= align_len;
    while (w_len) {
        u32 cnt = w_len > 4096 ? 4096 : w_len;
        fat_sfc_norflash_read(addr, flash_cache_buf, 4096, 0);
        flash_cache_addr = addr;
        memcpy(flash_cache_buf, w_buf, cnt);
        if ((addr + cnt) % 4096) {
            flash_cache_is_dirty = 1;
            if (flash_cache_timer) {
                sys_timer_re_run(flash_cache_timer);
            } else {
                flash_cache_timer = sys_timeout_add(0, _norflash_cache_sync_timer, FLASH_CACHE_SYNC_T_INTERVAL);
            }
        } else {
            flash_cache_is_dirty = 0;
            reg = _norflash_eraser(IOCTL_ERASE_SECTOR, addr);
            if (reg) {
                line_inf;
                goto __exit;
            }
            reg = _norflash_write_pages(addr, flash_cache_buf, 4096);
            if (reg) {
                line_inf;
                goto __exit;
            }
        }
        addr += cnt;
        w_buf += cnt;
        w_len -= cnt;
    }
#else
    reg = _norflash_write_pages(addr, w_buf, w_len);
#endif
__exit:
    os_mutex_post(&_norflash.mutex);
    return reg;
}

static int _norflash_eraser(u8 eraser, u32 addr)
{
    int ret = 0;

    ret = sfc_spi_eraser(eraser, addr);

    return ret;
}

static int _norflash_ioctl(u32 cmd, u32 arg, u32 unit, void *_part)
{
    u32 rets;//, reti;
    __asm__ volatile("%0 = rets":"=r"(rets));


    int reg = 0;
    struct norflash_partition *part = _part;
    os_mutex_pend(&_norflash.mutex, 0);
    switch (cmd) {
    case IOCTL_GET_STATUS:
        *(u32 *)arg = 1;
        break;
    case IOCTL_GET_ID:
        *((u32 *)arg) = _norflash.flash_id;
        break;
    case IOCTL_GET_CAPACITY:
        if (_norflash.flash_capacity == 0)  {
            *(u32 *)arg = 0;
        } else if (_norflash.part_num == 1 && part->start_addr == 0) {
            *(u32 *)arg = _norflash.flash_capacity / unit;
        } else {
            *(u32 *)arg = part->size / unit;
        }
        break;
    case IOCTL_GET_BLOCK_SIZE:
        *(u32 *)arg = 512;
        break;
    case IOCTL_ERASE_PAGE:
        reg = _norflash_eraser(IOCTL_ERASE_PAGE, arg * unit + part->start_addr);
        break;
    case IOCTL_ERASE_SECTOR:
        if ((arg * unit) + 4096 > part->size) {
            log_error("__func__ %s %x %x %x\n", __func__, rets, arg, unit);
            ASSERT(0);
        }
        reg = _norflash_eraser(IOCTL_ERASE_SECTOR, arg * unit + part->start_addr);
        break;
    case IOCTL_ERASE_BLOCK:
        if ((arg * unit) + (64 * 1024) > part->size) {
            log_error("__func__ %s %x %x %x\n", __func__, rets, arg, unit);
            ASSERT(0);
        }
        reg = _norflash_eraser(IOCTL_ERASE_BLOCK, arg * unit + part->start_addr);
        break;
    case IOCTL_ERASE_CHIP:
        reg = _norflash_eraser(IOCTL_ERASE_CHIP, 0);
        break;
    case IOCTL_FLUSH:
        break;
    case IOCTL_CMD_RESUME:
        break;
    case IOCTL_CMD_SUSPEND:
        break;
    case IOCTL_GET_PART_INFO:
        u32 *info = (u32 *)arg;
        u32 *start_addr = &info[0];
        u32 *part_size = &info[1];
        *start_addr = part->start_addr;
        *part_size = part->size;
        break;
    default:
        reg = -EINVAL;
        break;
    }
__exit:
    os_mutex_post(&_norflash.mutex);
    return reg;
}


/*************************************************************************************
 *                                  ?????? device_api
 ************************************************************************************/

static int norflash_sfc_dev_init(const struct dev_node *node, void *arg)
{
    struct norflash_sfc_dev_platform_data *pdata = arg;

    return _norflash_init(node->name, pdata);
}

static int norflash_sfc_dev_open(const char *name, struct device **device, void *arg)
{
    struct norflash_partition *part;
    part = norflash_find_part(name);
    if (!part) {
        log_error("no norflash partition is found\n");
        return -ENODEV;
    }
    *device = &part->device;
    (*device)->private_data = part;
    if (atomic_read(&part->device.ref)) {
        return 0;
    }
    return _norflash_open(arg);
}
static int norflash_sfc_dev_close(struct device *device)
{
    return _norflash_close();
}
static int norflash_sfc_dev_read(struct device *device, void *buf, u32 len, u32 offset)
{
    int reg;
    /* printf("flash read sector = %d, num = %d\n", offset, len); */
    offset = offset * 512;
    len = len * 512;
    struct norflash_partition *part;
    part = (struct norflash_partition *)device->private_data;
    if (!part) {
        log_error("norflash partition invalid\n");
        return -EFAULT;
    }
    offset += part->start_addr;
    reg = fat_sfc_norflash_read(offset, buf, len, 1);
    if (reg) {
        r_printf(">>>[r error]:\n");
        len = 0;
    }

    len = len / 512;
    return len;
}
static int norflash_sfc_dev_write(struct device *device, void *buf, u32 len, u32 offset)
{
    /* printf("flash write sector = %d, num = %d\n", offset, len); */
    int reg = 0;
    offset = offset * 512;
    len = len * 512;
    struct norflash_partition *part = device->private_data;
    if (!part) {
        log_error("norflash partition invalid\n");
        return -EFAULT;
    }
    offset += part->start_addr;
    reg = _norflash_write(offset, buf, len, 1);
    if (reg) {
        r_printf(">>>[w error]:\n");
        len = 0;
    }
    len = len / 512;
    return len;
}
static bool norflash_sfc_dev_online(const struct dev_node *node)
{
    return 1;
}
static int norflash_sfc_dev_ioctl(struct device *device, u32 cmd, u32 arg)
{
    struct norflash_partition *part = device->private_data;
    if (!part) {
        log_error("norflash partition invalid\n");
        return -EFAULT;
    }
    return _norflash_ioctl(cmd, arg, 512, part);
}

/*
 * 1. ??????????????????512???????????????????????????????????????????????????write?????????????????????
 * ?????????norflash_sfc_dev_ops??????????????????????????????????????????ops
 *
 * 2. ??????????????????????????????????????????????????????FLASH_CACHE_ENABLE??????????????????
 * norflash_sfc_dev_read()???????????????fat_sfc_norflash_read()?????????cache???0???
 * norflash_sfc_dev_write()??????
 *
 * 3. norflash_sfc_dev_ops?????????????????????????????????????????????????????????????????????????????????
 * ????????????????????????????????????????????????????????????????????????????????????????????????????????????
 * flash??????????????????ASSERT()
 *
 * 4. ??????IOCTL_GET_CAPACITY?????????????????????????????????????????????????????????????????????1
 * ?????????????????????????????? == 0?????????flash????????????????????? != 0????????????????????????
 * norflash_sfc_dev_ops??????????????????512???????????????
 *
 * 5. ?????????????????????ops??????????????????
 */
//???512??????????????????
const struct device_operations norflash_sfc_dev_ops = {
    .init   = norflash_sfc_dev_init,
    .online = norflash_sfc_dev_online,
    .open   = norflash_sfc_dev_open,
    .read   = norflash_sfc_dev_read,
    .write  = norflash_sfc_dev_write,
    .ioctl  = norflash_sfc_dev_ioctl,
    .close  = norflash_sfc_dev_close,
};


static int norflash_sfc_fs_dev_init(const struct dev_node *node, void *arg)
{
    struct norflash_sfc_dev_platform_data *pdata = arg;

    return _norflash_init(node->name, pdata);
}

static int norflash_sfc_fs_dev_open(const char *name, struct device **device, void *arg)
{
    struct norflash_partition *part;
    part = norflash_find_part(name);
    if (!part) {
        log_error("no norflash partition is found\n");
        return -ENODEV;
    }
    *device = &part->device;
    (*device)->private_data = part;
    if (atomic_read(&part->device.ref)) {
        return 0;
    }
    return _norflash_open(arg);
}
static int norflash_sfc_fs_dev_close(struct device *device)
{
    return _norflash_close();
}
static int norflash_sfc_fs_dev_read(struct device *device, void *buf, u32 len, u32 offset)
{
    u32 rets;//, reti;
    __asm__ volatile("%0 = rets":"=r"(rets));


    int reg;
    /* printf("flash read sector = %d, num = %d\n", offset, len); */
    struct norflash_partition *part;
    part = (struct norflash_partition *)device->private_data;
    if (!part) {
        log_error("norflash partition invalid\n");
        return -EFAULT;
    }


    if (offset + len > part->size) {
        log_error("__func__ %s %x %x %x\n", __func__, rets, offset, len);
        ASSERT(0);
    }

    offset += part->start_addr;
    reg = fat_sfc_norflash_read(offset, buf, len, 0);
    if (reg) {
        r_printf(">>>[r error]:\n");
        len = 0;
    }

    return len;
}

static int norflash_sfc_fs_dev_write(struct device *device, void *buf, u32 len, u32 offset)
{
    //printf("flash write addr = 0x%x, len = 0x%x\n", offset, len);
    //
    u32 rets;//, reti;
    __asm__ volatile("%0 = rets":"=r"(rets));


    int reg = 0;
    struct norflash_partition *part = device->private_data;
    if (!part) {
        log_error("norflash partition invalid\n");
        return -EFAULT;
    }


    if (offset + len > part->size) {
        log_error("__func__ %s %x %x %x\n", __func__, rets, offset, len);
        ASSERT(0);
    }


    offset += part->start_addr;


    reg = _norflash_write(offset, buf, len, 0);
    if (reg) {
        r_printf(">>>[w error]:\n");
        len = 0;
    }
    return len;
}
static bool norflash_sfc_fs_dev_online(const struct dev_node *node)
{
    return 1;
}

static int norflash_sfc_fs_dev_ioctl(struct device *device, u32 cmd, u32 arg)
{
    struct norflash_partition *part = device->private_data;
    if (!part) {
        log_error("norflash partition invalid\n");
        return -EFAULT;
    }
    return _norflash_ioctl(cmd, arg, 1, part);
}

/*
 * 1. ??????????????????1?????????????????????????????????????????????write????????????????????????
 * ?????????norflash_sfc_fs_dev_ops??????????????????????????????????????????ops???????????????????????????
 * ??????????????????????????????????????????????????????????????????????????????????????????????????????
 * ????????????????????????????????????????????????????????????????????????ops
 *
 * 2. ???????????????????????????????????????????????????FLASH_CACHE_ENABLE???1??????
 * norflash_sfc_fs_dev_read()???????????????fat_sfc_norflash_read()?????????cache???1???
 * norflash_sfc_fs_dev_write()??????
 *
 * 3. norflash_sfc_fs_dev_ops?????????????????????????????????????????????????????????????????????????????????
 * ????????????????????????????????????????????????????????????????????????????????????????????????????????????
 * flash??????????????????ASSERT()
 *
 * 4. ??????IOCTL_GET_CAPACITY?????????????????????????????????????????????????????????????????????1
 * ?????????????????????????????? == 0?????????flash????????????????????? != 0?????????????????????
 * norflash_sfc_fs_dev_ops??????????????????1???????????????
 *
 * 5. ?????????????????????ops??????????????????
 */
//???1??????????????????
const struct device_operations norflash_sfc_fs_dev_ops = {
    .init   = norflash_sfc_fs_dev_init,
    .online = norflash_sfc_fs_dev_online,
    .open   = norflash_sfc_fs_dev_open,
    .read   = norflash_sfc_fs_dev_read,
    .write  = norflash_sfc_fs_dev_write,
    .ioctl  = norflash_sfc_fs_dev_ioctl,
    .close  = norflash_sfc_fs_dev_close,
};

/*
 * ???ops?????????????????????????????????????????????????????????????????????????????????????????????ops???
 * ???????????????????????????????????????ops
 */

#endif /* #if (defined(TCFG_NORFLASH_SFC_DEV_ENABLE) && TCFG_NORFLASH_SFC_DEV_ENABLE) */
