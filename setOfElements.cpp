#include "setOfElements.h"

const qreal ConnectionManager::SNAP_DISTANCE = 15.0;
const qreal ConnectionManager::SNAP_DISTANCE_SQUARED = SNAP_DISTANCE * SNAP_DISTANCE;



// ---------- Балка ----------
BeamItem::BeamItem(qreal x1, qreal y1, qreal length, qreal width)
    : QGraphicsRectItem(0, 0, length, width), _ox(x1), _oy(y1), _len(length), _width(width)
{
    setBrush(QBrush(Qt::white));
    setPen(QPen(Qt::black, 2));
    setPos(x1, y1 - width / 2);  // 

    // точки соединения
    pointConnect_left = PointConnector(0, width / 2);
    pointConnect_right = PointConnector(length, width / 2);

    updateGreenPoints();

    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
}

QGraphicsEllipseItem* BeamItem::createGreenPoint(const PointConnector& pc)
{
    const qreal radius = 4.0;
    QGraphicsEllipseItem* ellipse = new QGraphicsEllipseItem(
        pc.o_x - radius, pc.o_y - radius,
        radius * 2, radius * 2,
        this
    );
    ellipse->setBrush(Qt::green);
    ellipse->setPen(Qt::NoPen);
    return ellipse;
}

void BeamItem::updateGreenPoints()
{
    // Удаление старых точек
    if (leftPoint) {
        delete leftPoint;
        leftPoint = nullptr;
    }
    if (rightPoint) {
        delete rightPoint;
        rightPoint = nullptr;
    }

    // Создаем новые
    leftPoint = createGreenPoint(pointConnect_left);
    rightPoint = createGreenPoint(pointConnect_right);
}

PointConnector BeamItem::getLeftConnector() const {
    QPointF scenePos = mapToScene(QPointF(0, _width / 2));
    return PointConnector(scenePos.x(), scenePos.y());
}

PointConnector BeamItem::getRightConnector() const {
    QPointF scenePos = mapToScene(QPointF(_len, _width / 2));
    return PointConnector(scenePos.x(), scenePos.y());
}

void BeamItem::setConnectedTo(QGraphicsItem* other) {
    if (other == this || !other || m_beingDestroyed) return;

    connectedTo = other;
    setBrush(QBrush(QColor(200, 255, 200)));
    setFlag(QGraphicsItem::ItemIsMovable, false);

    QTimer::singleShot(200, [this]() {
        if (this && this->scene() && !m_beingDestroyed) {
            setBrush(QBrush(Qt::white));
        }
        });
}

void BeamItem::disconnect() {
    if (m_beingDestroyed || !connectedTo) {
        return; // Защита от повторного вызова
    }

    QGraphicsItem* oldConnected = connectedTo;
    connectedTo = nullptr; // Сначала разрываем связь у себя

    // Безопасное отключение партнера
    if (oldConnected) {
        // Проверяем, что объект все еще существует в сцене
        if (scene() && scene()->items().contains(oldConnected)) {
            if (auto beam = dynamic_cast<BeamItem*>(oldConnected)) {
                if (beam->connectedTo == this && !beam->m_beingDestroyed) {
                    beam->connectedTo = nullptr;
                    beam->setFlag(QGraphicsItem::ItemIsMovable, true);
                    beam->setBrush(QBrush(Qt::white));
                }
            }
            else if (auto support = dynamic_cast<FixedSupportItem*>(oldConnected)) {
                if (support->connectedTo == this && !support->isBeingDestroyed()) {
                    support->connectedTo = nullptr;
                    support->setFlag(QGraphicsItem::ItemIsMovable, true);
                }
            }
        }
    }

    setFlag(QGraphicsItem::ItemIsMovable, true);
    setBrush(QBrush(Qt::white));

    // Безопасное смещение только если объект еще в сцене
    if (scene() && scene()->items().contains(this)) {
        setPos(pos() + QPointF(20, 0));
    }
}

void BeamItem::safeDelete() {
    if (m_beingDestroyed) return;

    m_beingDestroyed = true;
    disconnect(); // Сначала отсоединяемся

    // Для QGraphicsItem используем отложенное удаление через QTimer
    if (scene()) {
        scene()->removeItem(this);

        // Используем QTimer для отложенного удаления
        QTimer::singleShot(0, [this]() {
            delete this;
            });
    }
    else {
        delete this; // Если не в сцене, удаляем сразу
    }
}

