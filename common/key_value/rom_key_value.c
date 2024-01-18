/**
 * @file rom_key_value.c
 * @author qiquan.cui 
 * @brief 此文件用于保存和读取掉电不丢失的关键数据
 * @version 0.1
 * @date 2023-08-21
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#define MODULE_TAG	"key_value"

#include "rom_key_value.h"
#include "rom_key_value_cfg.h"
#include "rom_key_value_port.h"
#include "string.h"

// 配置性宏定义
//#define KEY_VALUE_TEST_WR
//#define KEY_VALUE_TEST_RD
//#define PRINT_RW_OPERATION

#define STATUS_CODE_NUM     2
#define STATUS_CODE_SIZE    STATUS_CODE_NUM * 4
#define CHECKSUM_SIZE       1

#define SPACE0_BASE_ADDR    START_ADDR
#define SPACE1_BASE_ADDR    START_ADDR + USED_SPACE_SIZE

#define KV_STATUS_UNUSED    {0xffffffff, 0xffffffff}
#define KV_STATUS_USING     {0x5a5a5a5a, 0xffffffff}
#define KV_STATUS_DELETE    {0x5a5a5a5a, 0x5a5a5a5a}

#define GET_ANOTHER_SPACE(space)	(space == ROM_SPACE_0 ? ROM_SPACE_1 : ROM_SPACE_0)

typedef enum __kv_exist_enum
{
    KV_NOT_EXIST = 0,
    KV_EXIST
}kv_exist_enum;

typedef enum __kv_status
{
    STATUS_UNUSED = 0,
    STATUS_USING,
    STATUS_DELETE,
    STATUS_NUM,
    STATUS_ERROR
}kv_status;

typedef enum __space_num
{
    ROM_SPACE_0 = 0,
    ROM_SPACE_1,
	SPACE_NUM,
}space_num;

#pragma pack(1)
/**
 * @brief 保存在rom中的数据结构
 * 
 */
typedef struct __rom_data_struct
{
    uint32_t status_code[STATUS_CODE_NUM]; // 数据状态
    char name[MAX_NAME_SIZE];               // 数据名称字符串
    uint8_t data[MAX_VALUE_SIZE];            // 数据值
    uint8_t sum_check;                      // 求和校验
}rom_data_struct;
#pragma pack()

#define ROM_DATA_STRUCT_SIZE    sizeof(rom_data_struct)

static const uint32_t status_code_set[STATUS_NUM][STATUS_CODE_NUM] = {
    KV_STATUS_UNUSED,
    KV_STATUS_USING,
    KV_STATUS_DELETE
};

static const uint32_t space_base_addr_set[SPACE_NUM] = {
	SPACE0_BASE_ADDR,
	SPACE1_BASE_ADDR
};

static space_num cur_space;
static uint32_t base_addr;
static uint32_t final_addr_offset = 0;

static uint32_t get_final_kv_addr(void);
static uint32_t get_next_kv(uint32_t last_addr);
static int8_t get_last_space(space_num *last_space);
static kv_exist_enum find_key_value(const char *name, uint32_t *phy_addr);
static kv_status get_kv_status(uint32_t phy_addr);
static int8_t del_key_value(uint32_t phy_addr);
static int8_t key_value_read(uint32_t kv_addr_offset, void *data, uint16_t len);
static int8_t key_value_write(uint32_t kv_addr_offset, const char *name, void *data, uint16_t len);
static int8_t check_sum(rom_data_struct *pstruct);

/**
 * @brief 初始化过程，主要检查存储空间中写入的多个value值，将该存储空间的变量读取出
 * 存储到另一个存储空间，保证空间被高效地利用起来。
 * 
 * @return int8_t 
 */
