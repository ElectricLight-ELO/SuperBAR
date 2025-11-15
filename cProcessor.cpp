#include "cProcessor.h"
#include <QFuture>
#include <QtConcurrent>
#include <cmath>

cProcessor::cProcessor(std::vector<Core_of_Beam>* beamData, QWidget* parent)
    : QWidget(parent), m_beamData(beamData), m_watcher(nullptr)
{
    ui.setupUi(this);
    setFixedSize(size());
    MenuBar();

    ui.textEdit_p_2->setText("30");
    
    //QFont font;
    //font.setStyleHint(QFont::TypeWriter);  // Qt автоматически выберет моноширинный шрифт
    //font.setPointSize(10);
    //ui.textEdit_p_1->setFont(font);

    QFont font("Consolas", 10);
    ui.textEdit_p_1->setFont(font);
}

cProcessor::~cProcessor()
{
    if (m_watcher) {
        m_watcher->waitForFinished();
    }
}

void cProcessor::MenuBar()
{
    QMenuBar* menuBar = new QMenuBar(this);

    QMenu* fileMenu = menuBar->addMenu("Настройки");
  //  fileMenu->addAction("Сохранить");
    QAction* save_action = fileMenu->addAction("Сохранить результаты расчета");
    QAction* clear_action = fileMenu->addAction("Очистить");

    /*QMenu* helpMenu = menuBar->addMenu("Справка");
    helpMenu->addAction("О программе");*/

    // Добавление menuBar в layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setMenuBar(menuBar);  // Для QVBoxLayout
    setLayout(mainLayout);

    connect(clear_action, &QAction::triggered, this, &cProcessor::clear_textEdit);
    connect(save_action, &QAction::triggered, this, &cProcessor::save_calc_results);
}

// ==================== СЛОТЫ ====================

void cProcessor::on_pushButton_p_1_clicked()
{
    ui.pushButton_p_1->setEnabled(false);
    m_watcher = new QFutureWatcher<void>(this);
    connect(m_watcher, &QFutureWatcher<void>::finished,
        this, &cProcessor::onCalculationFinished);

    QFuture<void> future = QtConcurrent::run([this]() {
        calculateData();
        });

    m_watcher->setFuture(future);
    ui.textEdit_p_1->append("Расчет запущен...\n");
}

void cProcessor::onCalculationFinished()
{
    ui.textEdit_p_1->append("Расчет завершен!\n\n");
    if (m_watcher) {
        m_watcher->deleteLater();
        m_watcher = nullptr;
    }
    ui.pushButton_p_1->setEnabled(true);
}

void cProcessor::on_pushButton_p_2_clicked()
{
    emit sendResults(results_force);

    this->hide();
}

void cProcessor::on_pushButton_p_3_clicked()
{
    QString beamNumText = ui.textEdit_p_3->toPlainText().trimmed();
    bool okBeam;
    int beamNum = beamNumText.toInt(&okBeam);

    // Считываем координату
    QString coordinateText = ui.textEdit_p_4->toPlainText().trimmed();
    bool okCoord;
    double coordinate = coordinateText.toDouble(&okCoord);

    // Проверка корректности ввода номера стержня
    if (!okBeam || beamNumText.isEmpty()) {
        QMessageBox::warning(this, "Ошибка ввода",
            "Введите корректный номер стержня (целое число)");
        return;
    }

    // Проверка корректности ввода координаты
    if (!okCoord || coordinateText.isEmpty()) {
        QMessageBox::warning(this, "Ошибка ввода",
            "Введите корректное значение координаты (число)");
        return;
    }

    // Проверка наличия результатов расчета
    if (results_force.empty()) {
        QMessageBox::warning(this, "Нет данных",
            "Сначала выполните расчет (нажмите кнопку 'Рассчитать')");
        return;
    }

    // Проверка диапазона номера стержня
    if (beamNum < 1 || beamNum > static_cast<int>(results_force.size())) {
        QMessageBox::warning(this, "Ошибка",
            QString("Номер стержня должен быть от 1 до %1").arg(results_force.size()));
        return;
    }

    // Получаем результаты для указанного стержня (индекс = beamNum - 1)
    const BeamResults& res = results_force[beamNum - 1];

    // Проверка диапазона координаты (от 0 до L)
    if (coordinate < 0.0 || coordinate > res.L) {
        QMessageBox::warning(this, "Ошибка",
            QString("Координата должна быть в диапазоне от 0 до %1 м (длина стержня)")
            .arg(res.L, 0, 'f', 4));
        return;
    }

    // Получаем параметры в указанной точке
    QString result = getBeamParametersAtPoint(beamNum, coordinate);

    // Выводим результат
    ui.textEdit_p_1->append(result);
}

