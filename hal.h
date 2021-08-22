#if defined(__NUVO_M032K)
#define UART_0_USE_PF2_3
/* UI defn's */
#define BL_LED_PORT GPIO_PORT
#define BL_LED_PIN 14
#define BL_SW_PORT GPIO_PORTB
#define BL_SW_PIN 14
/*************/
#elif defined(__SAMR21) || defined(__SAMD21)
/* UI defn's */
#define BL_LED_PORT GPIO_PORTA
#define BL_LED_PIN 22
#define BL_SW_PORT GPIO_PORTA
#define BL_SW_PIN 14
/*************/
#endif