/*
 * PI_regulator.c
 *
 *  Created on: 3 Feb 2026
 *      Author: vhrysenk
 */

#include "PI_regulator.h"

#define current_out_margin 0.3; //granicy wyjscia regulatora prądu
#define speed_out_margin 1.5; //granicy wyjscia regulatora prędkości
#define sample_time 1/1000; //jedyna +- normalna możliwość importu parametru który nie jest częścią struktur a musi być użyty w funkcji

float *speed_ref; //referencja zadanej prędkości ( wartość nominalna domyślnie podczas inicjalizacji)
float speed_out = 0; //wyjście regulatora prędkości
float current_out = 0; //wyjście regulatora prądu


#define CURRENT_REG_KI  0.002
#define CURRENT_REG_KP  10

#define SPEED_REG_KI    0.005
#define SPEED_REG_KP    0.01
PI_regulator_struct CurrentReg = { 0 };
PI_regulator_struct SpeedReg = { 0 }; //deklaracja zerowych struktur

/** @brief funkcja obliczenia struktury regulatora */
float PI_regulator(PI_regulator_struct *Reg_typedef, float reference) {
//
  float input_ki = (reference - (*Reg_typedef->input_signal)) * Reg_typedef->k;
  if (!Reg_typedef->integrator.Int_block) {
    Reg_typedef->integrator.Int_pres += Reg_typedef->integrator.Int_prev;
  } //w przypadku blokowania całki - blokowane są dalsze obliczenia

  Reg_typedef->integrator.Int_prev = input_ki / Reg_typedef->sample_freq; //aktualizacja wejścia całki
  Reg_typedef->integrator.Int_out = Reg_typedef->integrator.Int_pres/ Reg_typedef->T; //obliczenia całki

  Reg_typedef->output = input_ki + Reg_typedef->integrator.Int_out; //wyjście regulatora

  //negative test
  if ((input_ki * Reg_typedef->integrator.Int_out) <= 0) {
    Reg_typedef->integrator.Int_block = 0; //jesli wynik mnozenia ujemny - rozblokuje calke - bo bedzie to logiczna 1 w schemacie blokowym
  }

  //limit test
  if (Reg_typedef->output > Reg_typedef->high_lim) {
    Reg_typedef->integrator.Int_block = 1;
    return Reg_typedef->high_lim;
  } else if (Reg_typedef->output < Reg_typedef->low_lim) {
    Reg_typedef->integrator.Int_block = 1;
    return Reg_typedef->low_lim;
  } else {
    Reg_typedef->integrator.Int_block = 0;
    return Reg_typedef->output;
  }

}

void PI_regulators_Init(float * input_speed,float* input_current){
  //speed_ref=(double*)&InputSignal(0,2);//ParamRealData(5,0);

  //regulator prądu
  CurrentReg.k=CURRENT_REG_KI;
  CurrentReg.T=CURRENT_REG_KI/CURRENT_REG_KP;
  CurrentReg.low_lim=-current_out_margin;
  CurrentReg.high_lim=current_out_margin;
  CurrentReg.input_signal=input_current;
  //konfiguracja calkującej części regulatora
  CurrentReg.integrator.Int_prev=0;
  CurrentReg.integrator.Int_pres=0;
  CurrentReg.integrator.Int_block=0;
  CurrentReg.sample_freq=20000;

  //regulator prędkości
  SpeedReg.k=SPEED_REG_KI;
  SpeedReg.T=SPEED_REG_KI/SPEED_REG_KP;
  SpeedReg.low_lim=-4;
  SpeedReg.high_lim=4;
  SpeedReg.input_signal=input_speed;
  //konfiguracja calkującej części regulatora
  SpeedReg.integrator.Int_prev=0;
  SpeedReg.integrator.Int_pres=0;
  SpeedReg.integrator.Int_block=0;
  SpeedReg.sample_freq=100;
//  speed_out=PI_regulator(&SpeedReg,*speed_ref);
//  current_out=PI_regulator(&CurrentReg,speed_out);
}
