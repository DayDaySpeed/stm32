#include "GPIO.h"
#include "KEY.h"
#include "LED.h"
#include "SYS.h"

int main(void)
{
  systick_init();
  output_init();
  key_init();

  //自检,确保该GPIO可正常输出
  for(int i = 0; i < 3; ++i){
    GPIOC_ODR ^= PC13_ODR_BIT;
    GPIOA_ODR ^= PA1_ODR_BIT;
    delay_ms(300);

    GPIOC_ODR ^= PC13_ODR_BIT;
    GPIOA_ODR ^= PA1_ODR_BIT;
    delay_ms(300);
  }

  while (1) {
    if (key_scan_event(KEY_PB0) == 1U) {
      output_toggle_pc13();
    }

    if (key_scan_event(KEY_PB1) == 1U) {
      output_toggle_aux();
    }
  }
}
