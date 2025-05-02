#ifndef BINVIEWER_H
#define BINVIEWER_H

#include <QDialog>
#include <QObject>
#include <QTextEdit>
#include <QPushButton>
#include <QFileDialog>

class BinViewer : public QDialog {
    Q_OBJECT
public:
    explicit BinViewer(QWidget *parent = nullptr);
    void showAndDisplay();

private:
    QTextEdit *m_textEdit;
    QPushButton *m_openButton;
};

#endif // BINVIEWER_H
