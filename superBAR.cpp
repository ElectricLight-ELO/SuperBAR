#include "superBAR.h"

superBAR::superBAR(QWidget* parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    this->setWindowTitle("super-BAR");

    ui.widget_beam->hide();
    ui.widget_force->hide();
    ui.widget_l_load->hide();
    centerWindowOnScreen();
    setupSceneAndView();
    connectUi();
}

superBAR::~superBAR()
{
    disconnectAllItems();

    if (m_scene) {
        m_scene->clear();
        delete m_scene;
        m_scene = nullptr;
    }
}

void superBAR::disconnectAllItems()
{
    if (!m_scene) return;

    // Сначала отключаем все соединения безопасным способом
    QList<QGraphicsItem*> allItems = m_scene->items();

    // Отключаем все балки
    for (auto item : allItems) {
        if (auto beam = dynamic_cast<BeamItem*>(item)) {
            if (!beam->isBeingDestroyed()) {
                beam->disconnect();
            }
        }
    }

    // Отключаем все заделки
    for (auto item : allItems) {
        if (auto support = dynamic_cast<FixedSupportItem*>(item)) {
            if (!support->isBeingDestroyed()) {
                support->disconnect();
            }
        }
    }
}

void superBAR::on_pushButton_8_clicked()
{

    collectBeamInfo(collectedBeam_info);
    if (collectedBeam_info.size() < 1) {
        QMessageBox::warning(this, "", "Нет исходных данных");
        return;
    }
    cProcessor* form = new cProcessor(&collectedBeam_info);

    connect(form, &cProcessor::sendResults,
        this, &superBAR::create_Plot);

    form->setAttribute(Qt::WA_DeleteOnClose);
    form->setWindowModality(Qt::ApplicationModal);
    form->show();
   
}

void superBAR::onScalesConfirmed(int nxScale, int uxScale, int sigmaScale)
{
    removeItemsOfType<PlotItem>();
    removeItemsOfType<DiagramItem>();
    Nx_scaling = nxScale;
    Ux_scaling = uxScale;
    sigma_scaling = sigmaScale;
    displayDiagrams(_results);
}

void superBAR::closeEvent(QCloseEvent* event)
{
    disconnectAllItems();

    if (m_scene) {
        m_scene->clear();
        delete m_scene;
        m_scene = nullptr;
    }

    QMainWindow::closeEvent(event);
}

bool superBAR::eventFilter(QObject* obj, QEvent* event)
{
    // Обработка зума колесиком мыши
    if (obj == ui.graphicsView_1->viewport() && event->type() == QEvent::Wheel) {
        QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);

        if (wheelEvent->modifiers() & Qt::ControlModifier) {
            const int delta = wheelEvent->angleDelta().y();
            const qreal factor = 1.15;

            if (delta > 0) {
                // Приближение
                if (m_currentZoom * factor <= 10.0) {
                    ui.graphicsView_1->scale(factor, factor);
                    m_currentZoom *= factor;
                }
            }
            else {
                // Отдаление
                if (m_currentZoom / factor >= 0.1) {
                    ui.graphicsView_1->scale(1.0 / factor, 1.0 / factor);
                    m_currentZoom /= factor;
                }
            }

            return true;
        }
    }

    return QMainWindow::eventFilter(obj, event);
}
void superBAR::create_Plot(std::vector<BeamResults> results) {
    _results = results;

    removeItemsOfType<PlotItem>();
    removeItemsOfType<DiagramItem>();

    displayDiagrams(results);
}
std::tuple<double, double, double> superBAR::findMinMax(const std::vector<double>& values)
{
    if (values.empty()) {
        return std::make_tuple(0.0, 0.0, 0.0);
    }

    auto [minIt, maxIt] = std::minmax_element(values.begin(), values.end());

    double mod1 = std::abs(*minIt);
    double mod2 = std::abs(*maxIt);
    double mod_max = std::max(mod1, mod2);
    return std::make_tuple(*minIt, *maxIt, mod_max);
}
void superBAR::displayDiagrams(const std::vector<BeamResults>& results)
{
    auto connectors = collectAllConnectors();

    if (connectors.empty()) {
        qDebug() << "No connectors found!";
        return;
    }

    const qreal lineHeight = 2100.0;
    const qreal yOffset = 25;

    // Для каждого узла создаем PlotItem
    for (size_t i = 0; i < connectors.size(); i++)
    {
        const auto& conn = connectors[i];

        // Создаем PlotItem для этого узла
        PlotItem* plotItem = new PlotItem(
            conn.o_x,                // X координата узла
            conn.o_y+ yOffset,      // Y координата + смещение вниз
            lineHeight              // Высота линии
        );
        m_scene->addItem(plotItem);

    }

    std::vector<double> Nx;
    std::vector<double> Ux;
    std::vector<double> sigma_x;
    for (int i = 0; i < results.size(); i++) {
        Nx.insert(Nx.end(), results[i].N_x.begin(), results[i].N_x.end());
        Ux.insert(Ux.end(), results[i].U_x.begin(), results[i].U_x.end());
        sigma_x.insert(sigma_x.end(), results[i].sigma.begin(), results[i].sigma.end());
    }
    auto[min_Nx, max_Nx, mod_Nx] = findMinMax(Nx);
    auto[min_Ux, max_Ux, mod_Ux] = findMinMax(Ux);
    auto[min_sig, max_sig, mod_sig] = findMinMax(sigma_x);
    
    for (int i = 0; i < connectors.size()-1; i++) {

        double offset;


        double opt = std::min(std::abs(min_Nx), std::abs(max_Nx));
        offset = metersToQreal(opt);

        if (offset < 100) {
            offset = 300;
        }
        else if (offset > 500 && offset < 800) {
            offset = offset / 1.7f;
        }
        else if (offset > 800) {
            offset = offset / 3.0f;
        }
        double now_x = connectors[i].o_x;

        std::string param_nx = (i == 0) ? "Nx" : "";

        bool Nx_right = !shouldHideRightLabel(i, results,
            [](const BeamResults& r) { return r.N_x; });

        DiagramItem* diag1 = new DiagramItem(now_x, connectors[0].o_y + offset, metersToQreal(results[i].L), 
            results[i].N_x, QString::fromStdString(param_nx), Nx_scaling, true, Nx_right);

        m_scene->addItem(diag1);
        offset += metersToQreal(mod_Ux) / 4;
        offset += 70.0;

        
        
      //  bool Ux_right = true;

        //const double eps = 1e-9;

        //// Проверяем только правую подпись
        //if (i < static_cast<int>(results.size()) - 1) {
        //    if (!results[i].U_x.empty() && !results[i + 1].U_x.empty()) {
        //        double rightCurrent = results[i].U_x[results[i].U_x.size() - 1];
        //        double leftNext = results[i + 1].U_x[0];

        //        // Используем сравнение с допуском
        //        if (std::abs(rightCurrent - leftNext) < eps) {
        //            Ux_right = false;
        //        }
        //    }
        //}

        std::string param_ux = (i == 0) ? "Ux" : "";

        bool Ux_right = !shouldHideRightLabel(i, results,
            [](const BeamResults& r) { return r.U_x; });

        DiagramItem* diag2 = new DiagramItem(now_x, connectors[0].o_y + offset, metersToQreal(results[i].L), 
            results[i].U_x, QString::fromStdString(param_ux), Ux_scaling, true, Ux_right);
        m_scene->addItem(diag2);
        offset += metersToQreal(mod_sig) / 4;
        offset += 70.0;

        std::string param_sigma = (i == 0) ? "σ" : "";
        bool Sigma_x_right = !shouldHideRightLabel(i, results,
            [](const BeamResults& r) { return r.sigma; });
        DiagramItem* diag3 = new DiagramItem(now_x, connectors[0].o_y + offset, metersToQreal(results[i].L), results[i].sigma,
            QString::fromStdString(param_sigma), sigma_scaling, true, Sigma_x_right);
        m_scene->addItem(diag3);

    }
}