// ==================== ОСНОВНОЙ РАСЧЕТ ====================

std::vector<double> cProcessor::get_rangeLen(double start_L, double stop_L, double step)
{
    std::vector<double> range;

    if (step <= 0.0) {
        throw std::invalid_argument("Step must be positive");
    }

    if (start_L > stop_L) {
        throw std::invalid_argument("start_L must be less than or equal to stop_L");
    }

    // Генерация диапазона
    double current = start_L;
    while (current <= stop_L) {
        range.push_back(current);
        current += step;
    }

    // Гарантируем, что последняя точка будет ровно stop_L
    if (range.empty() || std::abs(range.back() - stop_L) > 1e-9) {
        range.push_back(stop_L);
    }

    return range;

}

QString cProcessor::getBeamParametersAtPoint(int beamNum, double coordinate)
{
    const BeamResults& res = results_force[beamNum - 1];

    // Вычисляем параметры в заданной точке
    double Nx = calculateNormalForce(res.E, res.A, res.L,
        res.delta_left, res.delta_right,
        res.q, coordinate);

    double Ux = calculateDisplacement(res.delta_left, res.delta_right,
        res.E, res.A, res.L,
        res.q, coordinate);

    double sigma = calculateStress(Nx, res.A);

    // Формируем выходную строку
    QString output;
    output += QString("=").repeated(60) + "\n";
    output += QString("Параметры стержня №%1 в точке x = %2 м\n")
        .arg(beamNum)
        .arg(coordinate, 0, 'f', 4);
    output += QString("=").repeated(60) + "\n";

    output += QString("Длина стержня L = %1 м\n").arg(res.L, 0, 'f', 4);
    output += QString("Площадь сечения A = %1 м²\n").arg(res.A, 0, 'e', 4);
    output += QString("Модуль упругости E = %1 Па\n").arg(res.E, 0, 'e', 2);
    output += QString("Распред. нагрузка q = %1 Н/м\n").arg(res.q, 0, 'f', 2);
    output += "\n";

    output += QString("Продольная сила N(x) = %1 Н\n").arg(Nx, 0, 'f', 2);
    output += QString("Перемещение u(x) = %1 м\n").arg(Ux, 0, 'e', 6);
    output += QString("Напряжение σ(x) = %1 Па\n").arg(sigma, 0, 'e', 4);

    // Проверка прочности
    const Core_of_Beam& beam = (*m_beamData)[beamNum - 1];
    double maxVoltage = beam.maxVoltage;
    output += "\n";
    output += QString("Предельное напряжение σ_max = %1 Па\n").arg(maxVoltage, 0, 'e', 2);

    if (std::abs(sigma) <= maxVoltage) {
        output += QString("✓ Прочность обеспечена (|σ| ≤ σ_max)\n");
    }
    else {
        output += QString("✗ ПРОЧНОСТЬ НЕ ОБЕСПЕЧЕНА! (|σ| > σ_max)\n");
    }

    output += QString("=").repeated(60) + "\n\n";

    return output;

}

void cProcessor::clear_textEdit()
{
    ui.textEdit_p_1->clear();
}

