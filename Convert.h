#pragma once
#include <string>

extern double rubles;
extern double dollars;
extern double euros;

void error(std::string msg);

double RubToUsd(double rub);

double RubToEur(double rub);

double UsdToRub(double usd);

double UsdToEur(double usd);

double EurToRub(double eur);

double EurToUsd(double eur);