void superBAR::collectBeamInfo(std::vector<Core_of_Beam>& data)
{
    collectedBeam_info.clear();

    if (!m_scene) return;

    // 1. Собираем ВСЕ балки из сцены
    std::vector<BeamItem*> allBeams;

    for (auto* item : m_scene->items()) {
        if (auto* beam = dynamic_cast<BeamItem*>(item)) {
            if (!beam->isBeingDestroyed()) {
                allBeams.push_back(beam);
            }
        }
    }

    // 2. Сортируем балки слева направо по координате X
    std::sort(allBeams.begin(), allBeams.end(),
        [](BeamItem* a, BeamItem* b) {
            auto leftConnA = a->getLeftConnector();
            auto leftConnB = b->getLeftConnector();
            return leftConnA.o_x < leftConnB.o_x;
        });

    // 3. Собираем информацию по каждой балке
    for (BeamItem* beam : allBeams) {
        Core_of_Beam beamInfo;

        // Получаем характеристики балки
        auto [len, sectArea, modElast, maxStress] = beam->getInfo();
        beamInfo.len_L = qrealToMeters(len);
        beamInfo.selectArea_A = sectArea;
        beamInfo.mod_elasticity = modElast;
        beamInfo.maxVoltage = maxStress;

        // Получаем координаты концов балки
        auto leftConn = beam->getLeftConnector();
        auto rightConn = beam->getRightConnector();

        // Собираем информацию об узлах (включая lineLoad_q)
        beamInfo.Joint_left = collectJointInfo(leftConn);
        beamInfo.Joint_right = collectJointInfo(rightConn);

        collectedBeam_info.push_back(beamInfo);
    }

}