int8_t rom_key_value_init(void)
{
    space_num last_space;
    uint32_t last_base_addr;
    uint32_t read_addr_offset = 0;
    uint8_t change_space_pend = 0;
    kv_status status;

    final_addr_offset = 0;

    LM_LOG_INFO("======= key value init =======\n");

    //TODO 加入选项字节判断

    // 找到上次使用的存储空间在哪里
    if(get_last_space(&last_space) == 0)
    {
        LM_LOG_INFO("get last using space is space_%d", last_space);
        last_base_addr = space_base_addr_set[last_space];
        
        // 遍历一遍所有value的状态
        do{
			read_addr_offset = get_next_kv(read_addr_offset);

            if(read_addr_offset + ROM_DATA_STRUCT_SIZE > USED_SPACE_SIZE)
                break;
            status = get_kv_status(last_base_addr + read_addr_offset);
            // 若已经到达最后一个，则退出
            if(status == STATUS_UNUSED)
            {
            	change_space_pend = 0;
                break;
            }
            // 如果发现存在被删除的value则表明有压缩空间，可以将value搬运到另一片区域
            if(status == STATUS_DELETE)
            {
            	change_space_pend = 1;
                break;
            }
        }while(1);

        // 如果需要切换存储区域
        if(change_space_pend == 1)
        {
        	uint32_t value_move_num = 0;

        	cur_space = GET_ANOTHER_SPACE(last_space);
            base_addr = space_base_addr_set[cur_space];
            LM_LOG_INFO("space need to change to space_%d", cur_space);

            // 标记使用该区域
            set_key_value("space_is_using", &cur_space, sizeof(cur_space));

        	read_addr_offset = 0;
        	do{
				read_addr_offset = get_next_kv(read_addr_offset);
				// TODO:直接改成统一的变量有效性判断
				if(read_addr_offset + ROM_DATA_STRUCT_SIZE > USED_SPACE_SIZE)
					break;

				status = get_kv_status(last_base_addr + read_addr_offset);

				// 若已经到达最后一个，则退出
				if(status == STATUS_UNUSED)
				{
					break;
				}

				if(status == STATUS_USING)
				{
					rom_data_struct read_struct;
					flash_read(last_base_addr + read_addr_offset, &read_struct, sizeof(read_struct));

					if(check_sum(&read_struct) == 0)
					{
						set_key_value(read_struct.name, read_struct.data, MAX_VALUE_SIZE);
						value_move_num ++;
					}
				}

			}while(1);

			LM_LOG_INFO("%d values are moved from space_%d to space_%d", value_move_num, last_space, cur_space);

			flash_erase(last_base_addr, USED_SPACE_SIZE);
        }
        else
        {
        	cur_space = last_space;
            base_addr = last_base_addr;

        	final_addr_offset = read_addr_offset;
            LM_LOG_INFO("now using space_%d", cur_space);
        }
    }
    else
    {
        // 默认使用space0，清空space0和space1的内容
        cur_space = ROM_SPACE_0;
        base_addr = SPACE0_BASE_ADDR;
        LM_LOG_ERROR("last space not found, erase all data");
        flash_erase(SPACE0_BASE_ADDR, USED_SPACE_SIZE);
        flash_erase(SPACE1_BASE_ADDR, USED_SPACE_SIZE);
        set_key_value("space_is_using", &cur_space, sizeof(cur_space));
    }
    LM_LOG_INFO("======= key value init finish =======\n");

// 测试性代码，检验写入和读取功能
#ifdef	KEY_VALUE_TEST_WR
	uint32_t output = 20232011;
	float double_output = 1.2345654321;

	if(SET_KEY_VALUE((char *)"version", output) == 0)
	  LM_LOG_INFO("v %d", output);
	if(SET_KEY_VALUE((char *)"PI", double_output) == 0)
	  LM_LOG_INFO("PI %e", double_output);
#endif
#ifdef	KEY_VALUE_TEST_RD
	uint32_t output;
	float double_output;

	if(GET_KEY_VALUE((char *)"version", output) == 0)
	  LM_LOG_INFO("v %d", output);
	if(GET_KEY_VALUE((char *)"PI", double_output) == 0)
	  LM_LOG_INFO("PI %e", double_output);
#endif
    return 0;
}

/**
 * @brief 获取最后一个变量所处的地址
 * 目前是直接返回当前的final_kv_addr，也可以换一种形式，遍历整个列表直到找不到下一个
 * @return int8_t 
 */
