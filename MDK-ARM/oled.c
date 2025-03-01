#include "oled.h"
#include "asc.h"    //字库（可以自己制作）
#include "main.h"
#include "i2c.h" 
#define OLED_HEIGHT 128
#define OLED_WIDTH 64
uint8_t Bef[3],Cur[3];
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
void OLED_UpdatePage() {
     uint8_t page, x;

    for (page = 0; page < 8; page++)
    {
        for (x = 0; x < OLED_WIDTH; x++)
        {
            // 设置页地址：SSD1306 使用 0xB0 ~ 0xB7 表示页 0 ~ 7
            WriteCmd(0xB0 + page);

            // 设置列地址低位：(0x00 + (x & 0x0F))
            WriteCmd(0x00 + (x & 0x0F));

            // 设置列地址高位：(0x10 + ((x >> 4) & 0x0F))
            WriteCmd(0x10 + ((x >> 4) & 0x0F));

            // 将对应的缓冲区数据写入 OLED
            WriteDat(oled_buffer[page][x]);
        }
    }
}
//显示数字
void OLED_ShowNum(u8 x,u8 y,u32 num,u8 len,u8 size2)//size参数固定为2
{         	
	u8 t,temp;
	u8 enshow=0;						   
	for(t=0;t<len;t++)
	{
		temp=(num/oled_pow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				OLED_ShowChar(x+(size2)*3*t,y,' ',size2);
				continue;
			}else enshow=1; 
		}
	 	OLED_ShowChar(x+(size2)*3*t,y,temp+'0',size2); 
	}
} 
void OLED_ShowNum1(uint8_t x, uint8_t y, uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
    {
        OLED_ShowChar(x, y, Number / oled_pow(10, Length - i - 1) % 10 + '0',1);
    }
}
void OLED_ShowChar(u8 x,u8 y,u8 chr,u8 Char_Size)
{      	
	unsigned char c=0,i=0;	
		c=chr-' ';//???????			
		if(x>128-1){x=0;y=y+2;}
		if(Char_Size ==16)
			{
			OLED_SetPos(x,y);	
			for(i=0;i<8;i++)
			WriteDat(F8X16[c*16+i]);
			OLED_SetPos(x,y+1);
			for(i=0;i<8;i++)
			WriteDat(F8X16[c*16+i+8]);
			}
			else {	
				OLED_SetPos(x,y);
				for(i=0;i<6;i++)
				WriteDat(F6x8[c][i]);
				
			}
}
u32 oled_pow(u8 m,u8 n)
{
	u32 result=1;	 
	while(n--)result*=m;    
	return result;
}	
// 修改后的画点函数
void OLED_DrawPixel(uint8_t x, uint8_t y) {
//    // 坐标校验（注意y范围是像素坐标0~63）
//    if (x >= 128 || y >= 64) return;

//    // 计算页和位
//    uint8_t page = y / 8;          // 页号0~7（与OLED_ShowStr的y参数对齐）
//    uint8_t bit_pos = y % 8;       // 页内位0~7

//    // 修改缓冲区
//    oled_buffer[page][x] |= (1 << bit_pos);

//    // 通过OLED_SetPos设置位置并更新单个字节（即时刷新）
//    OLED_SetPos(x, page);          // 复用OLED_ShowStr的位置设置逻辑
//    WriteDat(oled_buffer[page][x]);
	 // 坐标检查（确保在 OLED 有效范围内）
    if (x >= 128 || y >= 64)
    {
        return;
    }

    // 计算页号和页内位位置（SSD1306 每页存储8行像素）
    uint8_t page = y / 8;
    uint8_t bit_pos = y % 8;

    // 在缓冲区的对应位置设置相应位
    oled_buffer[page][x] |= (1 << bit_pos);

    // 设置 SSD1306 地址：
    // 1. 选择页：0xB0 + page
    WriteCmd(0xB0 + page);
    // 2. 设定列地址低位：0x00 + (x & 0x0F)
    WriteCmd(0x00 + (x & 0x0F));
    // 3. 设定列地址高位：0x10 + ((x >> 4) & 0x0F)
    WriteCmd(0x10 + ((x >> 4) & 0x0F));
    // 更新该字节的数据到 OLED
    WriteDat(oled_buffer[page][x]);
}
void OLED_ClearBuffer(void)
{
    // 方法1：使用嵌套循环
    for (uint8_t page = 0; page < 8; page++)
    {
        for (uint8_t col = 0; col < 128; col++)
        {
            oled_buffer[page][col] = 0;
        }
    }

    // 方法2：如果 oled_buffer 在内存中是连续存储的，
    // 可以直接使用 memset 将整个区域清零
    // memset(oled_buffer, 0, sizeof(oled_buffer));
}
void ClearRectFrom(uint8_t x, uint8_t y)
{
    // 检查输入坐标是否有效
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT) {
        return;
    }
    
    // 计算受影响的页面范围
    uint8_t firstPage = y / 8;          // 起始页面
    uint8_t lastPage  = (OLED_HEIGHT - 1) / 8;   // 最后一页（63/8 = 7）

    // 对每一页进行遍历，从首个受影响页面到最后页面
    for (uint8_t page = firstPage; page <= lastPage; page++)
    {
        uint8_t mask;
        if (page == firstPage) {
            // 首个页面中，清除的起始像素为 y % 8（即该页面内从此比特位开始）
            // 例如 y=10 时，10%8==2，则清除此页中第2位到第7位（0xFF << 2 => 0xFC）
            mask = 0xFF << (y % 8);
        } else {
            // 对于后续页面，整个页面都在清除范围内
            mask = 0xFF;
        }
        // 以列为单位，遍历从给定x到右侧边界
        for (uint8_t col = x; col < OLED_WIDTH; col++)
        {
            /* 
             * 对应位置的字节中，利用 bitwise AND 清零那些位：
             * 原因：oled_buffer[page][col]中每个比特对应一个像素，将其与 ~mask 相与，
             * 那些 mask 中为 1 的位就会被清0，而其他位保持不变。
             */
            oled_buffer[page][col] &= ~mask;
						
        }
    }
		OLED_UpdatePage();
}
void ClearRectangleStr()//TextSize请默认为1，直接除了第一行清空
{
    for (int y = 1; y < 8; y++)
		{
			OLED_ShowStr(0,y,"                      ",1);
		}   
}
void OLED_ShowFloat(u8 x, u8 y, float num_f, u8 int_len, u8 dec_len, u8 size2)
{
	 uint32_t scale = oled_pow(10, dec_len);
    uint32_t scaled = (uint32_t)(num_f * scale + 0.5f);  // 四舍五入到指定小数位
    uint32_t int_part = scaled / scale;
    uint32_t dec_part = scaled % scale;

    // 显示整数部分（继承前导零处理逻辑）
    OLED_ShowNum(x, y, int_part, int_len, size2);

    // 若小数位数为 0，不显示小数点和小数部分
    if (dec_len == 0) return;
		// 计算整数部分占用的像素宽度
    u8 int_width = int_len * (size2 * 3);

    // 显示小数点
    OLED_ShowChar(x + int_width, y, '.', size2);

    // 显示小数部分（固定显示 dec_len 位，自动补零）
    for (u8 t = 0; t < dec_len; t++) {
        u8 temp = (dec_part / oled_pow(10, dec_len - t - 1)) % 10;
        OLED_ShowChar(x + int_width + (size2 * 3) * (t + 1), y, temp + '0', size2);
    }
}
//修改缓冲区实现画点
void setPixel(uint8_t x, uint8_t y) //弃用
{
//    if (x >= OLED_WIDTH || y >= OLED_HEIGHT)
//        return;
//    uint8_t page = y / 8;          // 每页 8 行像素
//    uint8_t bit_position = y % 8;
//    oled_buffer[page][x] |= (1 << bit_position);
}
void Before_State_Update(uint8_t y)//根据y的值，求出前一个数据的有关参数
{
	Bef[0]=7-y/8;
	Bef[1]=7-y%8;
	Bef[2]=1<<Bef[1];
}
void Current_State_Update(uint8_t y)//根据Y值，求出当前数据的有关参数
{
	Cur[0]=7-y/8;//数据写在第几页
	Cur[1]=7-y%8;//0x01要移动的位数
	Cur[2]=1<<Cur[1];//要写什么数据
}
 
 
void OLED_SetPos2(unsigned char x, unsigned char y) //设置起始点坐标
{ 
	WriteCmd(0xb0+x);
	WriteCmd((y&0x0f)|0x00);//LOW
	WriteCmd(((y&0xf0)>>4)|0x10);//HIGHT
}
 
