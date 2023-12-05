#include <Arduino.h>

static void write_header(uint8_t cmd, uint8_t data_len) {
    uint8_t buf[] = {
        0xFF,
        cmd,
        data_len
    };
    Serial.write((char*)buf, sizeof(buf));
}

template<typename T>
static void write_scalar(const T data) {
    Serial.write((char*)&data, sizeof(T));
}


static int gIter = 0;

void setup() {
    Serial.begin(115200);
    
    // !!!
    ///
    // Kanály A0 a A1 jsou na pinech PA2 a PA1, na které je na dev kitech připojený krystal, nejdou tedy použít.
    //
    // Kanály A5, A6 a A7 se zdají být na pinech, které používá UART, nejdou tedy asi použít zároveň s ním.
    //
    // A7 je UCK pin, a jde použít když uděláme manuální inicializaci ADC, pouze analogRead zdá se vypne UART, když se z A7 pokusíme číst.
    //
    // !!!

    // Varianta přes manuální nastavení periferie.
    // Nastaví ADC1 do režimu s Injected sequencer,
    // kdy zvládne převést dva kanály v "sekvenci"
#if 0
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div8);

    ADC_InitTypeDef  ADC_InitStructure = {0};
    ADC_DeInit(ADC1);
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = ENABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 2;
    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_Calibration_Vol(ADC1, ADC_CALVOL_50PERCENT);

    ADC_InjectedSequencerLengthConfig(ADC1, 2);
    ADC_InjectedChannelConfig(ADC1, ADC_Channel_2, 1, ADC_SampleTime_9Cycles); // ADC_Channel_2 == PC4
    ADC_InjectedChannelConfig(ADC1, ADC_Channel_7, 2, ADC_SampleTime_9Cycles); // ADC_Channel_2 == PD2
    ADC_AutoInjectedConvCmd(ADC1, ENABLE);

    ADC_Cmd(ADC1, ENABLE);

    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
#endif

}

void loop() {
    write_header(0x00, 4);
    write_scalar(millis());


#if 1
    // Arduino vyčítání, funguje dobře, ale je pomalé - ADC převodní se celý zapne,
    // přečte a vypne při každém analogRead volání
    write_header(0x01, 2*4);
    //write_scalar(analogRead(A0)); // -> pin PA2 -- krystal
    //write_scalar(analogRead(A1)); // -> pin PA1 -- krystal
    write_scalar(analogRead(A2)); // -> pin PC4
    write_scalar(analogRead(A3)); // -> pin PD2
    //write_scalar(analogRead(A4)); // -> pin PD3
    //write_scalar(analogRead(A5)); // -> pin PD5 -- UART
    //write_scalar(analogRead(A6)); // -> pin PD6 -- UART
    //write_scalar(analogRead(A7)); // -> pin PD4 -- UART
#else
    // Varianta vyčítání při použití manuální inicializace a sekvenceru
    ADC_ClearFlag(ADC1, ADC_FLAG_JEOC); // JEOC == end of injected sequence conversion

    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    ADC_SoftwareStartInjectedConvCmd(ADC1, ENABLE);
    while(ADC_GetFlagStatus(ADC1, ADC_FLAG_JEOC) == RESET); // čekání na konec konverze

    uint32_t value1 = ADC_GetInjectedConversionValue(ADC1, ADC_InjectedChannel_1);
    uint32_t value2 = ADC_GetInjectedConversionValue(ADC1, ADC_InjectedChannel_2);

    write_header(0x01, 4*2);
    write_scalar(value1);
    write_scalar(value2);
#endif


    delay(100);
    //digitalWrite(PD4, gIter%2);
    gIter++;
}