Joint_info superBAR::collectJointInfo(const PointConnector& nodePos)
{
    Joint_info jointInfo;
    jointInfo.fixedSupport = 0;
    jointInfo.lineLoad_q = 0.0;
    jointInfo.force_f = 0.0;

    const qreal tolerance = 15.0; // Используем SNAP_DISTANCE для согласованности

    // Проходим по всем элементам сцены
    for (auto* item : m_scene->items()) {

        // Проверяем наличие заделки
        if (auto* support = dynamic_cast<FixedSupportItem*>(item)) {
            if (!support->isBeingDestroyed()) {
                auto supportPos = support->getPointConnector();
                if (std::abs(supportPos.o_x - nodePos.o_x) < tolerance &&
                    std::abs(supportPos.o_y - nodePos.o_y) < tolerance) {
                    jointInfo.fixedSupport = 1;
                }
            }
        }

        // Проверяем наличие сосредоточенной силы
        if (auto* force = dynamic_cast<ForceItem*>(item)) {
            if (force->scene()) {
                auto [lenFromBeam, forceH, posBeam] = force->getInfo();

                // Получаем фактическую позицию силы в координатах сцены
                QPointF forceScenePos = force->scenePos();

                if (std::abs(forceScenePos.x() - nodePos.o_x) < tolerance &&
                    std::abs(forceScenePos.y() - nodePos.o_y) < tolerance) {
                    jointInfo.force_f += forceH; // Суммируем силы
                }
            }
        }

        // Проверяем наличие распределенной нагрузки
        if (auto* lineLoad = dynamic_cast<LineLoadItem*>(item)) {
            if (lineLoad->scene()) {
                auto [q, beamDig] = lineLoad->getInfo();

                // Получаем позицию начала распределенной нагрузки
                QPointF lineLoadPos = lineLoad->scenePos();

                // Проверяем, находится ли узел в начальной точке нагрузки
                if (std::abs(lineLoadPos.x() - nodePos.o_x) < tolerance &&
                    std::abs(lineLoadPos.y() - nodePos.o_y) < tolerance) {
                    jointInfo.lineLoad_q = q;
                }
            }
        }
    }

    return jointInfo;

}

bool superBAR::shouldHideRightLabel(int currentIndex, const std::vector<BeamResults>& results, const std::function<std::vector<double>(const BeamResults&)>& getValuesFunc)
{
    const double eps = 1e-9;

    // Если это последний участок - показываем подпись
    if (currentIndex >= static_cast<int>(results.size()) - 1) {
        return false;
    }

    // Получаем значения текущего и следующего участков
    std::vector<double> currentValues = getValuesFunc(results[currentIndex]);
    std::vector<double> nextValues = getValuesFunc(results[currentIndex + 1]);

    // Проверяем не пусты ли векторы
    if (currentValues.empty() || nextValues.empty()) {
        return false;
    }

    // Сравниваем с допуском
    double rightCurrent = currentValues[currentValues.size() - 1];
    double leftNext = nextValues[0];

    return std::abs(rightCurrent - leftNext) < eps;
}

int superBAR::getNodeCount() const
{
    auto connectors = const_cast<superBAR*>(this)->collectAllConnectors();
    return static_cast<int>(connectors.size());
}

int superBAR::getBeamCount() const
{
    if (!m_scene) return 0;

    int count = 0;
    for (auto* item : m_scene->items()) {
        if (auto* beam = dynamic_cast<BeamItem*>(item)) {
            if (!beam->isBeingDestroyed()) {
                count++;
            }
        }
    }
    return count;
}

void superBAR::saveOnHotKey()
{
}

void superBAR::serialization(std::string filename)
{
    if (!m_scene) return;


    XMLDocument doc;

    // Корневой элемент
    XMLElement* root = doc.NewElement("Items");
    doc.InsertFirstChild(root);

    QList<QGraphicsItem*> allItems = m_scene->items();
    std::reverse(allItems.begin(), allItems.end());
    for (auto& item : allItems) {
        if (auto beam = dynamic_cast<BeamItem*>(item)) {
            if (!beam->isBeingDestroyed()) {
                XMLElement* beam_main = doc.NewElement("Beam");
                QPointF pos = beam->scenePos();
                XMLElement* ox = doc.NewElement("ox");
                ox->SetText(QString::number(pos.x()).toStdString().c_str());
                XMLElement* oy = doc.NewElement("oy");
                oy->SetText(QString::number(pos.y()).toStdString().c_str());

                auto [len, sectArea_A, mod_Elast_E, maxStress_q] = beam->getInfo();
                XMLElement* lenBeam = doc.NewElement("LengthBeam");
                lenBeam->SetText(QString::number(len).toStdString().c_str());

                XMLElement* Area = doc.NewElement("SectArea");
                Area->SetText(QString::number(sectArea_A).toStdString().c_str());

                XMLElement* ModElast = doc.NewElement("ModulusElastic");
                ModElast->SetText(QString::number(mod_Elast_E).toStdString().c_str());

                XMLElement* MaxStress = doc.NewElement("MaxStress");
                MaxStress->SetText(QString::number(maxStress_q).toStdString().c_str());

                beam_main->InsertEndChild(ox);
                beam_main->InsertEndChild(oy);
                beam_main->InsertEndChild(lenBeam);
                beam_main->InsertEndChild(Area);
                beam_main->InsertEndChild(ModElast);
                beam_main->InsertEndChild(MaxStress);
                root->InsertEndChild(beam_main);
            }
        }
        else if (auto supportItem = dynamic_cast<FixedSupportItem*>(item)) {
            if (!supportItem->isBeingDestroyed()) {
                XMLElement* fix_supp = doc.NewElement("FixedSupport");
                auto [_ox, _oy, el] = supportItem->getInfo();

                QPointF pos = supportItem->scenePos();
                XMLElement* ox = doc.NewElement("ox");
                ox->SetText(QString::number(pos.x()).toStdString().c_str());
                XMLElement* oy = doc.NewElement("oy");
                oy->SetText(QString::number(pos.y()).toStdString().c_str());

                XMLElement* direction = doc.NewElement("Direction");
                std::string direct = (el == ElementDirection::Left) ? "Left" : "Right";
                direction->SetText(direct.c_str());

                fix_supp->InsertEndChild(ox);
                fix_supp->InsertEndChild(oy);
                fix_supp->InsertEndChild(direction);
                root->InsertEndChild(fix_supp);
            }
        }
        else if (auto forceItem = dynamic_cast<ForceItem*>(item)) {
            auto [_len_b, forceH, pos] = forceItem->getInfo();
            XMLElement* force_item = doc.NewElement("Force");

            XMLElement* force = doc.NewElement("force_H");
            force->SetText(QString::number(forceH).toStdString().c_str());

            XMLElement* posOnbeam = doc.NewElement("pos");
            posOnbeam->SetText(QString::number(pos).toStdString().c_str());

            force_item->InsertEndChild(force);
            force_item->InsertEndChild(posOnbeam);

            root->InsertEndChild(force_item);
        }
        else if (auto lineLoadItem = dynamic_cast<LineLoadItem*>(item)) {
            auto [q, _beamD] = lineLoadItem->getInfo();

            XMLElement* force_item = doc.NewElement("LineLoad");

            XMLElement* force = doc.NewElement("q");
            force->SetText(QString::number(q).toStdString().c_str());

            XMLElement* beamD = doc.NewElement("beamDig");
            beamD->SetText(QString::number(_beamD).toStdString().c_str());

            force_item->InsertEndChild(force);
            force_item->InsertEndChild(beamD);
            root->InsertEndChild(force_item);
        }
    }
    doc.SaveFile(filename.c_str());
}

