#include "Convert.h"

void error(std::string msg)
{
    perror(msg.c_str());
    exit(1);
}

double RubToUsd(double rub)
{
    return rub * 0.014;
}

double RubToEur(double rub)
{
    return rub * 0.011;
}

double UsdToRub(double usd)
{
    return usd * 74.01;
}

double UsdToEur(double usd)
{
    return usd * 0.82;
}

double EurToRub(double eur)
{
    return eur * 89.89;
}

double EurToUsd(double eur)
{
    return eur * 1.21;
}
