#pragma once
#include <QtGlobal>
#include <iostream>
#include <vector>
constexpr qreal SCALE = 150.0;

qreal metersToQreal(double meters);

double qrealToMeters(qreal value);

struct Joint_info {
    int fixedSupport; // 1 = exist, 0 = no exist
    double lineLoad_q;
    double force_f;
};
struct Core_of_Beam {
    Joint_info Joint_left;
    Joint_info Joint_right;

    double len_L;
    double selectArea_A;
    double maxVoltage;
    double mod_elasticity;
};


struct BeamResults {
    int beamNum;
    double E;              // Модуль упругости
    double A;              // Площадь сечения
    double L;              // Длина
    double q;              // Распределенная нагрузка
    double delta_left;     // Перемещение левого узла
    double delta_right;    // Перемещение правого узла


    std::vector<double> N_x; // Продольные силы N(x)
    std::vector<double> U_x; // Перемещения u(x)
    std::vector<double> sigma; // Напряжения σ(x)
};