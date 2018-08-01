/* 
 * File:   MedicionYControl.c
 * Author: Alejandro Cabral y Fabricio Pacha
 *
 * Created on 25 de julio de 2018, 17:38
 */

//Libreria del pic a utilizar
#include <18F452.h>
#include "Definiciones.h"

#device adc=8

#FUSES HS                    	//High speed Osc (> 4mhz for PCM/PCH) (>10mhz for PCD)
#FUSES WDT                   	//Watch Dog Timer
#FUSES WDT128                	//Watch Dog Timer uses 1:128 Postscale
#FUSES PUT                   	//Power Up Timer
#FUSES NOOSCSEN              	//Oscillator switching is disabled, main oscillator is source
#FUSES BROWNOUT              	//Reset when brownout detected
#FUSES BORV42                	//Brownout reset at 4.2V
#FUSES STVREN                	//Stack full/underflow will cause reset
#FUSES NOLVP                 	//No low voltage prgming, B3(PIC16) or B5(PIC18) used for I/O
#FUSES PROTECT               	//Code protected from reads
#FUSES NOCPB                 	//No Boot Block code protection
#FUSES NOCPD                 	//No EE protection
#FUSES NOWRT                 	//Program memory not write protected
#FUSES NOWRTC                	//Configuration registers not write protected
#FUSES NOWRTB                	//Boot block not write protected
#FUSES NOWRTD                	//Data EEPROM not write protected
#FUSES NOEBTR                	//Memory not protected from table reads
#FUSES NOEBTRB               	//Boot block not protected from table reads


#use delay(clock=20000000)                      //Frecuecnia del cristal
#use rs232(baud=57600,parity=N,xmit=PIN_C6,rcv=PIN_C7,bits=8,enable=PIN_C5) 
#ZERO_RAM


/*#include <stdio.h>
#include <stdlib.h>
 */
/*
 * 
 */
/*Definición de métodos*/
void Leer_Parametros_EEPROM(void); //lee los parametros de la eeprom y la temperatura
char analiza_checksum(void); //lee una por una las instrucciones y va calculando el checksum
char lee_temperatura(void);
char analiza_temp_vbat(void);
void pulsa_encendido_pc(void);

void main(void) {
    setup_adc_ports(NO_ANALOGS);
    setup_adc(ADC_OFF);
    setup_psp(PSP_DISABLED);
    setup_spi(SPI_SS_DISABLED);
    setup_wdt(WDT_OFF);
    setup_timer_0(RTCC_INTERNAL);
    setup_timer_1(T1_INTERNAL | T1_DIV_BY_8);
    setup_timer_2(T2_DIV_BY_16, 249, 5); //1,249,1
    setup_timer_3(T3_DISABLED | T3_DIV_BY_1);
    setup_ccp1(CCP_COMPARE_RESET_TIMER); //
    CCP_1_LOW = 0x28; //valores de comparacion del timer1 para generar una interrup cada 100 ms
    CCP_1_HIGH = 0xf4; // low=60 high=c3
    //enable_interrupts(int_timer2);
    enable_interrupts(INT_RDA);
    //enable_interrupts(int_tbe);
    enable_interrupts(INT_CCP1);
    enable_interrupts(INT_TIMER2);
    enable_interrupts(GLOBAL);


    output_a(0);
    output_c(0);
    output_d(0);
    output_e(0);

#use standard_io(d)

    set_tris_a(0x23);
    set_tris_b(0xff); //todas como entradas
    set_tris_c(0x80);
    set_tris_d(0xff); //sensor del semaforo
    set_tris_e(0x00);



    //    return (EXIT_SUCCESS);
    Leer_Parametros_EEPROM();
    if (analiza_checksum() != 0) {
        //sigue el programa

    }
}

void Leer_Parametros_EEPROM(void) {

    temper_sup = read_eeprom(9); //temperaturas de encendido y apagado equipo
    temper_inf = read_eeprom(10);
    checksum_rom = read_eeprom(13);

}

char analiza_checksum() {

    long int l_digit_long;
    char valor;

    valor = 0;
    l_digit_long = 0;
    checksum_rom = read_eeprom(13);
    printf("Checksum rom cpu: %2X\r\n", checksum_rom);

    for (l_digit_long = 1960; l_digit_long < 8191; l_digit_long++) //lee 8k de rom
    {
        valor ^= read_program_eeprom(l_digit_long); //valor_long or exclusive con el anterior, calcula checksum
    }
    valor = valor^0x4d;
    ////// atento PACHA. programar el CRC-8

    printf("Checksum calculado cpu: %2X\r\n", valor);

    valor = valor^checksum_rom; //el xor del checsum 

    if (valor != 0) {
        printf("Checksum incorrecto CPU DETENIDA\r\n");
        reset_cpu();
    }
    return valor;
}

