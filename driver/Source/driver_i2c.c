#include "driver_i2c.h"
#include "gd32f30x.h"
#include "chip_resource.h"

#define DRV_I2Cn   2

static uint32_t I2C_PERIPH[DRV_I2Cn] = {I2C0, I2C1};

// RCU clock
static rcu_periph_enum I2C_CLK[DRV_I2Cn] = {RCU_I2C0, RCU_I2C1};
static rcu_periph_enum I2C_GPIO_CLK[DRV_I2Cn] = {RCU_GPIOB, RCU_GPIOB};
static IRQn_Type    I2C_EV_IRQ[DRV_I2Cn] = {I2C0_EV_IRQn, I2C1_EV_IRQn};
static IRQn_Type    I2C_ER_IRQ[DRV_I2Cn] = {I2C0_ER_IRQn, I2C1_ER_IRQn};
static uint32_t I2C_GPIO_PORT[DRV_I2Cn] = {GPIOB, GPIOB};
static uint32_t I2C_SCL_PIN[DRV_I2Cn] = {GPIO_PIN_6, GPIO_PIN_10};
static uint32_t I2C_SDA_PIN[DRV_I2Cn] = {GPIO_PIN_7, GPIO_PIN_11};

int8_t I2cInit(I2cStruct *i2c, I2cInitStruct *init)
{
    if(i2c->inited == 1)
    {
        return 0;
    }
    i2c->inited = 1;

    if(i2c == &I2c0)
    {
        i2c->i2c_id = 0;
    }
    else if(i2c == &I2c1)
    {
        i2c->i2c_id = 1;
    }
    
    /* NVIC enable */
    nvic_irq_enable(I2C_EV_IRQ[i2c->i2c_id], 0, 2);
    nvic_irq_enable(I2C_ER_IRQ[i2c->i2c_id], 0, 1);
    
    /* GPIO rcu enable */
    rcu_periph_clock_enable(I2C_GPIO_CLK[i2c->i2c_id]);
    
    /* GPIO init */
    gpio_init(I2C_GPIO_PORT[i2c->i2c_id], GPIO_MODE_AF_OD, GPIO_OSPEED_50MHZ, I2C_SCL_PIN[i2c->i2c_id]);
    gpio_init(I2C_GPIO_PORT[i2c->i2c_id], GPIO_MODE_AF_OD, GPIO_OSPEED_50MHZ, I2C_SDA_PIN[i2c->i2c_id]);

    /* enable I2C clock */
    rcu_periph_clock_enable(I2C_CLK[i2c->i2c_id]);
    /* configure I2C clock */
    i2c_clock_config(I2C_PERIPH[i2c->i2c_id], init->speed, I2C_DTCY_2);
    /* configure I2C address */
    i2c_mode_addr_config(I2C_PERIPH[i2c->i2c_id], I2C_I2CMODE_ENABLE, I2C_ADDFORMAT_7BITS, init->local_addr);
    /* enable I2C_PERIPH[i2c->i2c_id] */
    i2c_enable(I2C_PERIPH[i2c->i2c_id]);
    /* enable acknowledge */
    i2c_ack_config(I2C_PERIPH[i2c->i2c_id], I2C_ACK_ENABLE);

    return 0;
}

int8_t I2cWrite(I2cStruct *i2c, uint8_t dev_addr, uint8_t *data, uint16_t data_len)
{
    if(i2c->write_info.writing == 1 || i2c->read_info.reading == 1)
        return -1;
    
    if(i2c_flag_get(I2C_PERIPH[i2c->i2c_id], I2C_FLAG_I2CBSY) == 1)
        return -1;

    i2c->write_info.writing = 1;
    i2c->write_info.total_data_len = data_len;
    i2c->write_info.cur_data_num = 0;
    i2c->write_info.pdata = data;
    i2c->write_info.slave_addr = dev_addr;
    
    i2c_interrupt_enable(I2C_PERIPH[i2c->i2c_id], I2C_INT_ERR);
    i2c_interrupt_enable(I2C_PERIPH[i2c->i2c_id], I2C_INT_BUF);
    i2c_interrupt_enable(I2C_PERIPH[i2c->i2c_id], I2C_INT_EV);

    i2c_start_on_bus(I2C_PERIPH[i2c->i2c_id]);

    return 0;
}

int8_t I2cRead(I2cStruct *i2c, uint8_t dev_addr, uint8_t *data, uint16_t data_len)
{
    if(i2c->write_info.writing == 1 || i2c->read_info.reading == 1)
        return -1;
    
    if(i2c_flag_get(I2C_PERIPH[i2c->i2c_id], I2C_FLAG_I2CBSY) == 1)
        return -1;

    i2c->read_info.reading = 1;
    i2c->read_info.total_data_len = data_len;
    i2c->read_info.cur_data_num = 0;
    i2c->read_info.pdata = data;
    i2c->read_info.slave_addr = dev_addr;
    
    i2c_interrupt_enable(I2C_PERIPH[i2c->i2c_id], I2C_INT_ERR);
    i2c_interrupt_enable(I2C_PERIPH[i2c->i2c_id], I2C_INT_BUF);
    i2c_interrupt_enable(I2C_PERIPH[i2c->i2c_id], I2C_INT_EV);
    
    i2c_start_on_bus(I2C_PERIPH[i2c->i2c_id]);

    return 0;
}

