#pragma once

#include <QtWidgets/QMainWindow>
#include <QMessageBox>
#include <QLabel>
#include <QToolButton>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDockWidget>
#include <QGraphicsTextItem>
#include <QScreen>
#include <QGuiApplication>
#include <QWheelEvent>
#include <QScrollBar>
#include <QStringList>
#include <QRegularExpression>
#include <QFileDialog>
#include <QDebug>
#include "setOfElements.h"
#include <ui_superBAR.h>
#include "Help.h"
#include "tinyxml2.h"
#include "cProcessor.h"
#include "sliderDialog.h"
using namespace tinyxml2;


#define FixSupportLen 70
class superBAR : public QMainWindow
{
    
    std::vector<Core_of_Beam> collectedBeam_info;

    

    Q_OBJECT
        qreal m_currentZoom = 1.0;
    bool beams_sign_b = false;
    bool joins_sign_b = false;
public:
    bool fixedSupport_left = true;
    bool fixedSupport_right = true;
    superBAR(QWidget *parent = nullptr);
    ~superBAR();


    template <typename T>
    void removeItemsOfType();

public slots:
    void create_Plot(std::vector<BeamResults> results);

private slots:
    void onMenuActionTriggered();
    void on_pushButton_8_clicked();
    void onScalesConfirmed(int nxScale, int uxScale, int sigmaScale);


protected:
	void closeEvent(QCloseEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private:

    int Nx_scaling = 20, Ux_scaling= 20, sigma_scaling = 20;

    std::tuple<double, double, double> findMinMax(const std::vector<double>& values);
    std::vector<BeamResults> _results;
    void displayDiagrams(const std::vector<BeamResults>& results);
    void collectBeamInfo(std::vector<Core_of_Beam>& data);
    Joint_info collectJointInfo(const PointConnector& nodePos);

    bool shouldHideRightLabel(int currentIndex,
        const std::vector<BeamResults>& results,
        const std::function<std::vector<double>(const BeamResults&)>& getValuesFunc);


    int getNodeCount() const;
    int getBeamCount() const;

    std::string _file_name;
    void saveOnHotKey();
    void serialization(std::string filename);
    void deserialization(std::string filename);
    std::string choosePath(bool isSave);

    bool checkNotEmpty(QTextEdit* edit);
    bool checkAllFilled(const QVector<QTextEdit*>& edits);

	qreal firstBeam_x = 0;
    void disconnectAllItems();
    void changeBeamLength(BeamItem* beam, qreal newLength);
    std::tuple< qreal, qreal> getLastPointBeam();
    std::tuple< qreal, qreal> getFirstPointBeam();


    std::tuple< qreal, qreal> getPointBeam(int order_beam);
    std::tuple< qreal, qreal, qreal, qreal> getPointsBeam(int order_beam);

    std::vector<PointConnector> collectAllConnectors();
    Ui::superBARClass ui;
    QGraphicsScene* m_scene;
    void setupSceneAndView();
    void connectUi();

	void centerWindowOnScreen();


    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event);


};


