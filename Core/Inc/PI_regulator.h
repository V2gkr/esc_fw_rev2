/*
 * PI_regulator.h
 *
 *  Created on: 3 Feb 2026
 *      Author: vhrysenk
 */

#ifndef INC_PI_REGULATOR_H_
#define INC_PI_REGULATOR_H_

typedef struct {
  float Int_prev; //obecna wartość całki
  float Int_pres; //poprzednia iteracja wartości całki
  float Int_out; //dodano dla zapisywania gdzieś w pamięci wyniku całki dla testu na ujemny wynik
  unsigned char Int_block; //odpowiada tylko i wyłącznie za logikę elementu mnożenia przed częścią całkującą
} Integrator_struct;

typedef struct {
  float k; //wzmocnienie całego regulatora
  float T; //czas podwajania
  float low_lim; //niska granica saturacji
  float high_lim; //wysoka granica saturacji
  float *input_signal; //sygnał wejściowy (amperomierz lub miernik prędkości silnika), w postaci wskaznika , bo to nie dziala z normalną wartością jeśli używam stepu na wejściu (dbg) , prawdopodobnie coś się dzieje na etapie kompilacji
  float output; //dane wyjściowe po obliczeniu i sprawdzeniu na saturacje
  Integrator_struct integrator; // struktura integratora dla każdego z regulatorów
  unsigned char saturation_flag;
  unsigned short int sample_freq;
} PI_regulator_struct;

extern PI_regulator_struct CurrentReg;
extern PI_regulator_struct SpeedReg; //deklaracja zerowych struktur

float PI_regulator(PI_regulator_struct *Reg_typedef, float reference);

void PI_regulators_Init(float * input_speed,float* input_current);


#endif /* INC_PI_REGULATOR_H_ */