void cProcessor::save_calc_results()
{
    QString fileName;
    fileName = QFileDialog::getSaveFileName(
        this,
        tr("Сохранить файл"),
        QDir::currentPath() + "/results.txt",
        tr("TXT файлы (*.txt);;Все файлы (*.*)")
    );

    std::string out_result = ui.textEdit_p_1->toPlainText().toStdString();

    std::ofstream file_result(fileName.toStdString());
    if (file_result.is_open()) {

        file_result << out_result;

        file_result.close();
    }
}

void cProcessor::calculateData()
{
    try {
        // Параллельное создание матрицы A и вектора B
        QFuture<std::vector<std::vector<double>>> futureA =
            QtConcurrent::run([this]() {
            return this->createMatrix_A();
                });

        QFuture<std::vector<double>> futureB =
            QtConcurrent::run([this]() {
            return this->createVector_B();
                });

        futureA.waitForFinished();
        futureB.waitForFinished();

        auto A = futureA.result();
        auto B = futureB.result();

        // Применение граничных условий
        applyBoundaryConditions(A, B);

        auto deltas = findDeltas(A, B);
        m_deltas = deltas; // Сохраняем для пост-процессинга
        displayResults(A, deltas);

        // Пост-процессорные расчеты
        auto postResults = calculatePostProcessing(deltas);
	//	showPostProcessingResults(postResults);
        showPostProcessingResultsAsTable(postResults);

        /*output += QString("\n Проверка макс. напряжений σ(x): \n");
        for (size_t j = 0; j < res.sigma.size(); j++) {
            const Core_of_Beam& beam = (*m_beamData)[i];
            double max_voltage = beam.maxVoltage;

            if (std::abs(res.sigma[i]) > max_voltage) {

            }
        }*/

        results_force = postResults;
    }
    catch (const std::exception& e) {
        QString errorMsg = QString("Ошибка расчета: %1").arg(e.what());
        QMetaObject::invokeMethod(this, [this, errorMsg]() {
            ui.textEdit_p_1->append(errorMsg);
            }, Qt::QueuedConnection);
    }
}

void cProcessor::displayResults(const std::vector<std::vector<double>>& A,
    const std::vector<double>& deltas)
{
    QString output;
    output += QString(60, '=') + "\n";
    output += "РЕЗУЛЬТАТЫ РАСЧЕТА ПРОЦЕССОРА\n";
    output += QString(60, '=') + "\n\n";

    /*output += "Матрица жесткости A:\n";
    
    for (size_t i = 0; i < A.size(); i++) {
        for (size_t j = 0; j < A[i].size(); j++) {
            output += QString("%1 ").arg(A[i][j], 9, 'f', 4);
        }
        output += "\n";
    }*/

    output += "\nУзловые перемещения Δ:\n";
    for (size_t i = 0; i < deltas.size(); i++) {
        output += QString("  Δ[%1] = %2\n").arg(i).arg(deltas[i]);
    }

    QMetaObject::invokeMethod(this, [this, output]() {
        ui.textEdit_p_1->append(output);
        }, Qt::QueuedConnection);
}

std::vector<std::vector<double>> cProcessor::createMatrix_A()
{
    if (!m_beamData || m_beamData->empty()) {
        throw std::runtime_error("No beam data available");
    }

    int num_beams = m_beamData->size();
    int num_dof = num_beams + 1;

    std::vector<std::vector<double>> A(num_dof, std::vector<double>(num_dof, 0.0));

    for (int i = 0; i < num_beams; ++i) {
        const Core_of_Beam& beam = (*m_beamData)[i];
        double E = beam.mod_elasticity;
        double A_area = beam.selectArea_A;
        double L = beam.len_L;

        if (std::abs(L) < 1e-9) {
            throw std::runtime_error("Beam has near-zero length");
        }

        // Локальная жесткость: k = EA/L
        double k_local = (E * A_area) / L;

        // Добавление вклада в глобальную матрицу
        A[i][i] += k_local;
        A[i][i + 1] += -k_local;
        A[i + 1][i] += -k_local;
        A[i + 1][i + 1] += k_local;
    }

    return A;
}