void BeamItem::set_Length(qreal newLength)
{
    if (qFuzzyCompare(_len, newLength) || m_beingDestroyed) return;

    QGraphicsItem* oldConnection = connectedTo;
    LenBeam = qrealToMeters(newLength);
    _len = newLength;
    setRect(0, 0, newLength, _width);
    pointConnect_right = PointConnector(newLength, _width / 2);

    updateGreenPoints();
    connectedTo = oldConnection;
}

void BeamItem::setInfo(double _cross_sectArea_A, double _mod_Elasticity_E, double _maxStressBeam_q)
{
    cross_sectArea_A = _cross_sectArea_A;
    mod_Elasticity_E = _mod_Elasticity_E;
    maxStressBeam_q = _maxStressBeam_q;
}
std::tuple<double, double, double, double> BeamItem::getInfo() {
    return { _len ,cross_sectArea_A,mod_Elasticity_E, maxStressBeam_q };
}
PointConnector BeamItem::getPointConnector() const
{
    QPointF scenePos = mapToScene(QPointF(_len, _width / 2));
    return PointConnector(scenePos.x(), scenePos.y());
}

void BeamItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (m_beingDestroyed) return;
    QGraphicsRectItem::mousePressEvent(event);
    setSelected(true);
}

void BeamItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    if (m_beingDestroyed) return;

    QMenu menu;
    QAction* detachAction = menu.addAction(connectedTo ? "Отсоединить (соединен)" : "Отсоединить (не соединен)");
    QAction* editAction = menu.addAction("Изменить");
    QAction* deleteAction = menu.addAction("Удалить");

    QAction* chosen = menu.exec(event->screenPos());

    if (!chosen || m_beingDestroyed) return; // Дополнительная проверка

    if (chosen == detachAction) {
        disconnect();
    }
    else if (chosen == editAction) {
        // Логика редактирования
    }
    else if (chosen == deleteAction) {
        safeDelete();
    }
}

// ---------- ЗАДЕЛКА ----------
FixedSupportItem::FixedSupportItem(qreal x, qreal y, qreal height, ElementDirection dir) : el_d(dir), _ox(x), _oy(y)
{
    const QPen pen(Qt::black, 2);
    QPainterPath path;

    path.moveTo(0, 0);
    path.lineTo(0, height);

    qreal spacing = 5;
    qreal diagLen = 12;
    int count = static_cast<int>(height / spacing);

    for (int i = 0; i < count; ++i) {
        qreal yPos = i * spacing;
        path.moveTo(0, yPos);
        if (dir == ElementDirection::Left) {
            path.lineTo(-diagLen, yPos + spacing / 2);
        }
        else if (dir == ElementDirection::Right) {
            path.lineTo(diagLen, yPos - spacing / 2);
        }
    }

    auto* hatch = new QGraphicsPathItem(path);
    hatch->setPen(pen);
    addToGroup(hatch);

    setPos(x, y);
    pointConnect = PointConnector(0, height / 2);

    // Создаем зеленую точку
    const qreal radius = 4.0;
    auto ellipse = new QGraphicsEllipseItem(
        pointConnect.o_x - radius, pointConnect.o_y - radius,
        radius * 2, radius * 2, this
    );
    ellipse->setBrush(Qt::green);
    ellipse->setPen(Qt::NoPen);

    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
}

PointConnector FixedSupportItem::getPointConnector() const
{
    QPointF scenePos = mapToScene(QPointF(pointConnect.o_x, pointConnect.o_y));
    return PointConnector(scenePos.x(), scenePos.y());
}

void FixedSupportItem::setConnectedTo(BeamItem* other) {
    if (!other || m_beingDestroyed) return;

    connectedTo = other;

    for (auto item : childItems()) {
        if (auto pathItem = dynamic_cast<QGraphicsPathItem*>(item)) {
            pathItem->setBrush(QBrush(QColor(200, 255, 200)));
        }
    }

    QTimer::singleShot(200, [this]() {
        if (this && this->scene() && !m_beingDestroyed) {
            for (auto item : childItems()) {
                if (auto pathItem = dynamic_cast<QGraphicsPathItem*>(item)) {
                    pathItem->setBrush(QBrush());
                }
            }
        }
        });
}