void superBAR::deserialization(std::string filename)
{
    fixedSupport_left = true;
    fixedSupport_right = true;
    XMLDocument doc;
    if (doc.LoadFile(filename.c_str()) != XML_SUCCESS) {
        std::cerr << "Ошибка загрузки XML\n";
        return;
    }

    XMLElement* root = doc.FirstChildElement("Items");
    if (!root) return;

    for (XMLElement* elem = root->FirstChildElement();
        elem != nullptr;
        elem = elem->NextSiblingElement()) {

        std::string tag = elem->Name();

        if (tag == "Beam") {
            qreal ox = 0, oy = 0, length = 0, selectArea=0, mod =0, maxStr =0 ;
            elem->FirstChildElement("ox")->QueryDoubleText(&ox);
            elem->FirstChildElement("oy")->QueryDoubleText(&oy);
            elem->FirstChildElement("LengthBeam")->QueryDoubleText(&length);
            elem->FirstChildElement("SectArea")->QueryDoubleText(&selectArea);
            elem->FirstChildElement("ModulusElastic")->QueryDoubleText(&mod);
            elem->FirstChildElement("MaxStress")->QueryDoubleText(&maxStr);

            auto* beam = new BeamItem(ox, oy, length, 45);
            beam->setPos(ox, oy);
            m_scene->addItem(beam);
            ConnectionManager::checkAndSnapNewItem(beam, m_scene);
            beam->setInfo(selectArea, mod, maxStr);
        }
        else if (tag == "FixedSupport") {
            qreal ox = 0, oy = 0;
            std::string type;
            ElementDirection el_type;
            elem->FirstChildElement("ox")->QueryDoubleText(&ox);
            elem->FirstChildElement("oy")->QueryDoubleText(&oy);
            type = elem->FirstChildElement("Direction")->GetText();
            el_type = (type == "Left") ? ElementDirection::Left : ElementDirection::Right;
            if (el_type == ElementDirection::Left) {
                fixedSupport_left = false;
            }
            if (el_type == ElementDirection::Right) {
                fixedSupport_right = false;
            }

            FixedSupportItem* support = nullptr;
            support = new FixedSupportItem(0, 0, FixSupportLen, el_type);
            support->setPos(ox, oy);
            if (support) {

                connect(support, &FixedSupportItem::supportDeleted, this,
                    [this](ElementDirection dir) {
                        if (dir == ElementDirection::Left) {
                            fixedSupport_left = true;
                        }
                        else {
                            fixedSupport_right = true;
                        }
                    });

                m_scene->addItem(support);
                ConnectionManager::checkAndSnapNewItem(support, m_scene);
            }
        }
        else if (tag == "Force") {
            qreal H = 0;
            int pos_b = 0;
            elem->FirstChildElement("force_H")->QueryDoubleText(&H);
            elem->FirstChildElement("pos")->QueryIntText(&pos_b);
            auto [o_x, o_y] = getFirstPointBeam();
            ForceItem* force = nullptr;
            force = new ForceItem(o_x, o_y, ElementDirection::Right, 40);
            m_scene->addItem(force);


            auto [f_ox, f_oy] = getFirstPointBeam();
            auto [_ox, _oy] = getPointBeam(pos_b);

            if (H > 0) {

                force->changeDirection(ElementDirection::Right);
            }
            else
            {
                force->changeDirection(ElementDirection::Left);
            }

            force->set_Len_fr_beam(_ox - f_ox);
            force->setForce_H(H, pos_b);
        }
        else if (tag == "LineLoad") {
            qreal q = 0;
            int _beamDig = 0;
            elem->FirstChildElement("q")->QueryDoubleText(&q);
            elem->FirstChildElement("beamDig")->QueryIntText(&_beamDig);

            auto [o_x_1, o_y_1] = getPointBeam(1);
            auto [o_x_2, o_y_2] = getPointBeam(2);
            LineLoadItem* line_l = new LineLoadItem(o_x_1, o_y_1, o_x_2, o_y_2, ElementDirection::Right);
            m_scene->addItem(line_l);


            auto [_ox, _oy, _ox2, _oy2] = getPointsBeam(_beamDig);
            line_l->change_loc_joins(_ox, _oy, _ox2, _oy2);
            line_l->set_LineLoad(q, _beamDig);
            if (q > 0) {
                line_l->change_direction(ElementDirection::Right);
            }
            else {
                line_l->change_direction(ElementDirection::Left);
            }
        }
    }
}