std::vector<double> cProcessor::createVector_B()
{
    int num_beams = m_beamData->size();
    int num_dof = num_beams + 1;
    std::vector<double> B(num_dof, 0.0);

    //  Распределенные нагрузки
    for (int i = 0; i < num_beams; ++i) {
        const Core_of_Beam& beam = (*m_beamData)[i];
        double L = beam.len_L;
        double q = beam.Joint_left.lineLoad_q;

        // Эквивалентные узловые силы: q*L/2 на каждый узел
        double q_left = q * L / 2.0;
        double q_right = q * L / 2.0;

        B[i] += q_left;
        B[i + 1] += q_right;
    }

    //  Сосредоточенные силы (избегаем дублирования)
    B[0] += (*m_beamData)[0].Joint_left.force_f;

    for (int i = 0; i < num_beams - 1; ++i) {
        B[i + 1] += (*m_beamData)[i].Joint_right.force_f;
    }

    B[num_dof - 1] += (*m_beamData)[num_beams - 1].Joint_right.force_f;

    return B;
}


void cProcessor::applyBoundaryConditions(
    std::vector<std::vector<double>>& A,
    std::vector<double>& B)
{
    int num_dof = A.size();

    for (int i = 0; i < m_beamData->size(); ++i) {
        const Core_of_Beam& beam = (*m_beamData)[i];

        // Левое закрепление (первая балка)
        if (i == 0 && beam.Joint_left.fixedSupport == 1) {
            for (int j = 0; j < num_dof; ++j) {
                A[0][j] = 0.0;
                A[j][0] = 0.0;
            }
            A[0][0] = 1.0;
            B[0] = 0.0;
        }

        // Правое закрепление (последняя балка)
        if (i == m_beamData->size() - 1 && beam.Joint_right.fixedSupport == 1) {
            int last = num_dof - 1;
            for (int j = 0; j < num_dof; ++j) {
                A[last][j] = 0.0;
                A[j][last] = 0.0;
            }
            A[last][last] = 1.0;
            B[last] = 0.0;
        }
    }
}

std::vector<double> cProcessor::findDeltas(
    std::vector<std::vector<double>>& A,
    std::vector<double>& B)
{
    int n = B.size();
    if (A.size() != n || A[0].size() != n) {
        throw std::runtime_error("Matrix A must be square");
    }

    // Создание расширенной матрицы [A|B]
    std::vector<std::vector<double>> augmented(n, std::vector<double>(n + 1));
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            augmented[i][j] = A[i][j];
        }
        augmented[i][n] = B[i];
    }

    // Прямой ход метода Гаусса с выбором главного элемента
    for (int k = 0; k < n; ++k) {
        // Поиск строки с максимальным элементом
        int pivotRow = k;
        double maxPivot = std::abs(augmented[k][k]);
        for (int i = k + 1; i < n; ++i) {
            if (std::abs(augmented[i][k]) > maxPivot) {
                maxPivot = std::abs(augmented[i][k]);
                pivotRow = i;
            }
        }

        // Проверка вырожденности
        if (std::abs(maxPivot) < 1e-12) {
            throw std::runtime_error("Matrix is singular");
        }

        // Перестановка строк
        if (pivotRow != k) {
            std::swap(augmented[k], augmented[pivotRow]);
        }

        // Исключение переменной
        for (int i = k + 1; i < n; ++i) {
            double factor = augmented[i][k] / augmented[k][k];
            for (int j = k; j <= n; ++j) {
                augmented[i][j] -= factor * augmented[k][j];
            }
        }
    }

    // Обратный ход
    std::vector<double> delta(n);
    for (int i = n - 1; i >= 0; --i) {
        double sum = augmented[i][n];
        for (int j = i + 1; j < n; ++j) {
            sum -= augmented[i][j] * delta[j];
        }
        delta[i] = sum / augmented[i][i];
    }

    return delta;
}