void analiza_temp_vbat(void) {

    lee_temperatura(); //devuelve habilit_temp=1, temperatura en rango, hablit_temp=0 temperatura mal


    if (equipo_on == 0 && (habilit_temp == 0)) //la pc, esta apagada, debe hacer un ruido para indicar condicion de temperatura
    {
        output_bit(BUZER, 1);
        delay_ms(100);
        output_bit(BUZER, 0);
        delay_ms(300);
    }

    if (equipo_on == 0 && habilit_temp == 1) // PC apagada, temperatura en rango, debe encender pc, se resetea up
    {
        reset_cpu(); //resetea el up, para encender la PC
        equipo_on = 1;
    }

    if (equipo_on == 1 && (habilit_temp == 0)) //PC encendida, tension u temperatura u ambas fuera de rango, se debe apagar PC y resetear up
    {
        pulsa_encendido_pc(); //presiona encendido pc, para apagarla
        equipo_on = 0; //indica que la PC esta apagada
    }

} //final del void

void lee_temperatura(void) {
    char temperatura;

    setup_port_a(ALL_ANALOG);
    setup_adc(ADC_CLOCK_DIV_64);
    habilit_temp = 0; //pone en cero la habilitacion para el encendido del equipo


    set_adc_channel(0);
    delay_us(20);
    temperatura = read_adc(); //al valor lelido hay que restarle el de la referencia

    if (temperatura <= temper_sup && temperatura >= temper_inf) {
        habilit_temp = 1; //habilita para el encendido del equipo por temperatura en rango
    }
    //	habilit_temp=1;		//colocado para pasar interferencias sen el 2013

    Setup_adc_ports(NO_ANALOGS);



} //final del void

void pulsa_encendido_pc(void) {

    output_bit(ONPC, 1); //APAGA LA CPU
    delay_ms(1000); //espera 1 segundo con el pulsador activado
    output_bit(ONPC, 0); //Saca condicion APAGADO

}


void arranca_cpu(void) {
    int rtcc_on_off;
    rtcc_on_off = 0;
    flag_f5 = 0;
    while (flag_f5 == 0 && rtcc_on_off <= 100) //espera 10 segundos que responda la pc
    {

        putc(0xf5); //envia f5 por el puerto rs232
        delay_ms(200);
    }

    if (flag_f5 == 0) //La CPU se encuentra o colgada o apagada		
    {

        h_digit = 5;
        retardo_segundos(h_digit); //retardo para que se estabilicen las tensiones
        lee_temperatura();

        if (habilit_temp == 1) // si la temperatura  esta en rango el equipo  arranca y la hora esta ok
        {
            printf("\r\nEncendiendo quipo = %2u\n\r", temperatura);

            pulsa_encendido_pc(); //enciende pc	
            equipo_on = 1; //variable que indica equipo encendido

            rtcc_on_off = 0; //pone en cero el rtcc, que se incrementa cada 100 ms
            valor = 1;

            while (valor) {
                while (flag_f5 == 0 && rtcc_on_off <= 3600) // si no se recibio f5 o paso 6 minutos
                {

                    putc(0xf5), putc(temperatura), putc(v_nominal); //envia f5 por el puerto rs232
                    delay_ms(200); //retardo de 100 ms
                } //final del while

                if (rtcc_on_off >= 3600 && flag_f5 == 0) //pasaron 5 minutos y no se recibio respuesta
                {
                    output_bit(BUZER, 1);
                    output_bit(ONPC, 1); //resetea PC			

                    h_digit = 5;
                    retardo_segundos(h_digit);

                    output_bit(ONPC, 0); //pone en condicion 0
                    output_bit(BUZER, 0);

                    h_digit = 180; //retardo de 3 minuto 0 segundos
                    retardo_segundos(h_digit);

                    reset_cpu();
                }

                if (flag_f5 == 1) //se recibio f5 como rta
                {
                    valor = 0;
                    flag_f5 = 0;
                    equipo_on = 1;
                }
            } //final while valor
            //			envia_la_fecha();		//transmite la fecha y hora al cpu para sincronizar

        }
        else if (habilit_temp == 0) {

            if (habilit_temp == 0)
                printf("\r\nDetenido x temper.=%2u\n\r", temperatura);

            output_bit(BUZER, 1);
            delay_ms(700);
            output_bit(BUZER, 0);
            reset_cpu();
        }
    } else if (flag_f5 == 1) {
        equipo_on = 1; //la CPU, ya se encuentra encendida anda todo ok
        estado_deteccion_1 = 0;
        estado_deteccion_2 = 0;
        estado_deteccion_3 = 0;
        //	lee_temperatura();			//mide la tension y genera el estado de los flag
        analiza_temp_vbat();
    
} //FINAL DE LA FUNCION