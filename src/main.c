#include "app/app.h"
#include "drivers/systick.h"

void SysTick_Handler(void)
{
  systick_on_interrupt();
}

int main(void)
{
  app_init();
  app_run_forever();
}