//  ПОСТ-процессор

std::vector<BeamResults> cProcessor::calculatePostProcessing(
    const std::vector<double>& deltas)
{
    std::vector<BeamResults> results;
    int num_beams = m_beamData->size();

    for (int i = 0; i < num_beams; ++i) {
        const Core_of_Beam& beam = (*m_beamData)[i];
        BeamResults res;

        // Исходные данные
        res.beamNum = i + 1;
        res.E = beam.mod_elasticity;
        res.A = beam.selectArea_A;
        res.L = beam.len_L;
        res.q = beam.Joint_left.lineLoad_q;
        res.delta_left = deltas[i];
        res.delta_right = deltas[i + 1];

        double step = ui.textEdit_p_2->toPlainText().toDouble();
		std::vector<double> range = get_rangeLen(0.0, res.L, res.L / step);

        for(double x : range) {
            // Продольные силы N(x)
            double N_x = calculateNormalForce(res.E, res.A, res.L,
                res.delta_left, res.delta_right,
                res.q, x);
            res.N_x.push_back(N_x);
            // Перемещения u(x)
            double U_x = calculateDisplacement(res.delta_left, res.delta_right,
                res.E, res.A, res.L,
                res.q, x);
            res.U_x.push_back(U_x);
            // Напряжения σ(x)
            double sigma_x = calculateStress(N_x, res.A);
            res.sigma.push_back(sigma_x);
		}


        results.push_back(res);
    }

    return results;
}

void cProcessor::showPostProcessingResults(const std::vector<BeamResults>& results)
{
    QString output;
    output += QString(60, '=') + "\n";
    output += "РЕЗУЛЬТАТЫ РАСЧЕТА ПОСТ-ПРОЦЕССОРА\n";
    output += QString(60, '=') + "\n\n";

    for (int i = 0; i < results.size(); ++i) {
        const BeamResults& res = results[i];
        output += QString(30, '-') + "\n";
        output += QString("Балка №%1:\n").arg(res.beamNum);
        output += QString(30, '-') + "\n";

        output += QString(" Продольные силы N(x): \n");
        output += QString("N(0)=%1  ").arg(res.N_x[0]);
        output += QString("N(%2L)=%1\n").arg(res.N_x[res.N_x.size()-1]).arg(res.L);

        output += QString("\n Перемещения u(x): \n");
        for(size_t j = 0; j < res.U_x.size(); j++) {
            if(j % 5 == 0 || j == 0 || j == res.U_x.size()-1 ) {
                output += QString("u(%1*L)=%2 \n").arg(j * (res.L / (res.U_x.size() - 1))).arg(res.U_x[j]);
			}
		}
        output += QString("\n Напряжения σ(x): \n");
        for (size_t j = 0; j < res.sigma.size(); j++) {
            if (j % 5 == 0 || j == 0 || j == res.sigma.size() - 1) {
                output += QString("σ(%1*L)=%2 \n").arg(j * (res.L / (res.sigma.size() - 1))).arg(res.sigma[j]);
            }
        }

        output += QString("\n -------- Проверка макс. напряжений σ(x) на стержне №%1: --------\n").arg(i+1);
        const Core_of_Beam& beam = (*m_beamData)[i];
        double max_voltage = beam.maxVoltage;
        output += QString("Макс. напряжение σ=%1 \n").arg(max_voltage);
        for (size_t j = 0; j < res.sigma.size(); j++) {
            

            if (std::abs(res.sigma[j]) > max_voltage) {
                output += QString("Сломается в σ(%1*L)=%2 \n").arg(j * (res.L / (res.sigma.size() - 1))).arg(res.sigma[j]);
                break;
            }
            else {
                if (j % 5 == 0) {
                    output += QString("Всё хорошо σ(%1*L)=%2 \n").arg(j * (res.L / (res.sigma.size() - 1))).arg(res.sigma[j]);
                }
            }
        }

		output += QString("\n\n");

    }
    QMetaObject::invokeMethod(this, [this, output]() {
        ui.textEdit_p_1->append(output);
        }, Qt::QueuedConnection); 
}

