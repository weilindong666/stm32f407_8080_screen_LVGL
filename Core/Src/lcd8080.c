#include "lcd8080.h"

/* FSMC Bank 4 is configured as a 16-bit asynchronous SRAM.  With A0 wired
 * to the LCD D/C (RS) input, the CPU half-word addresses are:
 *   0x6C000000: command (RS = 0)
 *   0x6C000002: data    (RS = 1)
 * A0 is therefore selected by adding one half-word to the Bank4 base. */
#define LCD8080_BASE        (0x6C000000UL)
#define LCD8080_REG_ADDR    (LCD8080_BASE)
#define LCD8080_DATA_ADDR   (LCD8080_BASE + 2UL)

#define LCD8080_REG         (*(__IO uint16_t *)LCD8080_REG_ADDR)
#define LCD8080_DATA        (*(__IO uint16_t *)LCD8080_DATA_ADDR)

static void lcd8080_write_command(uint8_t command)
{
    LCD8080_REG = command;
}

static void lcd8080_write_data(uint8_t data)
{
    LCD8080_DATA = data;
}

static void lcd8080_write_data16(uint16_t data)
{
    LCD8080_DATA = data;
}

static void lcd8080_write_command_data(uint8_t command,
                                       const uint8_t *data,
                                       uint32_t length)
{
    lcd8080_write_command(command);
    while (length-- != 0U) {
        lcd8080_write_data(*data++);
    }
}

static uint8_t lcd8080_madctl_value(void)
{
    /* ST7796 MADCTL: MY=0x80, MX=0x40, MV=0x20, BGR=0x08.
     * 以示例程序的显示方向为基准，再叠加独立的左右/上下翻转。 */
#if LCD8080_LANDSCAPE
    uint8_t madctl = 0x28U; /* 横向：MV + BGR */
#else
    uint8_t madctl = 0x48U; /* 竖向：MX + BGR */
#endif

#if LCD8080_LANDSCAPE
    /* 横向模式开启 MV 后，逻辑 X/Y 对应控制器的行/列，所以两个
     * 翻转位的对应关系与竖向模式相反。 */
#if LCD8080_MIRROR_X
    madctl ^= 0x80U;         /* 横向逻辑左右翻转：切换 MY */
#endif
#if LCD8080_MIRROR_Y
    madctl ^= 0x40U;         /* 横向逻辑上下翻转：切换 MX */
#endif
#else
#if LCD8080_MIRROR_X
    madctl ^= 0x40U;         /* 竖向逻辑左右翻转：切换 MX */
#endif
#if LCD8080_MIRROR_Y
    madctl ^= 0x80U;         /* 竖向逻辑上下翻转：切换 MY */
#endif
#endif
    return madctl;
}

static uint8_t lcd8080_glyph(char c, uint8_t row)
{
    /* 5x7 font, rows are LSB-first. */
    static const char alphabet[] = "ABDEFCILNOPRSTV";
    static const uint8_t glyphs[][7] = {
        {0x0EU,0x11U,0x11U,0x1FU,0x11U,0x11U,0x11U}, /* A */
        {0x1EU,0x11U,0x11U,0x1EU,0x11U,0x11U,0x1EU}, /* B */
        {0x1EU,0x11U,0x11U,0x11U,0x11U,0x11U,0x1EU}, /* D */
        {0x1FU,0x10U,0x10U,0x1EU,0x10U,0x10U,0x1FU}, /* E */
        {0x1FU,0x10U,0x10U,0x1EU,0x10U,0x10U,0x10U}, /* F */
        {0x0EU,0x11U,0x10U,0x10U,0x10U,0x11U,0x0EU}, /* C */
        {0x11U,0x11U,0x11U,0x1FU,0x11U,0x11U,0x11U}, /* I */
        {0x1FU,0x04U,0x04U,0x04U,0x04U,0x04U,0x1FU}, /* L */
        {0x11U,0x1BU,0x15U,0x15U,0x11U,0x11U,0x11U}, /* N */
        {0x0EU,0x11U,0x11U,0x11U,0x11U,0x11U,0x0EU}, /* O */
        {0x1EU,0x11U,0x11U,0x1EU,0x10U,0x10U,0x10U}, /* P */
        {0x1EU,0x11U,0x11U,0x1EU,0x14U,0x12U,0x11U}, /* R */
        {0x0FU,0x10U,0x10U,0x0EU,0x01U,0x01U,0x1EU}, /* S */
        {0x1FU,0x04U,0x04U,0x04U,0x04U,0x04U,0x04U}, /* T */
        {0x11U,0x11U,0x11U,0x11U,0x11U,0x0AU,0x04U}  /* V */
    };
    uint32_t i;

    for (i = 0U; i < (sizeof(alphabet) - 1U); ++i) {
        if (alphabet[i] == c) {
            return glyphs[i][row];
        }
    }
    return 0U;
}

static void lcd8080_draw_text(uint16_t x, uint16_t y,
                              const char *text, uint8_t scale)
{
    while (*text != '\0') {
        uint8_t row;
        uint8_t col;
        if (*text == ' ') {
            x = (uint16_t)(x + 6U * scale);
            ++text;
            continue;
        }
        for (row = 0U; row < 7U; ++row) {
            uint8_t bits = lcd8080_glyph(*text, row);
            for (col = 0U; col < 5U; ++col) {
                /* 字模每行按 5 位从高位到低位表示从左到右的像素。
                 * 之前从最低位开始取，导致每个字母左右镜像。 */
                if ((bits & (0x10U >> col)) != 0U) {
                    uint8_t dx;
                    uint8_t dy;
                    for (dy = 0U; dy < scale; ++dy) {
                        for (dx = 0U; dx < scale; ++dx) {
                            LCD8080_FillWindow(
                                (uint16_t)(x + col * scale + dx),
                                (uint16_t)(y + row * scale + dy),
                                (uint16_t)(x + col * scale + dx),
                                (uint16_t)(y + row * scale + dy),
                                LCD8080_WHITE);
                        }
                    }
                }
            }
        }
        x = (uint16_t)(x + 6U * scale);
        ++text;
    }
}

