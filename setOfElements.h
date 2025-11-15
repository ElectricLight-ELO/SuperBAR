#ifndef SETOFELEMENTS_H
#define SETOFELEMENTS_H
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QGraphicsPathItem>
#include <QGraphicsTextItem>
#include <QGraphicsItemGroup>
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QPen>
#include <QBrush>
#include <QPainterPath>
#include <QDebug>
#include <QTimer>
#include <QObject>
#include <unordered_map>
#include <vector>
#include <QPainter>
#include "Help.h"
enum class ElementDirection {
    Right,
    Left
};

struct PointConnector {
    qreal o_x;
    qreal o_y;

    PointConnector(qreal x = 0, qreal y = 0) : o_x(x), o_y(y) {}

    // вычисление квадрата расстояния
    inline qreal distanceSquaredTo(const PointConnector& other) const {
        qreal dx = o_x - other.o_x;
        qreal dy = o_y - other.o_y;
        return dx * dx + dy * dy;
    }

    bool operator==(PointConnector p_con);
};

struct IHasConnector {
    virtual ~IHasConnector() = default;
    virtual PointConnector getPointConnector() const = 0;
};

// Предварительные объявления
class BeamItem;
class FixedSupportItem;

// Балка
class BeamItem final : public QGraphicsRectItem, public IHasConnector {
private:
    bool m_beingDestroyed = false; // Флаг для предотвращения повторного вызова

    double LenBeam;
    double cross_sectArea_A;
    double mod_Elasticity_E;
    double maxStressBeam_q;
public:
    QGraphicsItem* connectedTo = nullptr;

    BeamItem(qreal x1, qreal y1, qreal length, qreal width);
    virtual ~BeamItem() override {
        m_beingDestroyed = true;
        // Явно удаляем дочерние элементы
        if (leftPoint) delete leftPoint;
        if (rightPoint) delete rightPoint;
    }
    PointConnector getPointConnector() const override;

    PointConnector getLeftConnector() const;
    PointConnector getRightConnector() const;
    void setConnectedTo(QGraphicsItem* other);
    void disconnect();
    void safeDelete(); // Новая безопасная функция удаления

    std::tuple<qreal, qreal> getParams() const { return { _len, _width };  }

    bool isConnected() const { return connectedTo != nullptr; }
    QGraphicsItem* getConnectedBeam() const { return connectedTo; }
    bool isBeingDestroyed() const { return m_beingDestroyed; }

    void set_Length(qreal newLength);

    void setInfo(double _cross_sectArea_A, double _mod_Elasticity_E, double _maxStressBeam_q);
    std::tuple<qreal, qreal> getCoordinates() { 
        QPointF pos = scenePos();
        return { pos.x(), pos.y() };
    
    }
    std::tuple<double, double, double, double> getInfo();
protected:
    qreal _ox, _oy, _len, _width;
    PointConnector pointConnect_left, pointConnect_right;

    // Кэшируем зеленые точки для повторного использования
    QGraphicsEllipseItem* leftPoint = nullptr;
    QGraphicsEllipseItem* rightPoint = nullptr;

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

private:
    void updateGreenPoints();
    QGraphicsEllipseItem* createGreenPoint(const PointConnector& pc);
};

// Заделка
class FixedSupportItem final : public QObject, public QGraphicsItemGroup, public IHasConnector {
    Q_OBJECT
private:
    bool m_beingDestroyed = false; // Флаг для предотвращения повторного вызова
    qreal _ox;
    qreal _oy;

signals:
    void supportDeleted(ElementDirection direction);
public:
    BeamItem* connectedTo = nullptr;

    FixedSupportItem(qreal x, qreal y, qreal height, ElementDirection dir);
    virtual ~FixedSupportItem() override {
        m_beingDestroyed = true;
    }
    PointConnector getPointConnector() const override;
    std::tuple<qreal, qreal, ElementDirection> getInfo() { return{ _ox, _oy, el_d }; }
    void setConnectedTo(BeamItem* other);
    void disconnect();
    void safeDelete();

    bool isConnected() const { return connectedTo != nullptr; }
    BeamItem* getConnectedBeam() const { return connectedTo; }
    bool isBeingDestroyed() const { return m_beingDestroyed; }

protected:
    ElementDirection el_d;
    PointConnector pointConnect;

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
};

// Сила
class ForceItem final : public QGraphicsItem, public IHasConnector {