void cProcessor::showPostProcessingResultsAsTable(const std::vector<BeamResults>& results)
{
    

    bool strengthOk_full = true;
    QString output;

    
    bool showAllValues = ui.checkBox_p1->isChecked();
    int step = showAllValues ? 1 : 5;

    output += QString(60, '=') + "\n";
    output += "         РЕЗУЛЬТАТЫ РАСЧЕТА СТЕРЖНЕВОЙ КОНСТРУКЦИИ\n";
    output += QString(60, '=') + "\n\n";

    for (int i = 0; i < results.size(); ++i) {
        const BeamResults& res = results[i];

        
        output += QString(60, '-') + "\n";
        output += QString("  СТЕРЖЕНЬ №%1\n").arg(res.beamNum);
        output += QString(60, '-') + "\n";

        
        output += "┌────────────────────────────────────────┬──────────────────┐\n";
        output += "│ Параметр                               │ Значение         │\n";
        output += "├────────────────────────────────────────┼──────────────────┤\n";
        output += QString("│ Модуль упругости E, Па                 │ %1 │\n")
            .arg(res.E, 16, 'e', 2, ' ');
        output += QString("│ Площадь сечения A, м²                  │ %1 │\n")
            .arg(res.A, 16, 'e', 4, ' ');
        output += QString("│ Длина L, м                             │ %1 │\n")
            .arg(res.L, 16, 'f', 4, ' ');
        output += QString("│ Распред. нагрузка q, Н/м               │ %1 │\n")
            .arg(res.q, 16, 'f', 2, ' ');
        output += QString("│ Перемещение левого узла, м             │ %1 │\n")
            .arg(res.delta_left, 16, 'e', 6, ' ');
        output += QString("│ Перемещение правого узла, м            │ %1 │\n")
            .arg(res.delta_right, 16, 'e', 6, ' ');
        output += "└────────────────────────────────────────┴──────────────────┘\n";

        /*output += "┌───────────────────────────────────────────────────────\n";
        output += "│ Параметр                                  │ Значение         \n";
        output += "|────────────────────────────────────────-──────────────\n";
        output += QString("│ Модуль упругости E, Па                 │ %1  \n")
            .arg(res.E, 16, 'e', 2, ' ');
        output += QString("│ Площадь сечения A, м²                  │ %1  \n")
            .arg(res.A, 16, 'e', 4, ' ');
        output += QString("│ Длина L, м                                       │ %1  \n")
            .arg(res.L);
        output += "|────────────────────────────────-─────────────────────────|\n\n";*/

        //  N(x)
        output += "  ПРОДОЛЬНЫЕ СИЛЫ N(x), Н:\n";
        output += "┌───────────────────┬───────────────────┐\n";
        output += "│ Координата x, м   │ N(x), Н           │\n";
        output += "|───────────────────┼───────────────────|\n";

        for (size_t j = 0; j < res.N_x.size(); ++j) {
            
            if (j % step == 0 || j == 0 || j == res.N_x.size() - 1) {
                double x = j * res.L / (res.N_x.size() - 1);
                output += QString("│ %1 │ %2 │\n")
                    .arg(x, 17, 'f', 4)
                    .arg(res.N_x[j], 17, 'f', 2);
            }
        }
        output += "|───────────────────-───────────────────|\n\n";

        // u(x)
        output += "  ПЕРЕМЕЩЕНИЯ u(x), м:\n";
        output += "┌───────────────────┬───────────────────┐\n";
        output += "│ Координата x, м   │ u(x), м           │\n";
        output += "|───────────────────┼───────────────────|\n";

        for (size_t j = 0; j < res.U_x.size(); ++j) {
            if (j % step == 0 || j == 0 || j == res.U_x.size() - 1) {
                double x = j * res.L / (res.U_x.size() - 1);
                output += QString("│ %1 │ %2 │\n")
                    .arg(x, 17, 'f', 4)
                    .arg(res.U_x[j], 17, 'e', 6);
            }
        }
        output += "|───────────────────-───────────────────|\n\n";

        // σ(x)
        const Core_of_Beam& beam = (*m_beamData)[i];
        double max_voltage = beam.maxVoltage;

        output += QString("  НАПРЯЖЕНИЯ σ(x), Па (Допустимое: %1 Па):\n")
            .arg(max_voltage, 0, 'e', 2);
        output += "┌───────────────────┬───────────────────┬──────────┐\n";
        output += "│ Координата x, м   | σ(x), Па          │ Статус   │\n";
        output += "|───────────────────┼───────────────────┼──────────|\n";

        bool strengthOk = true;
        for (size_t j = 0; j < res.sigma.size(); ++j) {
            if (j % step == 0 || j == 0 || j == res.sigma.size() - 1) {
                double x = j * res.L / (res.sigma.size() - 1);

                QString status = " OK     ";
                if (std::abs(res.sigma[j]) > max_voltage) {
                    status = " FAIL!  ";
                    strengthOk = false;
                    strengthOk_full = false;
                }

                output += QString("│ %1 │ %2 │%3│\n")
                    .arg(x, 17, 'f', 4)
                    .arg(res.sigma[j], 17, 'e', 4)
                    .arg(status);
            }
        }
        output += "|───────────────────-───────────────────-──────────|\n";

        // Вердикт по прочности одного стержня
        if (strengthOk) {
            output += "  ✓ УСЛОВИЕ ПРОЧНОСТИ ВЫПОЛНЕНО\n\n";
        }
        else {
            output += "  ✗ УСЛОВИЕ ПРОЧНОСТИ НЕ ВЫПОЛНЕНО!\n\n";
        }
    }

    output += QString(60, '=') + "\n";
    output += "                        КОНЕЦ ОТЧЕТА\n";

    std::string strongInfo;
    strongInfo += "Условию прочности относительно всей балки: ";
    strongInfo += (strengthOk_full == true) ? "✓ УСЛОВИЕ ПРОЧНОСТИ ВЫПОЛНЕНО \n" : "✗ УСЛОВИЕ ПРОЧНОСТИ НЕ ВЫПОЛНЕНО! \n";
    output += strongInfo;
    output += QString(60, '=') + "\n";

    QMetaObject::invokeMethod(this, [this, output]() {
        ui.textEdit_p_1->append(output);
        }, Qt::QueuedConnection);
}




