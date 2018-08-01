/* 
 * File:   Definiciones.h
 * Author: Ale
 *
 * Created on 25 de julio de 2018, 18:09
 */

#ifndef DEFINICIONES_H
#define	DEFINICIONES_H

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef	__cplusplus
}
#endif



char temper_sup = 50; //temperaturas de encendido y apagado equipo
char temper_inf = 4;
char checksum_rom = 0; //contiene el checksum de la rom del equipo

//#define enable PIN_C3
#define	BUZER PIN_C4
//#define	RS485 PIN_C5
#define	ONPC PIN_E1


//Estados del sistema (banderas):
int1 equipo_on = 0; //Flag que indica si la unidad de registro esta encendida o apagada
int1 habilit_temp = 0; //Habilitacion por temperatura de encendio 1=habilita encendido, cero lo niega


#endif	/* DEFINICIONES_H */