void FixedSupportItem::disconnect() {
    if (m_beingDestroyed || !connectedTo) {
        return;
    }

    BeamItem* oldConnected = connectedTo;
    connectedTo = nullptr;

    if (oldConnected) {
        // Проверяем, что объект все еще существует в сцене
        if (scene() && scene()->items().contains(oldConnected)) {
            if (oldConnected->connectedTo == this && !oldConnected->isBeingDestroyed()) {
                oldConnected->connectedTo = nullptr;
                oldConnected->setFlag(QGraphicsItem::ItemIsMovable, true);
                oldConnected->setBrush(QBrush(Qt::white));
            }
        }
    }

    setFlag(QGraphicsItem::ItemIsMovable, true);

    if (scene() && scene()->items().contains(this)) {
        setPos(pos() + QPointF(20, 0));
    }
}

void FixedSupportItem::safeDelete() {
    if (m_beingDestroyed) return;

    m_beingDestroyed = true;

 
    disconnect();

    if (scene()) {
        scene()->removeItem(this);

        // Используем QTimer для отложенного удаления
        QTimer::singleShot(0, [this]() {
            delete this;
            });

    }
    else {
        delete this;
    }
    emit supportDeleted(el_d);
}

void FixedSupportItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (m_beingDestroyed) return;
    QGraphicsItemGroup::mousePressEvent(event);
}

void FixedSupportItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    if (m_beingDestroyed) return;

    QMenu menu;
    QAction* detachAction = menu.addAction(connectedTo ? "Отсоединить от балки" : "Отсоединить (не соединена)");
 //   QAction* editAction = menu.addAction("Изменить");
    QAction* deleteAction = menu.addAction("Удалить");

    detachAction->setEnabled(connectedTo != nullptr);

    QAction* chosen = menu.exec(event->screenPos());

    if (!chosen || m_beingDestroyed) return;

    if (chosen == detachAction && connectedTo) {
        disconnect();
    }
    else if (chosen == deleteAction) {
        safeDelete();
    }
}

QVariant FixedSupportItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemPositionHasChanged) {
        QPointF newPos = value.toPointF();
        _ox = newPos.x();
        _oy = newPos.y();
    }
    return QGraphicsItemGroup::itemChange(change, value);
}

// ---------- СИЛА ----------
ForceItem::ForceItem(qreal x, qreal y, ElementDirection dir,
    qreal length, const QPen& pen)
    : el_d(dir), _ox(x), _oy(y), arrowLength(length), m_pen(pen)
{
    // изначально сила на начале балки
    len_fr_beam = 0;

    // позиционируем в сцене: смещение от начала балки
    setPos(_ox + len_fr_beam, _oy);

    setFlag(QGraphicsItem::ItemIsSelectable);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    
    setZValue(10);
}

PointConnector ForceItem::getPointConnector() const
{
    return PointConnector(_ox, _oy);
}

void ForceItem::set_Len_fr_beam(qreal newLength) {
    len_fr_beam = newLength;
    // позиция в сцене — только через setPos, локальная геометрия не меняется
    setPos(_ox + len_fr_beam, _oy);
}

void ForceItem::changeDirection(ElementDirection dir)
{
    if (el_d == dir) return;
    el_d = dir;
    // просто перерисовываем локальную геометрию, позиция не трогается
    update();
}

void ForceItem::setForce_H(qreal force_digital, int pos_beam)
{
    force_H = force_digital;
    force_pos_beam = pos_beam;

    update();
}

std::tuple<qreal, qreal, int> ForceItem::getInfo()
{
    return std::make_tuple(len_fr_beam, force_H, force_pos_beam);
}

QRectF ForceItem::boundingRect() const
{
    // Даем запас по рамке, чтобы поместилась стрелка и текст
    // Симметрично относительно локального (0,0)
    qreal margin = 20.0;
    return QRectF(-arrowLength - margin, -margin,
        2 * arrowLength + 2 * margin, 2 * margin);
}

void ForceItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->setPen(m_pen);
    painter->setBrush(Qt::NoBrush);

    const qreal dx = (el_d == ElementDirection::Right ? arrowLength : -arrowLength);

    // линия
    painter->drawLine(0, 0, dx, 0);

    // стрелка
    const qreal head = 10.0;
    if (el_d == ElementDirection::Right) {
        QPolygonF arrowHead;
        arrowHead << QPointF(dx, 0)
            << QPointF(dx - head, -head / 2)
            << QPointF(dx - head, head / 2);
        painter->drawPolygon(arrowHead);
    }
    else {
        QPolygonF arrowHead;
        arrowHead << QPointF(dx, 0)
            << QPointF(dx + head, -head / 2)
            << QPointF(dx + head, head / 2);
        painter->drawPolygon(arrowHead);
    }

    // текст "F"
    painter->setPen(m_pen.color());
    const QPointF textPos = (el_d == ElementDirection::Right)
        ? QPointF(dx / 2 - 5, -10)
        : QPointF(dx / 2 - 5, 15);

    QString tmp = QString("%1 (H) F").arg(force_H);
    painter->drawText(textPos, tmp);
}

void ForceItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItem::mousePressEvent(event);
}

void ForceItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    QMenu menu;
    QAction* detachAction = menu.addAction("Отсоединить");
    QAction* editAction = menu.addAction("Изменить");
    QAction* deleteAction = menu.addAction("Удалить");

    QAction* chosen = menu.exec(event->screenPos());
    if (chosen == deleteAction) {
        if (scene()) {
            scene()->removeItem(this);
            // Для ForceItem используем прямое удаление
            delete this;
        }
        else {
            delete this;
        }
    }
}

LineLoadItem::LineLoadItem(qreal ox_1, qreal oy_1, qreal ox_2, qreal oy_2,
    ElementDirection dir, const QPen& pen)
    : m_pen(pen), el_d(dir)
{
    q_line_load = 0;

    // локальные координаты: начало всегда (0,0)
    start_x1 = 0;
    stop_y1 = 0;
    start_x2 = ox_2 - ox_1;
    stop_y2 = oy_2 - oy_1;

    // глобальное смещение
    setPos(ox_1, oy_1);

    setFlag(QGraphicsItem::ItemIsSelectable);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    setZValue(10);
}

std::tuple<qreal, int> LineLoadItem::getInfo() {

    return std::make_tuple(q_line_load, _beamDig);
}

PointConnector LineLoadItem::getPointConnector() const
{
    return PointConnector();
}

void LineLoadItem::change_loc_joins(qreal ox_1, qreal oy_1, qreal ox_2, qreal oy_2)
{
    prepareGeometryChange();

    setPos(ox_1, oy_1);

    start_x1 = 0;
    stop_y1 = 0;
    start_x2 = ox_2 - ox_1;
    stop_y2 = oy_2 - oy_1;

    
    update();
}

void LineLoadItem::set_LineLoad(qreal q_l_load, int beamD) {
    _beamDig = beamD;
    q_line_load = q_l_load;
}

void LineLoadItem::change_direction(ElementDirection direction)
{
    el_d = direction;

    update();
}

QRectF LineLoadItem::boundingRect() const
{
    qreal minX = std::min(start_x1, start_x2);
    qreal maxX = std::max(start_x1, start_x2);
    qreal minY = std::min(stop_y1, stop_y2);
    qreal maxY = std::max(stop_y1, stop_y2);

    // запас под стрелочные головки
    const qreal headX = 10.0;
    const qreal headY = 6.0;
    const qreal margin = 4.0;

    QString tmp = QString("%1 (H/М) q").arg(q_line_load);

    // Примерная ширина символа ~8px, высота текста ~15px
    qreal textWidth = tmp.length() * 8.0;
    qreal textHeight = 15.0;

    // Позиция текста
    qreal textX = (start_x1 + start_x2) / 2 - 5;
    qreal textY = -10;

    // Расширяем границы с учетом текста
    qreal finalMinX = std::min(minX - headX - margin, textX - margin);
    qreal finalMaxX = std::max(maxX + headX + margin, textX + textWidth + margin);
    qreal finalMinY = std::min(minY - headY - margin, textY - textHeight - margin);
    qreal finalMaxY = std::max(maxY + headY + margin, textY + margin);

    return QRectF(finalMinX, finalMinY,
        finalMaxX - finalMinX,
        finalMaxY - finalMinY);
}

void LineLoadItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->setPen(m_pen);
    painter->setBrush(Qt::NoBrush);

    // основная линия
    painter->drawLine(start_x1, stop_y1, start_x2, stop_y2);

    qreal len = start_x2 - start_x1;
    if (qFuzzyIsNull(len)) return;

    const qreal step = 20.0;
    const qreal headX = 8.0;
    const qreal headY = 4.0;

    if (el_d == ElementDirection::Right) {
        // Стрелки вправо - острие справа
        for (qreal x = start_x1 + step; x <= start_x2; x += step) {
            qreal y = stop_y1 + (stop_y2 - stop_y1) * (x - start_x1) / len;

            QPolygonF arrowHead;
            arrowHead << QPointF(x, y)              // острие стрелки
                << QPointF(x - headX, y - headY)    // верх хвоста
                << QPointF(x - headX, y + headY);   // низ хвоста

            painter->setBrush(QBrush(m_pen.color()));
            painter->drawPolygon(arrowHead);
            painter->setBrush(Qt::NoBrush);
        }
    }
    else { // Left
        // Стрелки влево - острие СЛЕВА
        for (qreal x = start_x1 + step; x <= start_x2; x += step) {
            qreal y = stop_y1 + (stop_y2 - stop_y1) * (x - start_x1) / len;

            QPolygonF arrowHead;
            arrowHead << QPointF(x - headX, y)      // острие стрелки (слева)
                << QPointF(x, y - headY)            // верх хвоста (справа)
                << QPointF(x, y + headY);           // низ хвоста (справа)

            painter->setBrush(QBrush(m_pen.color()));
            painter->drawPolygon(arrowHead);
            painter->setBrush(Qt::NoBrush);
        }
    }

    painter->setPen(m_pen.color());
    const QPointF textPos = (el_d == ElementDirection::Right)
        ? QPointF((start_x1 + start_x2) / 2 - 5, -10)
        : QPointF((start_x1 + start_x2) / 2 - 5, -10);

    QString tmp = QString("%1 (H/м) q").arg(q_line_load);
    painter->drawText(textPos, tmp);
}


void LineLoadItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItem::mousePressEvent(event);
}

void LineLoadItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    QMenu menu;
    QAction* deleteAction = menu.addAction("Удалить");

    QAction* chosen = menu.exec(event->screenPos());
    if (chosen == deleteAction) {
        if (scene()) {
            scene()->removeItem(this);
            delete this;
        }
        else {
            delete this;
        }
    }
}
BeamSign::BeamSign(qreal _ox, qreal _oy, int numb, const QPen& pen) :
    x_pos(_ox), y_pos(_oy), number(numb), _pen(pen)
{


    setPos(x_pos, y_pos);

    setFlag(QGraphicsItem::ItemIsSelectable);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    setZValue(10);
}
QRectF BeamSign::boundingRect() const
{
    return QRectF(-radius, -radius, 2*radius, 2*radius);
}
void BeamSign::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(_pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawEllipse(boundingRect());

    QFont font = painter->font();
    font.setPointSize(12);   // размер в пунктах (чем больше, тем крупнее)
    // font.setPixelSize(20); // альтернатива — фиксированный размер в пикселях
    font.setBold(true);      // можно сделать жирным
    painter->setFont(font);

    QString tmp = QString::number(number);
    painter->drawText(boundingRect(), Qt::AlignCenter, tmp);
}
// ---------- ConnectionManager ----------
std::vector<BeamItem*> ConnectionManager::getBeams(QGraphicsScene* scene)
{
    std::vector<BeamItem*> beams;
    beams.reserve(20); // Резервируем память

    for (auto& item : scene->items()) {
        if (auto beam = dynamic_cast<BeamItem*>(item)) {
            if (!beam->isBeingDestroyed()) { // Проверяем, что балка не удаляется
                beams.push_back(beam);
            }
        }
    }
    return beams;
}
void BeamSign::remove() {
    if (scene()) {
        scene()->removeItem(this);
        delete this;
    }
}
void PlotItem::remove() {
    if (scene()) {
        scene()->removeItem(this);
        delete this;
    }
}
void DiagramItem::remove() {
    if (scene()) {
        scene()->removeItem(this);
        delete this;
    }
}
BeamJoint::BeamJoint(qreal _ox, qreal _oy, int numb, const QPen& pen)
    : x_pos(_ox), y_pos(_oy), number(numb), _pen(pen){

    setPos(x_pos, y_pos);

    setFlag(QGraphicsItem::ItemIsSelectable);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    setZValue(10);
}
void BeamJoint::remove() {
    if (scene()) {
        scene()->removeItem(this);
        delete this;
    }
}
QRectF BeamJoint::boundingRect() const
{
    return QRectF(-square_side, -square_side, square_side, square_side);
}