uint8_t tbe_cnt = 0;
void I2cEventCallback(I2cStruct *i2c)
{
    if(i2c->write_info.writing == 1)
    {
        switch(i2c->i2c_process) {
        case I2C_SEND_ADDRESS_FIRST:
            if(i2c_interrupt_flag_get(I2C_PERIPH[i2c->i2c_id], I2C_INT_FLAG_SBSEND)) {
                /* send slave address */
                i2c_master_addressing(I2C_PERIPH[i2c->i2c_id], i2c->write_info.slave_addr, I2C_TRANSMITTER);
                i2c->i2c_process = I2C_CLEAR_ADDRESS_FLAG_FIRST;
            }
            break;
        case I2C_CLEAR_ADDRESS_FLAG_FIRST:
            if(i2c_interrupt_flag_get(I2C_PERIPH[i2c->i2c_id], I2C_INT_FLAG_ADDSEND)) {
                /*clear ADDSEND bit */
                i2c_interrupt_flag_clear(I2C_PERIPH[i2c->i2c_id], I2C_INT_FLAG_ADDSEND);
                i2c->i2c_process = I2C_TRANSMIT_DATA;
            }
            break;
        case I2C_TRANSMIT_DATA:
            tbe_cnt = 0;
            while(i2c_interrupt_flag_get(I2C_PERIPH[i2c->i2c_id], I2C_INT_FLAG_TBE) != 0) {
                tbe_cnt ++;
                /* the master sends a data byte */
                i2c_data_transmit(I2C_PERIPH[i2c->i2c_id], i2c->write_info.pdata[i2c->write_info.cur_data_num]);
                i2c->write_info.cur_data_num++;
                if(i2c->write_info.cur_data_num == i2c->write_info.total_data_len) {
                    i2c->i2c_process = I2C_STOP;
                    break;
                }
            }
            break;
        case I2C_STOP:
            /* the master sends a stop condition to I2C bus */
            i2c_stop_on_bus(I2C_PERIPH[i2c->i2c_id]);
            /* disable the I2C_PERIPH[i2c->i2c_id] interrupt */
            i2c_interrupt_disable(I2C_PERIPH[i2c->i2c_id], I2C_INT_ERR);
            i2c_interrupt_disable(I2C_PERIPH[i2c->i2c_id], I2C_INT_BUF);
            i2c_interrupt_disable(I2C_PERIPH[i2c->i2c_id], I2C_INT_EV);
            i2c->i2c_process = I2C_SEND_ADDRESS_FIRST;
            i2c->write_info.writing = 0;
            break;
        default:
            break;
        }
    }
    else if(i2c->read_info.reading == 1)
    {
        switch(i2c->i2c_process) {
        case I2C_SEND_ADDRESS_FIRST:
            if(i2c_interrupt_flag_get(I2C_PERIPH[i2c->i2c_id], I2C_INT_FLAG_SBSEND)) {
                i2c_master_addressing(I2C_PERIPH[i2c->i2c_id], i2c->read_info.slave_addr, I2C_RECEIVER);
                if(i2c->read_info.total_data_len - i2c->read_info.cur_data_num < 2) {
                    /* disable acknowledge */
                    i2c_ack_config(I2C_PERIPH[i2c->i2c_id], I2C_ACK_DISABLE);
                }
                else{
                    /* enable acknowledge */
                    i2c_ack_config(I2C_PERIPH[i2c->i2c_id], I2C_ACK_ENABLE);
                }
                i2c->i2c_process = I2C_CLEAR_ADDRESS_FLAG_FIRST;
            }
            break;
        case I2C_CLEAR_ADDRESS_FLAG_FIRST:
            if(i2c_interrupt_flag_get(I2C_PERIPH[i2c->i2c_id], I2C_INT_FLAG_ADDSEND)) {
                /*clear ADDSEND bit */
                i2c_interrupt_flag_clear(I2C_PERIPH[i2c->i2c_id], I2C_INT_FLAG_ADDSEND);
                i2c->i2c_process = I2C_TRANSMIT_DATA;
            }
            break;
        case I2C_TRANSMIT_DATA:
            if(i2c_interrupt_flag_get(I2C_PERIPH[i2c->i2c_id], I2C_INT_FLAG_RBNE)) {
                if(i2c->read_info.total_data_len - i2c->read_info.cur_data_num > 0) {
                    /* read a byte from the i2c */
                    i2c->read_info.pdata[i2c->read_info.cur_data_num] = i2c_data_receive(I2C_PERIPH[i2c->i2c_id]);
                    i2c->read_info.cur_data_num++;
                    if(i2c->read_info.total_data_len - i2c->read_info.cur_data_num == 1) {
                        /* disable acknowledge */
                        i2c_ack_config(I2C_PERIPH[i2c->i2c_id], I2C_ACK_DISABLE);
                    }
                    else if(i2c->read_info.total_data_len - i2c->read_info.cur_data_num == 0) {
                        i2c->i2c_process = I2C_SEND_ADDRESS_FIRST;
                        /* the master sends a stop condition to I2C bus */
                        i2c_stop_on_bus(I2C_PERIPH[i2c->i2c_id]);
                        /* disable the I2C_PERIPH[i2c->i2c_id] interrupt */
                        i2c_interrupt_disable(I2C_PERIPH[i2c->i2c_id], I2C_INT_ERR);
                        i2c_interrupt_disable(I2C_PERIPH[i2c->i2c_id], I2C_INT_BUF);
                        i2c_interrupt_disable(I2C_PERIPH[i2c->i2c_id], I2C_INT_EV);
                        i2c->read_info.reading = 0;
                    }
                }
            }
            break;
        default:
            break;
        }

    }
}


void I2cErrorCallback(I2cStruct *i2c)
{
    return;
}

