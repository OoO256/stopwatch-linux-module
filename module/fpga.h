#ifndef __FPGA_H__

unsigned char dot_matix_numbers[10][10] = {
    {0x3e,0x7f,0x63,0x73,0x73,0x6f,0x67,0x63,0x7f,0x3e}, // 0
    {0x0c,0x1c,0x1c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x1e}, // 1
    {0x7e,0x7f,0x03,0x03,0x3f,0x7e,0x60,0x60,0x7f,0x7f}, // 2
    {0xfe,0x7f,0x03,0x03,0x7f,0x7f,0x03,0x03,0x7f,0x7e}, // 3
    {0x66,0x66,0x66,0x66,0x66,0x66,0x7f,0x7f,0x06,0x06}, // 4
    {0x7f,0x7f,0x60,0x60,0x7e,0x7f,0x03,0x03,0x7f,0x7e}, // 5
    {0x60,0x60,0x60,0x60,0x7e,0x7f,0x63,0x63,0x7f,0x3e}, // 6
    {0x7f,0x7f,0x63,0x63,0x03,0x03,0x03,0x03,0x03,0x03}, // 7
    {0x3e,0x7f,0x63,0x63,0x7f,0x7f,0x63,0x63,0x7f,0x3e}, // 8
    {0x3e,0x7f,0x63,0x63,0x7f,0x3f,0x03,0x03,0x03,0x03} // 9
};

unsigned char dot_matix_full[10] = {
    0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f
};

unsigned char dot_matix_blank[10] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

#define IOM_LED_MAJOR 260
#define IOM_LED_NAME "fpga_led"
#define IOM_FND_MAJOR 261
#define IOM_FND_NAME "fpga_fnd"
#define IOM_FPGA_DOT_MAJOR 262
#define IOM_FPGA_DOT_NAME "fpga_dot"
#define IOM_FPGA_TEXT_LCD_MAJOR 263
#define IOM_FPGA_TEXT_LCD_NAME "fpga_text_lcd"

#define IOM_FPGA_DOT_ADDRESS 0x08000210
#define IOM_FND_ADDRESS 0x08000004
#define IOM_LED_ADDRESS 0x08000016
#define IOM_FPGA_TEXT_LCD_ADDRESS 0x08000090


#define __FPGA_H__
#endif