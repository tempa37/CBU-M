/*****************************************************************************
* | File      	:   OLED_1in5_rgb_test.c
* | Author      :   Waveshare team
* | Function    :   1.5inch OLED Module test demo
* | Info        :
*----------------
* |	This version:   V2.0
* | Date        :   2020-08-17
* | Info        :
* -----------------------------------------------------------------------------
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/
#include "test.h"
#include "FreeRTOS.h"
#include "task.h"
//#include "main.h"
#include "cmsis_os.h"
#include "OLED_1in5_rgb.h"
#include "rs_485.h"

uint32_t gOLEDCount = 0;
int OLED_1in5_rgb_test(void)
{
	OLED_1in5_rgb_Init();
//	Driver_Delay_ms(500);	
	OLED_1in5_rgb_Clear();
	
	// 0.Create a new image cache
	UBYTE *BlackImage;
	UWORD Imagesize = (OLED_1in5_RGB_WIDTH*2) * OLED_1in5_RGB_HEIGHT;
	if((BlackImage = (UBYTE *)malloc(Imagesize/4)) == NULL) {
			return -1;
	}
	
	Paint_NewImage(BlackImage, OLED_1in5_RGB_WIDTH, OLED_1in5_RGB_HEIGHT, 0, BLACK);	
	Paint_SetScale(65);
	
	//1.Select Image
	Paint_SelectImage(BlackImage);
	Driver_Delay_ms(500);
	Paint_Clear(BLACK);
        

        ///____________________________________________________________________///
//        Paint_DrawString_EN(25, 30, "***800YMG***", &Courier12R, WHITE, BLACK);
//        //Paint_DrawChar(20, 30, 'C', &Font12, BLACK, WHITE);
//        OLED_1in5_rgb_Display(BlackImage);
//        Paint_DrawString_EN(5, 60, "WHITE_IN_THE_HOOD", &Courier12R, WHITE, BLACK);
//        OLED_1in5_rgb_Display(BlackImage);
//        Paint_DrawString_EN(50, 90, "54/29", &Courier12R, WHITE, BLACK);
//        OLED_1in5_rgb_Display(BlackImage);
//        vTaskDelay(10000);
//        //for(int i = 0; i<100000000; i++);
//        Paint_Clear(BLACK);
        ///____________________________________________________________________///
        Paint_DrawString_EN(12, 0, "ТРАНСМАШ-ТОМСК", &Courier12R, GREEN, BLACK);
        Paint_DrawString_EN(0, 13, "УМВХ:", &Courier12R, GREEN, BLACK);
        Paint_DrawString_EN(32, 13, "ON", &Courier12R, WHITE, BLACK);
        Paint_DrawString_EN(0, 25, "К1:", &Courier12R, GREEN, BLACK);
        Paint_DrawString_EN(0, 37, "К2:", &Courier12R, GREEN, BLACK);
        Paint_DrawString_EN(0, 49, "К3:", &Courier12R, GREEN, BLACK);
        Paint_DrawString_EN(0, 61, "К4:", &Courier12R, GREEN, BLACK);
        Paint_DrawString_EN(0, 73, "К5:", &Courier12R, GREEN, BLACK);
        Paint_DrawString_EN(0, 85, "К6:", &Courier12R, GREEN, BLACK);
        Paint_DrawString_EN(0, 97, "К7:", &Courier12R, GREEN, BLACK);
        Paint_DrawString_EN(0, 109, "К8:", &Courier12R, GREEN, BLACK);
        Paint_DrawLine(86, 27, 86, 127, GREEN, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
        Paint_DrawString_EN(88, 13, "УРМ:", &Courier12R, GREEN, BLACK);
        Paint_DrawString_EN(113, 13, "ON", &Courier12R, WHITE, BLACK);
        Paint_DrawString_EN(88, 25, "Р1:", &Courier12R, GREEN, BLACK);
        Paint_DrawString_EN(88, 37, "Р2:", &Courier12R, GREEN, BLACK);
        Paint_DrawString_EN(88, 49, "Р3:", &Courier12R, GREEN, BLACK);
        Paint_DrawString_EN(88, 61, "Р4:", &Courier12R, GREEN, BLACK);
        Paint_DrawString_EN(88, 73, "Р5:", &Courier12R, GREEN, BLACK);
        Paint_DrawString_EN(88, 85, "Р6:", &Courier12R, GREEN, BLACK);
        Paint_DrawString_EN(88, 97, "Р7:", &Courier12R, GREEN, BLACK);
        Paint_DrawString_EN(88, 109, "Р8:", &Courier12R, GREEN, BLACK);
        OLED_1in5_rgb_Display(BlackImage);
        while(1){
          if(gRsBlock1.aRsModule[0].ReleOsState.ReleState &0x01){
              Paint_ClearWindows(113, 25, 127, 37, BLACK);
              Paint_DrawRectangle(110, 28, 117, 35, GRAY, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
              Paint_DrawRectangle(112, 30, 115, 34, GREEN, DOT_PIXEL_1X1, DRAW_FILL_FULL);
          }else{
              Paint_ClearWindows(113, 25, 127, 39, BLACK);
              Paint_DrawRectangle(110, 28, 117, 35, GRAY, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
          }
          if(gRsBlock1.aRsModule[0].ReleOsState.ReleState &0x02){
              Paint_ClearWindows(113, 37, 127, 49, BLACK);
              Paint_DrawRectangle(110, 40, 117, 47, GRAY, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
              Paint_DrawRectangle(112, 42, 115, 46, GREEN, DOT_PIXEL_1X1, DRAW_FILL_FULL);
          }
          else{
              Paint_ClearWindows(113, 39, 127, 51, BLACK);
              Paint_DrawRectangle(110, 40, 117, 47, GRAY, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
          }
          if(gRsBlock1.aRsModule[0].ReleOsState.ReleState &0x04){
              Paint_ClearWindows(113, 49, 127, 61, BLACK);
              Paint_DrawRectangle(110, 52, 117, 59, GRAY, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
              Paint_DrawRectangle(112, 54, 115, 58, GREEN, DOT_PIXEL_1X1, DRAW_FILL_FULL);
          }
          else{
              Paint_ClearWindows(113, 51, 127, 63, BLACK);
              Paint_DrawRectangle(110, 52, 117, 59, GRAY, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
          }
          if(gRsBlock1.aRsModule[0].ReleOsState.ReleState &0x08){
              Paint_ClearWindows(113, 61, 127, 73, BLACK);
              Paint_DrawRectangle(110, 64, 117, 71, GRAY, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
              Paint_DrawRectangle(112, 66, 115, 70, GREEN, DOT_PIXEL_1X1, DRAW_FILL_FULL);
          }
          else{
              Paint_ClearWindows(113, 63, 127, 75, BLACK);
              Paint_DrawRectangle(110, 64, 117, 71, GRAY, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
          }
          if(gRsBlock1.aRsModule[0].ReleOsState.ReleState &0x10){
              Paint_ClearWindows(113, 73, 127, 85, BLACK);
              Paint_DrawRectangle(110, 46, 117, 83, GRAY, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
              Paint_DrawRectangle(112, 48, 115, 82, GREEN, DOT_PIXEL_1X1, DRAW_FILL_FULL);
          }
          else{
              Paint_ClearWindows(113, 75, 127, 87, BLACK);
              Paint_DrawRectangle(110, 76, 117, 83, GRAY, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
          }
          if(gRsBlock1.aRsModule[0].ReleOsState.ReleState &0x20){
              Paint_ClearWindows(113, 85, 127, 97, BLACK);
              Paint_DrawRectangle(110, 88, 117, 95, GRAY, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
              Paint_DrawRectangle(112, 90, 115, 94, GREEN, DOT_PIXEL_1X1, DRAW_FILL_FULL);
          }
          else{
              Paint_ClearWindows(113, 87, 127, 99, BLACK);
              Paint_DrawRectangle(110, 88, 117, 95, GRAY, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
          }
          if(gRsBlock1.aRsModule[0].ReleOsState.ReleState &0x40){
              Paint_ClearWindows(113, 97, 127, 109, BLACK);
              Paint_DrawRectangle(110, 100, 117, 107, GRAY, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
              Paint_DrawRectangle(112, 102, 115, 106, GREEN, DOT_PIXEL_1X1, DRAW_FILL_FULL);
          }
          else{
              Paint_ClearWindows(113, 99, 127, 111, BLACK);
              Paint_DrawRectangle(110, 100, 117, 107, GRAY, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
          }
          if(gRsBlock1.aRsModule[0].ReleOsState.ReleState &0x80){
              Paint_ClearWindows(113, 109, 127, 121, BLACK);
              Paint_DrawRectangle(110, 112, 117, 119, GRAY, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
              Paint_DrawRectangle(112, 114, 115, 118, GREEN, DOT_PIXEL_1X1, DRAW_FILL_FULL);
          }
          else{
              Paint_ClearWindows(113, 111, 127, 123, BLACK);
              Paint_DrawRectangle(110, 112, 117, 119, GRAY, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
          }
///______________________________КАНАЛ 1___________________________________///
          switch(gRsBlock1.aRsModule[1].aIO_Item[0].Type){
            ///_________НЕТ Датчика_______///
            case 0:{
              Paint_ClearWindows(20, 25, 84, 37, BLACK);
              Paint_DrawString_EN(20, 25, "Н/Д", &Courier12R, CYAN, BLACK);
              break;
            }
            ///_________Датчик 4-20_______///
            case 1:
              {
              Paint_ClearWindows(20, 25, 44, 37, BLACK);
              Paint_DrawString_EN(20, 25, "I =", &Courier12R, CYAN, BLACK);
              char cStr[3];
              int16_t cValue = gRsBlock1.aRsModule[1].aIO_Item[1].Value;
              float fValue = cValue;
//          sprintf(cStr, "%d", cValue);
              sprintf(cStr, "%.2f", fValue/100);
              Paint_ClearWindows(45, 25, 44, 37, BLACK);
              Paint_DrawString_EN(45, 25, cStr, &Courier12R, WHITE, BLACK);
              break;
              }
              ///_________Датчик C/K_______///
            case 4:
              {
              Paint_ClearWindows(20, 25, 44, 37, BLACK);
              Paint_DrawString_EN(20, 25, "C/K", &Courier12R, CYAN, BLACK);
                  switch(gRsBlock1.aRsModule[1].aIO_Item[0].Value){
                    
                    case 1:
                      {
                        Paint_ClearWindows(45, 25, 84, 37, BLACK);
                        Paint_DrawString_EN(45, 25, "Норма", &Courier12R, WHITE, BLACK);
                        break;
                      }
                    case 2:
                      {
                        Paint_ClearWindows(45, 25, 84, 37, BLACK);
                      Paint_DrawString_EN(45, 25, "Обрыв", &Courier12R, YELLOW, BLACK);
                        break;
                      }
                    case 3:
                      {
                        Paint_ClearWindows(45, 25, 84, 37, BLACK);
                        Paint_DrawString_EN(60, 25, "КЗ", &Courier12R, RED, BLACK);
                        break;
                      }
                    }
                  break;
              }
          }
///______________________________КАНАЛ 2___________________________________///          
          switch(gRsBlock1.aRsModule[1].aIO_Item[1].Type){
            ///_________НЕТ Датчика_______///
            case 0:{
              Paint_ClearWindows(20, 37, 84, 49, BLACK);
              Paint_DrawString_EN(20, 37, "Н/Д", &Courier12R, CYAN, BLACK);
              break;
            }
            ///_________Датчик 4-20_______///
            case 1:
              {
              Paint_ClearWindows(20, 37, 44, 49, BLACK);
              Paint_DrawString_EN(20, 37, "I =", &Courier12R, CYAN, BLACK);
              char cStr[3];
              int16_t cValue = gRsBlock1.aRsModule[1].aIO_Item[1].Value;
              float fValue = cValue;
//          sprintf(cStr, "%d", cValue);
              sprintf(cStr, "%.2f", fValue/100);
              Paint_ClearWindows(45, 37, 84, 49, BLACK);
              Paint_DrawString_EN(45, 37, cStr, &Courier12R, WHITE, BLACK);
              break;
              }
              ///_________Датчик C/K_______///
            case 4:
              {
              Paint_ClearWindows(20, 37, 44, 49, BLACK);
              Paint_DrawString_EN(20, 35, "C/K", &Courier12R, CYAN, BLACK);
                    switch(gRsBlock1.aRsModule[1].aIO_Item[1].Value){
                      
                      case 1:
                        {
                          Paint_ClearWindows(45, 37, 84, 49, BLACK);
                          Paint_DrawString_EN(45, 37, "Норма", &Courier12R, WHITE, BLACK);
                          break;
                        }
                      case 2:
                        {
                          Paint_ClearWindows(45, 37, 84, 49, BLACK);
                        Paint_DrawString_EN(45, 37, "Обрыв", &Courier12R, YELLOW, BLACK);
                          break;
                        }
                      case 3:
                        {
                          Paint_ClearWindows(45, 37, 84, 49, BLACK);
                          Paint_DrawString_EN(60, 37, "КЗ", &Courier12R, RED, BLACK);
                          break;
                        }
                      }
                    break;
              }
          }
///______________________________КАНАЛ 3___________________________________///          
          switch(gRsBlock1.aRsModule[1].aIO_Item[2].Type){
            ///_________НЕТ Датчика_______///
            case 0:{
              Paint_ClearWindows(20, 49, 84, 61, BLACK);
              Paint_DrawString_EN(20, 49, "Н/Д", &Courier12R, CYAN, BLACK);
              break;
            }
            ///_________Датчик 4-20_______///
            case 1:
              {
              Paint_ClearWindows(20, 49, 44, 61, BLACK);
              Paint_DrawString_EN(20, 49, "I =", &Courier12R, CYAN, BLACK);
              char cStr[3];
              int16_t cValue = gRsBlock1.aRsModule[1].aIO_Item[2].Value;
              float fValue = cValue;
//          sprintf(cStr, "%d", cValue);
              sprintf(cStr, "%.2f", fValue/100);
              Paint_ClearWindows(45, 49, 84, 61, BLACK);
              Paint_DrawString_EN(45, 49, cStr, &Courier12R, WHITE, BLACK);
              break;
              }
              ///_________Датчик C/K_______///
            case 4:
              {
              Paint_ClearWindows(20, 49, 44, 61, BLACK);
              Paint_DrawString_EN(20, 49, "C/K", &Courier12R, CYAN, BLACK);
                    switch(gRsBlock1.aRsModule[1].aIO_Item[2].Value){
                      
                      case 1:
                        {
                          Paint_ClearWindows(45, 49, 84, 61, BLACK);
                          Paint_DrawString_EN(45, 49, "Норма", &Courier12R, WHITE, BLACK);
                          break;
                        }
                      case 2:
                        {
                          Paint_ClearWindows(45, 49, 84, 61, BLACK);
                        Paint_DrawString_EN(45, 49, "Обрыв", &Courier12R, YELLOW, BLACK);
                          break;
                        }
                      case 3:
                        {
                          Paint_ClearWindows(45, 49, 84, 61, BLACK);
                          Paint_DrawString_EN(60, 49, "КЗ", &Courier12R, RED, BLACK);
                          break;
                        }
                      }
                    break;
              }
          }
///______________________________КАНАЛ 4___________________________________///          
          switch(gRsBlock1.aRsModule[1].aIO_Item[3].Type){
            ///_________НЕТ Датчика_______///
            case 0:{
              Paint_ClearWindows(20, 61, 84, 73, BLACK);
              Paint_DrawString_EN(20, 61, "Н/Д", &Courier12R, CYAN, BLACK);
              break;
            }
            ///_________Датчик 4-20_______///
            case 1:
              {
              Paint_ClearWindows(20, 61, 44, 73, BLACK);
              Paint_DrawString_EN(20, 61, "I =", &Courier12R, CYAN, BLACK);
              char cStr[3];
              int16_t cValue = gRsBlock1.aRsModule[1].aIO_Item[3].Value;
              float fValue = cValue;
//          sprintf(cStr, "%d", cValue);
              sprintf(cStr, "%.2f", fValue/100);
              Paint_ClearWindows(45, 61, 84, 73, BLACK);
              Paint_DrawString_EN(45, 61, cStr, &Courier12R, WHITE, BLACK);
              break;
              }
              ///_________Датчик C/K_______///
            case 4:
              {
              Paint_ClearWindows(20, 61, 44, 73, BLACK);
              Paint_DrawString_EN(20, 61, "C/K", &Courier12R, CYAN, BLACK);
                    switch(gRsBlock1.aRsModule[1].aIO_Item[3].Value){
                      
                      case 1:
                        {
                          Paint_ClearWindows(45, 61, 84, 73, BLACK);
                          Paint_DrawString_EN(45, 61, "Норма", &Courier12R, WHITE, BLACK);
                          break;
                        }
                      case 2:
                        {
                          Paint_ClearWindows(45, 61, 84, 73, BLACK);
                        Paint_DrawString_EN(45, 61, "Обрыв", &Courier12R, YELLOW, BLACK);
                          break;
                        }
                      case 3:
                        {
                          Paint_ClearWindows(45, 61, 84, 73, BLACK);
                          Paint_DrawString_EN(60, 61, "КЗ", &Courier12R, RED, BLACK);
                          break;
                        }
                      }
                    break;
              }
          }
///______________________________КАНАЛ 5___________________________________///          
          switch(gRsBlock1.aRsModule[1].aIO_Item[4].Type){
            ///_________НЕТ Датчика_______///
            case 0:{
              Paint_ClearWindows(20, 73, 84, 85, BLACK);
              Paint_DrawString_EN(20, 73, "Н/Д", &Courier12R, CYAN, BLACK);
              break;
            }
            ///_________Датчик 4-20_______///
            case 1:
              {
              Paint_ClearWindows(20, 73, 44, 85, BLACK);
              Paint_DrawString_EN(20, 73, "I =", &Courier12R, CYAN, BLACK);
              char cStr[3];
              int16_t cValue = gRsBlock1.aRsModule[1].aIO_Item[4].Value;
              float fValue = cValue;
//          sprintf(cStr, "%d", cValue);
              sprintf(cStr, "%.2f", fValue/100);
              Paint_ClearWindows(45, 73, 84, 85, BLACK);
              Paint_DrawString_EN(45, 73, cStr, &Courier12R, WHITE, BLACK);
              break;
              }
              ///_________Датчик C/K_______///
            case 4:
              {
              Paint_ClearWindows(20, 73, 44, 85, BLACK);
              Paint_DrawString_EN(20, 73, "C/K", &Courier12R, CYAN, BLACK);
                    switch(gRsBlock1.aRsModule[1].aIO_Item[4].Value){
                      
                      case 1:
                        {
                          Paint_ClearWindows(45, 73, 84, 85, BLACK);
                          Paint_DrawString_EN(45, 73, "Норма", &Courier12R, WHITE, BLACK);
                          break;
                        }
                      case 2:
                        {
                          Paint_ClearWindows(45, 73, 84, 85, BLACK);
                        Paint_DrawString_EN(45, 73, "Обрыв", &Courier12R, YELLOW, BLACK);
                          break;
                        }
                      case 3:
                        {
                          Paint_ClearWindows(45, 73, 84, 85, BLACK);
                          Paint_DrawString_EN(60, 73, "КЗ", &Courier12R, RED, BLACK);
                          break;
                        }
                      }
                    break;
              }
          }
///______________________________КАНАЛ 6___________________________________///          
          switch(gRsBlock1.aRsModule[1].aIO_Item[5].Type){
            ///_________НЕТ Датчика_______///
            case 0:{
              Paint_ClearWindows(20, 85, 84, 97, BLACK);
              Paint_DrawString_EN(20, 85, "Н/Д", &Courier12R, CYAN, BLACK);
              break;
            }
            ///_________Датчик 4-20_______///
            case 1:
              {
              Paint_ClearWindows(20, 85, 44, 97, BLACK);
              Paint_DrawString_EN(20, 85, "I =", &Courier12R, CYAN, BLACK);
              char cStr[3];
              int16_t cValue = gRsBlock1.aRsModule[1].aIO_Item[5].Value;
              float fValue = cValue;
//          sprintf(cStr, "%d", cValue);
              sprintf(cStr, "%.2f", fValue/100);
              Paint_ClearWindows(45, 85, 84, 97, BLACK);
              Paint_DrawString_EN(45, 85, cStr, &Courier12R, WHITE, BLACK);
              break;
              }
              ///_________Датчик C/K_______///
            case 4:
              {
              Paint_ClearWindows(20, 85, 44, 97, BLACK);
              Paint_DrawString_EN(20, 85, "C/K", &Courier12R, CYAN, BLACK);
                    switch(gRsBlock1.aRsModule[1].aIO_Item[5].Value){
                      
                      case 1:
                        {
                          Paint_ClearWindows(45, 85, 84, 97, BLACK);
                          Paint_DrawString_EN(45, 85, "Норма", &Courier12R, WHITE, BLACK);
                          break;
                        }
                      case 2:
                        {
                          Paint_ClearWindows(45, 85, 84, 97, BLACK);
                        Paint_DrawString_EN(45, 85, "Обрыв", &Courier12R, YELLOW, BLACK);
                          break;
                        }
                      case 3:
                        {
                          Paint_ClearWindows(45, 85, 84, 97, BLACK);
                          Paint_DrawString_EN(60, 85, "КЗ", &Courier12R, RED, BLACK);
                          break;
                        }
                      }
                    break;
              }
          }
///______________________________КАНАЛ 7___________________________________///          
          switch(gRsBlock1.aRsModule[1].aIO_Item[6].Type){
            ///_________НЕТ Датчика_______///
            case 0:{
              Paint_ClearWindows(20, 97, 84, 109, BLACK);
              Paint_DrawString_EN(20, 97, "Н/Д", &Courier12R, CYAN, BLACK);
//              vTaskDelay(10);
              break;
            }
            ///_________Датчик 4-20_______///
            case 1:
              {
              Paint_ClearWindows(20, 97, 44, 109, BLACK);
              Paint_DrawString_EN(20, 97, "I =", &Courier12R, CYAN, BLACK);
//              vTaskDelay(10);
              char cStr[3];
              int16_t cValue = gRsBlock1.aRsModule[1].aIO_Item[6].Value;
              float fValue = cValue;
//          sprintf(cStr, "%d", cValue);
              sprintf(cStr, "%.2f", fValue/100);
              Paint_ClearWindows(45, 97, 84, 109, BLACK);
              Paint_DrawString_EN(45, 97, cStr, &Courier12R, WHITE, BLACK);
//              vTaskDelay(10);
              break;
              }
              ///_________Датчик C/K_______///
            case 4:
              {
              Paint_ClearWindows(20, 97, 44, 109, BLACK);
              Paint_DrawString_EN(20, 97, "C/K", &Courier12R, CYAN, BLACK);
//              vTaskDelay(10);
                    switch(gRsBlock1.aRsModule[1].aIO_Item[6].Value){
                      
                      case 1:
                        {
                          Paint_ClearWindows(45, 97, 84, 109, BLACK);
                          Paint_DrawString_EN(45, 97, "Норма", &Courier12R, WHITE, BLACK);
//                          vTaskDelay(10);
                          break;
                        }
                      case 2:
                        {
                          Paint_ClearWindows(45, 97, 84, 109, BLACK);
                        Paint_DrawString_EN(45, 97, "Обрыв", &Courier12R, YELLOW, BLACK);
//                        vTaskDelay(10);
                          break;
                        }
                      case 3:
                        {
                          Paint_ClearWindows(45, 97, 84, 109, BLACK);
                          Paint_DrawString_EN(60, 97, "КЗ", &Courier12R, RED, BLACK);
//                          vTaskDelay(10);
                          break;
                        }
                      }
                    break;
              }
          }
///______________________________КАНАЛ 8___________________________________///          
          switch(gRsBlock1.aRsModule[1].aIO_Item[7].Type){
            ///_________НЕТ Датчика_______///
            case 0:{
              Paint_ClearWindows(20, 109, 84, 121, BLACK);
              Paint_DrawString_EN(20, 109, "Н/Д", &Courier12R, CYAN, BLACK);
//              vTaskDelay(10);
              break;
            }
            ///_________Датчик 4-20_______///
            case 1:
              {
              Paint_ClearWindows(20, 109, 44, 121, BLACK);
              Paint_DrawString_EN(20, 109, "I =", &Courier12R, CYAN, BLACK);
//              vTaskDelay(10);
              char cStr[3];
              int16_t cValue = gRsBlock1.aRsModule[1].aIO_Item[7].Value;
              float fValue = cValue;
//          sprintf(cStr, "%d", cValue);
              sprintf(cStr, "%.2f", fValue/100);
              Paint_ClearWindows(45, 109, 84, 121, BLACK);
              Paint_DrawString_EN(45, 109, cStr, &Courier12R, WHITE, BLACK);
//              vTaskDelay(10);
              break;
              }
              ///_________Датчик C/K_______///
            case 4:
              {
              Paint_ClearWindows(20, 109, 44, 121, BLACK);
              Paint_DrawString_EN(20, 109, "C/K", &Courier12R, CYAN, BLACK);
//              vTaskDelay(10);
                    switch(gRsBlock1.aRsModule[1].aIO_Item[7].Value){
                      
                      case 1:
                        {
                          Paint_ClearWindows(45, 109, 84, 121, BLACK);
                          Paint_DrawString_EN(45, 109, "Норма", &Courier12R, WHITE, BLACK);
//                          vTaskDelay(10);
                          break;
                        }
                      case 2:
                        {
                          Paint_ClearWindows(45, 109, 84, 121, BLACK);
                        Paint_DrawString_EN(45, 109, "Обрыв", &Courier12R, YELLOW, BLACK);
//                        vTaskDelay(10);
                          break;
                        }
                      case 3:
                        {
                          Paint_ClearWindows(45, 109, 84, 121, BLACK);
                          Paint_DrawString_EN(60, 109, "КЗ", &Courier12R, RED, BLACK);
//                          vTaskDelay(10);
                          break;
                        }
                      }
                    break;
              }
          }
        OLED_1in5_rgb_Display(BlackImage);
        vTaskDelay(2);
          ++gOLEDCount;
        }
         
        return 0;
}