static uint32_t get_final_kv_addr(void)
{
    return final_addr_offset;
}

/**
 * @brief 寻找下一个变量
 * 
 * @param last_addr 上一个变量的地址 
 * @return int8_t 
 */
static uint32_t get_next_kv(uint32_t last_addr_offset)
{
    if(last_addr_offset + ROM_DATA_STRUCT_SIZE > USED_SPACE_SIZE)
        return 0;

    return last_addr_offset + ROM_DATA_STRUCT_SIZE;
}

/**
 * @brief Get the last use space
 * 
 */
static int8_t get_last_space(space_num* _last_space)
{
    space_num last_space;
    base_addr = SPACE0_BASE_ADDR;
    if(get_key_value("space_is_using", &last_space, sizeof(last_space)) == 0)
    {
    	if(last_space != ROM_SPACE_0)
    		return -1;

        *_last_space = last_space;
        return 0;
    }
    base_addr = SPACE1_BASE_ADDR;
    if(get_key_value("space_is_using", &last_space, sizeof(last_space)) == 0)
    {
    	if(last_space != ROM_SPACE_1)
    		return -1;

        *_last_space = last_space;
        return 0;
    }

    return -1;
}

/**
 * @brief 新建某个变量或更改其数值
 * 
 * @param name 变量的名称
 * @param data 变量值
 * @param len 变量的长度
 * @return int8_t 
 */
int8_t set_key_value(const char *name, void *data, uint16_t len)
{
    uint32_t addr_offset;
    uint32_t wr_addr_offset;
    rom_data_struct kv_struct;
    uint16_t name_len = strlen(name);

    // 检验参数合法性
    if(name_len >= MAX_NAME_SIZE || len > MAX_VALUE_SIZE || data == NULL)
    {
        return -1;
    }

    // 寻找下一个可用空间
    wr_addr_offset = get_final_kv_addr();
    if(wr_addr_offset + ROM_DATA_STRUCT_SIZE > USED_SPACE_SIZE)
    {
        // TODO 加入逻辑判断如果空间使用满了需要向上反馈，停止写入
        LM_LOG_ERROR("rom already full");
        return -1;
    }

    // 找出原来是否具有相同值，有就不用再次写入了
    if(find_key_value(name, &addr_offset) == KV_EXIST)
    {
        flash_read(addr_offset + base_addr, &kv_struct, sizeof(kv_struct));

        // 求和校验
        if(check_sum(&kv_struct) != 0)
        {
        	LM_LOG_ERROR("value from 0x%08x check sum fail", addr_offset);
            del_key_value(addr_offset + base_addr);
        }
        else
        {
            // 如果与原有值相同，则认为写入成功
        	if(memcmp(data, kv_struct.data, len) == 0)
        	{
        		return 0;
        	}
        }
    }

    // 写入新值，如果写入失败则不删除原有值，报错并退出
    if(key_value_write(wr_addr_offset, name, data, len) != 0)
    {
        LM_LOG_ERROR("set %s to 0x%08x fail", name, wr_addr_offset);
        return -1;
    }

    // 将其它值都删掉
    while(find_key_value(name, &addr_offset) == KV_EXIST)
    {
    	// 如果找到的是刚刚写入的值则认为都已经删了
    	if(addr_offset == wr_addr_offset)
    	{
    		break;
    	}
        // 将原有的其他值值删除
        del_key_value(addr_offset + base_addr);
    }

    final_addr_offset += ROM_DATA_STRUCT_SIZE;
#ifdef PRINT_RW_OPERATION
    LM_LOG_DEBUG("set %s to 0x%08x success", name, wr_addr_offset);
#endif
    

    return 0;
}