void OLED_DrawWave(uint8_t x,uint8_t y)
{
 
	int8_t page_sub;
	uint8_t page_buff,i,j;
	Current_State_Update(y);//根据Y值，求出当前数据的有关参数
	page_sub=Bef[0]-Cur[0];//当前值与前一个值的页数相比较
	//确定当前列，每一页应该写什么数据
	if(page_sub>0)
	{
		page_buff=Bef[0];
		OLED_SetPos2(page_buff,x);
		WriteDat(Bef[2]-0x01);
		page_buff--;
		for(i=0;i<page_sub-1;i++)
		{
			OLED_SetPos2(page_buff,x);
			WriteDat(0xff);
			page_buff--;
		}
		OLED_SetPos2(page_buff,x);
		WriteDat(0xff<<Cur[1]);
	}
	else if(page_sub==0)
	{
		if(Cur[1]==Bef[1])
		{
			OLED_SetPos2(Cur[0],x);
			WriteDat(Cur[2]);
		}
		else if(Cur[1]>Bef[1])
		{
			OLED_SetPos2(Cur[0],x);
			WriteDat((Cur[2]-Bef[2])|Cur[2]);
		}
		else if(Cur[1]<Bef[1])
		{
			OLED_SetPos2(Cur[0],x);
			WriteDat(Bef[2]-Cur[2]);
		}
	}
	else if(page_sub<0)
	{
		page_buff=Cur[0];
		OLED_SetPos2(page_buff,x);
		WriteDat((Cur[2]<<1)-0x01);
		page_buff--;
		for(i=0;i<0-page_sub-1;i++)
		{
			OLED_SetPos2(page_buff,x);
			WriteDat(0xff);
			page_buff--;
		}
		OLED_SetPos2(page_buff,x);
		WriteDat(0xff<<(Bef[1]+1));
	}
	Before_State_Update(y);
	//把下一列，每一页的数据清除掉
	for(i=0;i<8;i++)
	{
		OLED_SetPos2(i, x+1) ;
		for(j=0;j<1;j++)
			WriteDat(0x00);
	}
}