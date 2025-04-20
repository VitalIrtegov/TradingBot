#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QTextEdit>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStartButtonClicked();  // Обработчик кнопки "Старт"
    void logMessage(const QString &message);  // Логирование

private:
    Ui::MainWindow *ui;
    void setupUI();  // Настройка интерфейса
};
#endif // MAINWINDOW_H
