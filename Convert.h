#pragma once
#include <string>

extern double rubles;
extern double dollars;
extern double euros;
extern int goods;

extern void error(std::string msg);

extern double RubToUsd(double rub);

extern double RubToEur(double rub);

extern double UsdToRub(double usd);

extern double UsdToEur(double usd);

extern double EurToRub(double eur);

extern double EurToUsd(double eur);