void BeamJoint::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(_pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(boundingRect());

    QFont font = painter->font();
    font.setPointSize(12);
    // font.setPixelSize(20); // альтернатива — фиксированный размер в пикселях
    font.setBold(true);
    painter->setFont(font);

    QString tmp = QString::number(number);
    painter->drawText(boundingRect(), Qt::AlignCenter, tmp);
}


std::vector<FixedSupportItem*> ConnectionManager::getSupports(QGraphicsScene* scene)
{
    std::vector<FixedSupportItem*> supports;
    supports.reserve(10);

    for (auto& item : scene->items()) {
        if (auto support = dynamic_cast<FixedSupportItem*>(item)) {
            if (!support->isBeingDestroyed()) { // Проверяем, что заделка не удаляется
                supports.push_back(support);
            }
        }
    }
    return supports;
}

bool ConnectionManager::tryConnectBeams(BeamItem* movable, BeamItem* target)
{
    if (!movable || !target || movable == target) return false;

    // Проверяем, что оба объекта еще существуют в сцене
    if (!movable->scene() || !target->scene()) return false;
    if (movable->isBeingDestroyed() || target->isBeingDestroyed()) return false;

    PointConnector leftMovable = movable->getLeftConnector();
    PointConnector rightMovable = movable->getRightConnector();
    PointConnector leftTarget = target->getLeftConnector();
    PointConnector rightTarget = target->getRightConnector();

    qreal distance1 = leftMovable.distanceSquaredTo(rightTarget);
    qreal distance2 = rightMovable.distanceSquaredTo(leftTarget);

    if (distance1 < SNAP_DISTANCE_SQUARED || distance2 < SNAP_DISTANCE_SQUARED) {
        bool connectLeftToRight = distance1 < distance2;

        QPointF targetPoint = connectLeftToRight ?
            QPointF(rightTarget.o_x, rightTarget.o_y) :
            QPointF(leftTarget.o_x, leftTarget.o_y);

        QPointF movablePoint = connectLeftToRight ?
            QPointF(leftMovable.o_x, leftMovable.o_y) :
            QPointF(rightMovable.o_x, rightMovable.o_y);

        QPointF offset = targetPoint - movablePoint;
        movable->setPos(movable->pos() + offset);
        movable->setFlag(QGraphicsItem::ItemIsMovable, false);

        movable->setConnectedTo(target);
        target->setConnectedTo(movable);

        return true;
    }
    return false;
}

bool ConnectionManager::tryConnectSupportToBeam(FixedSupportItem* support, BeamItem* beam)
{
    if (!support || !beam) return false;

    // Проверяем, что оба объекта еще существуют в сцене
    if (!support->scene() || !beam->scene()) return false;
    if (support->isBeingDestroyed() || beam->isBeingDestroyed()) return false;

    PointConnector supportPoint = support->getPointConnector();
    PointConnector leftTarget = beam->getLeftConnector();
    PointConnector rightTarget = beam->getRightConnector();

    qreal distance1 = supportPoint.distanceSquaredTo(leftTarget);
    qreal distance2 = supportPoint.distanceSquaredTo(rightTarget);

    if (distance1 < SNAP_DISTANCE_SQUARED || distance2 < SNAP_DISTANCE_SQUARED) {
        bool connectToLeft = distance1 < distance2;

        QPointF targetPoint = connectToLeft ?
            QPointF(leftTarget.o_x, leftTarget.o_y) :
            QPointF(rightTarget.o_x, rightTarget.o_y);

        QPointF supportScenePos = QPointF(supportPoint.o_x, supportPoint.o_y);
        QPointF offset = targetPoint - supportScenePos;
        support->setPos(support->pos() + offset);
        support->setFlag(QGraphicsItem::ItemIsMovable, false);

        support->setConnectedTo(beam);
        beam->setConnectedTo(support);

        return true;
    }
    return false;
}