double cProcessor::calculateNormalForce(double E, double A, double L,
    double delta_i, double delta_j,
    double q, double x)
{
    // N(x) = (E*A/L)*(Δ_j - Δ_i) + (q*L/2)*(1 - 2*x/L)
    if (std::abs(L) < 1e-12) {
        return 0.0;
    }

    double EA_over_L = (E * A) / L;
    double N_displacement = EA_over_L * (delta_j - delta_i);
    double N_load = (q * L / 2.0) * (1.0 - 2.0 * x / L);

    return N_displacement + N_load;
}

double cProcessor::calculateDisplacement(double delta_i, double delta_j,
    double E, double A, double L,
    double q, double x)
{
    // u(x) = Δ_i + (Δ_j - Δ_i)*(x/L) + (q*L²/(2*E*A))*(1 - x/L)*(x/L)
    if (std::abs(E * A) < 1e-12 || std::abs(L) < 1e-12) {
        return 0.0;
    }

    double u_linear = delta_i + (delta_j - delta_i) * (x / L);
    double u_distributed = (q * L * L / (2.0 * E * A)) * (1.0 - x / L) * (x / L);

    return u_linear + u_distributed;
}

double cProcessor::calculateStress(double N, double A)
{
    // σ = N / A
    if (std::abs(A) < 1e-12) {
        return 0.0;
    }
    return N / A;
}