bool superBAR::checkNotEmpty(QTextEdit* edit)
{
    QString text = edit->toPlainText().trimmed();
    if (text.isEmpty())
        return false;

    // Разбиваем по пробелам/переводам строк
    QStringList parts = text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    for (const QString& part : parts) {
        if (part.trimmed().isEmpty())
            return false;
    }
    return true;
}


bool superBAR::checkAllFilled(const QVector<QTextEdit*>& edits)
{
    for (auto* edit : edits) {
        if (edit->toPlainText().trimmed().isEmpty())
            return false;
    }
    return true;
}

void superBAR::changeBeamLength(BeamItem* beam, qreal newLength)
{
    if (!beam || beam->isBeingDestroyed()) return;

    auto [currentLength, currentWidth] = beam->getParams();

    if (qFuzzyCompare(currentLength, newLength)) {
        QMessageBox::information(this, "Информация", "Длина балки не изменилась");
        return;
    }

    bool wasConnected = beam->isConnected();
    QGraphicsItem* connectedItem = beam->connectedTo;

    beam->set_Length(newLength);

    // Если балка была соединена, обновляем привязку
    if (wasConnected && connectedItem && !beam->isBeingDestroyed()) {
        m_scene->clearSelection();
        beam->setSelected(true);
        ConnectionManager::trySnapItems(m_scene);
    }
}

std::tuple<qreal, qreal> superBAR::getLastPointBeam()
{
    qreal maxX = 0;
    qreal correspondingY = 0;

    if (!m_scene) return { 0, 0 };

    // Оптимизированный поиск самой правой точки
    for (auto& item : m_scene->items()) {
        if (auto hasConn = dynamic_cast<IHasConnector*>(item)) {
                auto point = hasConn->getPointConnector();
                if (point.o_x > maxX) {
                    maxX = point.o_x;
                    correspondingY = point.o_y;
                }
            }
        
    }

    return { maxX, correspondingY };
}

std::tuple<qreal, qreal> superBAR::getFirstPointBeam()
{
    qreal minX = 99999999;
    qreal correspondingY = 0;

    if (!m_scene) return { 0, 0 };

    // Оптимизированный поиск самой левой точки
    for (auto& item : m_scene->items()) {
        if (auto hasConn = dynamic_cast<BeamItem*>(item)) {
            if (!hasConn->isBeingDestroyed()) {
                auto point = hasConn->getLeftConnector();
                if (point.o_x < minX) {
                    minX = point.o_x;
                    correspondingY = point.o_y;
                }
            }
        }
    }

    if (minX == 99999999) {
        return { 0, 0 };
    }

    return { minX, correspondingY };
}
std::tuple<qreal, qreal> superBAR::getPointBeam(int order_beam)
{
    auto connectors = collectAllConnectors();
    if (order_beam <= 0 || order_beam > (int)connectors.size())
        return { 0, 0 };

    auto& point = connectors[order_beam - 1]; // индексация с 1
    return { point.o_x, point.o_y };
}
std::tuple<qreal, qreal, qreal, qreal> superBAR::getPointsBeam(int order_beam)
{
    auto connectors = collectAllConnectors();

    if (order_beam <= 0 || order_beam > (int)connectors.size())
        return { 0, 0, 0, 0};
    auto& point1 = connectors[order_beam - 1];
    auto& point2 = connectors[order_beam];
    return { point1.o_x, point1.o_y, point2.o_x, point2.o_y };
}
std::vector<PointConnector> superBAR::collectAllConnectors()
{
    std::vector<PointConnector> connectors;
    if (!m_scene) return connectors;

    // --- Сбор всех коннекторов ---
    for (auto& item : m_scene->items()) {
        if (auto beam = dynamic_cast<BeamItem*>(item)) {
            if (!beam->isBeingDestroyed()) {
                connectors.push_back(beam->getLeftConnector());
                connectors.push_back(beam->getRightConnector());
            }
        }
        else if (auto support = dynamic_cast<FixedSupportItem*>(item)) {
            if (!support->isBeingDestroyed()) {
                connectors.push_back(support->getPointConnector());
            }
        }
    }

    // --- Сортировка по X, затем по Y ---
    std::sort(connectors.begin(), connectors.end(),
        [](const PointConnector& a, const PointConnector& b) {
            if (!qFuzzyCompare(a.o_x, b.o_x))
                return a.o_x < b.o_x;
            return a.o_y < b.o_y;
        });

    // --- Удаление дубликатов ---
    connectors.erase(std::unique(connectors.begin(), connectors.end(),
        [](const PointConnector& a, const PointConnector& b) {
            return qFuzzyCompare(a.o_x, b.o_x) &&
                qFuzzyCompare(a.o_y, b.o_y);
        }),
        connectors.end());

    return connectors;
}


