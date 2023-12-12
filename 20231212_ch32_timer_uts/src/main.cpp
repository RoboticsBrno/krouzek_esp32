#include <Arduino.h>

static int gIter = 0;

//!< Default is speed at 25C, 50%, 101 kPa
static constexpr float defaultSpeedOfSound = 347.13f / 100.f;

static volatile bool gUtsMeasuring = false;
static uint16_t gUtsRisingMicros = 0;
static volatile uint16_t gUtsEchoUs = 0;

static void ultrasoundDone(uint16_t echoUs) {
    gUtsEchoUs = echoUs;
    gUtsMeasuring = false;
    TIM_Cmd(TIM2, DISABLE);
}

static void ultrasoundEchoIrq() {
    const auto now = TIM_GetCounter(TIM2);

    if(digitalReadFast(PD_3)) {
        // Rising edge
        gUtsRisingMicros = now;
    } else {
        // Falling edge
        if(now > gUtsRisingMicros) {
            ultrasoundDone(now - gUtsRisingMicros);
        }
    }
}

extern "C" void TIM2_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
extern "C" void TIM2_IRQHandler(void) {
    // 10us trigger - set trigger pin to 0
    if( TIM_GetITStatus( TIM2, TIM_IT_CC1 ) != RESET )
    {
        digitalWriteFast(PD_4, 0);
    }

    // 30ms timeout - end measuring
    if( TIM_GetITStatus( TIM2, TIM_IT_CC2 ) != RESET )
    {
       ultrasoundDone(0);
    }

    TIM_ClearITPendingBit( TIM2, TIM_IT_CC1 | TIM_IT_CC2 );
}


void setup() {
    Serial.begin(115200);

    pinMode(PD4, OUTPUT); // trigger

    // echo pin
    attachInterrupt(PD3, GPIO_Mode_IPD, ultrasoundEchoIrq, EXTI_Mode_Interrupt, EXTI_Trigger_Rising_Falling);


    RCC_APB1PeriphClockCmd( RCC_APB1Periph_TIM2, ENABLE );

    // Set-up counter where 1 timer unit == 1us
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure={0};
    TIM_TimeBaseInitStructure.TIM_Period = 0xFFFF;
    TIM_TimeBaseInitStructure.TIM_Prescaler = 48; // 48 Mhz to 1us
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);

    // Set-up TIM2 interrupt
    NVIC_InitTypeDef        NVIC_InitStructure = {0};
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // Trigger interrupt on two capture values
    TIM_SetCompare1(TIM2, 10); // 10us trigger signal
    TIM_SetCompare2(TIM2, 30000); // 30ms timeout
    TIM_ITConfig(TIM2, TIM_IT_CC1 | TIM_IT_CC2, ENABLE);
}

void loop() {

    // Trigger UTS measuring
    gUtsMeasuring = true;
    TIM_SetCounter(TIM2, 0);
    TIM_Cmd(TIM2, ENABLE);
    digitalWrite(PD4, 1);

    while(gUtsMeasuring);

    const auto distMm = uint32_t(defaultSpeedOfSound * (float(gUtsEchoUs) / 10.f)) / 2;

    Serial.printf("Counter: %d %d %dmm\n", TIM_GetCounter(TIM2), gUtsEchoUs, distMm);

    delay(100);
    gIter++;
}