    qreal len_fr_beam = 0; // длина от начала балки до силы (смещение позиции)
    qreal force_H = 0;
    int force_pos_beam = 0;
    qreal _ox;
    qreal _oy;

    qreal arrowLength = 50.0;

public:
    ForceItem(qreal x, qreal y, ElementDirection dir,
        qreal length = 50, const QPen& pen = QPen(Qt::red, 2));

    PointConnector getPointConnector() const override;
    void set_Len_fr_beam(qreal newLength);
    void changeDirection(ElementDirection dir);
    void setForce_H(qreal force_digital, int pos_beam);
    std::tuple<qreal, qreal, int> getInfo(); // len && force

protected:
    ElementDirection el_d;
    PointConnector pointConnect;
    QPen m_pen;

    // QGraphicsItem API
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
};

class LineLoadItem final : public QGraphicsItem, public IHasConnector {
    qreal start_x1 = 0, stop_y1 = 0;
   qreal start_x2 = 0, stop_y2 = 0;
   qreal q_line_load = 0;
   int _beamDig;
public:
    LineLoadItem(qreal ox_1, qreal oy_1, qreal ox_2, qreal oy_2, ElementDirection dir, 
        const QPen& pen = QPen(Qt::blue, 2));
    PointConnector getPointConnector() const override;

    void change_loc_joins(qreal ox_1, qreal oy_1, qreal ox_2, qreal oy_2);
    void set_LineLoad(qreal q_l_load, int beamD);
    void change_direction(ElementDirection direction);
    std::tuple<qreal, int> getInfo();
protected:
    ElementDirection el_d;
    QPen m_pen;
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

};

class BeamSign final : public QGraphicsItem {
    const qreal radius = 12;
    qreal x_pos, y_pos;
    
    int number;
    const QPen _pen;
public:
    
    BeamSign(qreal _ox, qreal _oy,int numb, const QPen& pen = QPen(Qt::darkCyan, 2));

    void remove();
protected:
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
};


class BeamJoint final : public QGraphicsItem {
    const qreal square_side = 20;
    qreal x_pos, y_pos;

    int number;
    const QPen _pen;
public:

    BeamJoint(qreal _ox, qreal _oy, int numb, const QPen& pen = QPen(Qt::darkGreen, 2));

    void remove();
protected:
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
};

class PlotItem : public QGraphicsItem
{
private:
    qreal mX, mY;
    qreal mLineHeight;

public:
    PlotItem(qreal x, qreal y, qreal lineHeight = 100.0)
        : mX(x), mY(y), mLineHeight(lineHeight)
    {
        setPos(mX, mY);
    }
    void remove();
    QRectF boundingRect() const override
    {
        qreal width = 40;
        return QRectF(-20, 0, width, mLineHeight + 30);
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override;
};


class DiagramItem : public QGraphicsItem {
private:
    std::vector<double> m_values;
    qreal mX, mY, m_beamLength;
    QString m_label;
    int _scaling;
    bool showLeftLabel = true;   // новое поле
    bool showRightLabel = true;  // новое поле

public:
    DiagramItem(qreal x, qreal y, qreal length,
        const std::vector<double>& values,
        const QString& label, int scalingparam,bool left_sign, bool right_sign)
        : mX(x), mY(y), m_beamLength(length),
        m_values(values), m_label(label), _scaling(scalingparam), showLeftLabel(left_sign), showRightLabel(right_sign){
        setPos(mX, mY);
    }

    void remove();
    QRectF boundingRect() const override {
        return QRectF(0, 0, m_beamLength, 100);
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem*,
        QWidget*) override;
};


class ConnectionManager : public QObject {
    Q_OBJECT
public:
    static const qreal SNAP_DISTANCE;
    static const qreal SNAP_DISTANCE_SQUARED; // Добавляем квадрат для оптимизации

    static bool trySnapItems(QGraphicsScene* scene);
    static bool checkAndSnapNewItem(QGraphicsItem* newItem, QGraphicsScene* scene);

private:
    // Оптимизированные helper функции
    static std::vector<BeamItem*> getBeams(QGraphicsScene* scene);
    static std::vector<FixedSupportItem*> getSupports(QGraphicsScene* scene);
    static bool tryConnectBeams(BeamItem* movable, BeamItem* target);
    static bool tryConnectSupportToBeam(FixedSupportItem* support, BeamItem* beam);
};

#endif
// SETOFELEMENTS_H