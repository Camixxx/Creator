﻿#ifndef NEWPRODIA_H
#define NEWPRODIA_H

//新建工程对话框
#include <QWidget>
#include "weh.h"

class NewProDia : public QWidget
{
    Q_OBJECT
public:
     NewProDia(QWidget *parent = 0);
     int type ;
     QString okdir ;
     QString okname ;

     bool WaitUser() ;

signals:

public slots:
     void LookDir() ;
     void SureOK() ;
private:
     QDialog *prodialog ;
     QPushButton *dir ;
     QPushButton *yes ;
     QPushButton *no ;
     QLineEdit *s1 ;
     QLineEdit *s2 ;
	 QCheckBox *cb;
};

#endif // NEWPRODIA_H