void LCD8080_DrawOrientationLabel(void)
{
    const char *label = "ABCD";
    const uint16_t length = 4U;
    /* 5x7 glyphs with one-cell spacing, scaled 3x. */
    uint16_t text_width = (uint16_t)(length * 6U * 3U - 3U);
    lcd8080_draw_text((uint16_t)((LCD8080_WIDTH - text_width) / 2U),
                      (uint16_t)((LCD8080_HEIGHT - 21U) / 2U), label, 3U);
}

void LCD8080_SetWindow(uint16_t x0, uint16_t y0,
                       uint16_t x1, uint16_t y1)
{
    uint8_t column[] = {
        (uint8_t)(x0 >> 8), (uint8_t)x0,
        (uint8_t)(x1 >> 8), (uint8_t)x1
    };
    uint8_t row[] = {
        (uint8_t)(y0 >> 8), (uint8_t)y0,
        (uint8_t)(y1 >> 8), (uint8_t)y1
    };

    lcd8080_write_command_data(0x2AU, column, sizeof(column));
    lcd8080_write_command_data(0x2BU, row, sizeof(row));
}

void LCD8080_DisplayOn(void)
{
    lcd8080_write_command(0x29U);
}

void LCD8080_DisplayOff(void)
{
    lcd8080_write_command(0x28U);
}

void LCD8080_FillWindow(uint16_t x0, uint16_t y0,
                        uint16_t x1, uint16_t y1, uint16_t color)
{
    uint32_t pixels;

    if ((x0 > x1) || (y0 > y1) ||
        (x1 >= LCD8080_WIDTH) || (y1 >= LCD8080_HEIGHT)) {
        return;
    }

    LCD8080_SetWindow(x0, y0, x1, y1);
    lcd8080_write_command(0x2CU); /* Memory Write */
    pixels = (uint32_t)(x1 - x0 + 1U) * (uint32_t)(y1 - y0 + 1U);
    while (pixels-- != 0U) {
        lcd8080_write_data16(color);
    }
}

void LCD8080_Fill(uint16_t color)
{
    LCD8080_FillWindow(0U, 0U, LCD8080_WIDTH - 1U,
                       LCD8080_HEIGHT - 1U, color);
}

void LCD8080_Init(void)
{
    static const uint8_t positive_gamma[] = {
        0xF0U, 0x09U, 0x0BU, 0x06U, 0x04U, 0x15U, 0x2FU,
        0x54U, 0x42U, 0x3CU, 0x17U, 0x14U, 0x18U, 0x1BU
    };
    static const uint8_t negative_gamma[] = {
        0xF0U, 0x09U, 0x0BU, 0x06U, 0x04U, 0x03U, 0x2DU,
        0x43U, 0x42U, 0x3BU, 0x16U, 0x14U, 0x17U, 0x1BU
    };
    static const uint8_t power_control_3[] = { 0xA7U };
    static const uint8_t gate_control[] = { 0xC6U };
    static const uint8_t panel_drive[] = {
        0x40U, 0x8AU, 0x00U, 0x00U, 0x29U, 0x19U, 0xA5U, 0x33U
    };

    /* The module reset is handled by its power-on reset; the controller
     * sequence below follows the vendor's working ST7796 example. */
    HAL_Delay(120U);
    lcd8080_write_command(0x01U); /* Software reset (no LCD_RST GPIO in IOC) */
    HAL_Delay(120U);
    lcd8080_write_command(0x11U); /* Sleep Out */
    HAL_Delay(120U);

    lcd8080_write_command_data(0xF0U, (const uint8_t[]){0xC3U}, 1U);
    lcd8080_write_command_data(0xF0U, (const uint8_t[]){0x96U}, 1U);
    {
        const uint8_t madctl = lcd8080_madctl_value();
        lcd8080_write_command_data(0x36U, &madctl, 1U);
    }
    lcd8080_write_command_data(0x3AU, (const uint8_t[]){0x55U}, 1U); /* RGB565 */
    lcd8080_write_command_data(0xB4U, (const uint8_t[]){0x01U}, 1U);
    lcd8080_write_command_data(0xB7U, gate_control, sizeof(gate_control));
    lcd8080_write_command_data(0xE8U, panel_drive, sizeof(panel_drive));
    lcd8080_write_command_data(0xC1U, (const uint8_t[]){0x06U}, 1U);
    lcd8080_write_command_data(0xC2U, power_control_3, sizeof(power_control_3));
    lcd8080_write_command_data(0xC5U, (const uint8_t[]){0x18U}, 1U);
    lcd8080_write_command_data(0xE0U, positive_gamma, sizeof(positive_gamma));
    lcd8080_write_command_data(0xE1U, negative_gamma, sizeof(negative_gamma));
    lcd8080_write_command_data(0xF0U, (const uint8_t[]){0x3CU}, 1U);
    lcd8080_write_command_data(0xF0U, (const uint8_t[]){0x69U}, 1U);

    /* Portrait address range: 0..319 by 0..479. */
    LCD8080_SetWindow(0U, 0U, LCD8080_WIDTH - 1U, LCD8080_HEIGHT - 1U);
    HAL_Delay(120U);
    LCD8080_DisplayOn();
}
