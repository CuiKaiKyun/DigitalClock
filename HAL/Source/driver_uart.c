#include "driver_uart.h"
#include "gd32f30x.h"
#include "board_resource.h"

#define DRV_UARTn                             2U

#define DRV_UART0                        USART0
#define DRV_UART0_CLK                    RCU_USART0
#define DRV_UART0_TX_PIN                 GPIO_PIN_9
#define DRV_UART0_RX_PIN                 GPIO_PIN_10
#define DRV_UART0_GPIO_PORT              GPIOA
#define DRV_UART0_GPIO_CLK               RCU_GPIOA

#define DRV_UART1                        USART1
#define DRV_UART1_CLK                    RCU_USART1
#define DRV_UART1_TX_PIN                 GPIO_PIN_2
#define DRV_UART1_RX_PIN                 GPIO_PIN_3
#define DRV_UART1_GPIO_PORT              GPIOA
#define DRV_UART1_GPIO_CLK               RCU_GPIOA

static uint32_t UART_PERIPH[DRV_UARTn] = {DRV_UART0, DRV_UART1};

// RCU clock
static rcu_periph_enum UART_CLK[DRV_UARTn] = {DRV_UART0_CLK, DRV_UART1_CLK};
static rcu_periph_enum UART_GPIO_CLK[DRV_UARTn] = {DRV_UART0_GPIO_CLK, DRV_UART1_GPIO_CLK};
static rcu_periph_enum UART_DMA_CLK[DRV_UARTn] = {RCU_DMA0, RCU_DMA0};

// GPIO
static uint32_t UART_TX_PIN[DRV_UARTn] = {DRV_UART0_TX_PIN, DRV_UART1_TX_PIN};
static uint32_t UART_RX_PIN[DRV_UARTn] = {DRV_UART0_RX_PIN, DRV_UART1_RX_PIN};
static uint32_t UART_GPIO_PORT[DRV_UARTn] = {DRV_UART0_GPIO_PORT, DRV_UART1_GPIO_PORT};

// UART TX
static uint32_t UART_TX_DMA[DRV_UARTn] = {DMA0, DMA0};
static IRQn_Type UART_TX_DMA_IRQ[DRV_UARTn] = {DMA0_Channel3_IRQn, DMA0_Channel6_IRQn};
static dma_channel_enum UART_TX_DMA_CHL[DRV_UARTn] = {DMA_CH3, DMA_CH6};
// UART RX
static uint32_t UART_RX_DMA[DRV_UARTn] = {DMA0, DMA0};
static IRQn_Type UART_RX_DMA_IRQ[DRV_UARTn] = {DMA0_Channel4_IRQn, DMA0_Channel5_IRQn};
static IRQn_Type UART_IRQ[DRV_UARTn] = {USART0_IRQn, USART1_IRQn};
static dma_channel_enum UART_RX_DMA_CHL[DRV_UARTn] = {DMA_CH4, DMA_CH5};

int8_t UartInit(UartStruct *Uart, UartInitStruct *Init)
{
    uint8_t uart_id = 0U;
    
    if(Uart == &Uart0){
        uart_id = 0U;
    }else if(Uart == &Uart1){
        uart_id = 1U;
    }

    /* enable DMA0 */
    rcu_periph_clock_enable(UART_DMA_CLK[uart_id]);
    
    /* enable GPIO clock */
    rcu_periph_clock_enable(UART_GPIO_CLK[uart_id]);

    /* enable USART clock */
    rcu_periph_clock_enable(UART_CLK[uart_id]);

    /* connect port to USARTx_Tx */
    gpio_init(UART_GPIO_PORT[uart_id], GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, UART_TX_PIN[uart_id]);

    /* connect port to USARTx_Rx */
    gpio_init(UART_GPIO_PORT[uart_id], GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, UART_RX_PIN[uart_id]);

    /* USART configure */
    usart_deinit(UART_PERIPH[uart_id]);
    usart_baudrate_set(UART_PERIPH[uart_id], Init->baudrate);
    usart_receive_config(UART_PERIPH[uart_id], USART_RECEIVE_ENABLE);
    usart_transmit_config(UART_PERIPH[uart_id], USART_TRANSMIT_ENABLE);
    
    switch(Init->stop_bit)
    {
        case StopBit_1Bit:
            usart_stop_bit_set(UART_PERIPH[uart_id], USART_STB_1BIT);
            break;
        case StopBit_2Bit:
            usart_stop_bit_set(UART_PERIPH[uart_id], USART_STB_2BIT);
            break;
        case StopBit_05Bit:
            usart_stop_bit_set(UART_PERIPH[uart_id], USART_STB_0_5BIT);
            break;
        case StopBit_15Bit:
            usart_stop_bit_set(UART_PERIPH[uart_id], USART_STB_1_5BIT);
            break;
        default:
            return -1;
    }
    
    switch(Init->parity)
    {
        case ParityNone:
            usart_parity_config(UART_PERIPH[uart_id], USART_PM_NONE);
            usart_word_length_set(UART_PERIPH[uart_id], USART_WL_8BIT);
            break;
        case ParityOdd:
            usart_parity_config(UART_PERIPH[uart_id], USART_PM_ODD);
            usart_word_length_set(UART_PERIPH[uart_id], USART_WL_9BIT);
            break;
        case ParityEven:
            usart_parity_config(UART_PERIPH[uart_id], USART_PM_EVEN);
            usart_word_length_set(UART_PERIPH[uart_id], USART_WL_9BIT);
            break;
        default:
            return -1;
    }
    usart_enable(UART_PERIPH[uart_id]);
    
    /* enable DMA0 clock */
    rcu_periph_clock_enable(RCU_DMA0);
    
    return 0;
}

