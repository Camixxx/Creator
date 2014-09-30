#ifndef CCONFIGDIA_H
#define CCONFIGDIA_H

#include "weh.h"
#include "dia/ctextedit.h"
#include <QDialog>

class CConfigdia : public QDialog
{
    Q_OBJECT
public:
    explicit CConfigdia(QWidget *parent = 0);

    QListWidget *itemlist ;
    QStackedWidget *stk ;

signals:

public slots:
    void itemChange() ;
private:
    int low ;

};

#endif // CCONFIGDIA_H