int8_t get_key_value(const char *name, void *data, uint16_t len)
{
    uint32_t addr_offset = 0;
    uint16_t name_len = strlen(name);

    if(name_len >= MAX_NAME_SIZE || len > MAX_VALUE_SIZE || data == NULL)
    {
        return -1;
    }

    if(find_key_value(name, &addr_offset) == KV_EXIST)
    {
        int8_t ret;
        ret = key_value_read(addr_offset, data, len);

#ifdef PRINT_RW_OPERATION
        if(ret == 0)
            LM_LOG_DEBUG("get %s from 0x%08x success", name, addr_offset);
#endif

        return ret;
    }

    LM_LOG_ERROR("get %s fail", name);
    
    return -1;
}

int8_t delete_key_value(const char *name)
{
    uint16_t name_len = strlen(name);
    uint32_t addr_offset = 0;

    if(name_len >= MAX_NAME_SIZE)
    {
        return -1;
    }

    while(find_key_value(name, &addr_offset) == KV_EXIST)
    {
        del_key_value(addr_offset + base_addr);
    }

    return 0;
}

/**
 * @brief 查找特定名字的变量值,返回找到的第一个变量的地址
 * 
 * @param name 变量的名字
 * @param phy_addr 如果值存在，则传出其物理地址
 * @return kv_exist_enum 如果值存在，则返回KV_EXIST
 */
kv_exist_enum find_key_value(const char *name, uint32_t *addr_offset)
{
    uint32_t kv_addr_offset;
    rom_data_struct kv_struct;
    kv_status status;
    uint16_t name_size = strlen(name) + 1;

    // 检验参数合法性
    if(name_size > MAX_NAME_SIZE)
    {
        return KV_NOT_EXIST;
    }

    // 遍历所有变量值
    for(kv_addr_offset = 0; kv_addr_offset < USED_SPACE_SIZE; kv_addr_offset += ROM_DATA_STRUCT_SIZE)
    {
        status = get_kv_status(base_addr + kv_addr_offset);

        // 如果该变量未使用，则认为后续变量均未使用，返回未找到变量
        if(status == STATUS_UNUSED)
        {
            return KV_NOT_EXIST;
        }

        // 如果变量不是正在使用，则跳过该变量
        if(status != STATUS_USING)
        {
            continue;
        }

        flash_read(base_addr + kv_addr_offset, &kv_struct, sizeof(kv_struct));
        // 如果变量名称一致，则检验通过，传出其地址
        if(memcmp(name, kv_struct.name, name_size) == 0)
        {
            *addr_offset = kv_addr_offset;
            return KV_EXIST;
        }
    }

    return KV_NOT_EXIST;
}

static kv_status get_kv_status(uint32_t phy_addr)
{
    uint32_t status_code[STATUS_CODE_NUM];

    flash_read(phy_addr, status_code, sizeof(status_code));
    for(uint8_t i = 0; i < STATUS_NUM; i ++)
    {
        if(memcmp(status_code, status_code_set[i], sizeof(status_code)) == 0)
            return (kv_status)i;
    }
    
    return STATUS_ERROR;
}

/**
 * @brief 设置变量的状态
 * 
 * @param phy_addr 
 * @param status 
 * @return int8_t 
 */
static int8_t set_kv_status(uint32_t phy_addr, kv_status status)
{
    uint32_t status_code[STATUS_CODE_NUM];

    flash_read(phy_addr, status_code, sizeof(status_code));
    for(uint8_t i = 0; i < STATUS_CODE_NUM; i ++)
    {
        if(status_code[i] != status_code_set[status][i] && status_code[i] == 0xffffffff)
            flash_write(phy_addr + i*4, (void *)&status_code_set[status][i], 4);
    }
    
    return 0;
}

static int8_t del_key_value(uint32_t phy_addr)
{
    int8_t ret;
    if(get_kv_status(phy_addr) != STATUS_DELETE)
    {
        ret = set_kv_status(phy_addr, STATUS_DELETE);
        if(ret < 0)
            LM_LOG_ERROR("set status fail");
    }
    else
    {
        ret = -1;
        LM_LOG_WARN("already delete");
    }
    
    return ret;
}

/**
 * @brief 读取某个地址的变量的值，未作参数合法性检查
 * 
 * @param kv_addr_offset 
 * @param data 
 * @param len 
 * @return int8_t 
 */