int8_t UartSendDMA(UartStruct *Uart, const uint8_t *data, uint16_t data_len)
{
    dma_parameter_struct dma_init_struct;
    uint8_t uart_id = 0U;
    
    if(Uart == &Uart0){
        uart_id = 0U;
    }else if(Uart == &Uart1){
        uart_id = 1U;
    }
    
    /* enable dma irq */
    nvic_irq_enable(UART_TX_DMA_IRQ[uart_id], 0, 0);

    /* deinitialize DMA channel3(USART0 tx) */
    dma_deinit(UART_TX_DMA[uart_id], UART_TX_DMA_CHL[uart_id]);
    dma_struct_para_init(&dma_init_struct);

    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_addr = (uint32_t)data;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_init_struct.number = data_len;
    dma_init_struct.periph_addr = ((uint32_t)&USART_DATA(UART_PERIPH[uart_id]));
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_init(UART_TX_DMA[uart_id], UART_TX_DMA_CHL[uart_id], &dma_init_struct);
    
    dma_circulation_disable(UART_TX_DMA[uart_id], UART_TX_DMA_CHL[uart_id]);
    dma_memory_to_memory_disable(UART_TX_DMA[uart_id], UART_TX_DMA_CHL[uart_id]);

    /* enable USART DMA for transmission */
    usart_dma_transmit_config(UART_PERIPH[uart_id], USART_TRANSMIT_DMA_ENABLE);
    /* enable DMA0 channel3 transfer complete interrupt */
    dma_interrupt_enable(UART_TX_DMA[uart_id], UART_TX_DMA_CHL[uart_id], DMA_INT_FTF);
    /* enable DMA0 channel3 */
    dma_channel_enable(UART_TX_DMA[uart_id], UART_TX_DMA_CHL[uart_id]);

    return 0;
}

int8_t UartReceiveToIdleDMA(UartStruct *Uart, uint8_t *data, uint16_t data_len)
{
    dma_parameter_struct dma_init_struct;
    uint8_t uart_id = 0U;

    if(Uart == &Uart0){
        uart_id = 0U;
    }else if(Uart == &Uart1){
        uart_id = 1U;
    }
	// unused
	uart_id = uart_id;
    /* enable dma irq */
    nvic_irq_enable(UART_RX_DMA_IRQ[uart_id], 0, 1);
	/* enable USART interrupt */
    nvic_irq_enable(UART_IRQ[uart_id], 0, 0);
    
    /* deinitialize DMA channel4 (USART0 rx) */
    dma_deinit(UART_RX_DMA[uart_id], UART_RX_DMA_CHL[uart_id]);
    dma_struct_para_init(&dma_init_struct);

    dma_init_struct.direction = DMA_PERIPHERAL_TO_MEMORY;
    dma_init_struct.memory_addr = (uint32_t)data;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_init_struct.number = data_len;
    dma_init_struct.periph_addr = ((uint32_t)&USART_DATA(UART_PERIPH[uart_id]));
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_init(UART_RX_DMA[uart_id], UART_RX_DMA_CHL[uart_id], &dma_init_struct);
    
    dma_circulation_disable(UART_RX_DMA[uart_id], UART_RX_DMA_CHL[uart_id]);
    dma_memory_to_memory_disable(UART_RX_DMA[uart_id], UART_RX_DMA_CHL[uart_id]);
    
    /* enable USART DMA for reception */
    usart_dma_receive_config(UART_PERIPH[uart_id], USART_RECEIVE_DMA_ENABLE);
    /* enable USART IDLE interrupt */
    usart_interrupt_enable(UART_PERIPH[uart_id], USART_INT_IDLE);
    /* enable DMA0 channel4 transfer complete interrupt */
    dma_interrupt_enable(UART_RX_DMA[uart_id], UART_RX_DMA_CHL[uart_id], DMA_INT_FTF);
    /* enable DMA0 channel4 */
    dma_channel_enable(UART_RX_DMA[uart_id], UART_RX_DMA_CHL[uart_id]);

    return 0;
}

int8_t UartCallbackRegister(UartStruct *Uart, CallBackFunc func)
{
	Uart->call_back_func = func;
	return 0;
}

void UartSendCompleteCallback(UartStruct *Uart)
{
	if(Uart->call_back_func != 0)
		(*Uart->call_back_func)();
}
