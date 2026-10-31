#include "touch_ft6336.h"

#include "i2c.h"
#include "lcd8080.h"
#include "cmsis_os.h"

/* FT6336/FT5x06 registers. */
#define FT6336_REG_NUM_TOUCHES  0x02U
volatile HAL_StatusTypeDef g_ft6336_status = HAL_ERROR;
volatile FT6336_PointTypeDef g_ft6336_latest_point;
volatile uint32_t g_ft6336_press_count = 0U;

static void ft6336_delay_ms(uint32_t ms)
{
    uint32_t ticks = (osKernelGetTickFreq() * ms + 999U) / 1000U;
    (void)osDelay(ticks);
}

static HAL_StatusTypeDef ft6336_read(uint8_t reg, uint8_t *data, uint16_t size)
{
    return HAL_I2C_Mem_Read(&FT6336_I2C_HANDLE, FT6336_I2C_ADDRESS, reg,
                            I2C_MEMADD_SIZE_8BIT, data, size,
                            FT6336_I2C_TIMEOUT_MS);
}

HAL_StatusTypeDef FT6336_Init(void)
{
    uint8_t device_mode;

    /* PB15 is the active-low FT6336 reset pin. */
    HAL_GPIO_WritePin(TP_RST_GPIO_Port, TP_RST_Pin, GPIO_PIN_RESET);
    ft6336_delay_ms(20U);
    HAL_GPIO_WritePin(TP_RST_GPIO_Port, TP_RST_Pin, GPIO_PIN_SET);
    ft6336_delay_ms(300U);

    /* A read also verifies that the selected I2C bus/address responds. */
    return ft6336_read(0x00U, &device_mode, 1U);
}

HAL_StatusTypeDef FT6336_ReadPoint(FT6336_PointTypeDef *point)
{
    uint8_t touches;
    uint8_t data[5];
    HAL_StatusTypeDef status;

    if (point == NULL) {
        return HAL_ERROR;
    }
    *point = (FT6336_PointTypeDef){0};
    point->event = 0x03U;

    /* 连续读取 0x02~0x06，避免分两次读取时触点数量和坐标不一致。 */
    status = ft6336_read(FT6336_REG_NUM_TOUCHES, data, sizeof(data));
    if (status != HAL_OK) {
        return status;
    }

    touches = data[0] & 0x0FU;
    if (touches == 0U) {
        point->event = 0x03U; /* 无事件/已松手 */
        point->raw_x = 0U;
        point->raw_y = 0U;
        point->x = 0U;
        point->y = 0U;
        return HAL_OK;
    }

    if (touches > 2U) { /* FT6336 最多支持两点，仅使用第一个触点 */
        return HAL_ERROR;
    }

    point->event = (uint8_t)((data[1] >> 6) & 0x03U);
    point->pressed = (point->event == 0x00U) || (point->event == 0x02U);
    point->raw_x = (uint16_t)(((uint16_t)(data[1] & 0x0FU) << 8) | data[2]);
    point->raw_y = (uint16_t)(((uint16_t)(data[3] & 0x0FU) << 8) | data[4]);
    point->coordinates_valid = (point->raw_x < FT6336_RAW_WIDTH) &&
                               (point->raw_y < FT6336_RAW_HEIGHT);
    if (point->coordinates_valid) {
        FT6336_TransformPoint(point->raw_x, point->raw_y,
                              &point->x, &point->y);
    }
    return HAL_OK;
}

void FT6336_TransformPoint(uint16_t raw_x, uint16_t raw_y,
                           uint16_t *x, uint16_t *y)
{
    uint32_t tx;
    uint32_t ty;

    if ((x == NULL) || (y == NULL)) {
        return;
    }

    /* 越界输入先限制量程，避免无符号减法下溢。ReadPoint 会另行标记无效。 */
    if (raw_x >= FT6336_RAW_WIDTH) {
        raw_x = FT6336_RAW_WIDTH - 1U;
    }
    if (raw_y >= FT6336_RAW_HEIGHT) {
        raw_y = FT6336_RAW_HEIGHT - 1U;
    }
#if FT6336_SWAP_XY
    tx = raw_y;
    ty = raw_x;
#else
    tx = raw_x;
    ty = raw_y;
#endif

#if (FT6336_SWAP_XY != LCD8080_LANDSCAPE)
#error "FT6336_SWAP_XY must match LCD8080_LANDSCAPE for the configured raw range"
#endif
#if FT6336_INVERT_X
    tx = (LCD8080_WIDTH - 1U) - tx;
#endif
#if FT6336_INVERT_Y
    ty = (LCD8080_HEIGHT - 1U) - ty;
#endif
#if LCD8080_MIRROR_X
    tx = (LCD8080_WIDTH - 1U) - tx;
#endif
#if LCD8080_MIRROR_Y
    ty = (LCD8080_HEIGHT - 1U) - ty;
#endif
    if (tx >= LCD8080_WIDTH) {
        tx = LCD8080_WIDTH - 1U;
    }
    if (ty >= LCD8080_HEIGHT) {
        ty = LCD8080_HEIGHT - 1U;
    }
    *x = (uint16_t)tx;
    *y = (uint16_t)ty;
}