static int8_t key_value_read(uint32_t kv_addr_offset, void *data, uint16_t len)
{
    rom_data_struct kv_struct;
    
    if(flash_read(base_addr + kv_addr_offset, &kv_struct, sizeof(kv_struct)) <0)
    {
    	return -1;
    	LM_LOG_ERROR("value from 0x%08x read fail", kv_addr_offset);
    }
    // 求和校验
    if(check_sum(&kv_struct) != 0)
    {
    	LM_LOG_ERROR("value from 0x%08x check sum fail", kv_addr_offset);
    	return -1;
    }

    memcpy(data, kv_struct.data, len);
    return 0;
}

/**
 * @brief 往对应的地址写入对应的变量
 * 
 * @param kv_addr_offset 
 * @param name 
 * @param data 
 * @param len 
 * @return int8_t 
 */
static int8_t key_value_write(uint32_t kv_addr_offset, const char *name, void *data, uint16_t len)
{
    rom_data_struct struct_write_in;
    rom_data_struct *struct_read_back;
    int8_t ret;

    // 初始化struct_write_in全部为0xff
    memset(&struct_write_in, 0xff, sizeof(struct_write_in));

    struct_write_in.status_code[0] = status_code_set[STATUS_USING][0];
    struct_write_in.status_code[1] = status_code_set[STATUS_USING][1];

    uint16_t name_size = strlen(name) + 1;

    if(len > MAX_VALUE_SIZE)
        return -1;

    if(name_size > MAX_NAME_SIZE)
        return -1;

    // 判断是否还有写入空间
    if(kv_addr_offset + ROM_DATA_STRUCT_SIZE > USED_SPACE_SIZE)
        return -1;

    // 判断是否可写入
    if(get_kv_status(base_addr + kv_addr_offset) != STATUS_UNUSED)
    {
    	LM_LOG_ERROR("value status not unused");
    	return -1;
    }

    // 设置kv的状态
    set_kv_status(base_addr + kv_addr_offset, STATUS_USING);

    // 预装填数据
    memcpy(struct_write_in.name, name, name_size);
    memcpy(struct_write_in.data, data, len);

    // 求和校验
    struct_write_in.sum_check = 0;
    for(uint8_t i = 0; i < sizeof(struct_write_in.name); i++)
    {
        uint8_t *temp = (uint8_t *)struct_write_in.name + i;
        struct_write_in.sum_check += *temp;
    }
    for(uint8_t i = 0; i < sizeof(struct_write_in.data); i++)
    {
        uint8_t *temp = (uint8_t *)struct_write_in.data + i;
        struct_write_in.sum_check += *temp;
    }

    // 将除了状态之外的数据写入
    ret = flash_write(base_addr + kv_addr_offset + STATUS_CODE_SIZE, \
        (void*)(&struct_write_in) + STATUS_CODE_SIZE, \
        ROM_DATA_STRUCT_SIZE - STATUS_CODE_SIZE);

    if(ret < 0)
        LM_LOG_ERROR("flash write error");

    // 回读检验
    struct_read_back = (void *)(base_addr + kv_addr_offset);
    if(memcmp(&struct_write_in, struct_read_back, sizeof(rom_data_struct)) != 0)
    {
    	LM_LOG_ERROR("flash read back check fail");
    	return -1;
    }

    return 0;
}

static int8_t check_sum(rom_data_struct *pstruct)
{
    uint8_t sum_value = 0;

    for(uint8_t i = 0; i < sizeof(pstruct->name); i++)
    {
        uint8_t *temp = (uint8_t *)pstruct->name + i;
        sum_value += *temp;
    }
    for(uint8_t i = 0; i < sizeof(pstruct->data); i++)
    {
        uint8_t *temp = (uint8_t *)pstruct->data + i;
        sum_value += *temp;
    }
    if(sum_value != pstruct->sum_check)
    {
        LM_LOG_ERROR("%s from sum check fail", pstruct->name);
        return -1;
    }

    return 0;
}