bool ConnectionManager::trySnapItems(QGraphicsScene* scene)
{
    if (!scene) return false;

    // Получаем все элементы одним проходом
    auto allBeams = getBeams(scene);
    auto allSupports = getSupports(scene);

    // Сначала пытаемся соединить подвижные балки
    for (auto* movable : allBeams) {
        if (!(movable->flags() & QGraphicsItem::ItemIsMovable)) continue;
        if (movable->isBeingDestroyed()) continue;

        for (auto* target : allBeams) {
            if (target->isBeingDestroyed()) continue;
            if (tryConnectBeams(movable, target)) {
                return true;
            }
        }
    }

    // Затем пытаемся соединить подвижные заделки с балками
    for (auto* support : allSupports) {
        if (!(support->flags() & QGraphicsItem::ItemIsMovable)) continue;
        if (support->isBeingDestroyed()) continue;

        for (auto* beam : allBeams) {
            if (beam->isBeingDestroyed()) continue;
            if (tryConnectSupportToBeam(support, beam)) {
                return true;
            }
        }
    }

    return false;
}

bool ConnectionManager::checkAndSnapNewItem(QGraphicsItem* newItem, QGraphicsScene* scene)
{
    if (!newItem || !scene) return false;

    bool wasMovable = newItem->flags() & QGraphicsItem::ItemIsMovable;
    newItem->setFlag(QGraphicsItem::ItemIsMovable, true);

    bool snapped = trySnapItems(scene);

    if (!snapped) {
        newItem->setFlag(QGraphicsItem::ItemIsMovable, wasMovable);
    }

    return snapped;
}

bool PointConnector::operator==(PointConnector p_con)
{
    return qFuzzyCompare(o_x, p_con.o_x) && qFuzzyCompare(o_y, p_con.o_y);
}

void DiagramItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->setPen(QPen(Qt::black, 2));
    painter->setBrush(Qt::NoBrush);
    painter->drawLine(0, 0, m_beamLength, 0);

    // Рисуем подпись (если есть)
    if (!m_label.isEmpty())
    {
        painter->setPen(Qt::black);
        QFont font = painter->font();
        font.setPixelSize(12);
        font.setBold(true);
        painter->setFont(font);
        painter->drawText(QPointF(-22, -5), m_label);
    }

    // Формируем график по точкам
    QPolygonF polyline;
    qreal stepX = m_beamLength / (m_values.size() - 1);

    for (size_t i = 0; i < m_values.size(); i++) {
        qreal x = i * stepX;
        qreal y = -m_values[i] * _scaling;
        polyline << QPointF(x, y);
    }

    QPolygonF fillPoly = polyline;
    fillPoly << QPointF(m_beamLength, 0) << QPointF(0, 0);

    painter->setBrush(QBrush(Qt::lightGray));
    painter->setPen(QPen(Qt::black, 1));
    painter->drawPolygon(fillPoly);

    painter->setPen(QPen(Qt::blue, 1));
    painter->drawPolyline(polyline);


    const double eps = 1e-9;
    if (!m_values.empty()) {
        double leftValue = m_values.front();
        if (std::abs(leftValue) > eps && showLeftLabel) {  // добавлена проверка
            painter->setPen(Qt::darkRed);
            painter->setFont(QFont("Arial", 9, QFont::Bold));
            painter->drawText(QPointF(0, -leftValue * _scaling - 8),
                QString::number(leftValue, 'f', 2));
        }

        double rightValue = m_values.back();
        if (std::abs(rightValue) > eps && showRightLabel) {  // добавлена проверка
            qreal xMax = m_beamLength;
            painter->setPen(Qt::darkRed);
            painter->setFont(QFont("Arial", 9, QFont::Bold));
            painter->drawText(QPointF(xMax - 25, -rightValue * _scaling - 8),
                QString::number(rightValue, 'f', 2));
        }
    }

}

void PlotItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->setRenderHint(QPainter::Antialiasing, true);

    // Рисуем вертикальную линию
    QPen linePen(Qt::black, 1);
    painter->setPen(linePen);
    painter->drawLine(0, 0, 0, mLineHeight);

    // Рисуем маркер в начале линии (узел)
    painter->setBrush(QBrush(Qt::black));
    painter->drawEllipse(QPointF(0, 0), 4, 4);

    
}
