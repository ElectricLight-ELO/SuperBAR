#pragma once

#include <QDialog>
#include "ui_sliderDialog.h"

class sliderDialog : public QDialog
{
	Q_OBJECT

public:
	sliderDialog(std::tuple<int, int, int>& scaling_default, QWidget *parent = nullptr);
	~sliderDialog();
	int getNxValue() const;
	int getUxValue() const;
	int getSigmaValue() const;
private slots:
	void onSlider1ValueChanged(int value);
	void onSlider2ValueChanged(int value);
	void onSlider3ValueChanged(int value);
	void onConfirmButtonClicked();

signals:
	void scalesConfirmed(int nxValue, int uxValue, int sigmaValue);

private:
	Ui::sliderDialogClass ui;

	QLabel* labelNxValue;
	QLabel* labelUxValue;
	QLabel* labelSigmaValue;
};