void superBAR::setupSceneAndView()
{
    m_scene = new QGraphicsScene(this);
    ui.graphicsView_1->setScene(m_scene);
    ui.graphicsView_1->setRenderHint(QPainter::Antialiasing);
    ui.graphicsView_1->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    ui.graphicsView_1->setFocusPolicy(Qt::StrongFocus);
    ui.graphicsView_1->setFocus();
    // Устанавливаем фильтр событий для обработки зума
    ui.graphicsView_1->viewport()->installEventFilter(this);

    connect(m_scene, &QGraphicsScene::changed, this, [this]() {
        if (m_scene) {
            ConnectionManager::trySnapItems(m_scene);
        }
        });
}
template<typename T>
inline void superBAR::removeItemsOfType()
{
    QList<QGraphicsItem*> allItems = m_scene->items();
    for (auto item : allItems) {
        if (auto sign = dynamic_cast<T*>(item)) {
            sign->remove();
        }
    }
}

std::string superBAR::choosePath(bool isSave)
{
    QString fileName;

    if (isSave) {
        fileName = QFileDialog::getSaveFileName(
            this,
            tr("Сохранить файл"),            
            QDir::currentPath() + "/example.xml",
            tr("XML файлы (*.xml);;Все файлы (*.*)")
        );
    }
    else {
        fileName = QFileDialog::getOpenFileName(
            this,
            tr("Открыть файл"),              
            QDir::currentPath(),
            tr("XML файлы (*.xml);;Все файлы (*.*)")
        );
    }

    return fileName.toStdString();
}

void superBAR::onMenuActionTriggered()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action) return;

    QString name = action->objectName();

    // открыть
    if (name == "action") {
        m_scene->clear();

        _file_name = choosePath(false);

        deserialization(_file_name);
    }
    // сохранить
    if (name == "action_2") {
        if (_file_name != "") {
            serialization(_file_name);
        }
        else {
            _file_name = choosePath(true);

            serialization(_file_name);
        }
        
    }
    // сохранить как
    if (name == "action_3") {
        
        _file_name = choosePath(true);

        serialization(_file_name);
    }
    // вкл/выкл подписи
    if (name == "action_4") {
        if (!beams_sign_b)  {
            int inx = 1;
            
            auto pConnectors = collectAllConnectors();
            int lenArray = pConnectors.size();
            for (int i = 0; i < lenArray - 1; i ++) {
                qreal sredX = (pConnectors[i].o_x + pConnectors[i + 1].o_x) / 2.0;
                qreal sredY = (pConnectors[i].o_y + pConnectors[i + 1].o_y) / 2.0;

                m_scene->addItem(new BeamSign(sredX, sredY - 60, inx));
                inx++;
            }

            
            beams_sign_b = true;
        }
        else {
            if (!m_scene) return;

            removeItemsOfType<BeamSign>();

            beams_sign_b = false;
        }
    }
    else if (name == "action_5") {
        if (!joins_sign_b) {
            int inx = 1;
            auto pConnectors = collectAllConnectors();
            int lenArray = pConnectors.size();
            for (int i = 0; i < lenArray; i++) {
                qreal sredX = pConnectors[i].o_x;
                qreal sredY = pConnectors[i].o_y ;

                m_scene->addItem(new BeamJoint(sredX+10, sredY + 60, inx));
                inx++;
            }

            joins_sign_b = true;
        }
        else {

            if (!m_scene) return;

            removeItemsOfType<BeamJoint>();
            joins_sign_b = false;
        }
    }
    else if (name == "action_6") {
        m_scene->clear();
        fixedSupport_left = true;
        fixedSupport_right = true;
        _file_name = "";
    }

    else if (name == "action_7") {
        removeItemsOfType<PlotItem>();
        removeItemsOfType<DiagramItem>();
    }

    else if (name == "action_8") {
        // перерисовка графика
        std::tuple<int, int, int> scaling_default = { Nx_scaling, Ux_scaling, sigma_scaling};

        sliderDialog* slider_form = new sliderDialog(scaling_default);
        connect(slider_form, &sliderDialog::scalesConfirmed,
            this, &superBAR::onScalesConfirmed);

        slider_form->setAttribute(Qt::WA_DeleteOnClose);
        slider_form->setWindowModality(Qt::ApplicationModal);
        slider_form->show();
       // removeItemsOfType<DiagramItem>();
    }
}

