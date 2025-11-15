#pragma once

#include <vector>
#include <QWidget>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <QString>
#include <QMetaObject>
#include <QMessageBox>
#include <QMenuBar>
#include <QVBoxLayout>
#include <QFileDialog>
#include <fstream>
#include "ui_cProcessor.h"
#include "Help.h"

class cProcessor : public QWidget
{
    Q_OBJECT

public:
    cProcessor(std::vector<Core_of_Beam>* beamData, QWidget* parent = nullptr);
    ~cProcessor();
    void MenuBar();
signals:
    void sendResults(std::vector<BeamResults> results);

private slots:
    void on_pushButton_p_1_clicked();
    void onCalculationFinished();
    void on_pushButton_p_2_clicked();
    void on_pushButton_p_3_clicked();
private:
    
    QFutureWatcher<void>* m_watcher;
    Ui::cProcessorClass ui;
    std::vector<Core_of_Beam>* m_beamData;
    std::vector<BeamResults> results_force;
    // Сохраненные результаты для пост-процессинга
    std::vector<double> m_deltas;
    std::vector<double> get_rangeLen(double start_L, double stop_L, double step);
    QString getBeamParametersAtPoint(int beamNum, double coordinate);

    // Основные методы расчета
    void clear_textEdit();
    void save_calc_results();

    void calculateData();
    void displayResults(const std::vector<std::vector<double>>& A,
        const std::vector<double>& deltas);
    std::vector<std::vector<double>> createMatrix_A();
    std::vector<double> createVector_B();
    void applyBoundaryConditions(
        std::vector<std::vector<double>>& A,
        std::vector<double>& B);
    std::vector<double> findDeltas(
        std::vector<std::vector<double>>& A,
        std::vector<double>& B);

    // Пост-процессорные методы
    

    std::vector<BeamResults> calculatePostProcessing(const std::vector<double>& deltas);
    void showPostProcessingResults(const std::vector<BeamResults>& results);
    void showPostProcessingResultsAsTable(const std::vector<BeamResults>& results);

    // Вспомогательные функции расчета (только N, u, σ)
    double calculateNormalForce(double E, double A, double L,
        double delta_i, double delta_j,
        double q, double x);
    double calculateDisplacement(double delta_i, double delta_j,
        double E, double A, double L,
        double q, double x);
    double calculateStress(double N, double A);
};
