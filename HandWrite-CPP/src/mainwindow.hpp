#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QFileDialog>
#include <QMessageBox>
#include <QImage>
#include <QPixmap>
#include <QProgressDialog>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <QWheelEvent>
#include <QDialog>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <map>

#include "config.hpp"
#include "tools.hpp"
#include "core.hpp"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

namespace HandWrite {

/**
 * @brief 字符覆盖属性编辑对话框
 */
class CharacterOverrideDialog : public QDialog {
    Q_OBJECT

public:
    explicit CharacterOverrideDialog(QWidget *parent = nullptr);
    ~CharacterOverrideDialog() = default;
    
    void setOverride(const CharacterOverride &override);
    CharacterOverride getOverride() const;
    
signals:
    void applied();  // 点击应用按钮时发出
    
private slots:
    void onApplyClicked();
    
private:
    QCheckBox *m_fontSizeCheck;
    QSpinBox *m_fontSizeSpin;
    QCheckBox *m_perturbXCheck;
    QDoubleSpinBox *m_perturbXSpin;
    QCheckBox *m_perturbYCheck;
    QDoubleSpinBox *m_perturbYSpin;
    QCheckBox *m_perturbThetaCheck;
    QDoubleSpinBox *m_perturbThetaSpin;
    QCheckBox *m_colorCheck;
    QSpinBox *m_colorRedSpin;
    QSpinBox *m_colorGreenSpin;
    QSpinBox *m_colorBlueSpin;
    QSpinBox *m_colorAlphaSpin;
};

/**
 * @brief 主窗口
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    
protected:
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    // 按钮处理函数
    void onPushButtonPreviewClicked();
    void onPushButtonExportClicked();
    void onPushButtonSaveConfigClicked();
    void onPushButtonLoadConfigClicked();
    void onPushButtonCharOverrideClicked();
    void onPushButtonClearOverridesClicked();
    
    // 分页按钮处理函数
    void onPushButtonFirstPageClicked();
    void onPushButtonPrevPageClicked();
    void onPushButtonNextPageClicked();
    void onPushButtonLastPageClicked();
    void onSpinBoxPageValueChanged(int value);
    
    // 缩放按钮处理函数
    void onPushButtonZoomInClicked();
    void onPushButtonZoomOutClicked();
    void onPushButtonZoomResetClicked();
    void onSliderZoomValueChanged(int value);
    
    // 下拉框处理函数
    void onComboBoxPaperTemplateCurrentIndexChanged(int index);

private:
    Ui::MainWindow *ui;
    
    // 组件
    BasicTools m_tools;
    HandwriteGenerator m_generator;
    
    // 预览图片存储
    std::map<int, std::string> m_previewImagePaths;
    std::vector<QImage> m_previewImages;
    
    // 分页状态
    int m_currentPage = 0;
    int m_totalPages = 0;
    
    // 预览图形场景
    QGraphicsScene *m_scene;
    QGraphicsPixmapItem *m_pixmapItem;
    
    // 缩放状态
    double m_zoomFactor = 1.0;
    static constexpr double ZOOM_MIN = 0.1;
    static constexpr double ZOOM_MAX = 5.0;
    static constexpr double ZOOM_STEP = 0.1;
    
    // 异步操作跟踪
    QFutureWatcher<std::vector<QImage>> *m_previewWatcher;
    QFutureWatcher<std::map<int, std::string>> *m_exportWatcher;
    QProgressDialog *m_progressDialog;
    
    // 存储实际导出倍率（用户选择的）
    int m_exportRate = 4;
    
    // 字符覆盖设置
    std::vector<CharacterOverrideRange> m_charOverrides;
    
    // 初始化
    void setupDefaults();
    void setupConnections();
    void populateComboBoxes();
    
    // UI辅助函数
    void showImage(const QString &imagePath);
    void showImage(const QImage &image);
    void updatePreview();
    void updatePaginationUI();
    void updatePageButtons();
    void goToPage(int page);
    void updateZoomDisplay();
    void applyZoom();
    void updateCharOverrideLabel();
    
    // 异步生成辅助函数
    std::vector<QImage> generatePreviewAsync(TemplateParams params, QString text, int previewRate);
    std::map<int, std::string> generateExportAsync(TemplateParams params, QString text, QString outputDir);
    void onPreviewFinished();
    void onExportFinished();
    void setupProgressDialog(const QString &title, int maximum = 0);
    
    // 表单数据处理
    TemplateParams getParamsFromForm();
    QString getTextFromTextEdit();
    
    // 配置
    void saveConfiguration(const QString &path);
    void loadConfiguration(const QString &path);
};

} // namespace HandWrite

#endif // MAINWINDOW_HPP