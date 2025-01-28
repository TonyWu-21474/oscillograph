#include "oled.h"
#include "asc.h"    //字库（可以自己制作）
#include "main.h"
#include "i2c.h" 
void WriteCmd(unsigned char I2C_Command) //写命令利用I2C通讯
 {
	HAL_I2C_Mem_Write(&hi2c2,OLED0561_ADD,COM,I2C_MEMADD_SIZE_8BIT,&I2C_Command,1,100);
 }
		
void WriteDat(unsigned char I2C_Data)    //写数据利用I2C通讯
 {
		HAL_I2C_Mem_Write(&hi2c2,OLED0561_ADD,DAT,I2C_MEMADD_SIZE_8BIT,&I2C_Data,1,100);
  }
 
void OLED_Init(void)
{
	HAL_Delay(100); //这里的延时很重要
	
	WriteCmd(0xAE); //display off
	WriteCmd(0x20);	//Set Memory Addressing Mode	
	WriteCmd(0x10);	//00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid
	WriteCmd(0xb0);	//Set Page Start Address for Page Addressing Mode,0-7
	WriteCmd(0xc8);	//Set COM Output Scan Direction
	WriteCmd(0x00); //---set low column address
	WriteCmd(0x10); //---set high column address
	WriteCmd(0x40); //--set start line address
	WriteCmd(0x81); //--set contrast control register
	WriteCmd(0xff); //亮度调节 0x00~0xff
	WriteCmd(0xa1); //--set segment re-map 0 to 127
	WriteCmd(0xa6); //--set normal display
	WriteCmd(0xa8); //--set multiplex ratio(1 to 64)
	WriteCmd(0x3F); //
	WriteCmd(0xa4); //0xa4,Output follows RAM content;0xa5,Output ignores RAM content
	WriteCmd(0xd3); //-set display offset
	WriteCmd(0x00); //-not offset
	WriteCmd(0xd5); //--set display clock divide ratio/oscillator frequency
	WriteCmd(0xf0); //--set divide ratio
	WriteCmd(0xd9); //--set pre-charge period
	WriteCmd(0x22); //
	WriteCmd(0xda); //--set com pins hardware configuration
	WriteCmd(0x12);
	WriteCmd(0xdb); //--set vcomh
	WriteCmd(0x20); //0x20,0.77xVcc
	WriteCmd(0x8d); //--set DC-DC enable
	WriteCmd(0x14); //
	WriteCmd(0xaf); //--turn on oled panel
}
 
void OLED_SetPos(unsigned char x, unsigned char y) //设置起始点坐标
{ 
	WriteCmd(0xb0+y);
	WriteCmd(((x&0xf0)>>4)|0x10);
	WriteCmd((x&0x0f)|0x01);
}
 
void OLED_Fill(unsigned char fill_Data)//全屏填充
{
	unsigned char m,n;
	for(m=0;m<8;m++)
	{
		WriteCmd(0xb0+m);		//page0-page1
		WriteCmd(0x00);		//low column start address
		WriteCmd(0x10);		//high column start address
		for(n=0;n<128;n++)
			{
				WriteDat(fill_Data);
			}
	}
}
 
 
void OLED_CLS(void)//清屏
{
	OLED_Fill(0x00);
}
 
void OLED_ON(void)
{
	WriteCmd(0X8D);  //设置电荷泵
	WriteCmd(0X14);  //开启电荷泵
	WriteCmd(0XAF);  //OLED唤醒
}
 
void OLED_OFF(void)
{
	WriteCmd(0X8D);  //设置电荷泵
	WriteCmd(0X10);  //关闭电荷泵
	WriteCmd(0XAE);  //OLED休眠
}
// Parameters     : x,y -- 起始点坐标(x:0~127, y:0~7); ch[] -- 要显示的字符串; TextSize -- 字符大小(1:6*8 ; 2:8*16)
// Description    : 显示codetab.h中的ASCII字符,有6*8和8*16可选择
void OLED_ShowStr(unsigned char x, unsigned char y, unsigned char ch[], unsigned char TextSize)
{
	unsigned char c = 0,i = 0,j = 0;
	switch(TextSize)
	{
		case 1:
		{
			while(ch[j] != '\0')
			{
				c = ch[j] - 32;
				if(x > 126)
				{
					x = 0;
					y++;
				}
				OLED_SetPos(x,y);
				for(i=0;i<6;i++)
					WriteDat(F6x8[c][i]);
				x += 6;
				j++;
			}
		}break;
		case 2:
		{
			while(ch[j] != '\0')
			{
				c = ch[j] - 32;
				if(x > 120)
				{
					x = 0;
					y++;
				}
				OLED_SetPos(x,y);
				for(i=0;i<8;i++)
					WriteDat(F8X16[c*16+i]);
				OLED_SetPos(x,y+1);
				for(i=0;i<8;i++)
					WriteDat(F8X16[c*16+i+8]);
				x += 8;
				j++;
			}
		}break;
	}
}
// 定义显存缓冲区，8页，每页128列（与SSD1306 GDDRAM结构一致）
static uint8_t oled_buffer[8][128] = {0};

// 新增函数：更新显存到OLED（类似OLED_ShowStr的即时写入逻辑）
void OLED_UpdatePage(uint8_t page, uint8_t start_col) {
    OLED_SetPos(start_col, page);
    for (uint8_t col = start_col; col < 128; col++) {
        WriteDat(oled_buffer[page][col]);
    }
}

// 修改后的画点函数
void OLED_DrawPixel(uint8_t x, uint8_t y) {
    // 坐标校验（注意y范围是像素坐标0~63）
    if (x >= 128 || y >= 64) return;

    // 计算页和位
    uint8_t page = y / 8;          // 页号0~7（与OLED_ShowStr的y参数对齐）
    uint8_t bit_pos = y % 8;       // 页内位0~7

    // 修改缓冲区
    oled_buffer[page][x] |= (1 << bit_pos);

    // 通过OLED_SetPos设置位置并更新单个字节（即时刷新）
    OLED_SetPos(x, page);          // 复用OLED_ShowStr的位置设置逻辑
    WriteDat(oled_buffer[page][x]);
}