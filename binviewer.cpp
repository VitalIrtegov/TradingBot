#include "binviewer.h"
#include <QVBoxLayout>
#include <QFile>
#include <QDataStream>
#include <QDateTime>

BinViewer::BinViewer(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Просмотр бинарных файлов");
    resize(800, 600);

    m_textEdit = new QTextEdit(this);
    m_textEdit->setReadOnly(true);

    m_openButton = new QPushButton("Выбрать файл", this);
    connect(m_openButton, &QPushButton::clicked, this, &BinViewer::showAndDisplay);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_openButton);
    layout->addWidget(m_textEdit);
}

void BinViewer::showAndDisplay() {
    QString filePath = QFileDialog::getOpenFileName(this, "Выберите файл данных", "", "Bin Files (*.bin)");

    if(!filePath.isEmpty()) {
        QFile file(filePath);
        if(file.open(QIODevice::ReadOnly)) {
            QDataStream in(&file);
            m_textEdit->clear();

            qint64 timestamp;
            double price, volume;

            while(!in.atEnd()) {
                in >> timestamp >> price >> volume;
                QString line = QString("%1 | Цена: %2 | Объем: %3")
                                   .arg(QDateTime::fromMSecsSinceEpoch(timestamp).toString("yyyy-MM-dd hh:mm:ss"))
                                   .arg(price, 0, 'f', 4)
                                   .arg(volume, 0, 'f', 2);
                m_textEdit->append(line);
            }
            file.close();
        }
    }
    show();
}