void superBAR::connectUi()
{
    connect(ui.action, &QAction::triggered, this, &superBAR::onMenuActionTriggered);
    connect(ui.action_2, &QAction::triggered, this, &superBAR::onMenuActionTriggered);
    connect(ui.action_3, &QAction::triggered, this, &superBAR::onMenuActionTriggered);
    connect(ui.action_4, &QAction::triggered, this, &superBAR::onMenuActionTriggered);
    connect(ui.action_5, &QAction::triggered, this, &superBAR::onMenuActionTriggered);
    connect(ui.action_6, &QAction::triggered, this, &superBAR::onMenuActionTriggered);
    connect(ui.action_7, &QAction::triggered, this, &superBAR::onMenuActionTriggered);
    connect(ui.action_8, &QAction::triggered, this, &superBAR::onMenuActionTriggered);


    ui.action_2->setShortcut(QKeySequence::Save);
    connect(ui.action_2, &QAction::triggered, this, &superBAR::saveOnHotKey);
    // Добавляем в меню
  //  menuBar()->addAction(ui.action_2);


    // Добавление балки
    connect(ui.pushButton_1, &QPushButton::clicked, this, [this]() {
        auto [o_x, o_y] = getLastPointBeam();
        auto* beam = new BeamItem(o_x, o_y, SCALE, 45);
        m_scene->addItem(beam);
        ConnectionManager::checkAndSnapNewItem(beam, m_scene);

        beam->setInfo(1, 1, 1);
        });

    // Добавление заделки
    connect(ui.pushButton_2, &QPushButton::clicked, this, [this]() {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Подтверждение");
        msgBox.setText("Заделка будет левая или правая?");

        auto* leftButton = msgBox.addButton("Левая", QMessageBox::AcceptRole);
        auto* rightButton = msgBox.addButton("Правая", QMessageBox::AcceptRole);
        msgBox.addButton("Отмена", QMessageBox::DestructiveRole);
        leftButton->setEnabled(fixedSupport_left);
        rightButton->setEnabled(fixedSupport_right);
        msgBox.exec();

        FixedSupportItem* support = nullptr;
        if (msgBox.clickedButton() == leftButton) {
            support = new FixedSupportItem(firstBeam_x, 50, FixSupportLen, ElementDirection::Left);
            fixedSupport_left = false;
        }
        else if (msgBox.clickedButton() == rightButton) {
            support = new FixedSupportItem(firstBeam_x, 50, FixSupportLen, ElementDirection::Right);
            fixedSupport_right = false;
        }
        firstBeam_x += 50;

        if (support) {

            connect(support, &FixedSupportItem::supportDeleted, this,
                [this](ElementDirection dir) {
                    if (dir == ElementDirection::Left) {
                        fixedSupport_left = true;
                    }
                    else {
                        fixedSupport_right = true;
                    }
                });


            m_scene->addItem(support);
            ConnectionManager::checkAndSnapNewItem(support, m_scene);
        }
        });

    // Добавление силы
    connect(ui.pushButton_3, &QPushButton::clicked, this, [this]() {
        auto [o_x, o_y] = getFirstPointBeam();

        ForceItem* item = new ForceItem(o_x, o_y, ElementDirection::Right, 40);
        m_scene->addItem(item);
        item->set_Len_fr_beam(1);
        item->setForce_H(1, 1);
        });
    // добавление погонной нагрузки
    connect(ui.pushButton_7, &QPushButton::clicked, this, [this]() {
        auto [o_x_1, o_y_1] = getPointBeam(1);
        auto [o_x_2, o_y_2] = getPointBeam(2);
        LineLoadItem* lineL = new LineLoadItem(o_x_1, o_y_1, o_x_2, o_y_2, ElementDirection::Right);
        m_scene->addItem(lineL);
        lineL->set_LineLoad(1, 1);
        });

    // Обработка выделения
    connect(m_scene, &QGraphicsScene::selectionChanged, this, [this]() {
        BeamItem* selectedBeam = nullptr;
        ForceItem* selectedForce = nullptr;
        LineLoadItem* selectedLineLoad = nullptr;

        for (auto item : m_scene->selectedItems()) {
            if (auto beam = dynamic_cast<BeamItem*>(item)) {
                if (!beam->isBeingDestroyed()) {
                    selectedBeam = beam;
                }
            }
            if (auto force = dynamic_cast<ForceItem*>(item)) {
                selectedForce = force;
            }
            if (auto l_load = dynamic_cast<LineLoadItem*>(item)) {
                selectedLineLoad = l_load;
            }
        }

        // --- Балка ---
        if (selectedBeam) {
            ui.widget_beam->show();
            qreal length = std::get<0>(selectedBeam->getParams());
            auto [d, d2, d3, d4] = selectedBeam->getInfo();
            ui.textEdit_3->setText(QString::number(qrealToMeters(length)));
            ui.textEdit_4->setText(QString::number(d2));
            ui.textEdit_5->setText(QString::number(d3));
            ui.textEdit_6->setText(QString::number(d4));
        }
        else {
            ui.widget_beam->hide();
            ui.textEdit_3->clear();
            ui.textEdit_4->clear();
            ui.textEdit_5->clear();
            ui.textEdit_6->clear();
        }
        // --- Сила ---
        if (selectedForce) {
            ui.widget_force->show();
            auto [d_l, d_f_h, pos_beam] = selectedForce->getInfo();
            ui.textEdit_7->setText(QString::number(pos_beam));
            ui.textEdit_8->setText(QString::number(d_f_h));
        }
        else {
            ui.widget_force->hide();
            ui.textEdit_7->clear();
            ui.textEdit_8->clear();
        }
        // --- погонная нагрузка ---
        if (selectedLineLoad) {
            auto [forc_line, beamD] = selectedLineLoad->getInfo();
            ui.textEdit_9->setText(QString::number(beamD));
            ui.textEdit_11->setText(QString::number(forc_line));

            ui.widget_l_load->show();
        }
        else {
            ui.widget_l_load->hide();
            ui.textEdit_9->clear();
            ui.textEdit_11->clear();
        }
    });


    // Изменение длины балки
    connect(ui.pushButton_4, &QPushButton::clicked, this, [this]() {
        QString lengthText = ui.textEdit_3->toPlainText();
        bool ok;

        qreal newLength = metersToQreal(lengthText.toDouble(&ok));
   //     qreal newLength = lengthText.toDouble(&ok);

        if (!ok || newLength <= 0) {
            QMessageBox::warning(this, "Ошибка", "Введите корректное значение длины!");
            return;
        }

        BeamItem* selectedBeam = nullptr;
        for (auto item : m_scene->selectedItems()) {
            if (auto beam = dynamic_cast<BeamItem*>(item)) {
                if (!beam->isBeingDestroyed()) {
                    selectedBeam = beam;
                    break;
                }
            }
        }

        if (!selectedBeam) {
            QMessageBox::warning(this, "Ошибка", "Не выбрана балка!");
            return;
        }
        QVector<QTextEdit*> vec = { ui.textEdit_3, ui.textEdit_4, ui.textEdit_5, ui.textEdit_6 };
        auto d = ui.textEdit_4->toPlainText().toDouble();
        auto d1 = ui.textEdit_5->toPlainText().toDouble();
        auto d2 = ui.textEdit_6->toPlainText().toDouble();
        if (checkAllFilled(vec)) {
            
            if (d > 0 && d1 > 0 && d2 > 0) {
                selectedBeam->setInfo(d, d1, d2);
                changeBeamLength(selectedBeam, newLength);
            }
            else {
                QMessageBox::information(this, "Info", "Какой-то параметр отрицательный");
                return;
            }
            
        }
        else
        {
            QMessageBox::information(this, "Info", "не все поля заполнены");
            return;
        }

        
    });

    // Применение параметров силы
    connect(ui.pushButton_5, &QPushButton::clicked, this, [this]() {
        ForceItem* forceM = nullptr;

        for (auto item : m_scene->selectedItems()) {
            if (auto forceTMP = dynamic_cast<ForceItem*>(item)) {
                forceM = forceTMP;
                break;
            }
        }

        if (forceM) {
            bool ok1, ok2;
            int dig = ui.textEdit_7->toPlainText().toInt(&ok1);
            qreal dig_forc = ui.textEdit_8->toPlainText().toDouble(&ok2);

            int nodeCount = getNodeCount();
            if (dig < 1 || dig > nodeCount) {
                QMessageBox::warning(
                    this,
                    "Ошибка валидации",
                    QString("Узел №%1 не существует!\nВсего узлов: %2\nДиапазон: 1-%2")
                    .arg(dig)
                    .arg(nodeCount)
                );
                return;
            }

            if (ok1 && ok2) {

                auto [f_ox, f_oy] = getFirstPointBeam();
                auto [_ox, _oy] = getPointBeam(dig);

                if (dig_forc > 0) {
                    
                    forceM->changeDirection(ElementDirection::Right);
                }
                else
                {
                    forceM->changeDirection(ElementDirection::Left);
                }
                
                forceM->set_Len_fr_beam(_ox - f_ox);
                forceM->setForce_H(dig_forc, dig);
            }
            else {
                QMessageBox::warning(this, "Ошибка", "Введите корректные числовые значения!");
            }
        }
    });
    // применение параметров погоннной нагрузки
    connect(ui.pushButton_6, &QPushButton::clicked, this, [this]() {
        
        LineLoadItem* selectedLineLoad = nullptr;
        for (auto item : m_scene->selectedItems()) {
            if (auto l_load = dynamic_cast<LineLoadItem*>(item)) {
                selectedLineLoad = l_load;
            }
        }
        if (selectedLineLoad == nullptr) {
            return;
        }
        bool ok1, ok3;
        int dig_beam = ui.textEdit_9->toPlainText().toInt(&ok1);
        qreal line_load = ui.textEdit_11->toPlainText().toDouble(&ok3);
        
        if (dig_beam < 1) {
            QMessageBox::warning(this, "Предупреждение", "Не может быть отрицательным");
            return;
        }
        int beamCount = getBeamCount();
        if (dig_beam < 1 || dig_beam > beamCount) {
            QMessageBox::warning(
                this,
                "Ошибка валидации",
                QString("Стержень №%1 не существует!\nВсего стержней: %2\nДиапазон: 1-%2")
                .arg(dig_beam)
                .arg(beamCount)
            );
            return;
        }
        auto [_ox, _oy, _ox2, _oy2] = getPointsBeam(dig_beam);
        selectedLineLoad->change_loc_joins(_ox, _oy, _ox2, _oy2);
        selectedLineLoad->set_LineLoad(line_load, dig_beam);
        if (line_load > 0) {
            selectedLineLoad->change_direction(ElementDirection::Right);
        }
        else {
            selectedLineLoad->change_direction(ElementDirection::Left);
        }
    });
}

void superBAR::centerWindowOnScreen()
{ //Центрирование окна по экрану
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->geometry();
        int x = (screenGeometry.width() - this->width()) / 2;
        int y = (screenGeometry.height() - this->height()) / 2;
        this->move(x, y);
	}

}
