#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QDebug>
#include <QElapsedTimer>

#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class tSystem;

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    QElapsedTimer systemTimerElapsed;

    Ui::MainWindow* ui;
   private:

    // Held rather than leaked so the audio device is handed back on exit. It
    // cannot be a plain member: the buttons read the clock as they are built,
    // and that goes through the mainWindow global, which is only set part way
    // through the constructor body.
    std::unique_ptr<tSystem> system_;

    QTimer loopTimer;
};
#endif  // MAINWINDOW_H
